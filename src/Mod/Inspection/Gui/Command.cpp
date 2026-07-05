// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Inventor/events/SoButtonEvent.h>

#include <algorithm>

#include <QMessageBox>


#include <App/Document.h>
#include <App/GeoFeature.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Inspection/App/CompareFeature.h>
#include <Mod/Inspection/App/InspectionFeature.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Points/App/PointsFeature.h>

#include "ViewProviderInspection.h"
#include "VisualInspection.h"


DEF_STD_CMD_A(CmdVisualInspection)

CmdVisualInspection::CmdVisualInspection()
    : Command("Inspection_VisualInspection")
{
    sAppModule = "Inspection";
    sGroup = QT_TR_NOOP("Inspection");
    sMenuText = QT_TR_NOOP("Visual Inspection…");
    sToolTipText = QT_TR_NOOP("Inspects the objects visually");
    sStatusTip = sToolTipText;
    sWhatsThis = "Inspection_VisualInspection";
}

void CmdVisualInspection::activated(int)
{
    InspectionGui::VisualInspection dlg(Gui::getMainWindow());
    dlg.exec();
}

bool CmdVisualInspection::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//--------------------------------------------------------------------------------------

namespace
{

bool isCompareCandidate(const App::DocumentObject* object)
{
    return object
        && (object->isDerivedFrom<Part::Feature>() || object->isDerivedFrom<Mesh::Feature>()
            || object->isDerivedFrom<Points::Feature>());
}

std::vector<App::DocumentObject*> getSelectedCompareCandidates(App::Document* document)
{
    std::vector<App::DocumentObject*> objects;
    if (!document) {
        return objects;
    }

    std::vector<Gui::SelectionSingleton::SelObj> selection
        = Gui::Selection().getSelection(document->getName(), Gui::ResolveMode::FollowLink);

    for (const auto& selected : selection) {
        App::DocumentObject* object = selected.pResolvedObject ? selected.pResolvedObject
                                                               : selected.pObject;
        if (isCompareCandidate(object)
            && std::find(objects.begin(), objects.end(), object) == objects.end()) {
            objects.push_back(object);
        }
    }

    return objects;
}

double defaultSearchRadius(App::DocumentObject* reference, App::DocumentObject* compared)
{
    Base::BoundBox3d bounds;
    for (App::DocumentObject* object : {reference, compared}) {
        auto* geoFeature = freecad_cast<App::GeoFeature*>(object);
        const App::PropertyComplexGeoData* geometry = geoFeature
            ? geoFeature->getPropertyOfGeometry()
            : nullptr;
        if (geometry) {
            bounds.Add(geometry->getBoundingBox());
        }
    }

    if (!bounds.IsValid()) {
        return 0.05;
    }

    const double diagonal = bounds.CalcDiagonalLength();
    if (diagonal <= 0.0) {
        return 0.05;
    }

    return std::max(0.05, diagonal * 0.05);
}

Inspection::Compare* createExactCompare(
    App::Document* document,
    App::DocumentObject* reference,
    App::DocumentObject* compared
)
{
    auto* compare = document->addObject<Inspection::Compare>("Compare");
    compare->Reference.setValue(reference);
    compare->Compared.setValue(compared);
    compare->Label.setValue("Compare");
    return compare;
}

Inspection::Feature* createDeviationCompare(
    App::Document* document,
    App::DocumentObject* reference,
    App::DocumentObject* compared
)
{
    auto* compare = document->addObject<Inspection::Feature>("CompareDeviation");
    compare->Actual.setValue(compared);
    compare->Nominals.setValues({reference});
    compare->SearchRadius.setValue(defaultSearchRadius(reference, compared));
    compare->Label.setValue("Compare Deviation");
    return compare;
}

}  // namespace

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdCompare)

CmdCompare::CmdCompare()
    : Command("Inspection_Compare")
{
    sAppModule = "Inspection";
    sGroup = QT_TR_NOOP("Inspection");
    sMenuText = QT_TR_NOOP("Compare…");
    sToolTipText = QT_TR_NOOP(
        "Compares two Part objects and highlights common, added, and removed material"
    );
    sStatusTip = sToolTipText;
    sWhatsThis = "Inspection_Compare";
    sPixmap = "inspection_compare";
}

void CmdCompare::activated(int)
{
    App::Document* appDocument = App::GetApplication().getActiveDocument();
    Gui::Document* guiDocument = Gui::Application::Instance->activeDocument();
    if (!appDocument || !guiDocument) {
        return;
    }

    std::vector<App::DocumentObject*> objects = getSelectedCompareCandidates(appDocument);

    if (objects.size() != 2) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Compare"),
            QObject::tr(
                "Select exactly two Part, Mesh, or Points objects. The first selection "
                "is the reference, the second is the compared object."
            )
        );
        return;
    }

    if (objects[0] == objects[1]) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Compare"),
            QObject::tr("The reference and compared object must be different.")
        );
        return;
    }

    guiDocument->openCommand(QT_TRANSLATE_NOOP("Command", "Compare"));

    if (objects[0]->isDerivedFrom<Part::Feature>() && objects[1]->isDerivedFrom<Part::Feature>()) {
        createExactCompare(appDocument, objects[0], objects[1]);
    }
    else {
        createDeviationCompare(appDocument, objects[0], objects[1]);
    }

    guiDocument->commitCommand();
    appDocument->recompute();
}

bool CmdCompare::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return false;
    }

    return getSelectedCompareCandidates(doc).size() >= 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdCompareDeviation)

CmdCompareDeviation::CmdCompareDeviation()
    : Command("Inspection_CompareDeviation")
{
    sAppModule = "Inspection";
    sGroup = QT_TR_NOOP("Inspection");
    sMenuText = QT_TR_NOOP("Compare by Deviation…");
    sToolTipText
        = QT_TR_NOOP("Compares two objects by measuring sampled deviation from the compared object to the reference");
    sStatusTip = sToolTipText;
    sWhatsThis = "Inspection_CompareDeviation";
    sPixmap = "inspection_compare";
}

void CmdCompareDeviation::activated(int)
{
    App::Document* appDocument = App::GetApplication().getActiveDocument();
    Gui::Document* guiDocument = Gui::Application::Instance->activeDocument();
    if (!appDocument || !guiDocument) {
        return;
    }

    std::vector<App::DocumentObject*> objects = getSelectedCompareCandidates(appDocument);

    if (objects.size() != 2) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Compare by Deviation"),
            QObject::tr(
                "Select exactly two Part, Mesh, or Points objects. The first selection "
                "is the reference, the second is the compared object."
            )
        );
        return;
    }

    if (objects[0] == objects[1]) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Compare by Deviation"),
            QObject::tr("The reference and compared object must be different.")
        );
        return;
    }

    guiDocument->openCommand(QT_TRANSLATE_NOOP("Command", "Compare by Deviation"));
    createDeviationCompare(appDocument, objects[0], objects[1]);
    guiDocument->commitCommand();
    appDocument->recompute();
}

bool CmdCompareDeviation::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return false;
    }

    return getSelectedCompareCandidates(doc).size() >= 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdInspectElement)

CmdInspectElement::CmdInspectElement()
    : Command("Inspection_InspectElement")
{
    sAppModule = "Inspection";
    sGroup = QT_TR_NOOP("Inspection");
    sMenuText = QT_TR_NOOP("Inspection…");
    sToolTipText = QT_TR_NOOP("Inspects distance information");
    sWhatsThis = "Inspection_InspectElement";
    sStatusTip = sToolTipText;
    sPixmap = "inspect_pipette";
}

void CmdInspectElement::activated(int)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setRedirectToSceneGraphEnabled(true);
        viewer->setRedirectToSceneGraph(true);
        viewer->setSelectionEnabled(false);
        viewer->setEditingCursor(
            QCursor(Gui::BitmapFactory().pixmapFromSvg("inspect_pipette", QSize(32, 32)), 4, 29)
        );
        viewer->addEventCallback(
            SoButtonEvent::getClassTypeId(),
            InspectionGui::ViewProviderInspection::inspectCallback
        );
    }
}

bool CmdInspectElement::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType<Inspection::Feature>() == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom<Gui::View3DInventor>()) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

void CreateInspectionCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdVisualInspection());
    rcCmdMgr.addCommand(new CmdCompare());
    rcCmdMgr.addCommand(new CmdCompareDeviation());
    rcCmdMgr.addCommand(new CmdInspectElement());
}
