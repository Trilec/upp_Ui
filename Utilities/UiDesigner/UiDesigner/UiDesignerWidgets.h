#ifndef _Utilities_UiDesigner_UiDesigner_UiDesignerWidgets_h_
#define _Utilities_UiDesigner_UiDesigner_UiDesignerWidgets_h_

#include <Utilities/UiDesigner/Services/UiDesignerServices.h>
#include "UiDesignerStyle.h"

namespace Upp {

Image UiDesignerResolveCatalogIcon(const String& key);

enum UiDesignerPaneWidth {
    PANE_CLOSED,
    PANE_NORMAL,
    PANE_MEDIUM,
    PANE_WIDE
};

class UiDesignerPillBar : public UiPanel {
public:
    typedef UiDesignerPillBar CLASSNAME;

    UiDesignerPillBar();

    UiDesignerPillBar& SetInset(int inset);
    UiDesignerPillBar& Vertical(bool on = true);
    UiDesignerPillBar& ShowAuxiliary(bool on = true);
    UiDesignerPillBar& ApplyTheme(const UiDesignerThemeSnapshot& theme);
    UiDesignerPillBar& AddSection(const String& tip, const Image& icon);
    UiDesignerPillBar& AddControl(Ctrl& ctrl, int extent);
    int GetSectionCount() const;

    Event<int> WhenSelect;

    virtual void Layout() override;

private:
    struct Item : Moveable<Item> {
        Ptr<Ctrl> ctrl;
        int extent = 0;
        bool section = false;
    };

    Vector<Item> items_;
    Array<UiToolButton> owned_buttons_;
    int inset_ = DPI(20);
    bool vertical_ = false;
    bool show_auxiliary_ = true;
};

class UiDesignerSideColumn : public ParentCtrl {
public:
    typedef UiDesignerSideColumn CLASSNAME;

    UiDesignerSideColumn();

    UiDesignerSideColumn& RightColumn(bool on = true);
    UiDesignerSideColumn& AddSection(const String& tip, const Image& icon,
                                     Ctrl& content);
    UiDesignerSideColumn& ApplyTheme(const UiDesignerThemeSnapshot& theme);

    void SetPaneWidth(UiDesignerPaneWidth width);
    UiDesignerPaneWidth GetPaneWidth() const { return width_; }
    int GetDesiredWidth() const;
    int GetActiveSection() const { return active_section_; }
    void SetActiveSection(int index);

    Event<> WhenWidthChanged;
    Event<int> WhenSectionChanged;

    virtual void Layout() override;

private:
    void Select(int index);
    void Cycle();
    void Close() override;

    UiDesignerPillBar tools_;
    UiToolButton close_;
    UiToolButton expand_;
    UiPanel content_surface_;
    UiStack pages_;

    UiDesignerPaneWidth width_ = PANE_NORMAL;
    int active_section_ = 0;
    bool right_ = false;
};

class UiDesignerCatalogList : public ParentCtrl {
public:
    typedef UiDesignerCatalogList CLASSNAME;

    UiDesignerCatalogList();

    void SetCatalog(const UiDesignerCatalog *catalog);
    void SetCategory(const String& category);
    void SetPresets(bool on = true);
    int GetContentHeight() const;

    Event<String> WhenActivate;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void MouseMove(Point p, dword flags) override;
    virtual void MouseLeave() override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;

private:
    int Count() const;
    String ItemId(int index) const;
    String ItemLabel(int index) const;
    String ItemHelp(int index) const;
    Image ItemIcon(int index) const;
    Rect ItemRect(int index) const;

    const UiDesignerCatalog *catalog_ = nullptr;
    String category_;
    bool presets_ = false;
    int hover_ = -1;
    int scroll_ = 0;
};

class UiDesignerHierarchyView : public ParentCtrl {
public:
    typedef UiDesignerHierarchyView CLASSNAME;

    void SetDocument(const UiDesignerDocument *document);
    void SetSelection(const UiDesignerSelection *selection);
    void Rebuild();

    Event<UiDesignerNodeId, bool> WhenSelectNode;

    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword flags) override;
    virtual void MouseWheel(Point p, int zdelta, dword flags) override;

private:
    struct Row : Moveable<Row> {
        UiDesignerNodeId node = 0;
        int depth = 0;
    };

    void BuildRows();
    void AddRows(UiDesignerNodeId node, int depth);

    const UiDesignerDocument *document_ = nullptr;
    const UiDesignerSelection *selection_ = nullptr;
    Vector<Row> rows_;
    int scroll_ = 0;
};

class UiDesignerCodeView : public UiMultiEdit {
public:
    UiDesignerCodeView();
    void SetCode(const String& code);
};

}

#endif
