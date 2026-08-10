#ifndef _Ui_UiDocCore_h_
#define _Ui_UiDocCore_h_

/*
    UiDocCore
    =========

    Purpose
    - Non-visual document engine shared by UiDoc and headless consumers.

    Design
    - Own logical document state only: text, sparse rich style runs, blocks,
      annotations, resources, embeds, metadata, anchors and revision mapping.
    - Keep caret, selection, scrolling, font measurement, layout, hit-testing
      and painting in UiDoc.
    - Keep mutation deterministic and range-based so agents, importers and UI
      commands use the same model contract.
*/

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <limits>

namespace Upp {

struct UiDocRange : Moveable<UiDocRange> {
    int from = 0;
    int to   = 0;

    UiDocRange() {}
    UiDocRange(int a, int b) : from(a), to(b) {}

    void Normalize() { if(from > to) Swap(from, to); }
    bool IsEmpty() const { return from == to; }
    int  GetLength() const { return abs(to - from); }
};

struct UiDocAnnotation : Moveable<UiDocAnnotation> {
    String     id;
    UiDocRange range;
    String     type;
    ValueMap   payload;
    ValueMap   meta;
    bool       expanded = true;
    bool       printable = true;
    bool       resolved = false;
};

struct UiDocResource : Moveable<UiDocResource> {
    String   key;
    String   resource_type;
    String   content_hash;
    String   bytes;
    String   mime;
    String   original_name;
    int      width = 0;
    int      height = 0;
    ValueMap meta;
};

struct UiDocEmbedBlock : Moveable<UiDocEmbedBlock> {
    String     block_id;
    String     embed_id;
    String     embed_type;
    UiDocRange range;
    ValueMap   payload;
    ValueMap   layout_hints;
    ValueMap   meta;
};

struct UiDocPositionMapEntry : Moveable<UiDocPositionMapEntry> {
    int at = 0;
    int old_len = 0;
    int new_len = 0;
};

struct UiDocPositionMap {
    enum Bias : byte { Left, Right };
    Vector<UiDocPositionMapEntry> edits;

    void Clear() { edits.Clear(); }
    bool IsEmpty() const { return edits.IsEmpty(); }

    int Map(int pos, Bias bias) const {
        int p = pos;
        for(const UiDocPositionMapEntry& e : edits) {
            if(e.old_len == 0) {
                if(p < e.at)
                    continue;
                if(p == e.at) {
                    if(bias == Right)
                        p = e.at + e.new_len;
                    continue;
                }
                p += e.new_len;
                continue;
            }
            int old_to = e.at + e.old_len;
            if(p < e.at)
                continue;
            if(p > old_to) {
                p += e.new_len - e.old_len;
                continue;
            }
            p = bias == Left ? e.at : e.at + e.new_len;
        }
        return p;
    }
};

struct UiDocBlockRecord : Moveable<UiDocBlockRecord> {
    String   id;
    int      line = 0;
    int      pos_from = 0;
    int      pos_to = 0;
    int      block_type = 0;
    byte     list_kind = 0;
    bool     commented = false;
    int      table_id = -1;
    byte     table_role = 0;
    int      table_cols = 0;
    int      margin_steps = 0;
    ValueMap meta;
};

struct UiDocBlockMeta : Moveable<UiDocBlockMeta> {
    String   id;
    int      block_type = 0;
    byte     list_kind = 0;
    bool     commented = false;
    int      table_id = -1;
    byte     table_role = 0;
    int      table_cols = 0;
    ValueMap meta;
};

struct UiDocStyleRun : Moveable<UiDocStyleRun> {
    enum Mark : byte { BOLD = 1, ITALIC = 2, UNDERLINE = 4, STRIKE = 8 };

    int    from = 0;
    int    to = 0;
    byte   flags = 0;
    Color  ink = Null;
    String font_face;
    int    font_height = 0;
    int    size_delta = 0;
    int    leading_delta = 0;
    int    tracking_delta = 0;

    bool IsDefault() const {
        return flags == 0 && IsNull(ink) && font_face.IsEmpty() && font_height == 0 &&
               size_delta == 0 && leading_delta == 0 && tracking_delta == 0;
    }
};

struct UiDocCoreChange : Moveable<UiDocCoreChange> {
    enum Type : byte {
        ReplaceText,
        SetStyle,
        AddAnnotation,
        RemoveAnnotation,
        UpdateAnnotation,
        SetAnnotationFlags,
        AddResource,
        RemoveResource,
        AddEmbed,
        RemoveEmbed,
        UpdateEmbed,
        SetDocumentMeta,
        SetAnchor,
        RemoveAnchor
    } type = ReplaceText;

    UiDocRange range;
    WString    text;
    UiDocStyleRun style;
    dword      style_mask = 0;

    UiDocAnnotation annotation;
    String          annotation_id;
    ValueMap        values;
    bool            expanded = true;
    bool            printable = true;
    bool            resolved = false;
    dword           flag_mask = 0;

    UiDocResource   resource;
    String          resource_key;
    UiDocEmbedBlock embed;
    String          embed_id;

    String key;
    Value  value;
    int    pos = 0;
};

struct UiDocCoreTransaction : Moveable<UiDocCoreTransaction> {
    Vector<UiDocCoreChange> changes;
    String                  label;
    uint64                  base_revision = 0;
    bool                    add_to_history = true;
};

struct UiDocApplyResult : Moveable<UiDocApplyResult> {
    bool             ok = false;
    uint64           revision_before = 0;
    uint64           revision_after = 0;
    UiDocPositionMap positions;
    UiDocRange       changed_range;
    String           error;
};

class UiDocCore {
public:
    enum StyleMask : dword {
        STYLE_FLAGS          = 1u << 0,
        STYLE_INK            = 1u << 1,
        STYLE_FONT_FACE      = 1u << 2,
        STYLE_FONT_HEIGHT    = 1u << 3,
        STYLE_SIZE_DELTA     = 1u << 4,
        STYLE_LEADING_DELTA  = 1u << 5,
        STYLE_TRACKING_DELTA = 1u << 6,
        STYLE_ALL            = 0x7fu
    };

    enum AnnotationFlagMask : dword {
        ANNOT_EXPANDED  = 1u << 0,
        ANNOT_PRINTABLE = 1u << 1,
        ANNOT_RESOLVED  = 1u << 2
    };

private:
    struct HistoryStep : Moveable<HistoryStep> {
        enum Kind : byte {
            Text, Styles, Annotations, Resources, Embeds, Meta, Anchors
        } kind = Text;

        int at = 0;
        WString before_text;
        WString after_text;
        Vector<UiDocStyleRun> before_styles;
        Vector<UiDocStyleRun> after_styles;
        Vector<UiDocAnnotation> before_annotations;
        Vector<UiDocAnnotation> after_annotations;
        Vector<UiDocResource> before_resources;
        Vector<UiDocResource> after_resources;
        Vector<UiDocEmbedBlock> before_embeds;
        Vector<UiDocEmbedBlock> after_embeds;
        VectorMap<String, int> before_anchors;
        VectorMap<String, int> after_anchors;
        ValueMap before_meta;
        ValueMap after_meta;
    };

    struct HistoryRecord : Moveable<HistoryRecord> {
        Vector<HistoryStep> steps;
    };

    WString                 text_;
    Vector<UiDocStyleRun>   styles_;
    Vector<UiDocBlockMeta>  blocks_;
    Vector<UiDocAnnotation> annotations_;
    Vector<UiDocResource>   resources_;
    Vector<UiDocEmbedBlock> embeds_;
    VectorMap<String, int>  anchors_;
    ValueMap                meta_;

    Vector<HistoryRecord> undo_;
    Vector<HistoryRecord> redo_;
    int                   history_limit_ = 128;
    uint64                revision_ = 1;
    int                   next_annotation_id_ = 1;
    int                   next_resource_id_ = 1;
    int                   next_embed_id_ = 1;

    int ClampPos(int pos) const;
    UiDocRange NormalizeRange(UiDocRange r) const;
    void ApplyHistoryStep(const HistoryStep& step, bool before);
    void ApplyHistoryRecord(const HistoryRecord& record, bool before);
    void Touch();
    void NormalizeStyles();
    void ReplaceStyleRange(UiDocRange range, const UiDocStyleRun& style, dword mask);
    void MapRanges(const UiDocPositionMap& map);
    int FindAnnotation(const String& id) const;
    int FindResource(const String& key) const;
    int FindEmbed(const String& id) const;
    bool ApplyOne(const UiDocCoreChange& change, UiDocApplyResult& result, HistoryStep* history);

public:
    UiDocCore();

    void Clear();

    uint64 GetRevision() const { return revision_; }
    int GetLength() const { return text_.GetCount(); }
    const WString& GetText() const { return text_; }
    String GetTextUtf8() const { return text_.ToString(); }
    WString GetSlice(UiDocRange range) const;

    void SetHistoryLimit(int count);
    int GetHistoryLimit() const { return history_limit_; }

    const Vector<UiDocStyleRun>& GetStyles() const { return styles_; }
    const Vector<UiDocBlockMeta>& GetBlockMeta() const { return blocks_; }
    const Vector<UiDocAnnotation>& GetAnnotations() const { return annotations_; }
    const Vector<UiDocResource>& GetResources() const { return resources_; }
    const Vector<UiDocEmbedBlock>& GetEmbeds() const { return embeds_; }
    const ValueMap& GetMeta() const { return meta_; }

    Value GetMeta(const String& key) const;
    void SetMeta(const String& key, const Value& value);
    void RemoveMeta(const String& key);

    UiDocApplyResult Apply(const UiDocCoreTransaction& tx);
    UiDocApplyResult Replace(UiDocRange range, const WString& text, uint64 base_revision = 0);
    UiDocApplyResult SetStyle(UiDocRange range, const UiDocStyleRun& style, dword mask = STYLE_ALL, uint64 base_revision = 0);

    String AddAnnotation(UiDocRange range, const String& type, const ValueMap& payload = ValueMap(), const ValueMap& meta = ValueMap());
    bool RemoveAnnotation(const String& id);
    bool UpdateAnnotation(const String& id, const ValueMap& values);
    bool SetAnnotationFlags(const String& id, dword mask, bool expanded, bool printable, bool resolved);
    Vector<UiDocAnnotation> QueryAnnotations(const UiDocRange* range = nullptr, const String& type = String()) const;

    String AddResource(const UiDocResource& resource, bool dedupe = true);
    bool RemoveResource(const String& key);
    bool GetResource(const String& key, UiDocResource& out) const;

    String AddEmbed(int pos, const String& type, const ValueMap& payload = ValueMap(), const ValueMap& layout = ValueMap(), const ValueMap& meta = ValueMap());
    bool RemoveEmbed(const String& id);
    bool UpdateEmbed(const String& id, const ValueMap& values);
    Vector<UiDocEmbedBlock> QueryEmbeds(const UiDocRange* range = nullptr, const String& type = String()) const;

    bool SetAnchor(const String& id, int pos);
    bool RemoveAnchor(const String& id);
    bool ResolveAnchor(const String& id, int& pos) const;

    bool Undo();
    bool Redo();
    bool CanUndo() const { return !undo_.IsEmpty(); }
    bool CanRedo() const { return !redo_.IsEmpty(); }

    Event<const UiDocApplyResult&> WhenChange;
};

}

#endif
