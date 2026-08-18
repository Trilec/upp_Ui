#ifndef _Ui_UiDataModels_h_
#define _Ui_UiDataModels_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.
    UiDataModels
    ============

    Purpose
    - Shared lightweight list, tree, table, and menu data models for Ui controls.

    Intent
    - Keep model ownership and change notification explicit so controls can bind
      to common item data without exposing control-specific helpers publicly.

    Thread context
    - Mutate on the GUI thread when models are bound to live controls.

    Usage
    - Use UiListModel and UiTreeModel as the canonical item sources for list,
      tree, and dropdown-style controls. UiGraphModel lives in Ui/UiGraph and
      reuses UiDataModelBase without duplicating this lightweight model layer.

    Changelog
    - 2026-03: documented as public shared model infrastructure.
    - 2026-04: standardized per-item icon rendering metadata as
      icon_render_mode so model-backed controls share one icon policy vocabulary.
    - 2026-04: normalized list-model move semantics so reorder operations may
      target the end position (`to == GetCount()`).
    - 2026-08: moved the expanded UiGraphModel into Ui/UiGraph and removed the
      original integer-ID placeholder model.
    - 2026-08: added optional image presentation content for shared item renders;
      icon remains the compact glyph while image represents thumbnail/media.
    - 2026-08: made shared model observer identity lifetime-aware so a destroyed
      inactive external model cannot block a later model reusing the same address.
*/

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <Ui/UiStyle.h>

namespace Upp {

struct UiModelColumn : Moveable<UiModelColumn> {
    String text;
    String tooltip;
    Image  icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;
    Color  ink = Null;
    int    align = ALIGN_CENTER;

    UiModelColumn() {}
    UiModelColumn(const String& s) : text(s) {}
    UiModelColumn(const Image& img, UiIconRenderMode mode = UiIconRenderMode::PreserveColor)
        : icon(img), icon_render_mode(mode) {}
};

struct UiModelItem : Moveable<UiModelItem> {
    String text;
    Value  data;
    bool   enabled = true;
    String description;
    String right_text;
    Vector<UiModelColumn> columns;
    int    text_align = ALIGN_LEFT;
    int    right_text_align = ALIGN_RIGHT;
    Image  image;
    Image  icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;
    bool   has_check = false;
    bool   checked = false;
    bool   group_header = false;
    bool   separator_before = false;
    Color  custom_ink_color;

    bool   use_custom_font = false;
    Font   custom_font = StdFont();
    bool   underline = false;
    Color  underline_color;

    bool   editable = false;
    bool   lazy_children = false;
    bool   lazy_loaded = false;
    bool   has_metadata = false;
    Color  metadata_color = Color(65, 167, 248);

    UiModelItem() {}
    UiModelItem(const String& t, const Value& d = Value(), bool en = true)
        : text(t), data(d), enabled(en) {}
    UiModelItem(const UiModelItem& o)
        : text(o.text), data(o.data), enabled(o.enabled), description(o.description),
          right_text(o.right_text), columns(clone(o.columns)), text_align(o.text_align),
          right_text_align(o.right_text_align), image(o.image), icon(o.icon),
          icon_render_mode(o.icon_render_mode), has_check(o.has_check), checked(o.checked),
          group_header(o.group_header), separator_before(o.separator_before),
          custom_ink_color(o.custom_ink_color), use_custom_font(o.use_custom_font),
          custom_font(o.custom_font), underline(o.underline), underline_color(o.underline_color),
          editable(o.editable), lazy_children(o.lazy_children), lazy_loaded(o.lazy_loaded),
          has_metadata(o.has_metadata), metadata_color(o.metadata_color) {}
    UiModelItem& operator=(const UiModelItem& o)
    {
        text = o.text;
        data = o.data;
        enabled = o.enabled;
        description = o.description;
        right_text = o.right_text;
        columns = clone(o.columns);
        text_align = o.text_align;
        right_text_align = o.right_text_align;
        image = o.image;
        icon = o.icon;
        icon_render_mode = o.icon_render_mode;
        has_check = o.has_check;
        checked = o.checked;
        group_header = o.group_header;
        separator_before = o.separator_before;
        custom_ink_color = o.custom_ink_color;
        use_custom_font = o.use_custom_font;
        custom_font = o.custom_font;
        underline = o.underline;
        underline_color = o.underline_color;
        editable = o.editable;
        lazy_children = o.lazy_children;
        lazy_loaded = o.lazy_loaded;
        has_metadata = o.has_metadata;
        metadata_color = o.metadata_color;
        return *this;
    }
};

enum UiModelChangeKind {
    UI_MODEL_RESET,
    UI_MODEL_INSERT,
    UI_MODEL_ERASE,
    UI_MODEL_UPDATE,
    UI_MODEL_MOVE,
    UI_MODEL_CLEAR
};

struct UiModelChange {
    UiModelChangeKind kind = UI_MODEL_RESET;
    int a = -1;
    int b = -1;
    int c = -1;
};

struct UiReorderRequest {
    int from = -1;
    int before = -1;
    bool accept = true;
    bool handled = false;
};

class UiDataModelBase : public Pte<UiDataModelBase> {
public:
    Event<const UiModelChange&> WhenChange;

    UiDataModelBase() {}
    UiDataModelBase(const UiDataModelBase& src)
        : revision_(src.revision_)
    {
        // A copied model is a new observable object. Do not copy callbacks or
        // Pte weak-identity state from the source object.
    }
    UiDataModelBase& operator=(const UiDataModelBase& src)
    {
        if(this != &src)
            revision_ = src.revision_;
        // Assignment changes semantic contents in derived models, not the
        // destination object's observer identity or installed callbacks.
        return *this;
    }

    int GetRevision() const { return revision_; }

protected:
    void Notify(UiModelChangeKind kind, int a = -1, int b = -1, int c = -1)
    {
        revision_++;
        UiModelChange ch;
        ch.kind = kind;
        ch.a = a;
        ch.b = b;
        ch.c = c;
        WhenChange(ch);
    }

private:
    int revision_ = 0;
};

// Views may leave callbacks attached to previously used external models and
// ignore them while inactive. Keep only weak model identities here: expired
// models are pruned before a new binding is remembered, so address reuse cannot
// suppress observation of a fresh model allocated at the same location.
template <class Model>
class UiModelObserverSet {
public:
    int GetCount() const { return models_.GetCount(); }

    Model* operator[](int i) const
    {
        return static_cast<Model*>(~models_[i]);
    }

    void Add(Model* model)
    {
        for(int i = models_.GetCount() - 1; i >= 0; --i)
            if(!models_[i])
                models_.Remove(i);
        if(!model)
            return;
        UiDataModelBase* base = static_cast<UiDataModelBase*>(model);
        for(int i = 0; i < models_.GetCount(); i++)
            if(~models_[i] == base)
                return;
        models_.Add(Ptr<UiDataModelBase>(base));
    }

private:
    Vector<Ptr<UiDataModelBase>> models_;
};

class UiListModel : public UiDataModelBase {
public:
    int GetCount() const { return items_.GetCount(); }
    bool IsEmpty() const { return items_.IsEmpty(); }

    const UiModelItem& Get(int i) const;
    UiModelItem& Get(int i);

    int Add(const UiModelItem& it);
    int Add(const String& text, const Value& data = Value(), bool enabled = true);
    UiListModel& AddRange(const Vector<UiModelItem>& items);
    bool Insert(int pos, const UiModelItem& it);
    bool Set(int pos, const UiModelItem& it);
    bool Remove(int pos);
    // Move item `from` before logical position `to`. `to == GetCount()`
    // appends the moved item at the end.
    bool Move(int from, int to);
    bool SwapItems(int a, int b);
    void Clear();
    void Reserve(int n) { items_.Reserve(max(0, n)); }

    Vector<UiModelItem> GetAll() const { return clone(items_); }

private:
    Vector<UiModelItem> items_;
};

struct UiTreeNodeRef {
    int id = -1;
    bool IsValid() const { return id >= 0; }
};

struct UiTreeMoveRequest {
    Vector<UiTreeNodeRef> nodes;
    UiTreeNodeRef new_parent;
    int insert_pos = -1;
    bool accept = true;
    bool handled = false;
};

class UiTreeModel : public UiDataModelBase {
public:
    UiTreeModel();

    UiTreeNodeRef Root() const { return UiTreeNodeRef{root_id_}; }
    bool IsValid(UiTreeNodeRef n) const;

    UiTreeNodeRef AddChild(UiTreeNodeRef parent, const UiModelItem& it);
    UiTreeNodeRef InsertChild(UiTreeNodeRef parent, int pos, const UiModelItem& it);

    int GetChildCount(UiTreeNodeRef parent) const;
    UiTreeNodeRef GetChild(UiTreeNodeRef parent, int index) const;
    UiTreeNodeRef GetParent(UiTreeNodeRef node) const;
    int GetChildIndex(UiTreeNodeRef node) const;

    const UiModelItem& Get(UiTreeNodeRef node) const;
    UiModelItem& Get(UiTreeNodeRef node);
    bool Set(UiTreeNodeRef node, const UiModelItem& it);

    bool Remove(UiTreeNodeRef node);
    bool Move(UiTreeNodeRef node, UiTreeNodeRef new_parent, int pos = -1);
    UiTreeNodeRef CloneSubtree(UiTreeNodeRef node, UiTreeNodeRef new_parent, int pos = -1);

    void Clear();
    int GetNodeCount() const;

    UiListModel ExportList(UiTreeNodeRef parent, bool recursive = false) const;
    void ImportList(UiTreeNodeRef parent, const UiListModel& list);

private:
    struct Node : Moveable<Node> {
        UiModelItem item;
        int parent = -1;
        Vector<int> children;
        bool alive = false;
    };

    int AllocNode();
    void FreeSubtree(int id);
    bool IsAncestor(int ancestor, int node) const;
    int CloneSubtreeRec(int src_id, int dst_parent, int pos);

    Vector<Node> nodes_;
    Vector<int> free_ids_;
    int root_id_ = -1;
};

enum UiTableAxis : byte {
    UITABLE_ROW_AXIS = 0,
    UITABLE_COLUMN_AXIS,
};

enum UiTableSortDirection : byte {
    UITABLE_SORT_NONE = 0,
    UITABLE_SORT_ASC,
    UITABLE_SORT_DESC,
};

struct UiTableHeader : Moveable<UiTableHeader> {
    String text;
    Value  data;
    bool   enabled = true;
    bool   sortable = false;
    UiTableSortDirection sort = UITABLE_SORT_NONE;
    int align = ALIGN_LEFT;
    String tooltip;
    Image  icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;
    Color  custom_ink_color;
    Color  custom_bg_color;

    UiTableHeader() {}
    UiTableHeader(const String& t, const Value& d = Value())
        : text(t), data(d) {}
};

struct UiTableCell : Moveable<UiTableCell> {
    Value  value;
    Value  edit_value;
    String display;
    String tooltip;
    bool   enabled = true;
    bool   editable = true;
    int align = ALIGN_LEFT;
    Image  icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;
    bool   use_custom_ink = false;
    Color  ink = Null;
    bool   use_custom_bg = false;
    Color  bg = Null;
    bool   use_custom_font = false;
    Font   font = StdFont();
    bool   has_warning = false;
    bool   has_error = false;
};

struct UiTableEditRequest {
    int row = -1;
    int col = -1;
    Value value;
    UiTableCell cell;
    bool accept = true;
    bool handled = false;
};

class UiTableModel : public UiDataModelBase {
public:
    UiTableModel();
    UiTableModel(int rows, int cols);

    int GetRowCount() const { return cells_.GetCount(); }
    int GetColumnCount() const { return column_headers_.GetCount(); }
    bool IsValidCell(int row, int col) const;

    void SetSize(int rows, int cols);
    void SetRowCount(int rows);
    void SetColumnCount(int cols);
    void Clear();

    bool InsertRow(int row);
    bool RemoveRow(int row);
    bool InsertColumn(int col);
    bool RemoveColumn(int col);

    const UiTableCell& GetCell(int row, int col) const;
    UiTableCell& GetCell(int row, int col);
    bool SetCell(int row, int col, const UiTableCell& cell);
    Value GetCellValue(int row, int col) const;
    bool SetCellValue(int row, int col, const Value& value);
    bool IsCellEditable(int row, int col) const;

    const UiTableHeader& GetHeader(UiTableAxis axis, int index) const;
    UiTableHeader& GetHeader(UiTableAxis axis, int index);
    bool SetHeader(UiTableAxis axis, int index, const UiTableHeader& header);
    Value GetHeaderValue(UiTableAxis axis, int index) const;
    bool SetHeaderValue(UiTableAxis axis, int index, const Value& value);

private:
    Vector< Vector<UiTableCell> > cells_;
    Vector<UiTableHeader> row_headers_;
    Vector<UiTableHeader> column_headers_;
};

struct UiMenuItem : Moveable<UiMenuItem> {
    String text;
    String description;
    String right_text;
    String shortcut_text;
    String tooltip;
    Value  data;
    Value  command_id;
    Image  icon;
    UiIconRenderMode icon_render_mode = UiIconRenderMode::PreserveColor;
    bool   enabled = true;
    bool   visible = true;
    bool   separator_before = false;
    bool   separator = false;
    bool   checkable = false;
    bool   checked = false;
    bool   radio = false;
    bool   default_item = false;

    UiMenuItem() {}
    UiMenuItem(const String& t, const Value& d = Value(), bool en = true)
        : text(t), data(d), enabled(en) {}
};

struct UiMenuNodeRef {
    int id = -1;
    bool IsValid() const { return id >= 0; }
};

struct UiMenuActionRequest {
    UiMenuNodeRef node;
    UiMenuItem item;
    bool accept = true;
    bool handled = false;
};

class UiMenuModel : public UiDataModelBase {
public:
    UiMenuModel();

    UiMenuNodeRef Root() const { return UiMenuNodeRef{root_id_}; }
    bool IsValid(UiMenuNodeRef node) const;

    UiMenuNodeRef AddChild(UiMenuNodeRef parent, const UiMenuItem& item);
    UiMenuNodeRef InsertChild(UiMenuNodeRef parent, int pos, const UiMenuItem& item);

    int GetChildCount(UiMenuNodeRef parent) const;
    UiMenuNodeRef GetChild(UiMenuNodeRef parent, int index) const;
    UiMenuNodeRef GetParent(UiMenuNodeRef node) const;
    int GetChildIndex(UiMenuNodeRef node) const;

    const UiMenuItem& Get(UiMenuNodeRef node) const;
    UiMenuItem& Get(UiMenuNodeRef node);
    bool Set(UiMenuNodeRef node, const UiMenuItem& item);

    bool Remove(UiMenuNodeRef node);
    bool RemoveChildren(UiMenuNodeRef parent);
    bool Move(UiMenuNodeRef node, UiMenuNodeRef new_parent, int pos = -1);
    UiMenuNodeRef CloneSubtree(UiMenuNodeRef node, UiMenuNodeRef new_parent, int pos = -1);
    bool Prune(UiMenuNodeRef node) { return RemoveChildren(node); }
    bool Graft(UiMenuNodeRef node, UiMenuNodeRef new_parent, int pos = -1) { return Move(node, new_parent, pos); }

    void Clear();
    int GetNodeCount() const;

private:
    struct Node : Moveable<Node> {
        UiMenuItem item;
        int parent = -1;
        Vector<int> children;
        bool alive = false;
    };

    int AllocNode();
    void FreeSubtree(int id);
    bool IsAncestor(int ancestor, int node) const;
    int CloneSubtreeRec(int src_id, int dst_parent, int pos);

    Vector<Node> nodes_;
    Vector<int> free_ids_;
    int root_id_ = -1;
};

}

#endif