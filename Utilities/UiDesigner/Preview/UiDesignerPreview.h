#ifndef _Utilities_UiDesigner_Preview_UiDesignerPreview_h_
#define _Utilities_UiDesigner_Preview_UiDesignerPreview_h_

#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/Services/UiDesignerProjection.h>
#include <Utilities/UiDesigner/Services/UiDesignerDrop.h>
#include "UiDesignerGeometrySnapshot.h"

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
    int layout_count = 0;
    int snapshot_publications = 0;
    int drop_region_publications = 0;
    int rejected = 0;

    void Clear() { *this = UiDesignerPreviewStats(); }
};

struct UiDesignerPreviewAdapter : Moveable<UiDesignerPreviewAdapter> {
    String id;
    bool semantic = false;
    Function<One<Ctrl>()> create;
    Function<void(Ctrl&, const UiDesignerControlSpec&)> initialize;
    Function<UiDesignerApplyResult(
        Ctrl&, const UiDesignerControlSpec&, const String&, const Value&)> apply;
};

class UiDesignerPreviewAdapterRegistry {
public:
    static UiDesignerPreviewAdapterRegistry& Global();

    void Register(UiDesignerPreviewAdapter adapter);
    const UiDesignerPreviewAdapter* Find(const String& id) const;
    void EnsureBuiltins();

private:
    Array<UiDesignerPreviewAdapter> adapters_;
    bool builtins_registered_ = false;
};

struct UiDesignerPreviewInstance {
    UiDesignerNodeId node = 0;
    UiDesignerNodeId runtime_parent = 0;
    String type;
    String adapter_id;
    One<Ctrl> control;
    bool semantic = false;
    int layout_item_index = -1;
    uint64 generation = 0;
};

String UiDesignerNodesDragText(const Vector<UiDesignerNodeId>& nodes);
bool UiDesignerParseNodesDragText(const String& text,
                                  Vector<UiDesignerNodeId>& nodes);
// Read Designer-owned drag data without accepting the drop. Targets must only
// accept once a concrete drop plan has been validated.
bool UiDesignerReadDragText(PasteClip& clip, String& text);

class UiDesignerPreviewFactory {
public:
    static const UiDesignerPreviewAdapter* Adapter(
        const UiDesignerControlSpec& spec);
    static One<Ctrl> Create(const UiDesignerControlSpec& spec);
    static void Initialize(Ctrl& ctrl, const UiDesignerControlSpec& spec);
    static UiDesignerApplyResult Apply(
        Ctrl& ctrl, const UiDesignerControlSpec& spec,
        const String& property, const Value& value);
};

class UiDesignerPreviewCanvas : public ParentCtrl,
                                public UiDesignerProjectionSink {
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
    UiDesignerNodeId HitNode(Point p) const;
    const UiDesignerGeometrySnapshot& GetGeometrySnapshot() const { return geometry_; }
    const UiDesignerGeometryRecord* FindGeometry(UiDesignerNodeId node) const { return geometry_.Find(node); }

    const UiDesignerPreviewStats& GetStats() const { return stats_; }
    void ResetStats() { stats_.Clear(); }

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;

private:
    int FindInstance(UiDesignerNodeId node) const;
    void DestroyInstances();
    void BuildNode(UiDesignerNodeId node, ParentCtrl& parent, int depth,
                   UiDesignerNodeId runtime_parent = 0);
    void AttachSemanticItem(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node,
                            UiDesignerPreviewInstance& parent_instance);
    void LayoutNode(UiDesignerNodeId node, int ordinal, int depth);
    void RemoveInstanceTree(UiDesignerNodeId node, bool include_root);
    bool IsRuntimeDescendant(UiDesignerNodeId candidate,
                             UiDesignerNodeId ancestor) const;
    void ApplyAllProperties(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node);
    Value Effective(const UiDesignerNode& node, const String& property,
                    const Value& fallback = Value()) const;
    void UpdateSemanticRect(UiDesignerPreviewInstance& instance,
                            const UiDesignerNode& node);
    void PaintSemantic(Draw& w, const UiDesignerPreviewInstance& instance,
                       const UiDesignerNode& node) const;

    const UiDesignerCatalog *catalog_ = nullptr;
    const UiDesignerDocument *document_ = nullptr;
    const UiDesignerTransientOverlay *overlay_ = nullptr;
    const UiDesignerSelection *selection_ = nullptr;

    Array<UiDesignerPreviewInstance> instances_;
    VectorMap<UiDesignerNodeId, Rect> rects_;
    UiDesignerGeometrySnapshot geometry_;
    uint64 generation_sequence_ = 0;
    UiDesignerPreviewStats stats_;
    Color accent_ = Color(37, 99, 235);
};

}

#endif
