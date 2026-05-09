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

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Selection/Selection.h>
#include <Mod/PartDesign/App/FeatureScale.h>

#include "ui_TaskScaleParameters.h"
#include "TaskScaleParameters.h"

using namespace PartDesignGui;

/* TRANSLATOR PartDesignGui::TaskScaleParameters */

TaskScaleParameters::TaskScaleParameters(ViewProviderScale* vp, QWidget* parent)
    : TaskFeatureParameters(vp, parent, "PartDesign_Scale", tr("Scale Parameters"))
    , Gui::SelectionObserver(vp)
    , ui(new Ui_TaskScaleParameters)
{
    auto* proxy = new QWidget(this);
    ui->setupUi(proxy);
    groupLayout()->addWidget(proxy);

    syncFromFeature();
    setupConnections();
}

TaskScaleParameters::~TaskScaleParameters()
{
    Gui::Selection().rmvSelectionGate();
}

void TaskScaleParameters::setupConnections()
{
    connect(
        ui->comboMode,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskScaleParameters::onModeChanged
    );

    connect(
        ui->spinScaleFactor,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &TaskScaleParameters::onScaleFactorChanged
    );

    connect(
        ui->spinScaleX,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &TaskScaleParameters::onScaleXChanged
    );

    connect(
        ui->spinScaleY,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &TaskScaleParameters::onScaleYChanged
    );

    connect(
        ui->spinScaleZ,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &TaskScaleParameters::onScaleZChanged
    );

    connect(
        ui->buttonReference,
        &QPushButton::toggled,
        this,
        &TaskScaleParameters::onReferenceButtonToggled
    );
}

void TaskScaleParameters::syncFromFeature()
{
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }

    {
        QSignalBlocker blocker(ui->comboMode);
        ui->comboMode->setCurrentIndex(feat->Mode.getValue());
    }
    {
        QSignalBlocker blocker(ui->spinScaleFactor);
        ui->spinScaleFactor->setValue(static_cast<double>(feat->ScaleFactor.getValue()));
    }
    {
        QSignalBlocker blocker(ui->spinScaleX);
        ui->spinScaleX->setValue(static_cast<double>(feat->ScaleX.getValue()));
    }
    {
        QSignalBlocker blocker(ui->spinScaleY);
        ui->spinScaleY->setValue(static_cast<double>(feat->ScaleY.getValue()));
    }
    {
        QSignalBlocker blocker(ui->spinScaleZ);
        ui->spinScaleZ->setValue(static_cast<double>(feat->ScaleZ.getValue()));
    }

    updateModeWidgets(feat->Mode.getValue());
    updateReferenceLabel();
}

void TaskScaleParameters::updateModeWidgets(int mode)
{
    bool isUniform = (mode == 0);
    ui->groupUniform->setVisible(isUniform);
    ui->groupXYZ->setVisible(!isUniform);
}

void TaskScaleParameters::updateReferenceLabel()
{
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }

    App::DocumentObject* refObj = feat->Reference.getValue();
    if (!refObj) {
        ui->buttonReference->setText(tr("Origin"));
        return;
    }

    std::vector<std::string> subs = feat->Reference.getSubValues();
    QString label = QString::fromUtf8(refObj->Label.getValue());
    if (!subs.empty() && !subs[0].empty()) {
        label += QStringLiteral(":") + QString::fromStdString(subs[0]);
    }
    ui->buttonReference->setText(label);
}

void TaskScaleParameters::onModeChanged(int index)
{
    if (isUpdateBlocked()) {
        return;
    }
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }
    feat->Mode.setValue(index);
    updateModeWidgets(index);
    recomputeFeature();
}

void TaskScaleParameters::onScaleFactorChanged(double val)
{
    if (isUpdateBlocked()) {
        return;
    }
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }
    feat->ScaleFactor.setValue(static_cast<float>(val));
    recomputeFeature();
}

void TaskScaleParameters::onScaleXChanged(double val)
{
    if (isUpdateBlocked()) {
        return;
    }
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }
    feat->ScaleX.setValue(static_cast<float>(val));
    recomputeFeature();
}

void TaskScaleParameters::onScaleYChanged(double val)
{
    if (isUpdateBlocked()) {
        return;
    }
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }
    feat->ScaleY.setValue(static_cast<float>(val));
    recomputeFeature();
}

void TaskScaleParameters::onScaleZChanged(double val)
{
    if (isUpdateBlocked()) {
        return;
    }
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }
    feat->ScaleZ.setValue(static_cast<float>(val));
    recomputeFeature();
}

void TaskScaleParameters::onReferenceButtonToggled(bool checked)
{
    selectingReference = checked;
    if (checked) {
        ui->buttonReference->setText(tr("Select..."));
        Gui::Selection().clearSelection();
    }
    else {
        updateReferenceLabel();
    }
}

void TaskScaleParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!selectingReference) {
        return;
    }
    if (msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    App::Document* doc = App::GetApplication().getDocument(msg.pDocName);
    if (!doc) {
        return;
    }
    App::DocumentObject* obj = doc->getObject(msg.pObjectName);
    if (!obj) {
        return;
    }

    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }

    std::vector<std::string> subs;
    if (msg.pSubName && msg.pSubName[0] != '\0') {
        subs.push_back(msg.pSubName);
    }
    feat->Reference.setValue(obj, subs);

    // Exit selection mode
    {
        QSignalBlocker blocker(ui->buttonReference);
        ui->buttonReference->setChecked(false);
    }
    selectingReference = false;
    updateReferenceLabel();
    recomputeFeature();
}

void TaskScaleParameters::apply()
{
    auto* feat = getObject<PartDesign::Scale>();
    if (!feat) {
        return;
    }

    FCMD_OBJ_CMD(feat, "Mode = " << feat->Mode.getValue());
    FCMD_OBJ_CMD(feat, "ScaleFactor = " << feat->ScaleFactor.getValue());
    FCMD_OBJ_CMD(feat, "ScaleX = " << feat->ScaleX.getValue());
    FCMD_OBJ_CMD(feat, "ScaleY = " << feat->ScaleY.getValue());
    FCMD_OBJ_CMD(feat, "ScaleZ = " << feat->ScaleZ.getValue());
}

// ---------------------------------------------------------------------------

TaskDlgScaleParameters::TaskDlgScaleParameters(ViewProviderScale* vp)
    : TaskDlgFeatureParameters(vp)
{
    auto* param = new TaskScaleParameters(vp);
    Content.push_back(param);
    Content.push_back(preview);
}

#include "moc_TaskScaleParameters.cpp"
