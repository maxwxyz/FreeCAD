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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/TransactionDefs.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Selection/Selection.h>
#include <Mod/PartDesign/App/FeatureTranslate.h>

#include "ui_TaskTranslateParameters.h"
#include "TaskTranslateParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTranslateParameters */

TaskTranslateParameters::TaskTranslateParameters(ViewProviderTranslate* vp, QWidget* parent)
    : TaskFeatureParameters(vp, parent, "PartDesign_Translate.svg", tr("Translate Parameters"))
    , SelectionObserver(vp)
    , ui(new Ui_TaskTranslateParameters)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    auto* feat = getObject<PartDesign::Translate>();

    // Populate mode combo from feature
    ui->comboMode->setCurrentIndex(feat->Mode.getValue());

    // Bind distance spinbox to expression engine
    ui->spinDistance->bind(feat->Distance);
    ui->spinDistance->setUnit(Base::Unit::Length);
    ui->spinDistance->setValue(feat->Distance.getValue());

    // Coordinate spinboxes - no expression binding since Delta is a vector property
    ui->spinDX->setUnit(Base::Unit::Length);
    ui->spinDY->setUnit(Base::Unit::Length);
    ui->spinDZ->setUnit(Base::Unit::Length);
    Base::Vector3d delta = feat->Delta.getValue();
    ui->spinDX->setValue(delta.x);
    ui->spinDY->setValue(delta.y);
    ui->spinDZ->setValue(delta.z);

    // Direction vector spinboxes (plain doubles, not quantities)
    Base::Vector3d dir = feat->Direction.getValue();
    ui->spinDirX->setValue(dir.x);
    ui->spinDirY->setValue(dir.y);
    ui->spinDirZ->setValue(dir.z);

    // Populate reference labels from current property values
    auto dirRef = feat->DirectionReference.getValue();
    if (dirRef) {
        auto subs = feat->DirectionReference.getSubValues();
        QString label = QString::fromStdString(dirRef->getNameInDocument());
        if (!subs.empty() && !subs[0].empty()) {
            label += QStringLiteral(":") + QString::fromStdString(subs[0]);
        }
        setDirectionRefLabel(label);
    }

    auto startObj = feat->StartPoint.getValue();
    if (startObj) {
        auto subs = feat->StartPoint.getSubValues();
        QString label = QString::fromStdString(startObj->getNameInDocument());
        if (!subs.empty() && !subs[0].empty()) {
            label += QStringLiteral(":") + QString::fromStdString(subs[0]);
        }
        setStartPointLabel(label);
    }

    auto endObj = feat->EndPoint.getValue();
    if (endObj) {
        auto subs = feat->EndPoint.getSubValues();
        QString label = QString::fromStdString(endObj->getNameInDocument());
        if (!subs.empty() && !subs[0].empty()) {
            label += QStringLiteral(":") + QString::fromStdString(subs[0]);
        }
        setEndPointLabel(label);
    }

    auto axisObj = feat->AxisSystem.getValue();
    if (axisObj) {
        setAxisSystemLabel(QString::fromStdString(axisObj->getNameInDocument()));
    }

    updateUI();

    // clang-format off
    connect(ui->comboMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskTranslateParameters::onModeChanged);
    connect(ui->buttonDirectionRef, &QToolButton::toggled,
            this, &TaskTranslateParameters::onDirectionRefToggled);
    connect(ui->buttonStartPoint, &QToolButton::toggled,
            this, &TaskTranslateParameters::onStartPointToggled);
    connect(ui->buttonEndPoint, &QToolButton::toggled,
            this, &TaskTranslateParameters::onEndPointToggled);
    connect(ui->buttonAxisSystem, &QToolButton::toggled,
            this, &TaskTranslateParameters::onAxisSystemToggled);
    connect(ui->spinDistance, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskTranslateParameters::onDistanceChanged);
    connect(ui->spinDirX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TaskTranslateParameters::onDirectionChanged);
    connect(ui->spinDirY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TaskTranslateParameters::onDirectionChanged);
    connect(ui->spinDirZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TaskTranslateParameters::onDirectionChanged);
    connect(ui->spinDX, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskTranslateParameters::onCoordChanged);
    connect(ui->spinDY, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskTranslateParameters::onCoordChanged);
    connect(ui->spinDZ, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskTranslateParameters::onCoordChanged);
    // clang-format on
}

TaskTranslateParameters::~TaskTranslateParameters()
{
    stopSelectionMode();
}

void TaskTranslateParameters::setupTransaction()
{
    auto* obj = getObject();
    if (!obj) {
        return;
    }
    if (obj->getDocument()->getBookedTransactionID() != App::NullTransaction) {
        return;
    }
    std::string name("Edit ");
    name += obj->Label.getValue();
    obj->getDocument()->openTransaction(name.c_str());
}

void TaskTranslateParameters::updateUI()
{
    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }
    ui->stackedWidget->setCurrentIndex(feat->Mode.getValue());
}

void TaskTranslateParameters::stopSelectionMode()
{
    selMode = SelectionMode::None;
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();

    // Uncheck all toggle buttons without triggering slots
    QSignalBlocker b1(ui->buttonDirectionRef);
    QSignalBlocker b2(ui->buttonStartPoint);
    QSignalBlocker b3(ui->buttonEndPoint);
    QSignalBlocker b4(ui->buttonAxisSystem);
    ui->buttonDirectionRef->setChecked(false);
    ui->buttonStartPoint->setChecked(false);
    ui->buttonEndPoint->setChecked(false);
    ui->buttonAxisSystem->setChecked(false);
}

void TaskTranslateParameters::setDirectionRefLabel(const QString& text)
{
    ui->labelDirectionRef->setText(text);
}

void TaskTranslateParameters::setStartPointLabel(const QString& text)
{
    ui->labelStartPoint->setText(text);
}

void TaskTranslateParameters::setEndPointLabel(const QString& text)
{
    ui->labelEndPoint->setText(text);
}

void TaskTranslateParameters::setAxisSystemLabel(const QString& text)
{
    ui->labelAxisSystemRef->setText(text);
}

void TaskTranslateParameters::onModeChanged(int index)
{
    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }
    stopSelectionMode();
    setupTransaction();
    feat->Mode.setValue(index);
    recomputeFeature();
    updateUI();
}

void TaskTranslateParameters::onDirectionRefToggled(bool checked)
{
    stopSelectionMode();
    if (checked) {
        selMode = SelectionMode::DirectionRef;
    }
}

void TaskTranslateParameters::onStartPointToggled(bool checked)
{
    stopSelectionMode();
    if (checked) {
        selMode = SelectionMode::StartPoint;
    }
}

void TaskTranslateParameters::onEndPointToggled(bool checked)
{
    stopSelectionMode();
    if (checked) {
        selMode = SelectionMode::EndPoint;
    }
}

void TaskTranslateParameters::onAxisSystemToggled(bool checked)
{
    stopSelectionMode();
    if (checked) {
        selMode = SelectionMode::AxisSystem;
    }
}

void TaskTranslateParameters::onDirectionChanged()
{
    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }
    setupTransaction();
    feat->Direction.setValue(
        Base::Vector3d(ui->spinDirX->value(), ui->spinDirY->value(), ui->spinDirZ->value())
    );
    recomputeFeature();
}

void TaskTranslateParameters::onDistanceChanged(double val)
{
    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }
    setupTransaction();
    feat->Distance.setValue(val);
    recomputeFeature();
}

void TaskTranslateParameters::onCoordChanged()
{
    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }
    setupTransaction();
    feat->Delta.setValue(
        Base::Vector3d(
            ui->spinDX->value().getValue(),
            ui->spinDY->value().getValue(),
            ui->spinDZ->value().getValue()
        )
    );
    recomputeFeature();
}

void TaskTranslateParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }
    if (selMode == SelectionMode::None) {
        return;
    }

    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }

    App::DocumentObject* obj = feat->getDocument()->getObject(msg.pObjectName);
    if (!obj) {
        return;
    }

    std::string sub = msg.pSubName ? std::string(msg.pSubName) : std::string();
    setupTransaction();

    switch (selMode) {
        case SelectionMode::DirectionRef: {
            feat->DirectionReference.setValue(obj, {sub});
            QString label = QString::fromStdString(obj->getNameInDocument());
            if (!sub.empty()) {
                label += QStringLiteral(":") + QString::fromStdString(sub);
            }
            setDirectionRefLabel(label);
            break;
        }
        case SelectionMode::StartPoint: {
            feat->StartPoint.setValue(obj, {sub});
            QString label = QString::fromStdString(obj->getNameInDocument());
            if (!sub.empty()) {
                label += QStringLiteral(":") + QString::fromStdString(sub);
            }
            setStartPointLabel(label);
            break;
        }
        case SelectionMode::EndPoint: {
            feat->EndPoint.setValue(obj, {sub});
            QString label = QString::fromStdString(obj->getNameInDocument());
            if (!sub.empty()) {
                label += QStringLiteral(":") + QString::fromStdString(sub);
            }
            setEndPointLabel(label);
            break;
        }
        case SelectionMode::AxisSystem: {
            feat->AxisSystem.setValue(obj);
            setAxisSystemLabel(QString::fromStdString(obj->getNameInDocument()));
            break;
        }
        default:
            break;
    }

    stopSelectionMode();
    recomputeFeature();
}

void TaskTranslateParameters::apply()
{
    auto* feat = getObject<PartDesign::Translate>();
    if (!feat) {
        return;
    }

    // Flush any pending expression changes from bound spinboxes
    ui->spinDistance->apply();
}

//=============================================================================
// TaskDlgTranslateParameters
//=============================================================================

TaskDlgTranslateParameters::TaskDlgTranslateParameters(ViewProviderTranslate* vp)
    : TaskDlgFeatureParameters(vp)
{
    Content.push_back(new TaskTranslateParameters(vp));
    Content.push_back(preview);
}

bool TaskDlgTranslateParameters::accept()
{
    auto* feat = getObject<PartDesign::Translate>();
    if (feat && !feat->isError()) {
        getViewObject<ViewProviderTranslate>()->showPreviousFeature(false);
    }
    return TaskDlgFeatureParameters::accept();
}

#include "moc_TaskTranslateParameters.cpp"
