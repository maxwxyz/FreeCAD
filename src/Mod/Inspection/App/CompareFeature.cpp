// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 The FreeCAD project association AISBL              *
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
 **************************************************************************/

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Mod/Part/App/PartFeature.h>

#include "CompareFeature.h"


using namespace Inspection;

namespace
{

struct ShapeMetrics
{
    double volume {0.0};
    double area {0.0};
    int regions {0};
};

bool isSupportedObject(const App::DocumentObject* object)
{
    return object && object->isDerivedFrom<Part::Feature>();
}

const TopoDS_Shape& shapeOf(const App::DocumentObject* object)
{
    if (!isSupportedObject(object)) {
        throw Base::TypeError("Inspection compare currently supports Part geometry only");
    }

    const auto* part = static_cast<const Part::Feature*>(object);
    const TopoDS_Shape& shape = part->Shape.getValue();
    if (shape.IsNull()) {
        throw Base::ValueError("Cannot compare an empty shape");
    }
    return shape;
}

double operationTolerance(double userTolerance)
{
    // Negative tolerance asks the Part boolean wrapper to use FreeCAD's auto-fuzzy policy.
    return userTolerance > 0.0 ? userTolerance : -1.0;
}

ShapeMetrics metrics(const TopoDS_Shape& shape)
{
    ShapeMetrics result;
    if (shape.IsNull()) {
        return result;
    }

    GProp_GProps volumeProps;
    BRepGProp::VolumeProperties(shape, volumeProps);
    result.volume = volumeProps.Mass();

    GProp_GProps surfaceProps;
    BRepGProp::SurfaceProperties(shape, surfaceProps);
    result.area = surfaceProps.Mass();

    TopTools_IndexedMapOfShape solids;
    TopExp::MapShapes(shape, TopAbs_SOLID, solids);
    if (!solids.IsEmpty()) {
        result.regions = solids.Extent();
        return result;
    }

    TopTools_IndexedMapOfShape faces;
    TopExp::MapShapes(shape, TopAbs_FACE, faces);
    result.regions = faces.Extent();
    return result;
}

TopoDS_Shape cutShape(const Part::TopoShape& shape, const TopoDS_Shape& tool, double tolerance)
{
    return shape.cut(std::vector<TopoDS_Shape> {tool}, tolerance);
}

TopoDS_Shape commonShape(const Part::TopoShape& shape, const TopoDS_Shape& tool, double tolerance)
{
    return shape.common(std::vector<TopoDS_Shape> {tool}, tolerance);
}

}  // namespace


PROPERTY_SOURCE(Inspection::Compare, App::DocumentObject)

Compare::Compare()
{
    ADD_PROPERTY(Reference, (nullptr));
    ADD_PROPERTY(Compared, (nullptr));
    ADD_PROPERTY(Tolerance, (0.0));

    ADD_PROPERTY_TYPE(CommonShape, (TopoDS_Shape()), "Result", App::Prop_Output, "Common material");
    ADD_PROPERTY_TYPE(AddedShape, (TopoDS_Shape()), "Result", App::Prop_Output, "Added material");
    ADD_PROPERTY_TYPE(RemovedShape, (TopoDS_Shape()), "Result", App::Prop_Output, "Removed material");

    ADD_PROPERTY_TYPE(ReferenceVolume, (0.0), "Statistics", App::Prop_Output, "Reference volume");
    ADD_PROPERTY_TYPE(ComparedVolume, (0.0), "Statistics", App::Prop_Output, "Compared volume");
    ADD_PROPERTY_TYPE(CommonVolume, (0.0), "Statistics", App::Prop_Output, "Common volume");
    ADD_PROPERTY_TYPE(AddedVolume, (0.0), "Statistics", App::Prop_Output, "Added material volume");
    ADD_PROPERTY_TYPE(RemovedVolume, (0.0), "Statistics", App::Prop_Output, "Removed material volume");
    ADD_PROPERTY_TYPE(ChangedVolume, (0.0), "Statistics", App::Prop_Output, "Added plus removed volume");
    ADD_PROPERTY_TYPE(CommonArea, (0.0), "Statistics", App::Prop_Output, "Common material surface area");
    ADD_PROPERTY_TYPE(AddedArea, (0.0), "Statistics", App::Prop_Output, "Added material surface area");
    ADD_PROPERTY_TYPE(RemovedArea, (0.0), "Statistics", App::Prop_Output, "Removed material surface area");
    ADD_PROPERTY_TYPE(ChangedRegionCount, (0), "Statistics", App::Prop_Output, "Changed region count");
}

Compare::~Compare() = default;

short Compare::mustExecute() const
{
    if (Reference.isTouched() || Compared.isTouched() || Tolerance.isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Compare::execute()
{
    App::DocumentObject* referenceObject = Reference.getValue();
    App::DocumentObject* comparedObject = Compared.getValue();

    if (!referenceObject) {
        throw Base::ValueError("No reference object specified");
    }
    if (!comparedObject) {
        throw Base::ValueError("No compared object specified");
    }
    if (referenceObject == comparedObject) {
        throw Base::ValueError("Reference and compared object must be different");
    }

    const TopoDS_Shape& referenceShape = shapeOf(referenceObject);
    const TopoDS_Shape& comparedShape = shapeOf(comparedObject);
    const auto* referencePart = static_cast<const Part::Feature*>(referenceObject);
    const auto* comparedPart = static_cast<const Part::Feature*>(comparedObject);
    const double tolerance = operationTolerance(Tolerance.getValue());

    TopoDS_Shape removed = cutShape(referencePart->Shape.getShape(), comparedShape, tolerance);
    TopoDS_Shape added = cutShape(comparedPart->Shape.getShape(), referenceShape, tolerance);
    TopoDS_Shape common = commonShape(referencePart->Shape.getShape(), comparedShape, tolerance);

    RemovedShape.setValue(removed);
    AddedShape.setValue(added);
    CommonShape.setValue(common);

    const ShapeMetrics referenceMetrics = metrics(referenceShape);
    const ShapeMetrics comparedMetrics = metrics(comparedShape);
    const ShapeMetrics removedMetrics = metrics(removed);
    const ShapeMetrics addedMetrics = metrics(added);
    const ShapeMetrics commonMetrics = metrics(common);

    ReferenceVolume.setValue(referenceMetrics.volume);
    ComparedVolume.setValue(comparedMetrics.volume);
    RemovedVolume.setValue(removedMetrics.volume);
    AddedVolume.setValue(addedMetrics.volume);
    CommonVolume.setValue(commonMetrics.volume);
    ChangedVolume.setValue(removedMetrics.volume + addedMetrics.volume);
    RemovedArea.setValue(removedMetrics.area);
    AddedArea.setValue(addedMetrics.area);
    CommonArea.setValue(commonMetrics.area);
    ChangedRegionCount.setValue(removedMetrics.regions + addedMetrics.regions);

    Base::Console().message(
        "Compared '%s' to '%s': added volume %.6g, removed volume %.6g\n",
        comparedObject->Label.getValue(),
        referenceObject->Label.getValue(),
        addedMetrics.volume,
        removedMetrics.volume
    );

    return nullptr;
}
