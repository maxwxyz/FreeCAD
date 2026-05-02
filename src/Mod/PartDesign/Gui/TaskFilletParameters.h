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


#pragma once

#include <QItemDelegate>
#include <QModelIndex>
#include <QTreeWidgetItem>

#include <Gui/Inventor/Draggers/Gizmo.h>

#include "TaskDressUpParameters.h"
#include "ViewProviderFillet.h"

class Ui_TaskFilletParameters;
class QTreeWidget;
class QTreeWidgetItem;

namespace Gui
{
class ExpressionBinding;
class LinearGizmo;
class GizmoContainer;
}  // namespace Gui

namespace PartDesignGui
{

class FilletSegmentDelegate: public QItemDelegate
{
    Q_OBJECT

public:
    explicit FilletSegmentDelegate(QObject* parent = nullptr);

    QWidget* createEditor(
        QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};


class TaskFilletParameters: public TaskDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskFilletParameters(ViewProviderDressUp* DressUpView, QWidget* parent = nullptr);
    ~TaskFilletParameters() override;

    void apply() override;
    void setBinding(Gui::ExpressionBinding* binding, const QModelIndex& index);

private Q_SLOTS:
    void onLengthChanged(double);
    void onRefDeleted() override;
    void onAddAllEdges();
    void onCheckBoxUseAllEdgesToggled(bool checked);

protected:
    void setButtons(const selectionModes mode) override;
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    void refresh();
    void newSegment(int editColumn = 0);
    void clearSegments();
    void removeSegments();
    void updateSegments(QTreeWidgetItem* edgeItem);
    void updateSegment(QTreeWidgetItem* item, int column);
    void setSegment(QTreeWidgetItem* item, double param, double radius, double length = 0.0);
    double getRadius() const;

    friend class FilletSegmentDelegate;

private:
    void referenceSelectedTree(const Gui::SelectionChanges& msg);

    std::unique_ptr<Ui_TaskFilletParameters> ui;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* radiusGizmo = nullptr;
    Gui::LinearGizmo* radiusGizmo2 = nullptr;
    void setupGizmos(ViewProviderDressUp* vp);
    void setGizmoPositions();
};

/// simulation dialog for the TaskView
class TaskDlgFilletParameters: public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgFilletParameters(ViewProviderFillet* DressUpView);
    ~TaskDlgFilletParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

}  // namespace PartDesignGui
