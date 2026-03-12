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

#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/BodyGroup.h>
#include <Mod/PartDesign/App/Feature.h>

#include "ViewProviderBodyGroup.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderBodyGroup, Gui::ViewProviderDocumentObject)

ViewProviderBodyGroup::ViewProviderBodyGroup()
{
    // _Revision is a hidden counter; the tree widget picks up its change to
    // know that this proxy's children may have changed.
    ADD_PROPERTY_TYPE(
        _Revision,
        (0),
        "Base",
        (App::PropertyType)(App::Prop_Hidden | App::Prop_Output | App::Prop_NoRecompute),
        "Internal revision counter; do not modify"
    );

    // Use a simple folder icon by default; getIcon() will refine it.
    sPixmap = "folder.svg";
}

ViewProviderBodyGroup::~ViewProviderBodyGroup() = default;

std::vector<App::DocumentObject*> ViewProviderBodyGroup::claimChildren() const
{
    auto* groupObj = static_cast<PartDesign::BodyGroup*>(getObject());
    auto* body = dynamic_cast<PartDesign::Body*>(groupObj->Body.getValue());
    if (!body) {
        return {};
    }

    const bool isSketchesGroup =
        (groupObj->GroupType.getValue() == static_cast<long>(PartDesign::BodyGroup::Sketches));

    std::vector<App::DocumentObject*> result;
    for (auto* obj : body->Group.getValues()) {
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }

        if (isSketchesGroup) {
            // Sketches are objects derived from Part::Part2DObject
            // (which includes Sketcher::SketchObject)
            if (obj->isDerivedFrom<Part::Part2DObject>()) {
                result.push_back(obj);
            }
        }
        else {
            // Datums: PartDesign datum features (plane, point, line, CS …)
            // Feature::isDatum() covers Part::Datum and App::DatumElement
            if (PartDesign::Feature::isDatum(obj)) {
                result.push_back(obj);
            }
        }
    }
    return result;
}

QIcon ViewProviderBodyGroup::getIcon() const
{
    auto* groupObj = static_cast<PartDesign::BodyGroup*>(getObject());
    const bool isSketchesGroup =
        (groupObj->GroupType.getValue() == static_cast<long>(PartDesign::BodyGroup::Sketches));

    if (isSketchesGroup) {
        return Gui::BitmapFactory().pixmap("Sketcher_Sketch.svg");
    }
    else {
        return Gui::BitmapFactory().pixmap("PartDesign_Point.svg");
    }
}
