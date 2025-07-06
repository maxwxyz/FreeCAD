#include "FeatureSectionPlane.h"
#include <Base/Console.h>

using namespace Part;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::SectionPlane, Part::DatumPlane)

SectionPlane::SectionPlane()
{
    ADD_PROPERTY_TYPE(Enabled, (true),   "Section", App::Prop_None,   "Activate clipping");
    ADD_PROPERTY_TYPE(FlipDirection, (false), "Section", App::Prop_None,   "Flip clip normal");
    ADD_PROPERTY_TYPE(CutObjects, (std::vector<App::DocumentObject*>()), "Section", App::Prop_None, "Objects to clip");
    ADD_PROPERTY_TYPE(ExcludeObjects, (std::vector<App::DocumentObject*>()), "Section", App::Prop_None, "Objects to exclude");
    ADD_PROPERTY_TYPE(SectionProfile, (static_cast<App::DocumentObject*>(nullptr)), "Result", App::Prop_Output, "Section result");
}

void SectionPlane::onChanged(const App::Property* prop)
{
    DatumPlane::onChanged(prop);
    if (prop == &Enabled || prop == &FlipDirection || prop == &CutObjects || prop == &ExcludeObjects) {
        this->touch();
    }
}

short SectionPlane::mustExecute() const
{
    return 1;
}

App::DocumentObjectExecReturn* SectionPlane::execute()
{
    Base::Console().message("[SectionPlane] execute\n");
    // (no geometry update yet)
    return App::DocumentObject::StdReturn;
}