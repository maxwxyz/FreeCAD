// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2024 FreeCAD contributors                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "ViewProvider.h"

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderTranslate: public ViewProvider
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderTranslate)
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderTranslate);

public:
    ViewProviderTranslate()
    {
        sPixmap = "PartDesign_Translate.svg";
        menuName = tr("Translate Parameters");
    }

    // Not an override - ViewProvider base does not declare this virtual
    virtual const std::string& featureName() const;

    void setupContextMenu(QMenu*, QObject*, const char*) override;

    // Name shown in context menu and task panel header
    QString menuName;

protected:
    TaskDlgFeatureParameters* getEditDialog() override;
};

}  // namespace PartDesignGui
