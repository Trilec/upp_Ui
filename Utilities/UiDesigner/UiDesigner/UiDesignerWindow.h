#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include "UiDesignerStyle.h"

namespace Upp {

enum UiDesignerPaneWidth { PANE_CLOSED, PANE_NORMAL, PANE_MEDIUM, PANE_WIDE };

class UiDesignerPillBar : public UiPanel {
public:
    typedef UiDesignerPillBar CLASSNAME;

    UiDesignerPillBar();
    UiDesignerPillBar& SetInset(int inset);
    UiDesignerPillBar& AddSection(const String& tip, const Image& icon);
    UiDesignerPillBar& AddControl(Ctrl& ctrl, int width);
    Event<int> WhenSelect;
    virtual void Layout() override;

private:
    struct Item : Moveable<Item> {
        Ptr<Ctrl> ctrl;
        int width = 0;
    };
    Vector<Item> items_;
    Array<UiToolButton> owned_buttons_;
    int inset_ = DPI(20);
};

class UiDesignerSideColumn : public ParentCtrl {
public:
    typedef UiDesignerSideColumn CLASSNAME;

    UiDesignerSideColumn();
    UiDesignerSideColumn& RightColumn(bool b = true);
    UiDesignerSideColumn& AddSection(const String& tip, const Image& icon, Ctrl& content);
    void SetPaneWidth(UiDesignerPaneWidth width);
    int GetDesiredWidth() const;
    Event<> WhenWidthChanged;
    virtual void Layout() override;

private:
    void Select(int i);
    void Cycle();
    void Close();

    UiDesignerPillBar tools_;
    UiToolButton close_;
    UiToolButton expand_;
    UiPanel content_surface_;
    UiStack pages_;
    UiDesignerPaneWidth width_ = PANE_NORMAL;
    bool right_ = false;
};

class UiDesignerWindow : public TopWindow {
public:
    typedef UiDesignerWindow CLASSNAME;
    UiDesignerWindow();
    virtual void Layout() override;

private:
    void BuildHeader();
    void BuildDesigner();
    void BuildTheme();
    void ShowDesigner();
    void ShowTheme();
    void PopulateThemeGallery();

    UiPanel header_surface_;
    UiTitleCard brand_;
    UiSplitButton save_;
    UiSplitButton load_;
    UiSplitButton export_;
    UiDropdown theme_select_;
    UiToolButton dark_;
    UiToolButton help_;
    UiButton designer_mode_;
    UiButton theme_mode_;

    UiStack workspaces_;
    ParentCtrl designer_page_;
    ParentCtrl theme_page_;

    UiDesignerSideColumn designer_left_;
    UiPanel designer_center_;
    UiDesignerPillBar aspect_pill_;
    UiToolButton portrait_;
    UiToolButton landscape_;
    UiSplitButton aspect_preset_;
    UiToolButton square_;
    UiScrollPanel preview_scroll_;
    UiPanel preview_surface_;
    UiDesignerSideColumn designer_right_;

    UiPanel theme_gallery_column_;
    UiDesignerPillBar theme_gallery_pill_;
    UiToolButton theme_all_;
    UiToolButton theme_inputs_;
    UiToolButton theme_containers_;
    UiScrollPanel gallery_scroll_;
    UiPanel gallery_surface_;
    UiDesignerSideColumn theme_right_;

    UiScrollPanel presets_, layouts_, containers_, controls_, composites_, upp_controls_;
    UiScrollPanel hierarchy_, overrides_, code_;
    PropertyEditor inspector_;
    PropertyEditor theme_inspector_;
    UiScrollPanel theme_code_;

    UiLabel gallery_heading_;
    UiButton gallery_button_;
    UiLineEdit gallery_line_edit_;
    UiCheckBox gallery_check_;
    UiDropdown gallery_dropdown_;
    UiSlider gallery_slider_;
    UiProgressBar gallery_progress_;
    UiColorPicker gallery_color_;
    UiGroupPanel gallery_group_;

    UiPanel footer_surface_;
    UiLabel footer_;
};

} // namespace Upp

#endif
