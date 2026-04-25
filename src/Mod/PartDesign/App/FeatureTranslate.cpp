// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2024 FreeCAD contributors                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <App/Datums.h>
#include <App/GeoFeature.h>
#include <Base/Exception.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Tools.h>

#include "Body.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include "FeatureTranslate.h"


using namespace PartDesign;

namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::Translate, PartDesign::Feature)

const char* Translate::ModeEnums[] = {"DirectionDistance", "PointToPoint", "Coordinates", nullptr};

Translate::Translate()
{
    ADD_PROPERTY_TYPE(
        Mode,
        (0L),
        "Translate",
        App::Prop_None,
        "Translation mode: DirectionDistance, PointToPoint, or Coordinates"
    );
    Mode.setEnums(ModeEnums);

    ADD_PROPERTY_TYPE(
        DirectionReference,
        (nullptr),
        "DirectionDistance",
        App::Prop_None,
        "Reference edge, face, or datum line/plane that defines the translation direction"
    );
    ADD_PROPERTY_TYPE(
        Direction,
        (Base::Vector3d(0, 0, 1)),
        "DirectionDistance",
        App::Prop_None,
        "Custom direction vector when no reference is set"
    );
    ADD_PROPERTY_TYPE(
        Distance,
        (10.0),
        "DirectionDistance",
        App::Prop_None,
        "Distance to translate along the direction"
    );

    ADD_PROPERTY_TYPE(
        StartPoint,
        (nullptr),
        "PointToPoint",
        App::Prop_None,
        "Start vertex for point-to-point translation"
    );
    ADD_PROPERTY_TYPE(
        EndPoint,
        (nullptr),
        "PointToPoint",
        App::Prop_None,
        "End vertex for point-to-point translation"
    );

    ADD_PROPERTY_TYPE(
        Delta,
        (Base::Vector3d(0, 0, 0)),
        "Coordinates",
        App::Prop_None,
        "Translation vector in the coordinate system of AxisSystem (or body origin if unset)"
    );
    ADD_PROPERTY_TYPE(
        AxisSystem,
        (nullptr),
        "Coordinates",
        App::Prop_None,
        "Coordinate system for interpreting Delta; defaults to body origin when unset"
    );
}

short Translate::mustExecute() const
{
    if (Mode.isTouched() || DirectionReference.isTouched() || Direction.isTouched()
        || Distance.isTouched() || StartPoint.isTouched() || EndPoint.isTouched()
        || Delta.isTouched() || AxisSystem.isTouched()) {
        return 1;
    }
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Translate::execute()
{
    if (!this->BaseFeature.getValue()) {
        Body* body = getFeatureBody();
        if (body) {
            body->setBaseProperty(this);
        }
    }

    Part::TopoShape baseTopoShape = getBaseTopoShape(/*silent=*/true);
    if (baseTopoShape.isNull()) {
        return new App::DocumentObjectExecReturn("Base shape is null");
    }

    // Strip the base feature's placement from the shape so we can re-apply it via Placement
    Part::Feature* baseFeature = getBaseObject(/*silent=*/true);
    Base::Placement basePlacement = baseFeature ? baseFeature->Placement.getValue()
                                                : Base::Placement();
    baseTopoShape.setTransform(Base::Matrix4D());

    gp_Vec vec;
    try {
        vec = computeTranslationVector();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // Compose: world placement = translation offset applied on top of base placement
    Base::Placement translationPl(Base::Vector3d(vec.X(), vec.Y(), vec.Z()), Base::Rotation());
    this->Placement.setValue(translationPl * basePlacement);
    this->Shape.setValue(baseTopoShape);
    return App::DocumentObject::StdReturn;
}

void Translate::onChanged(const App::Property* prop)
{
    Feature::onChanged(prop);
}

gp_Vec Translate::computeTranslationVector() const
{
    switch (Mode.getValue()) {
        case 0:
            return computeDirectionDistanceVector();
        case 1:
            return computePointToPointVector();
        case 2:
            return computeCoordinatesVector();
        default:
            throw Base::ValueError("Unknown translation mode");
    }
}

gp_Vec Translate::computeDirectionDistanceVector() const
{
    double dist = Distance.getValue();
    gp_Dir dir;

    App::DocumentObject* refObject = DirectionReference.getValue();
    if (refObject) {
        std::vector<std::string> subs = DirectionReference.getSubValues();

        if (auto* sketch = freecad_cast<Part::Part2DObject*>(refObject)) {
            Base::Axis axis;
            if (!subs.empty() && subs[0] == "H_Axis") {
                axis = sketch->getAxis(Part::Part2DObject::H_Axis);
                axis *= sketch->Placement.getValue();
            }
            else if (!subs.empty() && subs[0] == "V_Axis") {
                axis = sketch->getAxis(Part::Part2DObject::V_Axis);
                axis *= sketch->Placement.getValue();
            }
            else if (!subs.empty() && subs[0] == "N_Axis") {
                axis = sketch->getAxis(Part::Part2DObject::N_Axis);
                axis *= sketch->Placement.getValue();
            }
            else if (!subs.empty() && subs[0].compare(0, 4, "Edge") == 0) {
                Part::TopoShape refShape = sketch->Shape.getShape();
                TopoDS_Shape ref = refShape.getSubShape(subs[0].c_str());
                TopoDS_Edge refEdge = TopoDS::Edge(ref);
                if (refEdge.IsNull()) {
                    throw Base::ValueError("Failed to extract direction edge");
                }
                BRepAdaptor_Curve adapt(refEdge);
                if (adapt.GetType() != GeomAbs_Line) {
                    throw Base::TypeError("Direction edge must be a straight line");
                }
                gp_Dir d = adapt.Line().Direction();
                axis.setDirection(Base::Vector3d(d.X(), d.Y(), d.Z()));
            }
            dir = gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
        }
        else if (auto* plane = freecad_cast<PartDesign::Plane*>(refObject)) {
            Base::Vector3d d = plane->getNormal();
            dir = gp_Dir(d.x, d.y, d.z);
        }
        else if (auto* line = freecad_cast<PartDesign::Line*>(refObject)) {
            Base::Vector3d d = line->getDirection();
            dir = gp_Dir(d.x, d.y, d.z);
        }
        else if (auto* plane = freecad_cast<App::Plane*>(refObject)) {
            Base::Vector3d d = plane->getDirection();
            dir = gp_Dir(d.x, d.y, d.z);
        }
        else if (auto* line = freecad_cast<App::Line*>(refObject)) {
            Base::Vector3d d = line->getDirection();
            dir = gp_Dir(d.x, d.y, d.z);
        }
        else if (auto* refFeature = freecad_cast<Part::Feature*>(refObject)) {
            if (subs.empty() || subs[0].empty()) {
                throw Base::ValueError("No direction sub-element specified");
            }
            Part::TopoShape refShape = refFeature->Shape.getShape();
            TopoDS_Shape ref = refShape.getSubShape(subs[0].c_str());

            if (ref.ShapeType() == TopAbs_FACE) {
                TopoDS_Face refFace = TopoDS::Face(ref);
                BRepAdaptor_Surface adapt(refFace);
                if (adapt.GetType() != GeomAbs_Plane) {
                    throw Base::TypeError("Direction face must be planar");
                }
                dir = adapt.Plane().Axis().Direction();
            }
            else if (ref.ShapeType() == TopAbs_EDGE) {
                TopoDS_Edge refEdge = TopoDS::Edge(ref);
                BRepAdaptor_Curve adapt(refEdge);
                if (adapt.GetType() != GeomAbs_Line) {
                    throw Base::TypeError("Direction edge must be a straight line");
                }
                dir = adapt.Line().Direction();
            }
            else {
                throw Base::ValueError("Direction reference must be an edge or face");
            }
        }
        else {
            throw Base::ValueError("Unsupported direction reference type");
        }
    }
    else {
        // Use manual Direction property
        Base::Vector3d d = Direction.getValue();
        if (d.Length() < Precision::Confusion()) {
            throw Base::ValueError("Direction vector has zero length");
        }
        d.Normalize();
        dir = gp_Dir(d.x, d.y, d.z);
    }

    return gp_Vec(dir) * dist;
}

gp_Vec Translate::computePointToPointVector() const
{
    auto extractPoint = [](const App::PropertyLinkSub& prop, const char* label) -> gp_Pnt {
        App::DocumentObject* obj = prop.getValue();
        if (!obj) {
            throw Base::ValueError(std::string(label) + " is not set");
        }
        std::vector<std::string> subs = prop.getSubValues();
        if (subs.empty() || subs[0].empty()) {
            throw Base::ValueError(std::string(label) + " has no sub-element");
        }

        auto* feat = freecad_cast<Part::Feature*>(obj);
        if (!feat) {
            throw Base::ValueError(std::string(label) + " must reference a Part feature");
        }

        TopoDS_Shape sub = feat->Shape.getShape().getSubShape(subs[0].c_str());
        if (sub.ShapeType() != TopAbs_VERTEX) {
            throw Base::TypeError(std::string(label) + " sub-element must be a vertex");
        }

        return BRep_Tool::Pnt(TopoDS::Vertex(sub));
    };

    gp_Pnt start = extractPoint(StartPoint, "StartPoint");
    gp_Pnt end = extractPoint(EndPoint, "EndPoint");

    return gp_Vec(start, end);
}

gp_Vec Translate::computeCoordinatesVector() const
{
    Base::Vector3d d = Delta.getValue();

    Base::Placement axisPlacement;
    App::DocumentObject* axisObj = AxisSystem.getValue();
    if (axisObj) {
        auto* feat = freecad_cast<App::GeoFeature*>(axisObj);
        if (feat) {
            axisPlacement = feat->Placement.getValue();
        }
    }
    else {
        // Use body placement when no axis system is set
        Body* body = getFeatureBody();
        if (body) {
            axisPlacement = body->Placement.getValue();
        }
    }

    Base::Vector3d global = axisPlacement.getRotation().multVec(d);
    return gp_Vec(global.x, global.y, global.z);
}

}  // namespace PartDesign
