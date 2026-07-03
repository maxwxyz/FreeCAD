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

#include "ViewProviderAnalysis.h"

#include <Gui/BitmapFactory.h>
#include <Mod/Inspection/App/Analysis.h>


using namespace InspectionGui;

namespace
{

Base::Color classColor(long klass)
{
    switch (static_cast<Inspection::AnalysisElementClass>(klass)) {
        case Inspection::AnalysisElementClass::BelowBand:
        case Inspection::AnalysisElementClass::Clash:
            return Base::Color(0.9F, 0.1F, 0.08F);
        case Inspection::AnalysisElementClass::InsideBand:
            return Base::Color(0.05F, 0.65F, 0.18F);
        case Inspection::AnalysisElementClass::AboveBand:
            return Base::Color(0.12F, 0.35F, 0.95F);
        case Inspection::AnalysisElementClass::Contact:
            return Base::Color(1.0F, 0.78F, 0.08F);
        case Inspection::AnalysisElementClass::Clearance:
            return Base::Color(0.0F, 0.7F, 0.85F);
        default:
            return Base::Color(0.8F, 0.8F, 0.8F);
    }
}

std::vector<Base::Color> colorsFromClasses(const App::PropertyIntegerList& classes)
{
    std::vector<Base::Color> colors;
    colors.reserve(classes.getValues().size());
    for (long klass : classes.getValues()) {
        colors.push_back(classColor(klass));
    }
    return colors;
}

}  // namespace


PROPERTY_SOURCE(InspectionGui::ViewProviderAnalysis, PartGui::ViewProviderPart)

ViewProviderAnalysis::ViewProviderAnalysis()
{
    sPixmap = "Inspection_Analysis";
}

ViewProviderAnalysis::~ViewProviderAnalysis() = default;

void ViewProviderAnalysis::attach(App::DocumentObject* object)
{
    PartGui::ViewProviderPart::attach(object);
    applyAnalysisColors();
}

void ViewProviderAnalysis::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (!applyingColors) {
        applyAnalysisColors();
    }
}

void ViewProviderAnalysis::finishRestoring()
{
    PartGui::ViewProviderPart::finishRestoring();
    applyAnalysisColors();
}

QIcon ViewProviderAnalysis::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Inspection_Analysis");
}

void ViewProviderAnalysis::applyAnalysisColors()
{
    auto* analysis = freecad_cast<Inspection::AnalysisFeature*>(getObject());
    if (!analysis || applyingColors) {
        return;
    }

    applyingColors = true;
    ShapeAppearance.setDiffuseColors(colorsFromClasses(analysis->FaceClasses));
    LineColorArray.setValues(colorsFromClasses(analysis->EdgeClasses));
    PointColorArray.setValues(colorsFromClasses(analysis->PointClasses));
    applyingColors = false;
}
