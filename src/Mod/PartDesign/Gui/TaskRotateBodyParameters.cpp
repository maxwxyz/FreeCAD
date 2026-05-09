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

#include <App/Document.h>
#include <App/TransactionDefs.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/InputField.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/Selection.h>
#include <Mod/PartDesign/App/FeatureRotateBody.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

#include "ui_TaskRotateBodyParameters.h"
#include "TaskRotateBodyParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRotateBodyParameters */

// ---------------------------------------------------------------------------
// TaskRotateBodyParameters
// ---------------------------------------------------------------------------

TaskRotateBodyParameters::TaskRotateBodyParameters(ViewProviderRotateBody* vp, QWidget* parent)
    : TaskFeatureParameters(vp, parent, "PartDesign_RotateBody", tr("Rotate Body Parameters"))
    , Gui::SelectionObserver(vp)
    , ui(new Ui_TaskRotateBodyParameters)
{
    auto* proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    initUI();

    // Mode combo
    connect(
        ui->comboMode,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskRotateBodyParameters::onModeChanged
    );

    // Angle spinbox (Axis+Angle mode)
    connect(ui->spinAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, [this](double val) {
        auto* feat = getObject<PartDesign::RotateBody>();
        if (!feat) {
            return;
        }
        openTransactionIfNeeded();
        feat->Angle.setValue(val);
        recomputeFeature();
    });

    // Axis+Angle page
    connect(ui->buttonAxis, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonAxis);

    // Axis+TwoElements page
    connect(ui->buttonAxis2, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonAxis2);
    connect(ui->buttonRef1, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonRef1);
    connect(ui->buttonRef2, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonRef2);

    // ThreePoints page
    connect(ui->buttonPoint1, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonPoint1);
    connect(ui->buttonPoint2, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonPoint2);
    connect(ui->buttonPoint3, &QToolButton::toggled, this, &TaskRotateBodyParameters::onButtonPoint3);
}

TaskRotateBodyParameters::~TaskRotateBodyParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (...) {
    }
}

void TaskRotateBodyParameters::initUI()
{
    auto* feat = getObject<PartDesign::RotateBody>();
    if (!feat) {
        return;
    }

    // Populate mode combo from feature enums
    ui->comboMode->blockSignals(true);
    for (const auto& name : feat->Mode.getEnumVector()) {
        ui->comboMode->addItem(QString::fromStdString(name));
    }
    const int mode = feat->Mode.getValue();
    ui->comboMode->setCurrentIndex(mode);
    ui->stackedWidget->setCurrentIndex(mode);
    ui->comboMode->blockSignals(false);

    // Bind angle spinbox to the Angle property
    ui->spinAngle->bind(feat->Angle);
    ui->spinAngle->setUnit(Base::Unit::Angle);

    // Populate existing reference displays
    ui->lineAxis->setText(refText(feat->Axis.getValue(), feat->Axis.getSubValues()));
    ui->lineAxis2->setText(refText(feat->Axis.getValue(), feat->Axis.getSubValues()));
    ui->lineRef1->setText(refText(feat->Ref1.getValue(), feat->Ref1.getSubValues()));
    ui->lineRef2->setText(refText(feat->Ref2.getValue(), feat->Ref2.getSubValues()));
    ui->linePoint1->setText(refText(feat->Point1.getValue(), feat->Point1.getSubValues()));
    ui->linePoint2->setText(refText(feat->Point2.getValue(), feat->Point2.getSubValues()));
    ui->linePoint3->setText(refText(feat->Point3.getValue(), feat->Point3.getSubValues()));
}

QString TaskRotateBodyParameters::refText(App::DocumentObject* obj, const std::vector<std::string>& subs)
{
    return getRefStr(obj, subs);
}

void TaskRotateBodyParameters::openTransactionIfNeeded()
{
    auto* feat = getObject<PartDesign::RotateBody>();
    if (!feat) {
        return;
    }
    int bookedID = feat->getDocument()->getBookedTransactionID();
    if (bookedID != App::NullTransaction && bookedID == transactionID) {
        return;
    }
    std::string n("Edit ");
    n += feat->Label.getValue();
    transactionID = feat->getDocument()->openTransaction(n.c_str());
}

void TaskRotateBodyParameters::stopSelecting()
{
    const QSignalBlocker b1(ui->buttonAxis);
    const QSignalBlocker b2(ui->buttonAxis2);
    const QSignalBlocker b3(ui->buttonRef1);
    const QSignalBlocker b4(ui->buttonRef2);
    const QSignalBlocker b5(ui->buttonPoint1);
    const QSignalBlocker b6(ui->buttonPoint2);
    const QSignalBlocker b7(ui->buttonPoint3);
    activeButton = ActiveButton::None;
    ui->buttonAxis->setChecked(false);
    ui->buttonAxis2->setChecked(false);
    ui->buttonRef1->setChecked(false);
    ui->buttonRef2->setChecked(false);
    ui->buttonPoint1->setChecked(false);
    ui->buttonPoint2->setChecked(false);
    ui->buttonPoint3->setChecked(false);
    Gui::Selection().rmvSelectionGate();
}

void TaskRotateBodyParameters::onModeChanged(int index)
{
    auto* feat = getObject<PartDesign::RotateBody>();
    if (!feat) {
        return;
    }
    stopSelecting();
    ui->stackedWidget->setCurrentIndex(index);
    openTransactionIfNeeded();
    feat->Mode.setValue(index);
    recomputeFeature();
}

// Helper: install a selection gate and record which button is active
static void startAxisSelection(PartDesign::RotateBody* feat)
{
    Gui::Selection().clearSelection();
    Gui::Selection().addSelectionGate(
        new PartDesignGui::ReferenceSelection(feat, AllowSelection::EDGE | AllowSelection::PLANAR)
    );
}

static void startPointSelection(PartDesign::RotateBody* feat)
{
    Gui::Selection().clearSelection();
    Gui::Selection().addSelectionGate(
        new PartDesignGui::ReferenceSelection(feat, AllowSelection::POINT)
    );
}

static void startRefSelection(PartDesign::RotateBody* feat)
{
    Gui::Selection().clearSelection();
    Gui::Selection().addSelectionGate(new PartDesignGui::ReferenceSelection(
        feat,
        AllowSelection::EDGE | AllowSelection::POINT | AllowSelection::PLANAR
    ));
}

void TaskRotateBodyParameters::onButtonAxis(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Axis;
        {
            QSignalBlocker b(ui->buttonAxis);
            ui->buttonAxis->setChecked(true);
        }
        startAxisSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onButtonAxis2(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Axis2;
        {
            QSignalBlocker b(ui->buttonAxis2);
            ui->buttonAxis2->setChecked(true);
        }
        startAxisSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onButtonRef1(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Ref1;
        {
            QSignalBlocker b(ui->buttonRef1);
            ui->buttonRef1->setChecked(true);
        }
        startRefSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onButtonRef2(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Ref2;
        {
            QSignalBlocker b(ui->buttonRef2);
            ui->buttonRef2->setChecked(true);
        }
        startRefSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onButtonPoint1(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Point1;
        {
            QSignalBlocker b(ui->buttonPoint1);
            ui->buttonPoint1->setChecked(true);
        }
        startPointSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onButtonPoint2(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Point2;
        {
            QSignalBlocker b(ui->buttonPoint2);
            ui->buttonPoint2->setChecked(true);
        }
        startPointSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onButtonPoint3(bool checked)
{
    if (checked) {
        stopSelecting();
        activeButton = ActiveButton::Point3;
        {
            QSignalBlocker b(ui->buttonPoint3);
            ui->buttonPoint3->setChecked(true);
        }
        startPointSelection(getObject<PartDesign::RotateBody>());
    }
    else {
        stopSelecting();
    }
}

void TaskRotateBodyParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }
    if (activeButton == ActiveButton::None) {
        return;
    }

    auto* feat = getObject<PartDesign::RotateBody>();
    if (!feat) {
        return;
    }

    App::DocumentObject* selObj = nullptr;
    std::vector<std::string> selSubs;
    if (!getReferencedSelection(feat, msg, selObj, selSubs)) {
        return;
    }

    openTransactionIfNeeded();
    QString display = refText(selObj, selSubs);

    switch (activeButton) {
        case ActiveButton::Axis:
            feat->Axis.setValue(selObj, selSubs);
            ui->lineAxis->setText(display);
            // Keep Axis and Axis2 in sync: both pages reference the same property
            ui->lineAxis2->setText(display);
            break;
        case ActiveButton::Axis2:
            feat->Axis.setValue(selObj, selSubs);
            ui->lineAxis2->setText(display);
            ui->lineAxis->setText(display);
            break;
        case ActiveButton::Ref1:
            feat->Ref1.setValue(selObj, selSubs);
            ui->lineRef1->setText(display);
            break;
        case ActiveButton::Ref2:
            feat->Ref2.setValue(selObj, selSubs);
            ui->lineRef2->setText(display);
            break;
        case ActiveButton::Point1:
            feat->Point1.setValue(selObj, selSubs);
            ui->linePoint1->setText(display);
            break;
        case ActiveButton::Point2:
            feat->Point2.setValue(selObj, selSubs);
            ui->linePoint2->setText(display);
            break;
        case ActiveButton::Point3:
            feat->Point3.setValue(selObj, selSubs);
            ui->linePoint3->setText(display);
            break;
        default:
            break;
    }

    stopSelecting();
    recomputeFeature();
}

void TaskRotateBodyParameters::apply()
{
    // Properties were set live during interaction. Nothing extra to commit here.
    // The spinbox bound to Angle handles its own property updates.
}

void TaskRotateBodyParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(qobject_cast<QWidget*>(ui->comboMode->parent()->parent()));
    }
}

// ---------------------------------------------------------------------------
// TaskDlgRotateBodyParameters
// ---------------------------------------------------------------------------

TaskDlgRotateBodyParameters::TaskDlgRotateBodyParameters(ViewProviderRotateBody* vp)
    : TaskDlgFeatureParameters(vp)
    , parameter(new TaskRotateBodyParameters(vp))
{
    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgRotateBodyParameters::~TaskDlgRotateBodyParameters() = default;

bool TaskDlgRotateBodyParameters::accept()
{
    auto* feat = getObject<PartDesign::RotateBody>();
    if (feat && !feat->isError()) {
        getViewObject<ViewProviderRotateBody>()->showPreviousFeature(false);
    }
    parameter->apply();
    return TaskDlgFeatureParameters::accept();
}

#include "moc_TaskRotateBodyParameters.cpp"
