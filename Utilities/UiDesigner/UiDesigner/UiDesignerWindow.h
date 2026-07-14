#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include "UiDesignerStyle.h"

namespace Upp {

enum UiDesignerPaneWidth { PANE_CLOSED, PANE_NORMAL, PANE_MEDIUM, PANE_WIDE };

class UiDesignerIconStrip : public UiPanel {
public:
    typedef UiDesignerIconStrip CLASSNAME;

    UiDesignerIconStrip();
    UiDesignerIconStrip& AddSection(const String& tip, const Image& icon);
    UiDesignerIconStrip& RightStrip(bool b = true) { right_ = b; return *this; }
    int GetSelected() const { return selected_; }
    Event<int> WhenSelect;
    Event<> WhenCycle;
    Event<> WhenClose;
    virtual void Layout() override;

private:
    Array<UiToolButton> sections_;
    UiToolButton close_;
    UiToolButton expand_;
    int selected_ = 0;
    bool right_ = false;
};

class UiDesignerPane : public ParentCtrl {
public:
    typedef UiDesignerPane CLASSNAME;

    UiDesignerPane();
    UiDesignerPane& RightPane(bool b = true);
    UiDesignerPane& AddSection(const String& tip, const Image& icon, Ctrl& content);
    void SetPaneWidth(UiDesignerPaneWidth width);
    int GetDesiredWidth() const;
    Event<> WhenWidthChanged;
    virtual void Layout() override;

private:
    void Select(int i);
    void Cycle();
    UiDesignerIconStrip strip_;
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

    UiPanel header_surface_;
    UiTitleCard brand_;
    UiSplitButton save_;
    UiSplitButton load_;
    UiDropdown theme_select_;
    UiToolButton dark_;
    UiToolButton help_;
    UiButton designer_mode_;
    UiButton theme_mode_;

    UiStack workspaces_;
    ParentCtrl designer_page_;
    ParentCtrl theme_page_;

    UiDesignerPane designer_left_;
    UiDesignerPane designer_right_;
    UiPanel designer_center_;
    UiPanel designer_toolbar_pill_;
    UiDropdown aspect_;
    UiDropdown zoom_;
    UiToolButton fit_;
    UiToolButton guides_;
    UiPanel canvas_;

    UiDesignerPane theme_left_;
    UiDesignerPane theme_right_;
    UiPanel theme_center_;
    UiPanel theme_toolbar_pill_;
    UiLabel theme_title_;
    UiDropdown gallery_mode_;
    UiScrollPanel gallery_scroll_;
    UiPanel gallery_;

    UiScrollPanel presets_, layouts_, containers_, controls_, composites_, upp_controls_;
    UiScrollPanel hierarchy_, overrides_, code_;
    PropertyEditor inspector_;
    UiScrollPanel tokens_, roles_, theme_controls_, theme_code_;
    PropertyEditor theme_inspector_;

    UiPanel footer_surface_;
    UiLabel footer_;
};

} // namespace Upp

#endif
