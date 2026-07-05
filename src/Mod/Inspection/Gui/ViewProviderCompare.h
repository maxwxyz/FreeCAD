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

#pragma once

#include <Gui/ViewProviderDocumentObject.h>

class SoSeparator;

namespace Part
{
class PropertyPartShape;
}

namespace InspectionGui
{

class ViewProviderCompare: public Gui::ViewProviderDocumentObject
{
    using inherited = Gui::ViewProviderDocumentObject;

    PROPERTY_HEADER_WITH_OVERRIDE(InspectionGui::ViewProviderCompare);

public:
    ViewProviderCompare();
    ~ViewProviderCompare() override;

    void attach(App::DocumentObject* object) override;
    void updateData(const App::Property* property) override;
    void setDisplayMode(const char* modeName) override;
    std::vector<std::string> getDisplayModes() const override;
    QIcon getIcon() const override;

private:
    void rebuild();
    void addShape(const Part::PropertyPartShape* property, float r, float g, float b, float transparency);

private:
    SoSeparator* root;
};

}  // namespace InspectionGui
