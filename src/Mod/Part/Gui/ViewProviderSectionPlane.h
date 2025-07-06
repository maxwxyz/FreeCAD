#include <Mod/Part/Gui/Export.h>             // for PartGuiExport
#include <Gui/ViewProviderGeometryObject.h>
#include <Inventor/nodes/SoClipPlane.h>

namespace PartGui {

class PartGuiExport ViewProviderSectionPlane : public Gui::ViewProviderGeometryObject {
    PROPERTY_HEADER_WITH_EXTENSIONS(PartGui::ViewProviderSectionPlane);

    public:
    ViewProviderSectionPlane();
    ~ViewProviderSectionPlane() override = default;

    static void init();

    void attach(App::DocumentObject* obj) override;
    void detach() override;
    QIcon getIcon() const override;
    void onChanged(const App::Property* prop) override;
    void updateData(const App::Property* prop) override;

    private:
    SoClipPlane* clipPlane = nullptr;
};

}