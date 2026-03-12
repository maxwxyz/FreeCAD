// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2024 FreeCAD Project Association                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <App/PropertyStandard.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/PartDesign/PartDesignGlobal.h>
#include <QCoreApplication>

namespace PartDesignGui
{

/**
 * ViewProvider for PartDesign::BodyGroup.
 *
 * Displays a virtual "Sketches" or "Datums" group under a PartDesign Body in
 * the model tree. Children are computed dynamically by scanning the parent
 * Body's feature list — no Group property is used, so there is no redundant
 * data storage.
 *
 * A hidden integer property (_Revision) is incremented by the Body's
 * ViewProvider whenever the Body's Group changes, so the tree widget detects
 * the change and re-queries claimChildren().
 */
class PartDesignGuiExport ViewProviderBodyGroup: public Gui::ViewProviderDocumentObject
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderBodyGroup)
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesignGui::ViewProviderBodyGroup);

public:
    /// Internal revision counter bumped by ViewProviderBody when Group changes.
    /// The tree widget uses VP property changes to detect child list updates.
    App::PropertyInteger _Revision;

    ViewProviderBodyGroup();
    ~ViewProviderBodyGroup() override;

    /// Dynamically compute which objects belong to this group from the parent Body.
    std::vector<App::DocumentObject*> claimChildren() const override;

    /// Show appropriate icon based on group type (Sketches vs. Datums).
    QIcon getIcon() const override;

    /// Prevent users from deleting this structural object.
    bool onDelete(const std::vector<std::string>&) override
    {
        return false;
    }

    /// This proxy is always conceptually "shown" (it has no geometry of its own).
    bool isShow() const override
    {
        return false;
    }
};

}  // namespace PartDesignGui
