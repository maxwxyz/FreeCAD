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

#include <memory>

#include <Gui/Selection/Selection.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderTranslate.h"

class Ui_TaskTranslateParameters;

namespace PartDesignGui
{

class TaskTranslateParameters: public TaskFeatureParameters, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskTranslateParameters(ViewProviderTranslate* vp, QWidget* parent = nullptr);
    ~TaskTranslateParameters() override;

    void apply() override;

private Q_SLOTS:
    void onModeChanged(int index);
    void onDirectionRefToggled(bool checked);
    void onStartPointToggled(bool checked);
    void onEndPointToggled(bool checked);
    void onAxisSystemToggled(bool checked);
    void onDirectionChanged();
    void onDistanceChanged(double val);
    void onCoordChanged();

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateUI();
    void stopSelectionMode();
    void setupTransaction();
    void setDirectionRefLabel(const QString& text);
    void setStartPointLabel(const QString& text);
    void setEndPointLabel(const QString& text);
    void setAxisSystemLabel(const QString& text);

    enum class SelectionMode
    {
        None,
        DirectionRef,
        StartPoint,
        EndPoint,
        AxisSystem
    };

    SelectionMode selMode = SelectionMode::None;
    std::unique_ptr<Ui_TaskTranslateParameters> ui;
    QWidget* proxy = nullptr;
};

class TaskDlgTranslateParameters: public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgTranslateParameters(ViewProviderTranslate* vp);

    bool accept() override;
};

}  // namespace PartDesignGui
