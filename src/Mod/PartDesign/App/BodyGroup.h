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

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{

/**
 * A display-only proxy object representing a virtual group within a PartDesign Body.
 *
 * BodyGroup objects are automatically created and managed by the Body. They serve
 * purely as display containers in the model tree, grouping related items (e.g.,
 * all Sketches or all Datum features) under labelled headers.
 *
 * Unlike App::DocumentObjectGroup, BodyGroup does NOT contain its members in a
 * Group property. The corresponding ViewProvider dynamically queries the parent
 * Body to compute which objects belong to this group. This means:
 *   - No redundant data storage
 *   - Automatic updates when features are added/removed
 *   - No user-facing group membership management needed
 *
 * These objects are analogous to App::Origin - they live in the document but are
 * purely infrastructural and cannot be independently managed by the user.
 */
class PartDesignExport BodyGroup: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BodyGroup);

public:
    /// Enumeration for the type of this group
    App::PropertyEnumeration GroupType;

    /// Back-reference to the parent Body (set by Body::setupObject())
    App::PropertyLink Body;

    BodyGroup();
    ~BodyGroup() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderBodyGroup";
    }

    short mustExecute() const override
    {
        return 0;
    }

    enum GroupTypeEnum
    {
        Sketches = 0,
        Datums = 1,
    };

    static const char* GroupTypeEnums[];
};

}  // namespace PartDesign
