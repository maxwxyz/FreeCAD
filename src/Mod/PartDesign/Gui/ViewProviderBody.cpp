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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Precision.hxx>
# include <QMenu>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <App/VarSet.h>
#include <Base/Console.h>
#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Gui/ViewProviderDatum.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureBase.h>
#include <Mod/PartDesign/App/AuxGroup.h>

#include "ViewProviderBody.h"
#include "Utils.h"
#include "ViewProvider.h"
#include "ViewProviderDatum.h"


using namespace PartDesignGui;
namespace sp = std::placeholders;

const char* PartDesignGui::ViewProviderBody::BodyModeEnum[] = {"Through","Tip",nullptr};

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderBody,PartGui::ViewProviderPart)

ViewProviderBody::ViewProviderBody()
{
    ADD_PROPERTY(DisplayModeBody,((long)0));
    DisplayModeBody.setEnums(BodyModeEnum);

    sPixmap = "PartDesign_Body.svg";

    Gui::ViewProviderOriginGroupExtension::initExtension(this);
}

ViewProviderBody::~ViewProviderBody()
{
}

void ViewProviderBody::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderPart::attach(pcFeat);

    //set default display mode
    onChanged(&DisplayModeBody);
}

// TODO on activating the body switch to the "Through" mode (2015-09-05, Fat-Zer)
// TODO different icon in tree if mode is Through (2015-09-05, Fat-Zer)
// TODO drag&drop (2015-09-05, Fat-Zer)
// TODO Add activate () call (2015-09-08, Fat-Zer)

void ViewProviderBody::setDisplayMode(const char* ModeName) {

    //if we show "Through" we must avoid to set the display mask modes, as this would result
    //in going into "tip" mode. When through is chosen the child features are displayed, and all
    //we need to ensure is that the display mode change is propagated to them from within the
    //onChanged() method.
    if(DisplayModeBody.getValue() == 1)
        PartGui::ViewProviderPartExt::setDisplayMode(ModeName);
}

void ViewProviderBody::setOverrideMode(const std::string& mode) {

    //if we are in through mode, we need to ensure that the override mode is not set for the body
    //(as this would result in "tip" mode), it is enough when the children are set to the correct
    //override mode.

    if(DisplayModeBody.getValue() != 0)
        Gui::ViewProvider::setOverrideMode(mode);
    else
        overrideMode = mode;
}

void ViewProviderBody::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Q_UNUSED(receiver);
    Q_UNUSED(member);
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);

    QAction* act = menu->addAction(tr("Active body"));
    act->setCheckable(true);
    act->setChecked(isActiveBody());
    func->trigger(act, [this]() {
        this->toggleActiveBody();
    });

    act = menu->addAction(tr("Toggle Auto Group"));
    func->trigger(act,
        [this]() {
            auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
            if (!body)
                return;
            try {
                std::vector<App::DocumentObjectT> groups;
                for (auto obj : body->Group.getValues()) {
                    if (obj->isDerivedFrom(PartDesign::AuxGroup::getClassTypeId())) {
                        auto group = static_cast<PartDesign::AuxGroup*>(obj);
                        if (group->getGroupType() != PartDesign::AuxGroup::OtherGroup)
                            groups.emplace_back(obj);
                    }
                }
                App::AutoTransaction committer("Toggle group");
                if (groups.empty()) {
                    int pos = 0;
                    auto children = body->Group.getValues();
                    if (children.size() && children[0] == body->BaseFeature.getValue())
                        ++pos;
                    auto sketchGroup = static_cast<PartDesign::AuxGroup*>(
                            body->getDocument()->addObject("PartDesign::AuxGroup", "Sketches"));
                    sketchGroup->Label.setValue("Sketches");
                    auto datumGroup = static_cast<PartDesign::AuxGroup*>(
                            body->getDocument()->addObject("PartDesign::AuxGroup", "Datums"));
                    datumGroup->Label.setValue("Datums");
                    auto miscGroup = static_cast<PartDesign::AuxGroup*>(
                            body->getDocument()->addObject("PartDesign::AuxGroup", "Misc"));
                    miscGroup->Label.setValue("Misc");
                    children.insert(children.begin()+pos, miscGroup);
                    children.insert(children.begin()+pos, datumGroup);
                    children.insert(children.begin()+pos, sketchGroup);
                    body->Group.setValues(children);
                    sketchGroup->_Body.setValue(body);
                    datumGroup->_Body.setValue(body);
                    miscGroup->_Body.setValue(body);
                } else {
                    for (auto & objT : groups) {
                        auto group = static_cast<PartDesign::AuxGroup*>(objT.getObject());
                        if (group) {
                            group->_Body.setValue(nullptr);
                            body->getDocument()->removeObject(group->getNameInDocument());
                        }
                    }
                }
            } catch (Base::Exception &e) {
                e.ReportException();
            }
    });

    act = menu->addAction(tr("Add group"));
    func->trigger(act,
        [this]() {
            auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
            if (!body)
                return;
            App::AutoTransaction committer("Body add group");
            try {
                auto group = static_cast<PartDesign::AuxGroup*>(
                        body->getDocument()->addObject("PartDesign::AuxGroup", "Group"));
                body->addObject(group);
                group->_Body.setValue(body);
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        });

    Gui::ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderBody::isActiveBody()
{
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if(!activeDoc)
        activeDoc = getDocument();
    auto activeView = activeDoc->setActiveView(this);
    if(!activeView)
        return false;

    if (activeView->isActiveObject(getObject(),PDBODYKEY)){
        return true;
    } else {
        return false;
    }
}

void ViewProviderBody::toggleActiveBody()
{
    if (isActiveBody()) {
        //active body double-clicked. Deactivate.
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s', None)", PDBODYKEY);
    } else {

        // assure the PartDesign workbench
        if(App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign")->GetBool("SwitchToWB", true))
            Gui::Command::assureWorkbench("PartDesignWorkbench");

        // and set correct active objects
        auto* part = App::Part::getPartOfObject ( getObject() );
        if ( part && !isActiveBody() ) {
            Gui::Command::doCommand(Gui::Command::Gui,
                    "Gui.ActiveDocument.ActiveView.setActiveObject('%s',%s)",
                    PARTKEY, Gui::Command::getObjectCmd(part).c_str());
        }

        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s',%s)",
                PDBODYKEY, Gui::Command::getObjectCmd(getObject()).c_str());
    }
}

bool ViewProviderBody::doubleClicked()
{
    toggleActiveBody();
    return true;
}


// TODO To be deleted (2015-09-08, Fat-Zer)
//void ViewProviderBody::updateTree()
//{
//    if (ActiveGuiDoc == NULL) return;
//
//    // Highlight active body and all its features
//    //Base::Console().error("ViewProviderBody::updateTree()\n");
//    PartDesign::Body* body = getObject<PartDesign::Body>();
//    bool active = body->IsActive.getValue();
//    //Base::Console().error("Body is %s\n", active ? "active" : "inactive");
//    ActiveGuiDoc->signalHighlightObject(*this, Gui::Blue, active);
//    std::vector<App::DocumentObject*> features = body->Group.getValues();
//    bool highlight = true;
//    App::DocumentObject* tip = body->Tip.getValue();
//    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
//        //Base::Console().error("Highlighting %s: %s\n", (*f)->getNameInDocument(), highlight ? "true" : "false");
//        Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(Gui::Application::Instance->getViewProvider(*f));
//        if (vp != NULL)
//            ActiveGuiDoc->signalHighlightObject(*vp, Gui::LightBlue, active ? highlight : false);
//        if (highlight && (tip == *f))
//            highlight = false;
//    }
//}

bool ViewProviderBody::onDelete ( const std::vector<std::string> &) {
    // TODO May be do it conditionally? (2015-09-05, Fat-Zer)
    FCMD_OBJ_CMD(getObject(),"removeObjectsFromDocument()");
    return true;
}

void ViewProviderBody::updateData(const App::Property* prop)
{
    PartDesign::Body* body = getObject<PartDesign::Body>();

    if (prop == &body->Group || prop == &body->BaseFeature) {
        //ensure all model features are in visual body mode
        setVisualBodyMode(true);
    }

    if (prop == &body->Tip) {
        // We changed Tip
        App::DocumentObject* tip = body->Tip.getValue();

        auto features = body->Group.getValues();

        // restore icons
        for (auto feature : features) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(feature);
            if (vp && vp->isDerivedFrom<PartDesignGui::ViewProvider>()) {
                static_cast<PartDesignGui::ViewProvider*>(vp)->setTipIcon(feature == tip);
            }
        }
    }

    PartGui::ViewProviderPart::updateData(prop);
}

void ViewProviderBody::onChanged(const App::Property* prop) {

    if(prop == &DisplayModeBody) {
        auto body = getObject<PartDesign::Body>();

        if ( DisplayModeBody.getValue() == 0 )  {
            //if we are in an override mode we need to make sure to come out, because
            //otherwise the maskmode is blocked and won't go into "through"
            if(getOverrideMode() != "As Is") {
                auto mode = getOverrideMode();
                ViewProvider::setOverrideMode("As Is");
                overrideMode = mode;
            }
            setDisplayMaskMode("Group");
            if(body)
                body->setShowTip(false);
        }
        else {
            if(body)
                body->setShowTip(true);
            if(getOverrideMode() == "As Is")
                setDisplayMaskMode(DisplayMode.getValueAsString());
            else {
                Base::Console().message("Set override mode: %s\n", getOverrideMode().c_str());
                setDisplayMaskMode(getOverrideMode().c_str());
            }
        }

        // #0002559: Body becomes visible upon changing DisplayModeBody
        Visibility.touch();
    }
    else
        unifyVisualProperty(prop);

    PartGui::ViewProviderPartExt::onChanged(prop);
}


void ViewProviderBody::unifyVisualProperty(const App::Property* prop) {

    if (!pcObject || isRestoring()) {
        return;
    }

    if (prop == &Visibility ||
        prop == &Selectable ||
        prop == &DisplayModeBody ||
        prop == &PointColorArray ||
        prop == &ShowPlacement ||
        prop == &LineColorArray) {
        return;
    }

    // Fixes issue 11197. In case of affected projects where the bounding box of a sub-feature
    // is shown allow it to hide it
    if (prop == &BoundingBox) {
        if (BoundingBox.getValue()) {
            return;
        }
    }

    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;

    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    auto features = body->Group.getValues();
    for (auto feature : features) {

        if (!feature->isDerivedFrom<PartDesign::Feature>()) {
            continue;
        }

        //copy over the properties data
        if (Gui::ViewProvider* vp = gdoc->getViewProvider(feature)) {
            if (auto fprop = vp->getPropertyByName(prop->getName())) {
                fprop->Paste(*prop);
            }
        }
    }
}

void ViewProviderBody::setVisualBodyMode(bool bodymode) {

    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;

    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    auto features = body->Group.getValues();
    for(auto feature : features) {

        if(!feature->isDerivedFrom<PartDesign::Feature>())
            continue;

        auto* vp = static_cast<PartDesignGui::ViewProvider*>(gdoc->getViewProvider(feature));
        if (vp) vp->setBodyMode(bodymode);
    }
}

std::vector< std::string > ViewProviderBody::getDisplayModes() const {

    //we get all display modes and remove the "Group" mode, as this is what we use for "Through"
    //body display mode
    std::vector< std::string > modes = ViewProviderPart::getDisplayModes();
    modes.erase(modes.begin());
    return modes;
}

bool ViewProviderBody::canDropObject(App::DocumentObject* obj) const
{
    PartDesign::Body* body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || !obj)
        return false;

    // Selection context is set in TreeWidget::checkDropEvent/dropEvent().
    // Position 0 (default) stores the dragging object. We want to check
    // whether the parent of the dragging object is an aux group. More
    // specifically, we want to know if the user is dragging an object out of
    // an aux group and drop it on to the owner body, which indicates the user
    // wants to remove and object from the aux group.
    if (auto group = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(
                Gui::Selection().getContext().getParent().getSubObject()))
    {
        return PartDesign::Body::findBodyOf(group) == body
            && group->getGroupType() == PartDesign::AuxGroup::OtherGroup;
    }

    if (!PartDesign::Body::isAllowed(obj) || PartDesign::Body::findBodyOf(obj) == body)
        return false;

    if (!body->getPrevSolidFeature()
            && !body->BaseFeature.getValue()
            && obj->isDerivedFrom(Part::Feature::getClassTypeId()))
        return true;

    // App::Part checking is too restrictive. It may mess up things, or it may
    // not. Just let user undo if anything is wrong.

    // App::Part *actPart = PartDesignGui::getActivePart();
    // App::Part* partOfBaseFeature = App::Part::getPartOfObject(obj);
    // if (partOfBaseFeature != 0 && partOfBaseFeature != actPart)
    //     return false;

    return true;
}

bool ViewProviderBody::canDragAndDropObject(App::DocumentObject * obj) const
{
    PartDesign::Body* body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return true;

    // Check if the parent of the dragging object is an aux group
    if (auto group = Base::freecad_dynamic_cast<PartDesign::AuxGroup>(
                Gui::Selection().getContext().getParent().getSubObject()))
    {
        if (PartDesign::Body::findBodyOf(group) == body
                && group->getGroupType() == PartDesign::AuxGroup::OtherGroup)
            return true;
    }

    if (!body->getPrevSolidFeature()
             && !body->BaseFeature.getValue()
             && obj->isDerivedFrom(Part::Feature::getClassTypeId())
             && !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        return false;
    return true;
}

bool ViewProviderBody::canDragObject(App::DocumentObject *obj) const
{
    if (obj->isDerivedFrom(PartDesign::AuxGroup::getClassTypeId()))
        return false;
    PartDesign::Body* body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return true;

    // Selection context is set in TreeWidget::checkDropEvent/dropEvent().
    // position 1 stores full object path to the dropping target.
    // position 0 (default) stores the dragging source, which points to this object.
    auto target = Gui::Selection().getContext(1).getSubObject();
    if (PartDesign::Body::findBodyOf(target) == body)
        return false;

    if (body->BaseFeature.getValue() == obj)
        return false;

    return PartDesignGui::isFeatureMovable(obj);
}

void ViewProviderBody::dropObject(App::DocumentObject* obj)
{
    auto* body = getObject<PartDesign::Body>();
    if (obj->isDerivedFrom<Part::Part2DObject>()
        || obj->isDerivedFrom<App::DatumElement>()
        || obj->isDerivedFrom<App::LocalCoordinateSystem>()) {
        body->addObject(obj);
    }
    else if (PartDesign::Body::isAllowed(obj) && PartDesignGui::isFeatureMovable(obj)) {
        std::vector<App::DocumentObject*> move;
        move.push_back(obj);
        std::vector<App::DocumentObject*> deps = PartDesignGui::collectMovableDependencies(move);
        move.insert(std::end(move), std::begin(deps), std::end(deps));

        PartDesign::Body* source = PartDesign::Body::findBodyOf(obj);
        if (source == body) {
            // To allow object to be dragged out of an aux group, we do not
            // consider it as an error to add an object already in the body.
            //
            // FC_THROWM(Base::RuntimeError, "Feature already inside the body");
        }
        else {
            if(source)
                source->removeObjects(move);
            body->addObjects(move);
        }
    } else {
        bool canWrap = true;
        bool hasDep = false;
        auto deps = obj->getOutList();
        for (auto dep : deps) {
            if (PartDesign::Body::findBodyOf(dep) == body) {
                if (dep->isDerivedFrom(PartDesign::FeatureWrap::getClassTypeId()))
                    FC_THROWM(Base::RuntimeError, "Feature has already been wrapped inside the body");
                hasDep = true;
            } else
                canWrap = false;
        }
        if (hasDep && canWrap) {
            auto wrap = static_cast<PartDesign::FeatureWrap*>(
                    body->newObjectAt("PartDesign::FeatureWrap", "Wrap", deps, false));
            wrap->Label.setValue(obj->Label.getValue());
            wrap->WrapFeature.setValue(obj);
            obj = wrap;
        }
        else if (elements.size() || hasDep) {
            auto binder = static_cast<PartDesign::SubShapeBinder*>(
                    body->getDocument()->addObject("PartDesign::SubShapeBinder", "Binder"));
            std::map<App::DocumentObject *, std::vector<std::string> > links;
            auto & subs = links[owner];
            std::string sub(subname ? subname : "");
            for (auto & element : elements)
                subs.emplace_back(sub + element);
            binder->setLinks(std::move(links));
            body->addObject(binder);
            obj = binder;
        }
        else if (!body->getPrevSolidFeature() && !body->BaseFeature.getValue()) {
            if (owner == obj) {
                body->BaseFeature.setValue(obj);
                auto tip = body->Tip.getValue();
                if (tip)
                    return std::string(tip->getNameInDocument()) + ".";
                return std::string();
            }

            auto binder = static_cast<PartDesign::SubShapeBinder*>(
                    body->getDocument()->addObject("PartDesign::SubShapeBinder",
                                                    "BaseFeature"));
            std::map<App::DocumentObject *, std::vector<std::string> > links;
            auto & subs = links[owner];
            if (subname && subname[0])
                subs.emplace_back(subname);
            binder->setLinks(std::move(links));
            auto children = body->Group.getValues();
            children.insert(children.begin(), binder);
            body->Group.setValues(children);
            body->BaseFeature.setValue(binder);
            obj = binder;
        }
    }
    return std::string(obj->getNameInDocument()) + ".";
}

bool ViewProviderBody::canReplaceObject(App::DocumentObject *oldObj,
                                        App::DocumentObject *newObj)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || !oldObj || !newObj || oldObj == newObj
            || oldObj == body->BaseFeature.getValue()
            || newObj == body->BaseFeature.getValue())
        return false;

    // Selection context is set in TreeWidget::checkDropEvent/dropEvent().
    // position 1 stores full object path to the dropping target. Here, it is the newObj.
    // position 0 (default) stores the dragging source, which is the oldObj.
    auto newObjParent = Gui::Selection().getContext(1).getParent().getSubObject();
    auto oldObjParent = Gui::Selection().getContext(0).getParent().getSubObject();
    if (newObjParent && oldObjParent
            && newObjParent != oldObjParent
            && (newObjParent->isDerivedFrom(PartDesign::AuxGroup::getClassTypeId())
                || oldObjParent->isDerivedFrom(PartDesign::AuxGroup::getClassTypeId())))
        return false;

    if (!body->Group.find(oldObj->getNameInDocument())
            || !body->Group.find(newObj->getNameInDocument()))
        return false;

    if (!newObj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        return true;

    return body->isSibling(oldObj, newObj);
}

int ViewProviderBody::replaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || !canReplaceObject(oldObj, newObj))
        return 0;
    App::AutoTransaction committer("Reorder body feature");
    bool needCheckSiblings = false;
    if (!_reorderObject(body, newObj, oldObj, true, needCheckSiblings))
        return 0;
    if (needCheckSiblings) {
        checkingSiblings = false;
        if (!checkSiblings())
            buildExport();
    }
    Gui::Command::updateActive();
    return 1;
}

bool ViewProviderBody::reorderObjects(const std::vector<App::DocumentObject *> &objs,
                                      App::DocumentObject *before)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return false;

    App::DocumentObject *last = before;
    for (auto obj : objs) {
        if (!canReorderObject(obj, last))
            return false;
        last = obj;
    }

    App::Document* doc  = body->getDocument();
    doc->recompute();

    last = before;
    for (auto obj : objs) {
        if (!_reorderObject(body, obj, last, false, needCheckSiblings))
            return false;
        last = obj;
    }
    if (needCheckSiblings) {
        checkingSiblings = false;
        if (!checkSiblings())
            buildExport();
    }
    Gui::Command::updateActive();
    return 1;
}

bool ViewProviderBody::_reorderObject(PartDesign::Body *body,
                                      App::DocumentObject *obj,
                                      App::DocumentObject *before,
                                      bool canSwap,
                                      bool &needCheckSiblings)
{
    auto oldObj = before;
    auto newObj = obj;

    int i, j;
    if (!body->Group.find(oldObj->getNameInDocument(), &i)
            || !body->Group.find(newObj->getNameInDocument(), &j))
        return false;

    auto secondFeat = Base::freecad_dynamic_cast<PartDesign::Feature>(oldObj);
    auto firstFeat = Base::freecad_dynamic_cast<PartDesign::Feature>(newObj);

    // In case the old object has self sibling group, repoint the old object to
    // the earliest sibling
    if (i < j && secondFeat && secondFeat->_Siblings.getSize()) {
        const auto & siblings = secondFeat->_Siblings.getValues();
        for (auto rit=siblings.rbegin(); rit!=siblings.rend(); ++rit) {
            auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(*rit);
            if (feat) {
                if (feat != newObj && body->Group.find(feat->getNameInDocument(), &i))
                    secondFeat = feat;
                break;
            }
        }
    }

    // This function is used by both reorderObjects() and replaceObject().
    // 'first', 'second' here refers to the order after replaceObject()
    // operation
    if (canSwap && i > j)
        std::swap(secondFeat, firstFeat);

    auto objs = body->Group.getValues();
    objs.erase(objs.begin() + j);
    if (!canSwap && j < i)
        --i;
    objs.insert(objs.begin() + i, newObj);

    Base::StateLocker guard(checkingSiblings); // delay sibling check
    body->Group.setValues(objs);

    if (firstFeat && secondFeat) {
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard1(App::Property::User3, &firstFeat->BaseFeature);
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard2(App::Property::User3, &secondFeat->BaseFeature);

        if (secondFeat->NewSolid.getValue()) {
            secondFeat->NewSolid.setValue(false);
            firstFeat->NewSolid.setValue(true);
        }
        firstFeat->BaseFeature.setValue(secondFeat->BaseFeature.getValue());
        secondFeat->BaseFeature.setValue(firstFeat);
        for (auto obj : objs) {
            if (obj == secondFeat
                    || !body->isSolidFeature(obj)
                    || !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
                continue;
            auto feat = static_cast<PartDesign::Feature*>(obj);
            if (feat->BaseFeature.getValue() == firstFeat)
                feat->BaseFeature.setValue(secondFeat);
        }

        if (body->Tip.getValue() == firstFeat)
            body->setTip(secondFeat);

        needCheckSiblings = true;
    }
    return true;
}

bool ViewProviderBody::canReorderObject(App::DocumentObject* obj, App::DocumentObject* before)
{
    return canReplaceObject(before, obj);
}

std::vector<App::DocumentObject*> ViewProviderBody::claimChildren3D(void) const {
    auto children = inherited::claimChildren3D();
    for(auto it=children.begin();it!=children.end();) {
        auto obj = *it;
        if(obj->isDerivedFrom(PartDesign::Solid::getClassTypeId()))
            it = children.erase(it);
        else
            ++it;
    }
    return children;
}

void ViewProviderBody::groupSiblings(PartDesign::Feature *feat, bool collapse, bool all)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return;

    std::string cmd(collapse ? "Collapse" : "Expand");
    cmd += " body solid";
    App::AutoTransaction committer(cmd.c_str());
    if (all) {
        auto siblings = body->getSiblings(feat, true, true);
        PartDesign::Feature *feat = nullptr;
        while (siblings.size()) {
            feat = Base::freecad_dynamic_cast<PartDesign::Feature>(siblings.front());
            siblings.pop_front();
            if (feat)
                break;
            }
        }
    }
}
