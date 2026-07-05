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

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>

#include <Mod/Inspection/InspectionGlobal.h>
#include <Mod/Part/App/PropertyTopoShape.h>


namespace Inspection
{

class InspectionExport Compare: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Inspection::Compare);

public:
    Compare();
    ~Compare() override;

    App::PropertyLink Reference;
    App::PropertyLink Compared;
    App::PropertyFloat Tolerance;

    Part::PropertyPartShape CommonShape;
    Part::PropertyPartShape AddedShape;
    Part::PropertyPartShape RemovedShape;

    App::PropertyFloat ReferenceVolume;
    App::PropertyFloat ComparedVolume;
    App::PropertyFloat CommonVolume;
    App::PropertyFloat AddedVolume;
    App::PropertyFloat RemovedVolume;
    App::PropertyFloat ChangedVolume;
    App::PropertyFloat CommonArea;
    App::PropertyFloat AddedArea;
    App::PropertyFloat RemovedArea;
    App::PropertyInteger ChangedRegionCount;

    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "InspectionGui::ViewProviderCompare";
    }
};

}  // namespace Inspection
