#ifndef _examples_UiCollectionDemo_h_
#define _examples_UiCollectionDemo_h_

// C Edwards (dodobar), Apache-2.0. One native collection designer: author a
// List, Gallery or comparison without a second semantic item collection.
// PropertyEditor models own configuration; the Data page projects one record
// from the shared production model. Borrowed models/factory outlive their views.
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

namespace Upp {

class UiCollectionDemo : public TopWindow {
public:
    typedef UiCollectionDemo CLASSNAME;
    UiCollectionDemo();
    ~UiCollectionDemo();
    void Layout() override;
    void Paint(Draw& w) override;
    bool RunAcceptance(const String& directory);

private:
    void BuildShell();
    void BuildProperties();
    void BuildData(int count);
    void ConnectEvents();
    void ApplyConfiguration(const String& changed = String());
    void ApplyTheme();
    void SelectPage(int page);
    void SelectView(const String& view);
    void InspectItem(int index);
    void ApplyData(const String& id);
    void UpdateStatus();
    void UpdateCode();
    String GenerateCode() const;
    UiList::Style ListStyle() const;
    UiGallery::Style GalleryStyle() const;
    UiItemRenderStyle RenderStyle() const;
    Value Config(const String& id) const;
    Value Override(const String& id) const;
    bool Active(const String& id) const;
    bool AnyOverride(const String& prefix) const;
    void Reset(PropertyEditorModel& model, const String& id);

    UiListModel model;
    Vector<Image> sample_images;
    PropertyEditorFactory factory;
    PropertyEditorModel inspector_model, override_model, data_model;
    UiTitleCard tc_header;
    UiBoxLayout box_header { UiDirection::H }, box_view { UiDirection::H };
    UiBoxLayout box_pages { UiDirection::H }, box_data_tools { UiDirection::H };
    UiToolButton btn_theme, btn_help, btn_exit;
    UiToolButton btn_list, btn_gallery, btn_compare, btn_first, btn_last;
    UiToolButton btn_inspector, btn_overrides, btn_data, btn_code, btn_copy;
    UiToolButton btn_add, btn_remove;
    UiPanel pnl_preview, pnl_rail, pnl_inspector, pnl_overrides, pnl_data, pnl_code;
    UiStack stk_pages;
    UiList list;
    UiGallery gallery;
    UiLabel lbl_status, lbl_list, lbl_gallery;
    PropertyEditor pe_inspector, pe_overrides, pe_data;
    UiMultiEdit edit_code;
    String generated_code;
    Color window_face = SColorFace();
    int data_index = -1;
    bool projecting = false;
    bool projecting_data = false;
};

} // namespace Upp
#endif
