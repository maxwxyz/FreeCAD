// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Max Wilfinger                                       *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <Standard_Failure.hxx>

#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Tool.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax3.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureScale.h"

using namespace PartDesign;

namespace PartDesign
{

const char* Scale::ModeEnums[] = {"Uniform", "XYZ", nullptr};

PROPERTY_SOURCE(PartDesign::Scale, PartDesign::FeatureRefine)

Scale::Scale()
{
    ADD_PROPERTY_TYPE(
        Reference,
        (nullptr),
        "Scale",
        (App::PropertyType)(App::Prop_None),
        "Reference point, plane, or face for scaling origin"
    );
    ADD_PROPERTY_TYPE(
        Mode,
        (long(0)),
        "Scale",
        (App::PropertyType)(App::Prop_None),
        "Uniform or per-axis scale mode"
    );
    Mode.setEnums(ModeEnums);

    ADD_PROPERTY_TYPE(
        ScaleFactor,
        (1.0f),
        "Scale",
        (App::PropertyType)(App::Prop_None),
        "Uniform scale factor"
    );
    ADD_PROPERTY_TYPE(
        ScaleX,
        (1.0f),
        "Scale",
        (App::PropertyType)(App::Prop_None),
        "Scale factor along X axis"
    );
    ADD_PROPERTY_TYPE(
        ScaleY,
        (1.0f),
        "Scale",
        (App::PropertyType)(App::Prop_None),
        "Scale factor along Y axis"
    );
    ADD_PROPERTY_TYPE(
        ScaleZ,
        (1.0f),
        "Scale",
        (App::PropertyType)(App::Prop_None),
        "Scale factor along Z axis"
    );
}

short Scale::mustExecute() const
{
    if (Reference.isTouched() || Mode.isTouched() || ScaleFactor.isTouched() || ScaleX.isTouched()
        || ScaleY.isTouched() || ScaleZ.isTouched()) {
        return 1;
    }
    return FeatureRefine::mustExecute();
}

bool Scale::getReference(gp_Pnt& pnt, gp_Ax3& ax3, bool& hasAxes) const
{
    hasAxes = false;

    App::DocumentObject* refObj = Reference.getValue();
    if (!refObj) {
        pnt = gp_Pnt(0, 0, 0);
        ax3 = gp_Ax3();
        hasAxes = true;
        return true;
    }

    std::vector<std::string> subs = Reference.getSubValues();

    // Whole-object reference: datum plane, datum point, or coordinate system
    if (subs.empty() || subs[0].empty()) {
        auto* geo = freecad_cast<App::GeoFeature*>(refObj);
        if (!geo) {
            return false;
        }
        Base::Placement plm = geo->Placement.getValue();
        Base::Vector3d pos = plm.getPosition();
        Base::Rotation rot = plm.getRotation();

        Base::Vector3d zDir = rot.multVec(Base::Vector3d(0, 0, 1));
        Base::Vector3d xDir = rot.multVec(Base::Vector3d(1, 0, 0));

        pnt = gp_Pnt(pos.x, pos.y, pos.z);
        ax3 = gp_Ax3(pnt, gp_Dir(zDir.x, zDir.y, zDir.z), gp_Dir(xDir.x, xDir.y, xDir.z));
        hasAxes = true;
        return true;
    }

    // Sub-element reference on a Part::Feature
    auto* feat = freecad_cast<Part::Feature*>(refObj);
    if (!feat) {
        return false;
    }

    Part::TopoShape refShape = feat->Shape.getShape();
    TopoDS_Shape sub;
    try {
        sub = refShape.getSubShape(subs[0].c_str());
    }
    catch (...) {
        return false;
    }

    if (sub.ShapeType() == TopAbs_VERTEX) {
        gp_Pnt vp = BRep_Tool::Pnt(TopoDS::Vertex(sub));
        pnt = vp;
        ax3 = gp_Ax3(vp, gp_Dir(0, 0, 1));
        hasAxes = false;
        return true;
    }

    if (sub.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface surf(TopoDS::Face(sub));
        if (surf.GetType() == GeomAbs_Plane) {
            gp_Pln pln = surf.Plane();
            gp_Ax3 faceAx = pln.Position();

            GProp_GProps props;
            BRepGProp::SurfaceProperties(sub, props);
            gp_Pnt centroid = props.CentreOfMass();

            ax3 = gp_Ax3(centroid, faceAx.Direction(), faceAx.XDirection());
            pnt = centroid;
            hasAxes = true;
            return true;
        }
    }

    return false;
}

App::DocumentObjectExecReturn* Scale::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    Part::TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    bool uniformMode = (Mode.getValue() == 0);

    if (uniformMode) {
        if (ScaleFactor.getValue() <= 0.0f) {
            return new App::DocumentObjectExecReturn("Scale factor must be greater than zero");
        }
    }
    else {
        if (ScaleX.getValue() <= 0.0f || ScaleY.getValue() <= 0.0f || ScaleZ.getValue() <= 0.0f) {
            return new App::DocumentObjectExecReturn("All scale factors must be greater than zero");
        }
    }

    gp_Pnt refP;
    gp_Ax3 refAx;
    bool hasAxes = false;
    if (!getReference(refP, refAx, hasAxes)) {
        return new App::DocumentObjectExecReturn("Could not resolve scale reference");
    }

    TopoDS_Shape result;

    if (uniformMode) {
        try {
            gp_Trsf trsf;
            trsf.SetScale(refP, static_cast<double>(ScaleFactor.getValue()));
            BRepBuilderAPI_Transform mkTrf(base.getShape(), trsf, true);
            result = mkTrf.Shape();
        }
        catch (const Standard_Failure& e) {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    else {
        // Use global axes at the reference point when only a point was picked
        gp_Ax3 ax = hasAxes ? refAx : gp_Ax3(refP, gp_Dir(0, 0, 1));

        try {
            // Transform to reference coordinate system
            gp_Trsf toRef;
            toRef.SetTransformation(ax);
            BRepBuilderAPI_Transform mkToRef(base.getShape(), toRef, true);
            TopoDS_Shape s1 = mkToRef.Shape();

            // Build diagonal scale matrix in reference frame
            Base::Matrix4D mat;
            mat.scale(
                static_cast<double>(ScaleX.getValue()),
                static_cast<double>(ScaleY.getValue()),
                static_cast<double>(ScaleZ.getValue())
            );

            gp_GTrsf gtrsf;
            gtrsf.SetValue(1, 1, mat[0][0]);
            gtrsf.SetValue(2, 1, mat[1][0]);
            gtrsf.SetValue(3, 1, mat[2][0]);
            gtrsf.SetValue(1, 2, mat[0][1]);
            gtrsf.SetValue(2, 2, mat[1][1]);
            gtrsf.SetValue(3, 2, mat[2][1]);
            gtrsf.SetValue(1, 3, mat[0][2]);
            gtrsf.SetValue(2, 3, mat[1][2]);
            gtrsf.SetValue(3, 3, mat[2][2]);

            // Copy step eliminates gp_GTrsf::Trsf() non-orthogonal errors (issue #9651)
            BRepBuilderAPI_Copy copier(s1);
            BRepBuilderAPI_GTransform mkG(copier.Shape(), gtrsf, true);
            TopoDS_Shape s2 = mkG.Shape();

            // Transform back to world space
            gp_Trsf back = toRef.Inverted();
            BRepBuilderAPI_Transform mkBack(s2, back, true);
            result = mkBack.Shape();
        }
        catch (const Standard_Failure& e) {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }

    // Enforce single-solid result (Body rule)
    int solidCount = 0;
    for (TopExp_Explorer exp(result, TopAbs_SOLID); exp.More(); exp.Next()) {
        ++solidCount;
    }
    if (solidCount != 1) {
        return new App::DocumentObjectExecReturn("Scale result must be exactly one solid");
    }

    rawShape = Part::TopoShape(result);
    this->Shape.setValue(refineShapeIfActive(rawShape));
    return App::DocumentObject::StdReturn;
}

void Scale::updatePreviewShape()
{
    PreviewShape.setValue(Shape.getShape());
}

}  // namespace PartDesign
