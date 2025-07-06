#pragma once

#include <App/DocumentObject.h>
#include <Mod/Part/App/Datums.h>
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>

namespace Part {

class SectionPlane : public DatumPlane {
  PROPERTY_HEADER_WITH_EXTENSIONS(Part::SectionPlane);

public:
  SectionPlane();
  ~SectionPlane() override = default;

  App::PropertyBool Enabled;
  App::PropertyBool FlipDirection;
  App::PropertyLinkList CutObjects;
  App::PropertyLinkList ExcludeObjects;
  App::PropertyLink SectionProfile;

  void onChanged(const App::Property* prop) override;
  short mustExecute() const override;
  App::DocumentObjectExecReturn* execute() override;
};

}