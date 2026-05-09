// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2025 FreeCAD Project                                    *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>

#include <App/Datums.h>
#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/TopoShape.h>

#include "DatumLine.h"
#include "DatumPoint.h"
#include "FeatureRotateBody.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

// clang-format off
const char* RotateBody::ModeEnums[] = {
    "Axis and Angle",
    "Axis and Two Elements",
    "Three Points",
    nullptr
};
// clang-format on

PROPERTY_SOURCE(PartDesign::RotateBody, PartDesign::FeatureRefine)

RotateBody::RotateBody()
{
    ADD_PROPERTY_TYPE(Mode, (0L), "RotateBody", App::Prop_None, "Rotation mode");
    Mode.setEnums(ModeEnums);

    ADD_PROPERTY_TYPE(
        Axis,
        (nullptr),
        "RotateBody",
        App::Prop_None,
        "Rotation axis: linear edge, datum line, or body origin axis"
    );
    ADD_PROPERTY_TYPE(
        Angle,
        (45.0),
        "RotateBody",
        App::Prop_None,
        "Rotation angle in degrees (Axis and Angle mode)"
    );

    ADD_PROPERTY_TYPE(
        Ref1,
        (nullptr),
        "RotateBody",
        App::Prop_None,
        "First reference geometry for angle computation"
    );
    ADD_PROPERTY_TYPE(
        Ref2,
        (nullptr),
        "RotateBody",
        App::Prop_None,
        "Second reference geometry for angle computation"
    );

    ADD_PROPERTY_TYPE(Point1, (nullptr), "RotateBody", App::Prop_None, "First point (Three Points mode)");
    ADD_PROPERTY_TYPE(Point2, (nullptr), "RotateBody", App::Prop_None, "Pivot point (Three Points mode)");
    ADD_PROPERTY_TYPE(Point3, (nullptr), "RotateBody", App::Prop_None, "Third point (Three Points mode)");
}

short RotateBody::mustExecute() const
{
    if (Mode.isTouched() || Axis.isTouched() || Angle.isTouched() || Ref1.isTouched()
        || Ref2.isTouched() || Point1.isTouched() || Point2.isTouched() || Point3.isTouched()) {
        return 1;
    }
    return FeatureRefine::mustExecute();
}

// ---------------------------------------------------------------------------
// Static helper: extract a 3-D point from a PropertyLinkSub
// ---------------------------------------------------------------------------
gp_Pnt RotateBody::extractPoint(const App::PropertyLinkSub& prop, const char* propName)
{
    App::DocumentObject* obj = prop.getValue();
    if (!obj) {
        throw Base::ValueError(std::string(propName) + " is not set");
    }

    // PartDesign datum point (user-created reference point)
    if (obj->isDerivedFrom<PartDesign::Point>()) {
        Base::Vector3d v = static_cast<PartDesign::Point*>(obj)->getPoint();
        return gp_Pnt(v.x, v.y, v.z);
    }

    // App::DatumElement covers body origin points (App::Point)
    if (obj->isDerivedFrom<App::DatumElement>()) {
        Base::Vector3d b = static_cast<App::DatumElement*>(obj)->getBasePoint();
        return gp_Pnt(b.x, b.y, b.z);
    }

    // Generic Part::Feature sub-element (vertex)
    const std::vector<std::string>& subs = prop.getSubValues();
    if (subs.empty() || subs[0].empty()) {
        throw Base::ValueError(std::string(propName) + ": no sub-element selected");
    }
    if (!obj->isDerivedFrom<Part::Feature>()) {
        throw Base::TypeError(
            std::string(propName) + " must be a vertex, datum point, or origin point"
        );
    }
    auto* feat = static_cast<Part::Feature*>(obj);
    TopoDS_Shape sub = feat->Shape.getShape().getSubShape(subs[0].c_str());
    if (sub.IsNull() || sub.ShapeType() != TopAbs_VERTEX) {
        throw Base::TypeError(std::string(propName) + " sub-element must be a vertex");
    }
    return BRep_Tool::Pnt(TopoDS::Vertex(sub));
}

// ---------------------------------------------------------------------------
// Compute axis and rotation angle from the current property values
// ---------------------------------------------------------------------------
void RotateBody::computeAxisAndAngle(gp_Ax1& outAxis, double& outAngleRad) const
{
    const int mode = Mode.getValue();

    // -----------------------------------------------------------------------
    // Three Points: axis and angle both come from the three point geometry
    // -----------------------------------------------------------------------
    if (mode == 2) {
        gp_Pnt P1 = extractPoint(Point1, "Point1");
        gp_Pnt P2 = extractPoint(Point2, "Point2 (pivot)");
        gp_Pnt P3 = extractPoint(Point3, "Point3");

        gp_Vec v1(P2, P1);
        gp_Vec v3(P2, P3);

        if (v1.Magnitude() < Precision::Confusion()) {
            throw Base::ValueError("Point1 and Point2 (pivot) are coincident");
        }
        if (v3.Magnitude() < Precision::Confusion()) {
            throw Base::ValueError("Point3 and Point2 (pivot) are coincident");
        }

        gp_Vec axisVec = v1.Crossed(v3);
        if (axisVec.Magnitude() < Precision::Confusion()) {
            throw Base::ValueError("The three points are collinear; cannot define a rotation axis");
        }

        outAxis = gp_Ax1(P2, gp_Dir(axisVec));
        outAngleRad = v1.AngleWithRef(v3, axisVec);
        return;
    }

    // -----------------------------------------------------------------------
    // Axis modes: extract axis from the Axis property
    // -----------------------------------------------------------------------
    App::DocumentObject* axisObj = Axis.getValue();
    if (!axisObj) {
        throw Base::ValueError("No rotation axis selected");
    }

    gp_Pnt axisOrigin(0.0, 0.0, 0.0);
    gp_Dir axisDir;
    bool originKnown = false;

    if (axisObj->isDerivedFrom<PartDesign::Line>()) {
        auto* line = static_cast<PartDesign::Line*>(axisObj);
        Base::Vector3d d = line->getDirection();
        Base::Vector3d b = line->getBasePoint();
        axisDir = gp_Dir(d.x, d.y, d.z);
        axisOrigin = gp_Pnt(b.x, b.y, b.z);
        originKnown = true;
    }
    else if (axisObj->isDerivedFrom<App::DatumElement>()) {
        // Covers body origin axes (App::Line -- X-Axis, Y-Axis, Z-Axis)
        auto* datum = static_cast<App::DatumElement*>(axisObj);
        Base::Vector3d d = datum->getDirection();
        Base::Vector3d b = datum->getBasePoint();
        axisDir = gp_Dir(d.x, d.y, d.z);
        axisOrigin = gp_Pnt(b.x, b.y, b.z);
        originKnown = true;
    }
    else if (axisObj->isDerivedFrom<Part::Feature>()) {
        const std::vector<std::string>& subs = Axis.getSubValues();
        if (subs.empty() || subs[0].empty()) {
            throw Base::ValueError("No sub-element selected for rotation axis");
        }
        auto* feat = static_cast<Part::Feature*>(axisObj);
        TopoDS_Shape sub = feat->Shape.getShape().getSubShape(subs[0].c_str());
        if (sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
            throw Base::TypeError("Axis reference sub-element must be an edge");
        }
        BRepAdaptor_Curve adapt(TopoDS::Edge(sub));
        if (adapt.GetType() != GeomAbs_Line) {
            throw Base::TypeError("Axis reference edge must be linear");
        }
        gp_Lin lin = adapt.Line();
        axisDir = lin.Direction();
        axisOrigin = lin.Location();
        originKnown = true;
    }
    else {
        throw Base::TypeError(
            "Rotation axis must be a linear edge, a datum line, or a body origin axis"
        );
    }

    // Apply the feature's inverse placement so the axis is in the feature's local frame.
    // (Matches the pattern used in FeatureDraft for pull-direction references.)
    TopLoc_Location invObjLoc = this->getLocation().Inverted();
    axisDir.Transform(invObjLoc.Transformation());
    if (originKnown) {
        axisOrigin.Transform(invObjLoc.Transformation());
    }

    outAxis = gp_Ax1(axisOrigin, axisDir);

    // -----------------------------------------------------------------------
    // Compute angle
    // -----------------------------------------------------------------------
    if (mode == 0) {
        // Axis and Angle
        outAngleRad = Base::toRadians(Angle.getValue());
    }
    else {
        // Axis and Two Elements: project both references onto the plane
        // perpendicular to the axis, then measure the signed angle.
        App::DocumentObject* ref1Obj = Ref1.getValue();
        App::DocumentObject* ref2Obj = Ref2.getValue();
        if (!ref1Obj || !ref2Obj) {
            throw Base::ValueError(
                "Both reference elements must be set for Axis and Two Elements mode"
            );
        }

        // Helper lambda: extract a direction vector from a PropertyLinkSub.
        // For a vertex: vector from axis origin to the projected point.
        // For an edge: the edge direction, projected onto the axis-normal plane.
        auto extractDir = [&](const App::PropertyLinkSub& prop, const char* name) -> gp_Vec {
            App::DocumentObject* obj = prop.getValue();
            const std::vector<std::string>& subs = prop.getSubValues();

            if (obj->isDerivedFrom<PartDesign::Point>()) {
                Base::Vector3d v = static_cast<PartDesign::Point*>(obj)->getPoint();
                gp_Pnt pt(v.x, v.y, v.z);
                pt.Transform(invObjLoc.Transformation());
                // Project onto plane normal to axis
                gp_Vec r(axisOrigin, pt);
                gp_Vec proj = r - gp_Vec(axisDir) * r.Dot(gp_Vec(axisDir));
                if (proj.Magnitude() < Precision::Confusion()) {
                    throw Base::ValueError(
                        std::string(name) + " projects onto the rotation axis (zero vector)"
                    );
                }
                return proj;
            }

            if (obj->isDerivedFrom<Part::Feature>() && !subs.empty() && !subs[0].empty()) {
                auto* feat = static_cast<Part::Feature*>(obj);
                TopoDS_Shape sub = feat->Shape.getShape().getSubShape(subs[0].c_str());
                sub.Move(invObjLoc);
                if (!sub.IsNull() && sub.ShapeType() == TopAbs_VERTEX) {
                    gp_Pnt pt = BRep_Tool::Pnt(TopoDS::Vertex(sub));
                    gp_Vec r(axisOrigin, pt);
                    gp_Vec proj = r - gp_Vec(axisDir) * r.Dot(gp_Vec(axisDir));
                    if (proj.Magnitude() < Precision::Confusion()) {
                        throw Base::ValueError(
                            std::string(name) + " projects onto the rotation axis (zero vector)"
                        );
                    }
                    return proj;
                }
                if (!sub.IsNull() && sub.ShapeType() == TopAbs_EDGE) {
                    BRepAdaptor_Curve adapt(TopoDS::Edge(sub));
                    if (adapt.GetType() != GeomAbs_Line) {
                        throw Base::TypeError(std::string(name) + " edge must be linear");
                    }
                    gp_Vec d(adapt.Line().Direction());
                    gp_Vec proj = d - gp_Vec(axisDir) * d.Dot(gp_Vec(axisDir));
                    if (proj.Magnitude() < Precision::Confusion()) {
                        throw Base::ValueError(
                            std::string(name) + " edge is parallel to rotation axis"
                        );
                    }
                    return proj;
                }
            }
            throw Base::TypeError(std::string(name) + " must be a vertex or linear edge");
        };

        gp_Vec dir1 = extractDir(Ref1, "Ref1");
        gp_Vec dir2 = extractDir(Ref2, "Ref2");
        outAngleRad = dir1.AngleWithRef(dir2, gp_Vec(axisDir));
    }
}

// ---------------------------------------------------------------------------
// execute()
// ---------------------------------------------------------------------------
App::DocumentObjectExecReturn* RotateBody::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    Part::TopoShape baseShape;
    try {
        baseShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    gp_Ax1 axis;
    double angleRad = 0.0;
    try {
        computeAxisAndAngle(axis, angleRad);
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    if (std::fabs(angleRad) < Precision::Angular()) {
        // Zero rotation -- just pass the shape through unchanged
        rawShape = baseShape;
        this->Shape.setValue(baseShape);
        return App::DocumentObject::StdReturn;
    }

    gp_Trsf trsf;
    trsf.SetRotation(axis, angleRad);

    Part::TopoShape result;
    try {
        result = baseShape.makeElementTransform(Part::TopoShape::convert(trsf));
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    if (result.isNull()) {
        return new App::DocumentObjectExecReturn("Resulting shape is null after rotation");
    }

    // Verify the result contains at least one solid
    TopExp_Explorer explorer(result.getShape(), TopAbs_SOLID);
    if (!explorer.More()) {
        return new App::DocumentObjectExecReturn(
            "Rotation produced no solid; check that the base shape is a solid body"
        );
    }

    rawShape = result;
    result = refineShapeIfActive(result);
    this->Shape.setValue(result);
    return App::DocumentObject::StdReturn;
}
