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

#include <Gui/Selection/Selection.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderRotateBody.h"

class Ui_TaskRotateBodyParameters;

namespace PartDesign
{
class RotateBody;
}

namespace PartDesignGui
{

class TaskRotateBodyParameters: public TaskFeatureParameters, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskRotateBodyParameters(ViewProviderRotateBody* vp, QWidget* parent = nullptr);
    ~TaskRotateBodyParameters() override;

    void apply() override;

private Q_SLOTS:
    void onModeChanged(int index);
    void onButtonAxis(bool checked);
    void onButtonAxis2(bool checked);
    void onButtonRef1(bool checked);
    void onButtonRef2(bool checked);
    void onButtonPoint1(bool checked);
    void onButtonPoint2(bool checked);
    void onButtonPoint3(bool checked);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void changeEvent(QEvent* e) override;

private:
    enum class ActiveButton
    {
        None,
        Axis,   // page 0
        Axis2,  // page 1 axis
        Ref1,
        Ref2,
        Point1,
        Point2,
        Point3
    };

    void initUI();
    void stopSelecting();
    void openTransactionIfNeeded();
    static QString refText(App::DocumentObject* obj, const std::vector<std::string>& subs);

    std::unique_ptr<Ui_TaskRotateBodyParameters> ui;
    ActiveButton activeButton {ActiveButton::None};
    int transactionID {0};
};

class TaskDlgRotateBodyParameters: public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgRotateBodyParameters(ViewProviderRotateBody* vp);
    ~TaskDlgRotateBodyParameters() override;

    bool accept() override;

private:
    TaskRotateBodyParameters* parameter;
};

}  // namespace PartDesignGui
