#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerExportDialog_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerExportDialog_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/UiDesigner/Services/UiDesignerServices.h>

namespace Upp {

class UiDesignerExportDialog : public TopWindow {
public:
    typedef UiDesignerExportDialog CLASSNAME;

    UiDesignerExportDialog(UiDesignerSession& session,
                           UiDesignerExportProfile profile);

    bool Execute();
    const UiDesignerExportResult& GetResult() const { return result_; }

    virtual void Layout() override;

private:
    void BuildControls();
    void Browse();
    void ProfileChanged();
    void RefreshPreview();
    void ExportNow();
    UiDesignerExportProfile SelectedProfile() const;
    UiDesignerExportRequest BuildRequest(String& error) const;

    UiDesignerSession& session_;
    UiDesignerExportProfile initial_profile_;
    UiDesignerExportResult result_;

    UiLabel profile_label_;
    UiLabel destination_label_;
    UiLabel package_label_;
    UiLabel class_label_;
    UiLabel namespace_label_;
    UiLabel appearance_label_;
    UiLabel overwrite_label_;

    UiDropdown profile_;
    UiLineEdit destination_;
    UiButton browse_;
    UiLineEdit package_;
    UiLineEdit class_;
    UiLineEdit namespace_;
    UiDropdown appearance_;
    UiDropdown overwrite_;
    UiCheckBox include_design_;
    UiCheckBox include_theme_;
    UiCheckBox preserve_user_;

    UiLabel inventory_label_;
    UiMultiEdit inventory_;
    UiLabel status_;
    UiButton refresh_;
    UiButton export_;
    UiButton cancel_;
};

}

#endif
