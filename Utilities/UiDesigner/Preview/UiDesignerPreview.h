#ifndef _Utilities_UiDesigner_Preview_UiDesignerPreview_h_
#define _Utilities_UiDesigner_Preview_UiDesignerPreview_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/Services/UiDesignerProjection.h>

namespace Upp {

enum class UiDesignerApplyResult : byte {
    AppliedPaint = 0,
    AppliedControlState,
    AppliedLocalLayout,
    AppliedAncestorLayout,
    RequiresSubtreeRebuild,
    RequiresFullRebuild,
    Rejected,
};

struct UiDesignerPreviewStats {
    int live_applies = 0;
    int paint_updates = 0;
    int local_layouts = 0;
    int ancestor_layouts = 0;
    int subtree_rebuilds = 0;
    int full_rebuilds = 0;
    int rejected = 0;

    void Clear() { *this = UiDesignerPreviewStats(); }
};

struct UiDesignerPreviewInstance {
    UiDesignerNodeId node = 0;
    UiDesignerNodeId runtime_parent = 0;
    String type;
    One<Ctrl> control;
    uint64 generation = 0;
};

class UiDesignerPreviewFactory {
public:
    static One<Ctrl> Create(const UiDesignerControlSpec& spec);
    static void Initialize(Ctrl& ctrl, const UiDesignerControlSpec& spec);
    static UiDesignerApplyResult Apply(
        Ctrl& ctrl, const UiDesignerControlSpec& spec,
        const String& property, const Value& value);
};

class UiDesignerPreviewCanvas : public ParentCtrl, public UiDesignerProjectionSink {
public:
    typedef UiDesignerPreviewCanvas CLASSNAME;

    UiDesignerPreviewCanvas();

    void Bind(const UiDesignerDocument *document,
              const UiDesignerCatalog *catalog,
              const UiDesignerTransientOverlay *overlay,
              const UiDesignerSelection *selection) override;
    void SetCatalog(const UiDesignerCatalog *catalog);
    void SetDocument(const UiDesignerDocument *document);
    void SetOverlay(const UiDesignerTransientOverlay *overlay);
    void SetSelection(const UiDesignerSelection *selection) override;
    void SetAccent(Color accent) { accent_ = accent; Refresh(); }

    void RebuildDocument() override;
    bool RebuildSubtree(UiDesignerNodeId root);
    UiDesignerApplyResult ApplyProperty(UiDesignerNodeId node,
                                        const String& property,
                                        const Value& value);
    void ApplyChangeSet(const UiDesignerChangeSet& changes) override;
    void ApplyTransient(UiDesignerNodeId node, const String& property,
                        const Value& value) override
    {
        ApplyProperty(node, property, value);
    }

    Ctrl* FindRuntime(UiDesignerNodeId node);
    const Ctrl* FindRuntime(UiDesignerNodeId node) const;
    uint64 GetInstanceGeneration(UiDesignerNodeId node) const;
    Rect GetNodeRect(UiDesignerNodeId node) const;

    const UiDesignerPreviewStats& GetStats() const { return stats_; }
    void ResetStats() { stats_.Clear(); }

    Event<UiDesignerNodeId, bool> WhenSelectNode;

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;
    virtual void LeftDown(Point p, dword keyflags) override;

private:
    int FindInstance(UiDesignerNodeId node) const;
    void DestroyInstances();
    void BuildNode(UiDesignerNodeId node, ParentCtrl& parent, int depth,
                   UiDesignerNodeId runtime_parent = 0);
    void LayoutNode(UiDesignerNodeId node, int ordinal, int depth);
    void RemoveInstanceTree(UiDesignerNodeId node, bool include_root);
    bool IsRuntimeDescendant(UiDesignerNodeId candidate,
                             UiDesignerNodeId ancestor) const;
    void ApplyAllProperties(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node);
    Value Effective(const UiDesignerNode& node, const String& property,
                    const Value& fallback = Value()) const;

    const UiDesignerCatalog *catalog_ = nullptr;
    const UiDesignerDocument *document_ = nullptr;
    const UiDesignerTransientOverlay *overlay_ = nullptr;
    const UiDesignerSelection *selection_ = nullptr;

    Array<UiDesignerPreviewInstance> instances_;
    VectorMap<UiDesignerNodeId, Rect> rects_;
    uint64 generation_sequence_ = 0;
    UiDesignerPreviewStats stats_;
    Color accent_ = Color(37, 99, 235);
};

}

#endif
