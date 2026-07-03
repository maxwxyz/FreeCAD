// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 **************************************************************************/

#include "Analysis.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
#include <Mod/Part/App/TopoShape.h>


using namespace Inspection;

namespace
{

constexpr long classId(AnalysisElementClass klass)
{
    return static_cast<long>(klass);
}

TopoDS_Shape getShape(App::DocumentObject* obj)
{
    if (!obj) {
        return {};
    }

    TopoDS_Shape shape
        = Part::Feature::getShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (!shape.IsNull()) {
        return shape;
    }

    auto* tip = freecad_cast<App::PropertyLink*>(obj->getPropertyByName("Tip"));
    if (tip && tip->getValue() && tip->getValue() != obj) {
        return Part::Feature::getShape(
            tip->getValue(),
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
    }

    return {};
}

TopoDS_Face makeTriangleFace(const Base::Vector3d& p1, const Base::Vector3d& p2, const Base::Vector3d& p3)
{
    BRepBuilderAPI_MakePolygon polygon;
    polygon.Add(gp_Pnt(p1.x, p1.y, p1.z));
    polygon.Add(gp_Pnt(p2.x, p2.y, p2.z));
    polygon.Add(gp_Pnt(p3.x, p3.y, p3.z));
    polygon.Close();
    if (!polygon.IsDone()) {
        return {};
    }

    BRepBuilderAPI_MakeFace face(polygon.Wire());
    if (!face.IsDone()) {
        return {};
    }
    return face.Face();
}

double pointDistance(const TopoDS_Shape& target, const Base::Vector3d& point)
{
    BRepBuilderAPI_MakeVertex vertex(gp_Pnt(point.x, point.y, point.z));
    BRepExtrema_DistShapeShape distance(target, vertex.Vertex());
    if (!distance.IsDone()) {
        return std::numeric_limits<double>::max();
    }
    if (distance.NbSolution() == 0) {
        return std::numeric_limits<double>::max();
    }
    return distance.Value();
}

TopoDS_Shape emptyCompound()
{
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    return compound;
}

std::size_t addShape(
    BRep_Builder& builder,
    TopoDS_Compound& compound,
    const TopoDS_Shape& shape,
    AnalysisElementClass klass,
    std::vector<long>& faceClasses,
    std::vector<long>& edgeClasses,
    std::vector<long>& pointClasses
)
{
    if (shape.IsNull()) {
        return 0;
    }

    builder.Add(compound, shape);

    std::size_t elementCount = 0;
    for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
        faceClasses.push_back(classId(klass));
        ++elementCount;
    }
    for (TopExp_Explorer it(shape, TopAbs_EDGE); it.More(); it.Next()) {
        edgeClasses.push_back(classId(klass));
        ++elementCount;
    }
    for (TopExp_Explorer it(shape, TopAbs_VERTEX); it.More(); it.Next()) {
        pointClasses.push_back(classId(klass));
        ++elementCount;
    }

    return elementCount;
}

double shapeVolume(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return 0.0;
    }
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return std::abs(props.Mass());
}

std::string objectLabel(App::DocumentObject* obj)
{
    if (!obj) {
        return {};
    }
    const char* label = obj->Label.getValue();
    if (label && label[0]) {
        return label;
    }
    return obj->getNameInDocument();
}

std::vector<App::DocumentObject*> validObjects(const std::vector<App::DocumentObject*>& objects)
{
    std::vector<App::DocumentObject*> result;
    result.reserve(objects.size());
    for (auto* obj : objects) {
        if (!getShape(obj).IsNull()) {
            result.push_back(obj);
        }
    }
    return result;
}

}  // namespace


PROPERTY_SOURCE(Inspection::AnalysisFeature, Part::Feature)

AnalysisFeature::AnalysisFeature()
{
    ADD_PROPERTY(FaceClasses, ());
    ADD_PROPERTY(EdgeClasses, ());
    ADD_PROPERTY(PointClasses, ());
    ADD_PROPERTY(Statistics, ());
    ADD_PROPERTY(ResultMessages, ());
}

AnalysisFeature::~AnalysisFeature() = default;

void AnalysisFeature::clearResultProperties()
{
    FaceClasses.setValues();
    EdgeClasses.setValues();
    PointClasses.setValues();
    Statistics.setValues();
    ResultMessages.setValues();
    Shape.setValue(emptyCompound(), true);
}


PROPERTY_SOURCE(Inspection::BandAnalysis, Inspection::AnalysisFeature)

BandAnalysis::BandAnalysis()
{
    ADD_PROPERTY(Source, (nullptr));
    ADD_PROPERTY(Target, (nullptr));
    ADD_PROPERTY(MinimumDistance, (0.0));
    ADD_PROPERTY(MaximumDistance, (1.0));
    ADD_PROPERTY(Accuracy, (0.1));
}

BandAnalysis::~BandAnalysis() = default;

short BandAnalysis::mustExecute() const
{
    if (Source.isTouched() || Target.isTouched() || MinimumDistance.isTouched()
        || MaximumDistance.isTouched() || Accuracy.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* BandAnalysis::execute()
{
    auto* sourceObject = Source.getValue();
    auto* targetObject = Target.getValue();
    if (!sourceObject || !targetObject) {
        throw Base::ValueError("Band analysis requires a source and a target object");
    }

    const TopoDS_Shape sourceShape = ::getShape(sourceObject);
    const TopoDS_Shape targetShape = ::getShape(targetObject);
    if (sourceShape.IsNull() || targetShape.IsNull()) {
        throw Base::ValueError("Band analysis input does not provide valid Part geometry");
    }

    const double minDistance = MinimumDistance.getValue();
    const double maxDistance = MaximumDistance.getValue();
    const double accuracy = std::max(Accuracy.getValue(), Precision::Confusion());
    if (maxDistance < minDistance) {
        throw Base::ValueError("Band analysis maximum distance must be greater than minimum distance");
    }

    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> facets;
    Part::TopoShape(sourceShape).getFaces(points, facets, accuracy);

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    std::vector<long> faceClasses;
    faceClasses.reserve(facets.size());

    std::size_t belowCount = 0;
    std::size_t insideCount = 0;
    std::size_t aboveCount = 0;
    double minimumObserved = std::numeric_limits<double>::max();
    double maximumObserved = 0.0;

    for (const auto& facet : facets) {
        const auto& p1 = points[facet.I1];
        const auto& p2 = points[facet.I2];
        const auto& p3 = points[facet.I3];
        const Base::Vector3d centroid(
            (p1.x + p2.x + p3.x) / 3.0,
            (p1.y + p2.y + p3.y) / 3.0,
            (p1.z + p2.z + p3.z) / 3.0
        );

        const double distance = pointDistance(targetShape, centroid);
        minimumObserved = std::min(minimumObserved, distance);
        if (distance < std::numeric_limits<double>::max()) {
            maximumObserved = std::max(maximumObserved, distance);
        }

        AnalysisElementClass klass = AnalysisElementClass::AboveBand;
        if (distance < minDistance) {
            klass = AnalysisElementClass::BelowBand;
            ++belowCount;
        }
        else if (distance <= maxDistance) {
            klass = AnalysisElementClass::InsideBand;
            ++insideCount;
        }
        else {
            ++aboveCount;
        }

        const TopoDS_Face face = makeTriangleFace(p1, p2, p3);
        if (!face.IsNull()) {
            builder.Add(compound, face);
            faceClasses.push_back(classId(klass));
        }
    }

    Shape.setValue(compound, true);
    FaceClasses.setValues(faceClasses);
    EdgeClasses.setValues();
    PointClasses.setValues();
    Statistics.setValues(
        std::vector<double> {
            static_cast<double>(belowCount),
            static_cast<double>(insideCount),
            static_cast<double>(aboveCount),
            minimumObserved == std::numeric_limits<double>::max() ? 0.0 : minimumObserved,
            maximumObserved,
        }
    );

    std::vector<std::string> messages;
    std::ostringstream summary;
    summary << "Band: below=" << belowCount << ", inside=" << insideCount << ", above=" << aboveCount;
    messages.push_back(summary.str());
    ResultMessages.setValues(messages);

    return Part::Feature::execute();
}


PROPERTY_SOURCE(Inspection::InterferenceAnalysis, Inspection::AnalysisFeature)

InterferenceAnalysis::InterferenceAnalysis()
{
    ADD_PROPERTY(GroupA, ());
    ADD_PROPERTY(GroupB, ());
    ADD_PROPERTY(Clearance, (1.0));
    ADD_PROPERTY(ContactTolerance, (0.001));
    ADD_PROPERTY(ClashTolerance, (0.0));
}

InterferenceAnalysis::~InterferenceAnalysis() = default;

short InterferenceAnalysis::mustExecute() const
{
    if (GroupA.isTouched() || GroupB.isTouched() || Clearance.isTouched()
        || ContactTolerance.isTouched() || ClashTolerance.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* InterferenceAnalysis::execute()
{
    const std::vector<App::DocumentObject*> groupA = validObjects(GroupA.getValues());
    const std::vector<App::DocumentObject*> groupB = validObjects(GroupB.getValues());

    if (groupA.empty()) {
        throw Base::ValueError("Interference analysis requires at least one object in GroupA");
    }

    const bool compareInsideA = groupB.empty();
    const double clearance = std::max(0.0, Clearance.getValue());
    const double contactTolerance = std::max(0.0, ContactTolerance.getValue());
    const double clashTolerance = std::max(0.0, ClashTolerance.getValue());

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    std::vector<long> faceClasses;
    std::vector<long> edgeClasses;
    std::vector<long> pointClasses;
    std::vector<std::string> messages;

    std::size_t clashCount = 0;
    std::size_t contactCount = 0;
    std::size_t clearanceCount = 0;
    double minimumDistance = std::numeric_limits<double>::max();

    auto evaluatePair = [&](App::DocumentObject* objA, App::DocumentObject* objB) {
        const TopoDS_Shape shapeA = ::getShape(objA);
        const TopoDS_Shape shapeB = ::getShape(objB);
        if (shapeA.IsNull() || shapeB.IsNull()) {
            return;
        }

        FCBRepAlgoAPI_Common common(shapeA, shapeB);
        TopoDS_Shape commonShape;
        double overlap = 0.0;
        if (common.IsDone()) {
            commonShape = common.Shape();
            overlap = shapeVolume(commonShape);
        }

        if (overlap > clashTolerance) {
            ++clashCount;
            addShape(
                builder,
                compound,
                commonShape,
                AnalysisElementClass::Clash,
                faceClasses,
                edgeClasses,
                pointClasses
            );

            std::ostringstream line;
            line << "Clash: " << objectLabel(objA) << " / " << objectLabel(objB)
                 << " volume=" << overlap;
            messages.push_back(line.str());
            minimumDistance = 0.0;
            return;
        }

        BRepExtrema_DistShapeShape distance(shapeA, shapeB);
        if (!distance.IsDone()) {
            return;
        }
        if (distance.NbSolution() == 0) {
            return;
        }

        const double value = distance.Value();
        minimumDistance = std::min(minimumDistance, value);
        AnalysisElementClass klass = AnalysisElementClass::None;
        if (value <= contactTolerance) {
            klass = AnalysisElementClass::Contact;
            ++contactCount;
        }
        else if (value <= clearance) {
            klass = AnalysisElementClass::Clearance;
            ++clearanceCount;
        }

        if (klass == AnalysisElementClass::None) {
            return;
        }

        const gp_Pnt p1 = distance.PointOnShape1(1);
        const gp_Pnt p2 = distance.PointOnShape2(1);
        TopoDS_Shape marker;
        if (p1.Distance(p2) > Precision::Confusion()) {
            BRepBuilderAPI_MakeEdge edge(p1, p2);
            if (edge.IsDone()) {
                marker = edge.Edge();
            }
        }
        if (marker.IsNull()) {
            marker = BRepBuilderAPI_MakeVertex(p1).Vertex();
        }

        addShape(builder, compound, marker, klass, faceClasses, edgeClasses, pointClasses);

        std::ostringstream line;
        line << (klass == AnalysisElementClass::Contact ? "Contact: " : "Clearance: ")
             << objectLabel(objA) << " / " << objectLabel(objB) << " distance=" << value;
        messages.push_back(line.str());
    };

    if (compareInsideA) {
        for (std::size_t i = 0; i < groupA.size(); ++i) {
            for (std::size_t j = i + 1; j < groupA.size(); ++j) {
                evaluatePair(groupA[i], groupA[j]);
            }
        }
    }
    else {
        for (auto* objA : groupA) {
            for (auto* objB : groupB) {
                if (objA != objB) {
                    evaluatePair(objA, objB);
                }
            }
        }
    }

    Shape.setValue(compound, true);
    FaceClasses.setValues(faceClasses);
    EdgeClasses.setValues(edgeClasses);
    PointClasses.setValues(pointClasses);
    Statistics.setValues(
        std::vector<double> {
            static_cast<double>(clashCount),
            static_cast<double>(contactCount),
            static_cast<double>(clearanceCount),
            minimumDistance == std::numeric_limits<double>::max() ? 0.0 : minimumDistance,
        }
    );

    if (messages.empty()) {
        messages.emplace_back("No clash, contact, or clearance result found");
    }
    ResultMessages.setValues(messages);

    return Part::Feature::execute();
}
