#include "ViewProviderSectionPlane.h"
#include <Gui/ViewProviderBuilder.h>
#include <Mod/Part/App/FeatureSectionPlane.h>
#include <Inventor/nodes/SoSeparator.h>

using namespace PartGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartGui::ViewProviderSectionPlane, Gui::ViewProviderGeometryObject)

void ViewProviderSectionPlane::init()
{
    Gui::ViewProviderBuilder::add(
        Part::SectionPlane::getClassTypeId(),
        ViewProviderSectionPlane::getClassTypeId()
    );
}

ViewProviderSectionPlane::ViewProviderSectionPlane()
  : Gui::ViewProviderGeometryObject()
{
    clipPlane = new SoClipPlane;
}

void ViewProviderSectionPlane::attach(App::DocumentObject* obj)
{
    Gui::ViewProviderGeometryObject::attach(obj);
    SoSeparator* sep = static_cast<SoSeparator*>(pRoot);
    sep->addChild(clipPlane);
    onChanged(nullptr);
}

void ViewProviderSectionPlane::detach()
{
    SoSeparator* sep = static_cast<SoSeparator*>(pRoot);
    sep->removeChild(clipPlane);
    Gui::ViewProviderGeometryObject::detach();
}

QIcon ViewProviderSectionPlane::getIcon() const
{
    return QIcon(":/data/img/Part_Section.svg");
}

void ViewProviderSectionPlane::onChanged(const App::Property* /*prop*/)
{
    if (!getObject()) return;
    auto* fp = static_cast<Part::SectionPlane*>(getObject());

    // toggle clipping on/off
    clipPlane->on.setValue(fp->Enabled.getValue());

    // compute world‐space plane from local Z=0 plane
    Base::Vector3d normal, origin;
    fp->Placement.multVec(Base::Vector3d(0,0,1), normal);
    fp->Placement.multVec(Base::Vector3d(0,0,0), origin);
    double d = - normal.Dot(origin);

    SbPlane sbp(SbVec3f((float)normal.x, (float)normal.y, (float)normal.z), (float)d);
    clipPlane->plane.setValue(sbp);
}

void ViewProviderSectionPlane::updateData(const App::Property* prop)
{
    onChanged(prop);
}