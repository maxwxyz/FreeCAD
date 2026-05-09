// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Max Wilfinger                                       *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <gp_Ax3.hxx>
#include <gp_Pnt.hxx>

#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>

#include "FeatureRefine.h"

namespace PartDesign
{

class PartDesignExport Scale: public FeatureRefine
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Scale);

public:
    Scale();

    // optional reference: vertex, planar face, datum point or datum plane
    App::PropertyLinkSub Reference;

    // "Uniform" or "XYZ"
    App::PropertyEnumeration Mode;

    // uniform scale factor (used when Mode == Uniform)
    App::PropertyFloat ScaleFactor;

    // per-axis factors (used when Mode == XYZ)
    App::PropertyFloat ScaleX;
    App::PropertyFloat ScaleY;
    App::PropertyFloat ScaleZ;

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void updatePreviewShape() override;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderScale";
    }

private:
    // Resolves the Reference property into a point and axis system.
    // hasAxes is set true when a full coordinate system (not just a point) is available.
    // Returns false if resolution fails.
    bool getReference(gp_Pnt& pnt, gp_Ax3& ax3, bool& hasAxes) const;

    static const char* ModeEnums[];
};

}  // namespace PartDesign
