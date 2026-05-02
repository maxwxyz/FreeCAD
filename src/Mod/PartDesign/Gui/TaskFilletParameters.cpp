// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QAction>
# include <QApplication>
# include <QTreeWidgetItem>
#endif

#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ExpressionBinding.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/GizmoHelper.h>
#include <Mod/PartDesign/App/FeatureFillet.h>

#include "ui_TaskFilletParameters.h"
#include "TaskFilletParameters.h"
#include "Utils.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskFilletParameters */

// Returns the edge name stored in column 0 of a top-level tree item.
static QByteArray edgeName(QTreeWidgetItem* item)
{
    return item->text(0).toLatin1();
}

// -------------------------------------------------------------------------
// FilletSegmentDelegate
// -------------------------------------------------------------------------

FilletSegmentDelegate::FilletSegmentDelegate(QObject* parent)
    : QItemDelegate(parent)
{}

QWidget* FilletSegmentDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& /*option*/,
    const QModelIndex& index
) const
{
    if (index.column() < 1 || index.column() > 3) {
        return nullptr;
    }
    // Clicking a top-level item (edge row) creates a new segment
    if (!index.parent().isValid()) {
        if (auto owner = qobject_cast<TaskFilletParameters*>(this->parent())) {
            owner->newSegment(index.column());
        }
        return nullptr;
    }

    auto* editor = new Gui::QuantitySpinBox(parent);
    if (index.column() != 2) {
        editor->setUnit(Base::Unit::Length);
    }
    editor->setMinimum(0.0);
    editor->setMaximum(INT_MAX);
    editor->setSingleStep(0.1);
    if (auto owner = qobject_cast<TaskFilletParameters*>(this->parent())) {
        owner->setBinding(editor, index);
    }
    return editor;
}

void FilletSegmentDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    double value = index.model()->data(index, Qt::UserRole).toDouble();
    auto* spinBox = static_cast<Gui::QuantitySpinBox*>(editor);
    spinBox->setValue(value);
    spinBox->selectNumber();
}

void FilletSegmentDelegate::setModelData(
    QWidget* editor,
    QAbstractItemModel* model,
    const QModelIndex& index
) const
{
    auto* spinBox = static_cast<Gui::QuantitySpinBox*>(editor);
    spinBox->interpretText();
    Base::Quantity value = spinBox->value();
    model->setData(index, QString::fromStdString(value.getUserString()), Qt::DisplayRole);
    model->setData(index, value.getValue(), Qt::UserRole);
}

// -------------------------------------------------------------------------
// TaskFilletParameters
// -------------------------------------------------------------------------

TaskFilletParameters::TaskFilletParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , ui(new Ui_TaskFilletParameters)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Fillet* pcFillet = DressUpView->getObject<PartDesign::Fillet>();
    bool useAllEdges = pcFillet->UseAllEdges.getValue();
    ui->checkBoxUseAllEdges->setChecked(useAllEdges);
    ui->buttonRefSel->setEnabled(!useAllEdges);
    ui->treeWidgetReferences->setEnabled(!useAllEdges);

    ui->filletRadius->setUnit(Base::Unit::Length);
    ui->filletRadius->setMinimum(0);
    ui->filletRadius->selectNumber();
    ui->filletRadius->bind(pcFillet->Radius);
    QMetaObject::invokeMethod(ui->filletRadius, "setFocus", Qt::QueuedConnection);

    QMetaObject::connectSlotsByName(this);

    connect(
        ui->filletRadius,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &TaskFilletParameters::onLengthChanged
    );
    connect(ui->buttonRefSel, &QToolButton::toggled, this, &TaskFilletParameters::onButtonRefSel);
    connect(
        ui->checkBoxUseAllEdges,
        &QCheckBox::toggled,
        this,
        &TaskFilletParameters::onCheckBoxUseAllEdgesToggled
    );

    ui->treeWidgetReferences->setItemDelegate(new FilletSegmentDelegate(this));

    connect(ui->btnClear, &QPushButton::clicked, this, &TaskFilletParameters::clearSegments);
    connect(ui->btnAdd, &QPushButton::clicked, this, [this]() { newSegment(); });
    connect(ui->btnRemove, &QPushButton::clicked, this, &TaskFilletParameters::removeSegments);

    connect(ui->treeWidgetReferences, &QTreeWidget::itemChanged, this, &TaskFilletParameters::updateSegment);

    // Context menu: Add All Edges
    addAllEdgesAction = new QAction(tr("Add All Edges"), this);
    addAllEdgesAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")));
    addAllEdgesAction->setShortcutVisibleInContextMenu(true);
    ui->treeWidgetReferences->addAction(addAllEdgesAction);
    ui->treeWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(addAllEdgesAction, &QAction::triggered, this, &TaskFilletParameters::onAddAllEdges);

    // Context menu: Remove
    deleteAction = new QAction(tr("Remove"), this);
    deleteAction->setShortcut(Qt::Key_Delete);
    deleteAction->setShortcutVisibleInContextMenu(true);
    ui->treeWidgetReferences->addAction(deleteAction);
    connect(deleteAction, &QAction::triggered, this, &TaskFilletParameters::onRefDeleted);

    // Save/restore column widths
    static const char* prefPath
        = "User parameter:BaseApp/Preferences/General/Widgets/TaskFilletParameters";
    auto hParam = App::GetApplication().GetParameterGroupByPath(prefPath);
    for (int i = 0; i < ui->treeWidgetReferences->header()->count(); ++i) {
        std::string key("ColumnSize");
        key += std::to_string(i + 1);
        if (auto size = hParam->GetUnsigned(key.c_str(), 0)) {
            ui->treeWidgetReferences->header()->resizeSection(i, static_cast<int>(size));
        }
    }
    connect(
        ui->treeWidgetReferences->header(),
        &QHeaderView::sectionResized,
        [hParam](int idx, int, int newSize) {
            std::string key("ColumnSize");
            key += std::to_string(idx + 1);
            hParam->SetUnsigned(key.c_str(), static_cast<unsigned long>(newSize));
        }
    );

    refresh();

    if (ui->treeWidgetReferences->topLevelItemCount() == 0) {
        setSelectionMode(refSel);
    }
    else {
        hideOnError();
    }

    setupGizmos(DressUpView);
}

TaskFilletParameters::~TaskFilletParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (const Py::Exception&) {
        Base::PyException e;
        e.reportException();
    }
}

void TaskFilletParameters::refresh()
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet) {
        return;
    }

    {
        QSignalBlocker blocker(ui->filletRadius);
        ui->filletRadius->setValue(pcFillet->Radius.getValue());
    }

    QSignalBlocker blocker(ui->treeWidgetReferences);
    ui->treeWidgetReferences->clear();

    for (const auto& sub : pcFillet->Base.getSubValues()) {
        auto* item = new QTreeWidgetItem(ui->treeWidgetReferences, {QString::fromStdString(sub)});
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        for (const auto& seg : pcFillet->Segments.getValue(sub)) {
            auto* child = new QTreeWidgetItem(item);
            child->setFlags(child->flags() | Qt::ItemIsEditable);
            setSegment(child, seg.param, seg.radius, seg.length);
        }
    }
}

void TaskFilletParameters::setSegment(QTreeWidgetItem* item, double param, double radius, double length)
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet) {
        return;
    }

    QSignalBlocker blocker(ui->treeWidgetReferences);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    auto* parent = item->parent();
    if (!parent) {
        return;
    }

    App::ObjectIdentifier path(pcFillet->Segments);
    path << App::ObjectIdentifier::SimpleComponent(std::string(edgeName(parent).constData()))
         << App::ObjectIdentifier::ArrayComponent(parent->indexOfChild(item));
    auto linkColor = QVariant::fromValue(QApplication::palette().color(QPalette::Link));

    auto setupItem = [&](const char* key, int col, const Base::Quantity& q, bool hideText) {
        auto exprPath = App::ObjectIdentifier(path) << App::ObjectIdentifier::SimpleComponent(key);
        if (auto expr = pcFillet->getExpression(exprPath).expression) {
            item->setData(col, Qt::ToolTipRole, QString::fromUtf8(expr->toString().c_str()));
            item->setData(col, Qt::ForegroundRole, linkColor);
        }
        else {
            item->setData(col, Qt::ForegroundRole, QVariant());
            item->setData(col, Qt::ToolTipRole, QVariant());
        }
        item->setData(col, Qt::UserRole, q.getValue());
        item->setText(col, hideText ? QString() : QString::fromStdString(q.getUserString()));
    };

    setupItem("Radius", 1, Base::Quantity(radius, Base::Unit::Length), false);
    setupItem("Param", 2, Base::Quantity(param), length > 0.0);
    setupItem("Length", 3, Base::Quantity(length, Base::Unit::Length), length == 0.0);
}

void TaskFilletParameters::updateSegment(QTreeWidgetItem* item, int column)
{
    if (column < 1 || column > 3) {
        return;
    }
    QSignalBlocker blocker(ui->treeWidgetReferences);
    double param = item->data(2, Qt::UserRole).toDouble();
    double radius = item->data(1, Qt::UserRole).toDouble();
    double length = item->data(3, Qt::UserRole).toDouble();

    // Mutual exclusivity: param and length
    if (column == 3 && length > 0.0 && param != 0.0) {
        param = 0.0;
        item->setData(2, Qt::UserRole, 0.0);
    }
    else if (column == 2 && param > 0.0 && length != 0.0) {
        length = 0.0;
        item->setData(3, Qt::UserRole, 0.0);
    }

    setSegment(item, param, radius, length);
    updateSegments(item->parent() ? item->parent() : item);
}

void TaskFilletParameters::updateSegments(QTreeWidgetItem* edgeItem)
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet || !edgeItem) {
        return;
    }
    setupTransaction();
    Part::PropertyFilletSegments::Segments segs;
    segs.reserve(edgeItem->childCount());
    for (int i = 0; i < edgeItem->childCount(); ++i) {
        auto* child = edgeItem->child(i);
        segs.emplace_back(
            child->data(2, Qt::UserRole).toDouble(),
            child->data(1, Qt::UserRole).toDouble(),
            child->data(3, Qt::UserRole).toDouble()
        );
    }
    pcFillet->Segments.setValue(std::string(edgeName(edgeItem).constData()), std::move(segs));
    pcFillet->recomputeFeature();
    hideOnError();
}

void TaskFilletParameters::newSegment(int editColumn)
{
    auto* current = ui->treeWidgetReferences->currentItem();
    if (!current) {
        return;
    }
    auto* parent = current->parent();
    QSignalBlocker blocker(ui->treeWidgetReferences);

    double param = 0.0;
    auto* item = new QTreeWidgetItem;
    if (!parent) {
        // current is an edge item — add first child
        current->addChild(item);
        if (current->childCount() != 1) {
            param = 1.0;
        }
    }
    else {
        int index = parent->indexOfChild(current);
        if (index == 0 && parent->childCount() == 1) {
            parent->addChild(item);
            param = 1.0;
        }
        else {
            parent->insertChild(index, item);
            param = (index == 0) ? 0.0 : current->data(2, Qt::UserRole).toDouble();
        }
    }

    setSegment(item, param, getRadius());
    ui->treeWidgetReferences->setCurrentItem(item);
    if (editColumn) {
        ui->treeWidgetReferences->editItem(item, editColumn);
    }
    updateSegments(parent ? parent : current);
}

void TaskFilletParameters::clearSegments()
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet) {
        return;
    }
    std::set<QTreeWidgetItem*> edgeItems;
    for (auto* item : ui->treeWidgetReferences->selectedItems()) {
        edgeItems.insert(item->parent() ? item->parent() : item);
    }
    setupTransaction();
    QSignalBlocker blocker(ui->treeWidgetReferences);
    for (auto* edgeItem : edgeItems) {
        for (auto* child : edgeItem->takeChildren()) {
            delete child;
        }
        pcFillet->Segments.removeValue(std::string(edgeName(edgeItem).constData()));
    }
    pcFillet->recomputeFeature();
    hideOnError();
}

void TaskFilletParameters::removeSegments()
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet) {
        return;
    }
    setupTransaction();
    QSignalBlocker blocker(ui->treeWidgetReferences);
    for (auto* item : ui->treeWidgetReferences->selectedItems()) {
        auto* parent = item->parent();
        if (!parent) {
            continue;
        }
        pcFillet->Segments.removeValue(
            std::string(edgeName(parent).constData()),
            parent->indexOfChild(item)
        );
        delete item;
    }
    pcFillet->recomputeFeature();
    hideOnError();
}

void TaskFilletParameters::setBinding(Gui::ExpressionBinding* binding, const QModelIndex& index)
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet || !index.isValid()) {
        return;
    }
    auto* item = static_cast<QTreeWidgetItem*>(index.internalPointer());
    if (!item) {
        return;
    }
    auto* parent = item->parent();
    if (!parent) {
        return;
    }
    App::ObjectIdentifier path(pcFillet->Segments);
    path << App::ObjectIdentifier::SimpleComponent(std::string(edgeName(parent).constData()))
         << App::ObjectIdentifier::ArrayComponent(index.row())
         << App::ObjectIdentifier::SimpleComponent(
                index.column() == 1 ? "Radius" : (index.column() == 3 ? "Length" : "Param")
            );
    binding->bind(path);
}

void TaskFilletParameters::onRefDeleted()
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet) {
        return;
    }

    // If selected items include child segments only, remove those segments
    bool hasEdges = false;
    for (auto* item : ui->treeWidgetReferences->selectedItems()) {
        if (!item->parent()) {
            hasEdges = true;
            break;
        }
    }
    if (!hasEdges) {
        removeSegments();
        return;
    }

    // Remove selected top-level edge items
    std::vector<std::string> refs = pcFillet->Base.getSubValues();
    QSignalBlocker blocker(ui->treeWidgetReferences);
    for (auto* item : ui->treeWidgetReferences->selectedItems()) {
        if (item->parent()) {
            continue;
        }
        std::string name(edgeName(item).constData());
        refs.erase(std::remove(refs.begin(), refs.end(), name), refs.end());
        pcFillet->Segments.removeValue(name);
        delete item;
    }
    setupTransaction();
    pcFillet->Base.setValue(pcFillet->Base.getValue(), refs);
    pcFillet->recomputeFeature();
    hideOnError();
    setGizmoPositions();
}

void TaskFilletParameters::onAddAllEdges()
{
    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    if (!pcFillet) {
        return;
    }
    App::DocumentObject* base = pcFillet->Base.getValue();
    if (!base) {
        return;
    }
    int count = Part::Feature::getTopoShape(
                    base,
                    Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    )
                    .countSubShapes(TopAbs_EDGE);
    auto subValues = pcFillet->Base.getSubValues(false);
    std::size_t len = subValues.size();
    for (int i = 0; i < count; ++i) {
        std::string name = "Edge" + std::to_string(i + 1);
        if (std::find(subValues.begin(), subValues.begin() + len, name) == subValues.begin() + len) {
            subValues.push_back(name);
        }
    }
    if (subValues.size() == len) {
        return;
    }
    try {
        setupTransaction();
        pcFillet->Base.setValue(base, subValues);
        pcFillet->recomputeFeature();
        hideOnError();
        refresh();
    }
    catch (Base::Exception& e) {
        e.reportException();
    }
}

void TaskFilletParameters::referenceSelectedTree(const Gui::SelectionChanges& msg)
{
    if (strcmp(msg.pDocName, getDressUpView()->getObject()->getDocument()->getName()) != 0) {
        return;
    }
    Part::Feature* base = getBase();
    if (!base || strcmp(msg.pObjectName, base->getNameInDocument()) != 0) {
        return;
    }

    Gui::Selection().clearSelection();

    PartDesign::Fillet* pcFillet = getObject<PartDesign::Fillet>();
    const std::string subName(msg.pSubName);
    std::vector<std::string> refs = pcFillet->Base.getSubValues();

    QSignalBlocker blocker(ui->treeWidgetReferences);
    auto it = std::find(refs.begin(), refs.end(), subName);
    if (it != refs.end()) {
        // Edge already in list — remove it
        refs.erase(it);
        pcFillet->Segments.removeValue(subName);
        QString qsub = QString::fromStdString(subName);
        for (int i = 0; i < ui->treeWidgetReferences->topLevelItemCount(); ++i) {
            if (ui->treeWidgetReferences->topLevelItem(i)->text(0) == qsub) {
                delete ui->treeWidgetReferences->takeTopLevelItem(i);
                break;
            }
        }
    }
    else {
        // Edge not yet in list — add it
        refs.push_back(subName);
        auto* item = new QTreeWidgetItem(ui->treeWidgetReferences, {QString::fromStdString(subName)});
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    setupTransaction();
    pcFillet->Base.setValue(pcFillet->Base.getValue(), refs);
    pcFillet->recomputeFeature();
    hideOnError();
}

void TaskFilletParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == refSel) {
            referenceSelectedTree(msg);
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        setGizmoPositions();
    }
}

void TaskFilletParameters::onCheckBoxUseAllEdgesToggled(bool checked)
{
    ui->buttonRefSel->setEnabled(!checked);
    ui->treeWidgetReferences->setEnabled(!checked);
    if (auto* pcFillet = getObject<PartDesign::Fillet>()) {
        if (checked) {
            setSelectionMode(none);
        }
        pcFillet->UseAllEdges.setValue(checked);
        pcFillet->recomputeFeature();
        hideOnError();
    }
}

void TaskFilletParameters::onLengthChanged(double len)
{
    if (auto* pcFillet = getObject<PartDesign::Fillet>()) {
        setSelectionMode(none);
        setupTransaction();
        pcFillet->Radius.setValue(len);
        pcFillet->recomputeFeature();
        hideOnError();
    }
}

double TaskFilletParameters::getRadius() const
{
    return ui->filletRadius->value().getValue();
}

void TaskFilletParameters::setButtons(const selectionModes mode)
{
    ui->buttonRefSel->setChecked(mode == refSel);
    ui->buttonRefSel->setText(mode == refSel ? stopSelectionLabel() : startSelectionLabel());
}

void TaskFilletParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskFilletParameters::apply()
{
    ui->filletRadius->apply();

    if (ui->treeWidgetReferences->topLevelItemCount() == 0) {
        std::string text = tr("Empty fillet created!").toStdString();
        Base::Console().warning("%s\n", text.c_str());
    }
}

void TaskFilletParameters::setupGizmos(ViewProviderDressUp* vp)
{
    if (!GizmoContainer::isEnabled()) {
        return;
    }

    radiusGizmo = new Gui::LinearGizmo(ui->filletRadius);
    radiusGizmo2 = new Gui::LinearGizmo(ui->filletRadius);

    gizmoContainer = GizmoContainer::create({radiusGizmo, radiusGizmo2}, vp);

    setGizmoPositions();
}

void TaskFilletParameters::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }

    auto* fillet = getObject<PartDesign::Fillet>();
    if (!fillet || fillet->isError()) {
        gizmoContainer->visible = false;
        return;
    }
    Part::TopoShape baseShape = fillet->getBaseTopoShape(true);
    std::vector<Part::TopoShape> shapes = fillet->getContinuousEdges(baseShape);

    if (shapes.empty()) {
        gizmoContainer->visible = false;
        return;
    }
    gizmoContainer->visible = true;

    Part::TopoShape edge = shapes[0];
    auto [face1, face2] = getAdjacentFacesFromEdge(edge, baseShape);

    DraggerPlacementProps props1 = getDraggerPlacementFromEdgeAndFace(edge, face1);
    radiusGizmo->Gizmo::setDraggerPlacement(props1.position, props1.dir);

    DraggerPlacementProps props2 = getDraggerPlacementFromEdgeAndFace(edge, face2);
    radiusGizmo2->Gizmo::setDraggerPlacement(props2.position, props2.dir);

    double angle = props1.dir.GetAngle(props2.dir);
    double correction = 1.0 / std::tan(angle / 2.0);

    radiusGizmo->setMultFactor(correction);
    radiusGizmo2->setMultFactor(correction);
}

// -------------------------------------------------------------------------
// TaskDlgFilletParameters
// -------------------------------------------------------------------------

TaskDlgFilletParameters::TaskDlgFilletParameters(ViewProviderFillet* DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter = new TaskFilletParameters(DressUpView);

    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgFilletParameters::~TaskDlgFilletParameters() = default;

bool TaskDlgFilletParameters::accept()
{
    auto* obj = getObject();
    if (!obj->isError()) {
        getViewObject()->showPreviousFeature(false);
    }

    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskFilletParameters.cpp"
