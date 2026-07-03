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

#pragma once

#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Mod/Inspection/InspectionGlobal.h>
#include <Mod/Part/App/PartFeature.h>


namespace Inspection
{

enum class AnalysisElementClass : long
{
    None = 0,
    BelowBand = 1,
    InsideBand = 2,
    AboveBand = 3,
    Clash = 4,
    Contact = 5,
    Clearance = 6
};

class InspectionExport AnalysisFeature: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Inspection::AnalysisFeature);

public:
    AnalysisFeature();
    ~AnalysisFeature() override;

    App::PropertyIntegerList FaceClasses;
    App::PropertyIntegerList EdgeClasses;
    App::PropertyIntegerList PointClasses;
    App::PropertyFloatList Statistics;
    App::PropertyStringList ResultMessages;

    const char* getViewProviderName() const override
    {
        return "InspectionGui::ViewProviderAnalysis";
    }

    bool canRecomputeOnWorker() const override
    {
        return false;
    }

protected:
    void clearResultProperties();
};

class InspectionExport BandAnalysis: public AnalysisFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Inspection::BandAnalysis);

public:
    BandAnalysis();
    ~BandAnalysis() override;

    App::PropertyLink Source;
    App::PropertyLink Target;
    App::PropertyFloat MinimumDistance;
    App::PropertyFloat MaximumDistance;
    App::PropertyFloat Accuracy;

    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
};

class InspectionExport InterferenceAnalysis: public AnalysisFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Inspection::InterferenceAnalysis);

public:
    InterferenceAnalysis();
    ~InterferenceAnalysis() override;

    App::PropertyLinkList GroupA;
    App::PropertyLinkList GroupB;
    App::PropertyFloat Clearance;
    App::PropertyFloat ContactTolerance;
    App::PropertyFloat ClashTolerance;

    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace Inspection
