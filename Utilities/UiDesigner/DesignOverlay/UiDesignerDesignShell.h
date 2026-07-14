#ifndef _UiDesigner_DesignOverlay_UiDesignerDesignShell_h_
#define _UiDesigner_DesignOverlay_UiDesignerDesignShell_h_

#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include "UiDesignerDesignMetrics.h"

namespace Upp {

enum class UiDesignerPanelState : byte {
    Closed,
    Normal,
    Medium,
    Wide
};

class UiDesignerRail : public ParentCtrl {
public:
    typedef UiDesignerRail CLASSNAME;

    Event<int> WhenSection;
    Event<>    WhenCycle;
    Event<>    WhenClose;

    void SetSections(const VectorMap<String, Image>& sections);
    void Select(int index);
    int  GetSelected() const { return selected_; }

    virtual void Layout() override;

private:
    Array<UiToolButton> buttons_;
    UiToolButton cycle_;
    UiToolButton close_;
    int selected_ = 0;
};

class UiDesignerSidePanel : public ParentCtrl {
public:
    typedef UiDesignerSidePanel CLASSNAME;

    UiDesignerSidePanel();

    UiDesignerSidePanel& LeftSide(bool left = true);
    UiDesignerSidePanel& SetSections(const VectorMap<String, Image>& sections);
    UiDesignerSidePanel& SetSectionContent(int index, Ctrl& content);

    void SetState(UiDesignerPanelState state);
    UiDesignerPanelState GetState() const { return state_; }
    int  GetPreferredWidth() const;
    int  GetSelectedSection() const { return selected_; }

    Event<> WhenState;
    Event<int> WhenSection;

    virtual void Layout() override;

private:
    void SelectSection(int index);
    void CycleState();
    void ClosePanel();

    bool left_side_ = true;
    UiDesignerPanelState state_ = UiDesignerPanelState::Normal;
    int selected_ = 0;

    UiDesignerRail rail_;
    UiPanel body_surface_;
    UiStack body_stack_;
};

class UiDesignerHeader : public ParentCtrl {
public:
    typedef UiDesignerHeader CLASSNAME;

    UiDesignerHeader();
    virtual void Layout() override;

    Event<> WhenNew;
    Event<> WhenOpen;
    Event<> WhenSave;
    Event<> WhenExport;
    Event<> WhenUndo;
    Event<> WhenRedo;
    Event<> WhenDesigner;
    Event<> WhenTheme;
    Event<> WhenHelp;

private:
    UiPanel surface_;
    UiLabel brand_;
    UiToolButton new_;
    UiToolButton open_;
    UiToolButton save_;
    UiToolButton export_;
    UiToolButton undo_;
    UiToolButton redo_;
    UiButton designer_;
    UiButton theme_;
    UiDropdown preset_;
    UiCheckBox dark_;
    UiToolButton help_;
};

class UiDesignerCanvasWorkspace : public ParentCtrl {
public:
    typedef UiDesignerCanvasWorkspace CLASSNAME;

    UiDesignerCanvasWorkspace();
    virtual void Layout() override;

    UiPanel& CanvasSurface() { return canvas_surface_; }

private:
    UiPanel toolbar_surface_;
    UiDropdown aspect_;
    UiDropdown zoom_;
    UiToolButton fit_;
    UiToolButton guides_;
    UiLabel breadcrumb_;

    UiPanel canvas_shadow_;
    UiPanel canvas_surface_;
};

class UiDesignerThemeGallery : public ParentCtrl {
public:
    typedef UiDesignerThemeGallery CLASSNAME;

    UiDesignerThemeGallery();
    virtual void Layout() override;

private:
    UiPanel toolbar_surface_;
    UiLabel title_;
    UiDropdown gallery_;
    UiCheckBox states_;
    UiScrollPanel scroller_;
    UiPanel gallery_surface_;
};

class UiDesignerDesignShell : public TopWindow {
public:
    typedef UiDesignerDesignShell CLASSNAME;

    UiDesignerDesignShell();

    void ShowDesigner();
    void ShowTheme();

    virtual void Layout() override;
    virtual bool Key(dword key, int count) override;

private:
    void BuildDesignerWorkspace();
    void BuildThemeWorkspace();
    void RelayoutMiddle();

    UiDesignerHeader header_;
    UiStack workspace_stack_;
    UiPanel footer_surface_;
    UiLabel footer_status_;

    ParentCtrl designer_page_;
    UiDesignerSidePanel designer_left_;
    UiDesignerCanvasWorkspace designer_center_;
    UiDesignerSidePanel designer_right_;

    ParentCtrl theme_page_;
    UiDesignerSidePanel theme_left_;
    UiDesignerThemeGallery theme_center_;
    UiDesignerSidePanel theme_right_;

    // Designer left sections.
    UiScrollPanel presets_;
    UiScrollPanel layouts_;
    UiScrollPanel containers_;
    UiScrollPanel widgets_;
    UiScrollPanel composites_;
    UiScrollPanel upp_controls_;

    // Designer right sections.
    UiScrollPanel hierarchy_;
    PropertyEditor inspector_;
    UiScrollPanel overrides_;
    UiScrollPanel generated_code_;

    // Theme sections.
    UiScrollPanel theme_tokens_;
    UiScrollPanel theme_roles_;
    UiScrollPanel theme_controls_;
    PropertyEditor theme_inspector_;
    UiScrollPanel theme_code_;
};

} // namespace Upp

#endif
