// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2025 FreeCAD Project                                    *
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
 ***************************************************************************/

#pragma once

#include <gp_Ax1.hxx>

#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include "FeatureRefine.h"

namespace PartDesign
{

class PartDesignExport RotateBody: public FeatureRefine
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::RotateBody);

public:
    RotateBody();

    // "Axis and Angle" | "Axis and Two Elements" | "Three Points"
    App::PropertyEnumeration Mode;

    // Axis reference: linear edge, PartDesign::Line, or App::Line
    App::PropertyLinkSub Axis;
    // Rotation angle in degrees (Axis and Angle mode only)
    App::PropertyAngle Angle;

    // Two reference geometries whose projected angle defines the rotation
    App::PropertyLinkSub Ref1;
    App::PropertyLinkSub Ref2;

    // Three points: Point2 is the pivot; axis = normal of the plane they define
    App::PropertyLinkSub Point1;
    App::PropertyLinkSub Point2;
    App::PropertyLinkSub Point3;

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRotateBody";
    }

private:
    static const char* ModeEnums[];

    // Extracts axis from the Axis property (branches: PartDesign::Line, App::Line, edge).
    // For ThreePoints mode, outAxis is set from the three point properties and angleRad is
    // computed as well -- both outAxis and outAngle are set by this overload.
    void computeAxisAndAngle(gp_Ax1& outAxis, double& outAngleRad) const;

    // Extracts a 3-D point from a PropertyLinkSub (vertex or datum point).
    static gp_Pnt extractPoint(const App::PropertyLinkSub& prop, const char* propName);
};

}  // namespace PartDesign
