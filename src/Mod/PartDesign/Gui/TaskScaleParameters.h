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

#include <memory>

#include <Gui/Selection/Selection.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderScale.h"

class Ui_TaskScaleParameters;

namespace PartDesignGui
{

class TaskScaleParameters: public TaskFeatureParameters, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskScaleParameters(ViewProviderScale* vp, QWidget* parent = nullptr);
    ~TaskScaleParameters() override;

    void apply() override;

private Q_SLOTS:
    void onModeChanged(int index);
    void onScaleFactorChanged(double val);
    void onScaleXChanged(double val);
    void onScaleYChanged(double val);
    void onScaleZChanged(double val);
    void onReferenceButtonToggled(bool checked);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateModeWidgets(int mode);
    void updateReferenceLabel();
    void setupConnections();
    void syncFromFeature();

    std::unique_ptr<Ui_TaskScaleParameters> ui;
    bool selectingReference = false;
};

class TaskDlgScaleParameters: public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgScaleParameters(ViewProviderScale* vp);
    ~TaskDlgScaleParameters() override = default;
};

}  // namespace PartDesignGui
