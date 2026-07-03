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

#include <Mod/Part/Gui/ViewProvider.h>


namespace InspectionGui
{

class ViewProviderAnalysis: public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(InspectionGui::ViewProviderAnalysis);

public:
    ViewProviderAnalysis();
    ~ViewProviderAnalysis() override;

    void attach(App::DocumentObject* object) override;
    void updateData(const App::Property* prop) override;
    void finishRestoring() override;
    QIcon getIcon() const override;

private:
    void applyAnalysisColors();

private:
    bool applyingColors {false};
};

}  // namespace InspectionGui
