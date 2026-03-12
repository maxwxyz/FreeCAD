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

#include "BodyGroup.h"

using namespace PartDesign;

const char* BodyGroup::GroupTypeEnums[] = {"Sketches", "Datums", nullptr};

PROPERTY_SOURCE(PartDesign::BodyGroup, App::DocumentObject)

BodyGroup::BodyGroup()
{
    ADD_PROPERTY_TYPE(
        GroupType,
        (0L),
        "Base",
        (App::PropertyType)(App::Prop_ReadOnly | App::Prop_Hidden),
        "Type of this body display group"
    );
    GroupType.setEnums(GroupTypeEnums);

    ADD_PROPERTY_TYPE(
        Body,
        (nullptr),
        "Base",
        (App::PropertyType)(App::Prop_ReadOnly | App::Prop_Hidden),
        "Parent body of this group"
    );
}
