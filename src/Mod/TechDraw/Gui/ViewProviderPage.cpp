/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <QList>
# include <QMenu>
# include <QMessageBox>
# include <QPointer>
# include <QTextStream>

# include <boost/signals2.hpp>
# include <boost/signals2/connection.hpp>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "ViewProviderPage.h"
#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGITemplate.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "ViewProviderPageExtension.h"
#include "ViewProviderTemplate.h"
#include "ViewProviderViewPart.h"


using namespace TechDrawGui;
using namespace TechDraw;
namespace sp = std::placeholders;

PROPERTY_SOURCE(TechDrawGui::ViewProviderPage, Gui::ViewProviderDocumentObject)


//**************************************************************************
// Construction/Destruction

ViewProviderPage::ViewProviderPage()
    : m_mdiView(nullptr),  m_graphicsView(nullptr), m_graphicsScene(nullptr)
{
    initExtension(this);

    sPixmap = "TechDraw_TreePage";
    static const char* group = "Grid";

    // NOLINTBEGIN
    ADD_PROPERTY_TYPE(ShowFrames, (true), group, App::Prop_None,
                      "Show or hide View frames and Labels on this Page");
    ADD_PROPERTY_TYPE(ShowGrid, (PreferencesGui::showGrid()), group, App::Prop_None,
                      "Show or hide a grid on this Page");
    ADD_PROPERTY_TYPE(GridSpacing, (PreferencesGui::gridSpacing()), group,
                      (App::PropertyType::Prop_None), "Grid line spacing in mm");
    // NOLINTEND

    ShowFrames.setStatus(App::Property::Hidden, true);
    // Do not show in property editor   why? wf  WF: because DisplayMode applies only to coin and we
    // don't use coin.
    DisplayMode.setStatus(App::Property::Hidden, true);

    m_graphicsScene = new QGSPage(this);
    m_graphicsScene->setItemIndexMethod(
        QGraphicsScene::NoIndex);//this prevents crash when deleting dims.
                                 //scene(view?) indices of dirty regions gets
                                 //out of sync.  missing prepareGeometryChange
                                 //somewhere???? QTBUG-18021???
}

ViewProviderPage::~ViewProviderPage()
{
    removeMDIView();//if the MDIViewPage is still in MainWindow, remove it.
    m_graphicsScene->deleteLater();
}

void ViewProviderPage::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    //NOLINTBEGIN
    auto bnd = std::bind(&ViewProviderPage::onGuiRepaint, this, sp::_1);
    //NOLINTEND
    auto* feature = dynamic_cast<TechDraw::DrawPage*>(pcFeat);
    if (feature) {
        connectGuiRepaint = feature->signalGuiPaint.connect(bnd);
        if (feature->isAttachedToDocument()) {
            // it could happen that feature is not completely in the document yet and getNameInDocument returns
            // nullptr, so we only update m_myName if we got a valid string.
            m_pageName = feature->getNameInDocument();
        }
        m_graphicsScene->setObjectName(QString::fromLocal8Bit(m_pageName.c_str()));
    }
}

void ViewProviderPage::onChanged(const App::Property* prop)
{
    if (prop == &(ShowGrid) ||
        prop == &(GridSpacing)) {
        setGrid();
    }
    else if (prop == &Visibility) {
        //Visibility changes are handled in VPDO::onChanged -> show() or hide()
    }

    Gui::ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderPage::updateData(const App::Property* prop)
{
    auto page = getDrawPage();
    if (!page) {
        Gui::ViewProviderDocumentObject::updateData(prop);
        return;
    }
    if (prop == &(page->KeepUpdated)) {
        if (getDrawPage()->KeepUpdated.getValue()) {
            sPixmap = "TechDraw_TreePage";
        }
        else {
            sPixmap = "TechDraw_TreePageUnsync";
        }
        signalChangeIcon();
        //if the template is changed, rebuild the visual
    }
    else if (prop == &(page->Template)) {
        if (!page->isUnsetting()) {
            //check if a template has been added to scene first?
            m_graphicsScene->matchSceneRectToTemplate();
            m_graphicsScene->updateTemplate();
        }
    }
    else if (prop == &(page->Label)) {
        if (m_mdiView && !page->isUnsetting()) {
            m_mdiView->setTabText(page->Label.getValue());
        }
    }
    else if (prop == &page->Views) {
        if (!page->isUnsetting()) {
            m_graphicsScene->fixOrphans();
        }
    }

    Gui::ViewProviderDocumentObject::updateData(prop);
}

bool ViewProviderPage::onDelete(const std::vector<std::string>& parms)
{
    Q_UNUSED(parms)
    // warn the user if the Page is not empty
    // but don't do this if there is just the template

    // check if there are items in the group
    auto objs = claimChildren();

    // check if there is just a template
    // if there are several objects, the template is never the last one
    // the ExportName of a template always begins with "Template"
    bool isTemplate = false;
    for (auto objsIterator : objs) {
        isTemplate = (objsIterator->getExportName().substr(0, 8).compare(std::string("Template")) == 0);
    }

    if (!objs.empty() && !isTemplate) {
        // generate dialog
        QString bodyMessage;
        QTextStream bodyMessageStream(&bodyMessage);
        bodyMessageStream << qApp->translate(
            "Std_Delete",
            "The page is not empty, therefore the\nfollowing referencing objects might be lost:");
        bodyMessageStream << '\n';
        for (const auto& obj : objs) {
             bodyMessageStream << '\n' << QString::fromUtf8(obj->Label.getValue());

        }
        bodyMessageStream << "\n\n" << QObject::tr("Are you sure you want to continue?");
        // show and evaluate the dialog
        int DialogResult = QMessageBox::warning(
            Gui::getMainWindow(), qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
            QMessageBox::Yes, QMessageBox::No);
        if (DialogResult == QMessageBox::Yes) {
            removeMDIView();
            return true;
        }
        return false;
    }
    else {
        removeMDIView();
        return true;
    }
}

void ViewProviderPage::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
    QAction* act = menu->addAction(QObject::tr("Show drawing"), receiver, member);
    act->setData(QVariant((int)ShowDrawing));
    QAction* act2 = menu->addAction(QObject::tr("Toggle Keep Updated"), receiver, member);
    act2->setData(QVariant((int)ToggleUpdate));
}

bool ViewProviderPage::setEdit(const int ModNum)
{
    if (ModNum == ShowDrawing) {
        showMDIViewPage();// show the drawing
        return false;     //finished editing
    }

    if (ModNum == ToggleUpdate) {
        auto page = getDrawPage();
        if (page) {
            page->KeepUpdated.setValue(!page->KeepUpdated.getValue());
            page->recomputeFeature();
        }
        return false;
    }
    return Gui::ViewProviderDocumentObject::setEdit(ModNum);
}

void ViewProviderPage::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    return;
}

bool ViewProviderPage::doubleClicked()
{
    if (Preferences::switchOnClick()) {
        Gui::Command::assureWorkbench("TechDrawWorkbench");
        show();
    }

    return true;
}

void ViewProviderPage::show()
{
    showMDIViewPage();
    ViewProviderDocumentObject::show();
}

void ViewProviderPage::hide()
{
    if (getMDIView()) {
        getMDIView()->hide();  // this doesn't remove the mdiViewPage from the mainWindow
        removeMDIView();
    }
    ViewProviderDocumentObject::hide();
}

bool ViewProviderPage::showMDIViewPage()
{
    if (m_mdiView.isNull()) {
        // if our tab has been closed, or if this is first time
        createMDIViewPage();
        m_graphicsScene->addChildrenToPage();
        m_graphicsScene->updateTemplate(true);
        m_graphicsScene->redrawAllViews();
        m_graphicsScene->fixOrphans(true);

        m_mdiView->viewAll();
        m_mdiView->showMaximized();
    }
    else {
        // we already have a tab, so just tidy up and switch to it
        m_graphicsScene->redrawAllViews();
        m_graphicsScene->fixOrphans(true);
        m_graphicsView->update();
        Gui::getMainWindow()->setActiveWindow(m_mdiView);
    }

    m_graphicsView->centerOnPage();

    setGrid();

    Visibility.setValue(true);

    return true;
}

void ViewProviderPage::createMDIViewPage()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(pcObject->getDocument());
    m_mdiView = new MDIViewPage(this, doc, Gui::getMainWindow());
    if (!m_graphicsView) {
        m_graphicsView = new QGVPage(this, m_graphicsScene, m_mdiView);
        std::string objName = m_pageName + "View";
        m_graphicsView->setObjectName(QString::fromLocal8Bit(objName.c_str()));
    }
    m_mdiView->setScene(m_graphicsScene, m_graphicsView);
    QString tabTitle = QString::fromUtf8(getDrawPage()->Label.getValue());

    m_mdiView->setDocumentObject(getDrawPage()->getNameInDocument());
    m_mdiView->setDocumentName(pcObject->getDocument()->getName());

    m_mdiView->setWindowTitle(tabTitle + QStringLiteral("[*]"));
    m_mdiView->setWindowIcon(Gui::BitmapFactory().pixmap("TechDraw_TreePage"));
    Gui::getMainWindow()->addWindow(m_mdiView);
}

void ViewProviderPage::switchToMdiViewPage()
{
    show();
    m_graphicsView->setFocus();
}

//NOTE: removing MDIViewPage (parent) destroys QGVPage (eventually)
void ViewProviderPage::removeMDIView()
{
    if (!m_mdiView.isNull()) {//m_mdiView is a QPointer
        QList<QWidget*> wList = Gui::getMainWindow()->windows();
        if (wList.contains(m_mdiView)) {
            Gui::getMainWindow()->removeWindow(m_mdiView);
            m_mdiView = nullptr;     //m_mdiView will eventually be deleted and
            m_graphicsView = nullptr;//will take m_graphicsView with it
            Gui::MDIView* aw =
                Gui::getMainWindow()
                    ->activeWindow();//WF: this bit should be in the remove window logic, not here.
            if (aw) {
                aw->showMaximized();
            }
        }
    }
}

MDIViewPage* ViewProviderPage::getMDIViewPage() const
{
    if (m_mdiView.isNull()) {
        return nullptr;
    }
    return m_mdiView;
}

DrawTemplate* ViewProviderPage::getTemplate() const
{
    return freecad_cast<DrawTemplate*>(getDrawPage()->Template.getValue());
}


QGITemplate* ViewProviderPage::getQTemplate() const
{
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(getDrawPage()->getDocument());
    if (guiDoc) {
        Gui::ViewProvider* vp = guiDoc->getViewProvider(getTemplate());
        auto vpTemplate = freecad_cast<ViewProviderTemplate*>(vp);
        if (vpTemplate) {
            return vpTemplate->getQTemplate();
        }
    }
    return nullptr;
}

std::vector<App::DocumentObject*> ViewProviderPage::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;

    App::DocumentObject* templateFeat = nullptr;
    templateFeat = getDrawPage()->Template.getValue();

    if (templateFeat) {
        temp.push_back(templateFeat);
    }

    // Collect any child views
    // for Page, valid children are any View except: DrawViewDimension
    //                                               DrawViewBalloon
    //                                               any FeatureView in a DrawViewClip
    //                                               DrawHatch
    //                                               DrawGeomHatch
    //  ?? leaders?

    try {
        for (auto* obj : getDrawPage()->Views.getValues()) {
            if (!obj) {
                continue;
            }
            auto* featView = dynamic_cast<TechDraw::DrawView*>(obj);

            // If the child view appoints a parent, skip it
            if (featView && featView->claimParent()) {
                continue;
            }

            // Don't collect if dimension, balloon, hatch or member of ClipGroup as these should be grouped elsewhere
            if (obj->isDerivedFrom<TechDraw::DrawViewDimension>()
                || obj->isDerivedFrom<TechDraw::DrawHatch>()
                || obj->isDerivedFrom<TechDraw::DrawGeomHatch>()
                || obj->isDerivedFrom<TechDraw::DrawViewBalloon>()
                || (featView && featView->isInClip())) {
                continue;
            }
            temp.push_back(obj);
        }
        return temp;
    }
    catch (...) {
        return {};
    }
}

bool ViewProviderPage::isShow() const { return Visibility.getValue(); }

bool ViewProviderPage::getFrameState() const { return ShowFrames.getValue(); }

void ViewProviderPage::setFrameState(bool state) { ShowFrames.setValue(state); }

void ViewProviderPage::toggleFrameState()
{
    if (m_graphicsScene) {
        setFrameState(!getFrameState());
        m_graphicsScene->refreshViews();
        setTemplateMarkers(getFrameState());
    }
}

void ViewProviderPage::setTemplateMarkers(bool state) const
{
    App::DocumentObject* templateFeat = nullptr;
    templateFeat = getDrawPage()->Template.getValue();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(templateFeat->getDocument());
    Gui::ViewProvider* vp = guiDoc->getViewProvider(templateFeat);
    auto* vpt = freecad_cast<ViewProviderTemplate*>(vp);
    if (vpt) {
        vpt->setMarkers(state);
        QGITemplate* t = vpt->getQTemplate();
        if (t) {
            t->updateView(true);
        }
    }
}

bool ViewProviderPage::canDelete(App::DocumentObject* obj) const
{
    // deletions from a page don't necessarily destroy anything
    // thus we can pass this action
    // if an object could break something, like e.g. the template object
    // its ViewProvider handles this in the onDelete() function
    Q_UNUSED(obj)
    return true;
}

bool ViewProviderPage::canDragObjects() const
{
    return getVPPExtension()->extensionCanDragObjects();
}

bool ViewProviderPage::canDragObject(App::DocumentObject* docObj) const
{
    return getVPPExtension()->extensionCanDragObject(docObj);
}

bool ViewProviderPage::canDropObjectEx(App::DocumentObject* obj, App::DocumentObject* owner,
                                       const char* subname,
                                       const std::vector<std::string>& elements) const
{
    return getVPPExtension()->extensionCanDropObjectEx(obj, owner, subname, elements);
}

bool ViewProviderPage::canDropObject(App::DocumentObject* docObj) const
{
    return getVPPExtension()->extensionCanDropObject(docObj);
}

void ViewProviderPage::dropObject(App::DocumentObject* docObj)
{
    getVPPExtension()->extensionDropObject(docObj);
}

//! Redo the whole visual page
void ViewProviderPage::onGuiRepaint(const TechDraw::DrawPage* dp)
{
    if (dp == getDrawPage()) {
        //this signal is for us
        if (!getDrawPage()->isUnsetting()) {
            m_graphicsScene->fixOrphans();
        }
    }
}

TechDraw::DrawPage* ViewProviderPage::getDrawPage() const
{
    //during redo, pcObject can become invalid, but non-zero??
    if (!pcObject) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawPage*>(pcObject);
}

Gui::MDIView* ViewProviderPage::getMDIView() const { return m_mdiView.data(); }

void ViewProviderPage::setGrid()
{
    TechDraw::DrawPage* dPage = getDrawPage();
    if (!dPage) {
        return;
    }
    constexpr int A4LandscapeWide{298};
    constexpr int A4LandscapeHigh{215};

    int pageWidth{A4LandscapeWide};
    int pageHeight{A4LandscapeHigh};
    //
    double gridStep = GridSpacing.getValue() > 0 ? GridSpacing.getValue() : 10.0;
    if (dPage) {
        pageWidth = floor(dPage->getPageWidth()); // combining these 2 lines
        pageWidth = std::max(1, pageWidth);       // causes 'no matching function' error
        pageHeight = floor(dPage->getPageHeight());   // and again
        pageHeight = std::max(1, pageHeight);
    }
    QGVPage* widget = getQGVPage();
    if (widget) {
        if (ShowGrid.getValue()) {
            widget->showGrid(true);
            widget->makeGrid(pageWidth, pageHeight, gridStep);
        }
        else {
            widget->showGrid(false);
        }
        widget->updateViewport();
    }
}

QGSPage* ViewProviderPage::getQGSPage() const
{
    return m_graphicsScene;
}

QGVPage* ViewProviderPage::getQGVPage() const
{
    return m_graphicsView;
}

ViewProviderPageExtension* ViewProviderPage::getVPPExtension() const
{
    auto vpe = getExtensionByType<ViewProviderPageExtension>();
    auto vppe = static_cast<ViewProviderPageExtension*>(vpe);
    return vppe;
}

const char* ViewProviderPage::whoAmI() const { return m_pageName.c_str(); }


void ViewProviderPage::fixSceneDependencies()
{
    App::Document* doc = getDrawPage()->getDocument();
    std::vector<App::DocumentObject*> docObjs =
        doc->getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    for (auto& obj : docObjs) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;// can't fix this one
        }
        auto* vpvp = dynamic_cast<TechDrawGui::ViewProviderViewPart*>(vp);
        if (!vpvp) {
            continue;// can't fix this one
        }
        vpvp->fixSceneDependencies();
    }

}

//! convenient way to ask feature to redraw everything
void  ViewProviderPage::redrawPage() const
{
    auto feature = getDrawPage();
    if (feature) {
        feature->redrawCommand();
    }
}
