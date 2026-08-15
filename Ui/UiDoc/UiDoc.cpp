#include "UiDoc.h"

namespace Upp {

namespace {

template <class T>
T ClampValue(const T& value, const T& lo, const T& hi)
{
    return value < lo ? lo : hi < value ? hi : value;
}

bool StyleMarkAt(const UiDocTextStyle& style, UiDocTextStyle::Mark mark)
{
    return (style.flags & (byte)mark) != 0;
}

bool IsInlineImageEmbed(const UiDocEmbedBlock& embed)
{
    return embed.type == "image" && embed.layout.Find("mode") >= 0 &&
           AsString(embed.layout["mode"]) == "inline";
}

bool RangesOverlap(UiDocRange a, UiDocRange b)
{
    a.Normalize();
    b.Normalize();
    return a.from < b.to && b.from < a.to;
}

String NextInlineImageId(const UiDocCore& core)
{
    for(int serial = 1;; serial++) {
        String id = Format("inline_image_%d", serial);
        bool used = false;
        for(const UiDocEmbedBlock& embed : core.GetEmbeds())
            if(embed.id == id) {
                used = true;
                break;
            }
        if(!used)
            return id;
    }
}

}

const UiDoc::Style& UiDoc::StyleDefault()
{
    static Style style;
    static bool initialized = false;
    if(!initialized) {
        initialized = true;
        for(int i = 0; i < 4; i++) {
            style.palette.face[i] = UiFill::Solid(SColorPaper());
            style.palette.frame[i] = SColorShadow();
            style.palette.ink[i] = SColorText();
        }
        style.palette.frame[ST_HOT] = SColorHighlight();
        style.palette.frame[ST_PRESSED] = SColorHighlight();
        style.palette.ink[ST_DISABLED] = SColorDisabled();
        style.metrics.frame_width = DPI(1);
        style.metrics.radius = DPI(2);
        style.metrics.content_margin = Rect(DPI(6), DPI(5), DPI(6), DPI(5));
        style.metrics.use_text_font = false;
        style.metrics.text_font = StdFont();
        style.font = StdFont();
        style.page_face = SColorPaper();
        style.page_frame = SColorShadow();
        style.table_grid = SColorShadow();
    }
    return style;
}

UiDoc::UiDoc()
{
    AddFrame(sb_);
    sb_.WhenScroll = [=] {
        int max_scroll = max(0, sb_.GetTotal() - sb_.GetPage());
        scroll_y_ = ClampValue(sb_.Get(), 0, max_scroll);
        sb_.Set(scroll_y_);
        Refresh();
    };
    sb_.SetLine(DPI(18));

    style_ = StyleDefault();
    BindModel(internal_model_);

    AnnotationLane comments;
    comments.id = "comments";
    comments.label = "Comments";
    comments.annotation_types.Add("comment");
    comments.annotation_types.Add("review.comment");
    comments.color = style_.marker_comment;
    comments.icon = style_.marker_comment_icon;
    comments.shape = MARKER_CIRCLE;
    comments.side = LANE_RIGHT;
    annotation_lanes_.Add(pick(comments));

    AnnotationLane metadata;
    metadata.id = "metadata";
    metadata.label = "Metadata";
    metadata.color = style_.marker_annotation;
    metadata.icon = style_.marker_annotation_icon;
    metadata.shape = MARKER_SQUARE;
    metadata.side = LANE_AUTO;
    annotation_lanes_.Add(pick(metadata));

    BackPaint();
    WantFocus();
    RegisterBuiltinCommands();
}

void UiDoc::BindModel(UiDocCore& model)
{
    for(UiDocCore* bound : bound_models_)
        if(bound == &model)
            return;

    bound_models_.Add(&model);
    Ptr<UiDoc> self = this;
    UiDocCore* observed = &model;
    model.WhenChange << [self, observed](const UiDocApplyResult& result) {
        if(self && self->model_ == observed)
            self->OnCoreChange(result);
    };
}

void UiDoc::ResetViewForModel()
{
    anchor_pos_ = caret_pos_ = 0;
    preferred_x_ = -1;
    typing_style_ = UiDocTextStyle();
    drag_selecting_ = false;
    active_annotation_id_.Clear();
    ClearActiveObject();
    search_matches_.Clear();
    search_match_index_ = -1;
    scroll_y_ = 0;
    sb_.Set(0);
    glyph_width_cache_.Clear();
    InvalidateAllLayout();
    if(!search_query_.IsEmpty())
        RecomputeSearch();
    RefreshLayout();
    Refresh();
    WhenSelection();
}

UiDoc& UiDoc::SetModel(UiDocCore& model)
{
    if(model_ == &model)
        return *this;
    model_ = &model;
    BindModel(model);
    ResetViewForModel();
    return *this;
}

UiDoc& UiDoc::SetCustomStyle(const Style& style)
{
    style_ = style;
    OnStyleChanged();
    return *this;
}

void UiDoc::OnStyleChanged()
{
    glyph_width_cache_.Clear();
    InvalidateAllLayout();
    BackPaint();
    RefreshLayout();
    Refresh();
}

int UiDoc::ClampPos(int pos) const
{
    return ClampValue(pos, 0, Model().GetLength());
}

UiDocRange UiDoc::NormalizeRange(UiDocRange range) const
{
    range.Normalize();
    range.from = ClampPos(range.from);
    range.to = ClampPos(range.to);
    return range;
}

UiDocRange UiDoc::SelectionRange() const
{
    return NormalizeRange(UiDocRange(anchor_pos_, caret_pos_));
}

void UiDoc::MoveCaret(int pos, bool keep_selection)
{
    int next = ClampPos(pos);
    if(!keep_selection)
        anchor_pos_ = next;
    caret_pos_ = next;
    preferred_x_ = -1;
    ClearActiveObject();
    WhenSelection();
    Refresh();
}

void UiDoc::MapViewState(const UiDocPositionMap& map)
{
    if(map.IsEmpty())
        return;
    anchor_pos_ = ClampPos(map.Map(anchor_pos_, UiDocPositionMap::Left));
    caret_pos_ = ClampPos(map.Map(caret_pos_, UiDocPositionMap::Right));
}

void UiDoc::OnCoreChange(const UiDocApplyResult& result)
{
    if(!result.ok)
        return;

    MapViewState(result.positions);
    anchor_pos_ = ClampPos(anchor_pos_);
    caret_pos_ = ClampPos(caret_pos_);

    if(!active_annotation_id_.IsEmpty()) {
        bool found = false;
        for(const UiDocAnnotation& annotation : Model().GetAnnotations())
            if(annotation.id == active_annotation_id_) {
                found = true;
                break;
            }
        if(!found)
            active_annotation_id_.Clear();
    }

    bool cleared_active_object = false;
    if(!active_embed_id_.IsEmpty()) {
        bool found = false;
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(embed.id == active_embed_id_) {
                found = true;
                break;
            }
        if(!found) {
            ClearActiveObject();
            cleared_active_object = true;
        }
    }

    if(!cleared_active_object && !active_table_id_.IsEmpty()) {
        UiDocTable table;
        if(!Model().GetTable(active_table_id_, table) ||
           active_table_row_ < 0 || active_table_row_ >= table.rows.GetCount() ||
           active_table_column_ < 0 || active_table_column_ >= table.columns)
            ClearActiveObject();
    }

    if(!result.positions.IsEmpty()) {
        paragraph_index_dirty_ = true;
        layout_positions_dirty_ = true;
    }
    else if(result.changed_range.IsEmpty())
        InvalidateAllLayout();
    else
        InvalidateChangedRange(result.changed_range);

    if(!search_query_.IsEmpty())
        RecomputeSearch();
    SyncScrollBar();
    RefreshLayout();
    Refresh();

    if(!result.positions.IsEmpty())
        WhenMapped(result.positions);
    WhenChange();
}

Font UiDoc::BaseFont() const
{
    if(style_.metrics.use_text_font && !IsNull(style_.metrics.text_font))
        return style_.metrics.text_font;
    if(!IsNull(style_.font))
        return style_.font;
    return StdFont();
}

Font UiDoc::ResolveFont(const UiDocTextStyle& style, const String& block_role) const
{
    Font font = BaseFont();
    if(!style.font_face.IsEmpty() && Font::FindFaceNameIndex(style.font_face) >= 0)
        font.FaceName(style.font_face);
    if(style.font_height > 0)
        font.Height(style.font_height);
    else if(style.size_delta)
        font.Height(max(DPI(8), font.GetHeight() + style.size_delta));

    if(style.flags & UiDocTextStyle::BOLD)
        font.Bold();
    if(style.flags & UiDocTextStyle::ITALIC)
        font.Italic();
    if(style.flags & UiDocTextStyle::UNDERLINE)
        font.Underline();
    if(style.flags & UiDocTextStyle::STRIKE)
        font.Strikeout();

    if(block_role == "heading.1") {
        font.Bold();
        font.Height(max(font.GetHeight(), DPI(24)));
    }
    else if(block_role == "heading.2") {
        font.Bold();
        font.Height(max(font.GetHeight(), DPI(20)));
    }
    else if(block_role == "heading.3") {
        font.Bold();
        font.Height(max(font.GetHeight(), DPI(16)));
    }
    else if(block_role == "code") {
        Font mono = MonospaceZ(max(DPI(9), font.GetHeight()));
        if(style.flags & UiDocTextStyle::BOLD)
            mono.Bold();
        if(style.flags & UiDocTextStyle::ITALIC)
            mono.Italic();
        font = mono;
    }
    else if(block_role == "screenplay.scene" || block_role == "screenplay.character" || block_role == "screenplay.transition")
        font.Bold();

    return font;
}

Color UiDoc::ResolveInk(const UiDocTextStyle& text_style) const
{
    return IsNull(text_style.ink) ? style_.palette.ink[ST_NORMAL] : text_style.ink;
}

UiDocTextStyle UiDoc::StyleAt(int pos) const
{
    pos = ClampPos(pos);
    for(const UiDocStyleRun& run : Model().GetStyles()) {
        if(pos < run.from)
            break;
        if(pos >= run.from && pos < run.to)
            return run.style;
    }
    return UiDocTextStyle();
}

String UiDoc::BlockRoleAt(int pos) const
{
    UiDocRange probe(ClampPos(pos), ClampPos(pos));
    Vector<UiDocBlock> blocks = Model().QueryBlocks(&probe);
    String role;
    int best = INT_MAX;
    for(const UiDocBlock& block : blocks) {
        int length = block.range.GetLength();
        if(!block.role.IsEmpty() && length <= best) {
            role = block.role;
            best = length;
        }
    }
    return role;
}

void UiDoc::NewDocument()
{
    Model().Clear();
    anchor_pos_ = caret_pos_ = 0;
    typing_style_ = UiDocTextStyle();
    ClearActiveObject();
    search_query_.Clear();
    search_matches_.Clear();
    search_match_index_ = -1;
    scroll_y_ = 0;
    sb_.Set(0);
    WhenSelection();
}

bool UiDoc::Save(const String& path, String* error) const
{
    return Model().Save(path, error);
}

bool UiDoc::Load(const String& path, String* error)
{
    if(!Model().Load(path, error))
        return false;
    anchor_pos_ = caret_pos_ = 0;
    typing_style_ = UiDocTextStyle();
    ClearActiveObject();
    scroll_y_ = 0;
    sb_.Set(0);
    WhenSelection();
    return true;
}

void UiDoc::SetText(const String& text)
{
    Model().Clear();
    UiDocCoreTransaction tx;
    tx.add_to_history = false;
    UiDocCoreChange change;
    change.type = UiDocCoreChange::ReplaceText;
    change.range = UiDocRange(0, 0);
    change.text = ToUnicode(text, CHARSET_UTF8);
    tx.changes.Add(pick(change));
    Model().Apply(tx);
    anchor_pos_ = caret_pos_ = 0;
    typing_style_ = UiDocTextStyle();
    ClearActiveObject();
    WhenSelection();
}

void UiDoc::SetData(const Value& value)
{
    SetText(AsString(value));
}

Value UiDoc::GetData() const
{
    return GetText();
}

void UiDoc::Replace(UiDocRange range, const WString& text)
{
    range = NormalizeRange(range);
    UiDocCoreTransaction tx;
    tx.label = "Replace";

    if(!range.IsEmpty())
        for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
            if(IsInlineImageEmbed(embed) && RangesOverlap(range, embed.range)) {
                UiDocCoreChange remove;
                remove.type = UiDocCoreChange::RemoveEmbed;
                remove.embed_id = embed.id;
                tx.changes.Add(pick(remove));
            }

    UiDocCoreChange replace;
    replace.type = UiDocCoreChange::ReplaceText;
    replace.range = range;
    replace.text = text;
    tx.changes.Add(pick(replace));

    UiDocApplyResult result = Model().Apply(tx);
    if(result.ok) {
        int pos = ClampPos(range.from + text.GetCount());
        anchor_pos_ = caret_pos_ = pos;
        WhenSelection();
    }
}

UiDocSelection UiDoc::GetSelection() const
{
    UiDocSelection selection;
    selection.anchor = anchor_pos_;
    selection.caret = caret_pos_;
    return selection;
}

void UiDoc::SetSelection(const UiDocSelection& selection)
{
    anchor_pos_ = ClampPos(selection.anchor);
    caret_pos_ = ClampPos(selection.caret);
    preferred_x_ = -1;
    ClearActiveObject();
    WhenSelection();
    Refresh();
}

void UiDoc::SetSelection(UiDocRange range)
{
    range = NormalizeRange(range);
    anchor_pos_ = range.from;
    caret_pos_ = range.to;
    preferred_x_ = -1;
    ClearActiveObject();
    WhenSelection();
    Refresh();
}

void UiDoc::SelectAll()
{
    anchor_pos_ = 0;
    caret_pos_ = Model().GetLength();
    preferred_x_ = -1;
    ClearActiveObject();
    WhenSelection();
    Refresh();
}

void UiDoc::SetBold(bool enabled)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        if(enabled) typing_style_.flags |= UiDocTextStyle::BOLD;
        else typing_style_.flags &= ~UiDocTextStyle::BOLD;
        Refresh();
        return;
    }
    Model().SetMark(range, UiDocTextStyle::BOLD, enabled);
}

void UiDoc::SetItalic(bool enabled)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        if(enabled) typing_style_.flags |= UiDocTextStyle::ITALIC;
        else typing_style_.flags &= ~UiDocTextStyle::ITALIC;
        Refresh();
        return;
    }
    Model().SetMark(range, UiDocTextStyle::ITALIC, enabled);
}

void UiDoc::SetUnderline(bool enabled)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        if(enabled) typing_style_.flags |= UiDocTextStyle::UNDERLINE;
        else typing_style_.flags &= ~UiDocTextStyle::UNDERLINE;
        Refresh();
        return;
    }
    Model().SetMark(range, UiDocTextStyle::UNDERLINE, enabled);
}

void UiDoc::SetStrikeout(bool enabled)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        if(enabled) typing_style_.flags |= UiDocTextStyle::STRIKE;
        else typing_style_.flags &= ~UiDocTextStyle::STRIKE;
        Refresh();
        return;
    }
    Model().SetMark(range, UiDocTextStyle::STRIKE, enabled);
}

void UiDoc::ToggleBold() { SetBold(!StyleMarkAt(HasSelection() ? StyleAt(SelectionRange().from) : typing_style_, UiDocTextStyle::BOLD)); }
void UiDoc::ToggleItalic() { SetItalic(!StyleMarkAt(HasSelection() ? StyleAt(SelectionRange().from) : typing_style_, UiDocTextStyle::ITALIC)); }
void UiDoc::ToggleUnderline() { SetUnderline(!StyleMarkAt(HasSelection() ? StyleAt(SelectionRange().from) : typing_style_, UiDocTextStyle::UNDERLINE)); }
void UiDoc::ToggleStrikeout() { SetStrikeout(!StyleMarkAt(HasSelection() ? StyleAt(SelectionRange().from) : typing_style_, UiDocTextStyle::STRIKE)); }

void UiDoc::SetSelectionInk(Color ink)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        typing_style_.ink = ink;
        Refresh();
        return;
    }
    Model().SetInk(range, ink);
}

void UiDoc::SetSelectionFont(const String& face, int height)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        typing_style_.font_face = face;
        if(height >= 0)
            typing_style_.font_height = height;
        Refresh();
        return;
    }
    Model().SetFont(range, face, height);
}

void UiDoc::AdjustSelectionSize(int delta)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        typing_style_.size_delta = ClampValue(typing_style_.size_delta + delta, -16, 48);
        Refresh();
        return;
    }
    UiDocTextStyle style;
    style.size_delta = ClampValue(StyleAt(range.from).size_delta + delta, -16, 48);
    Model().SetStyle(range, style, UiDocCore::STYLE_SIZE_DELTA);
}

void UiDoc::AdjustSelectionLeading(int delta)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        typing_style_.leading_delta = ClampValue(typing_style_.leading_delta + delta, -16, 64);
        Refresh();
        return;
    }
    UiDocTextStyle style;
    style.leading_delta = ClampValue(StyleAt(range.from).leading_delta + delta, -16, 64);
    Model().SetStyle(range, style, UiDocCore::STYLE_LEADING_DELTA);
}

void UiDoc::AdjustSelectionTracking(int delta)
{
    UiDocRange range = SelectionRange();
    if(range.IsEmpty()) {
        typing_style_.tracking_delta = ClampValue(typing_style_.tracking_delta + delta, -8, 24);
        Refresh();
        return;
    }
    UiDocTextStyle style;
    style.tracking_delta = ClampValue(StyleAt(range.from).tracking_delta + delta, -8, 24);
    Model().SetStyle(range, style, UiDocCore::STYLE_TRACKING_DELTA);
}

void UiDoc::SetBlockRole(const String& role)
{
    UiDocRange range = SelectionRange();
    const WString& text = Model().GetText();
    int from = range.from;
    int to = range.to;
    while(from > 0 && text[from - 1] != '\n')
        --from;
    while(to < text.GetCount() && text[to] != '\n')
        ++to;
    UiDocRange paragraph(from, to);
    Vector<UiDocBlock> blocks = Model().QueryBlocks(&paragraph);
    for(UiDocBlock block : blocks) {
        if(block.range.from == from && block.range.to == to) {
            block.role = role;
            Model().UpdateBlock(block);
            return;
        }
    }
    Model().AddBlock(paragraph, role);
}

void UiDoc::SetBlockIndent(int indent)
{
    UiDocRange range = SelectionRange();
    const WString& text = Model().GetText();
    int from = range.from;
    int to = range.to;
    while(from > 0 && text[from - 1] != '\n') --from;
    while(to < text.GetCount() && text[to] != '\n') ++to;
    UiDocRange paragraph(from, to);
    Vector<UiDocBlock> blocks = Model().QueryBlocks(&paragraph);
    for(UiDocBlock block : blocks) {
        if(block.range.from == from && block.range.to == to) {
            block.indent = max(0, indent);
            Model().UpdateBlock(block);
            return;
        }
    }
    Model().AddBlock(paragraph, String(), max(0, indent));
}

String UiDoc::GetBlockRole() const
{
    return BlockRoleAt(caret_pos_);
}

String UiDoc::AddComment(const String& text, const ValueMap& meta)
{
    ValueMap payload;
    payload.Add("text", text);
    return Model().AddAnnotation(SelectionRange(), "review.comment", payload, meta);
}

bool UiDoc::UpdateComment(const String& id, const String& text)
{
    ValueMap payload;
    payload.Add("text", text);
    return Model().UpdateAnnotation(id, payload);
}

bool UiDoc::ResolveComment(const String& id, bool resolved)
{
    return Model().SetAnnotationFlags(id, UiDocCore::ANNOT_RESOLVED, true, true, resolved);
}

bool UiDoc::RemoveComment(const String& id)
{
    return Model().RemoveAnnotation(id);
}

Vector<UiDocAnnotation> UiDoc::GetComments(UiDocRange* range) const
{
    return Model().QueryAnnotations(range, "review.comment");
}

String UiDoc::InsertImage(const String& resource_key, int width, int height, const String& align)
{
    UiDocResource resource;
    if(!Model().GetResource(resource_key, resource))
        return String();

    width = width > 0 ? width : resource.width;
    height = height > 0 ? height : resource.height;
    width = max(DPI(16), width > 0 ? width : DPI(160));
    height = max(DPI(16), height > 0 ? height : DPI(100));

    if(!active_table_id_.IsEmpty())
        return InsertActiveTableImage(resource_key, width, height) ? active_table_id_ : String();

    ValueMap payload;
    payload.Add("resource_key", resource_key);
    payload.Add("width", width);
    payload.Add("height", height);
    ValueMap layout;

    if(align == "inline" || align.IsEmpty()) {
        int at = ClampPos(caret_pos_);
        String id = NextInlineImageId(Model());
        WString marker;
        marker.Cat((wchar)0xfffc);
        layout.Add("mode", "inline");
        layout.Add("align", "inline");

        UiDocCoreTransaction tx;
        tx.label = "Insert image";

        UiDocCoreChange replace;
        replace.type = UiDocCoreChange::ReplaceText;
        replace.range = UiDocRange(at, at);
        replace.text = marker;
        tx.changes.Add(pick(replace));

        UiDocCoreChange add;
        add.type = UiDocCoreChange::AddEmbed;
        add.embed.id = id;
        add.embed.type = "image";
        add.embed.range = UiDocRange(at, at + 1);
        add.embed.payload = clone(payload);
        add.embed.layout = clone(layout);
        tx.changes.Add(pick(add));

        if(!Model().Apply(tx).ok)
            return String();

        anchor_pos_ = caret_pos_ = ClampPos(at + 1);
        active_embed_id_ = id;
        WhenSelection();
        Refresh();
        return id;
    }

    layout.Add("mode", "block");
    layout.Add("align", align);
    String id = Model().AddEmbed(caret_pos_, "image", payload, layout);
    if(!id.IsEmpty()) {
        ClearActiveObject();
        active_embed_id_ = id;
        Refresh();
    }
    return id;
}

bool UiDoc::SetImageAlign(const String& embed_id, const String& align)
{
    for(const UiDocEmbedBlock& current : Model().GetEmbeds()) {
        if(current.id != embed_id || current.type != "image")
            continue;

        bool was_inline = IsInlineImageEmbed(current);
        bool make_inline = align == "inline";
        UiDocEmbedBlock next = current;
        next.layout.GetAdd("mode") = make_inline ? Value("inline") : Value("block");
        next.layout.GetAdd("align") = align;

        UiDocCoreTransaction tx;
        tx.label = "Image layout";

        if(was_inline && !make_inline && !current.range.IsEmpty()) {
            int at = current.range.from;
            UiDocCoreChange replace;
            replace.type = UiDocCoreChange::ReplaceText;
            replace.range = current.range;
            tx.changes.Add(pick(replace));
            next.range = UiDocRange(at, at);
        }
        else if(!was_inline && make_inline) {
            int at = current.range.from;
            WString marker;
            marker.Cat((wchar)0xfffc);
            UiDocCoreChange replace;
            replace.type = UiDocCoreChange::ReplaceText;
            replace.range = UiDocRange(at, at);
            replace.text = marker;
            tx.changes.Add(pick(replace));
            next.range = UiDocRange(at, at + 1);
        }

        UiDocCoreChange update;
        update.type = UiDocCoreChange::UpdateEmbed;
        update.embed_id = embed_id;
        update.embed = next;
        tx.changes.Add(pick(update));

        bool ok = Model().Apply(tx).ok;
        if(ok) {
            active_embed_id_ = embed_id;
            Refresh();
        }
        return ok;
    }
    return false;
}

bool UiDoc::RemoveEmbed(const String& id)
{
    const UiDocEmbedBlock* found = nullptr;
    for(const UiDocEmbedBlock& embed : Model().GetEmbeds())
        if(embed.id == id) {
            found = &embed;
            break;
        }
    if(!found)
        return false;

    UiDocRange marker = found->range;
    bool inline_image = IsInlineImageEmbed(*found) && !marker.IsEmpty();
    bool ok = false;

    if(inline_image) {
        UiDocCoreTransaction tx;
        tx.label = "Remove image";

        UiDocCoreChange remove;
        remove.type = UiDocCoreChange::RemoveEmbed;
        remove.embed_id = id;
        tx.changes.Add(pick(remove));

        UiDocCoreChange replace;
        replace.type = UiDocCoreChange::ReplaceText;
        replace.range = marker;
        tx.changes.Add(pick(replace));

        ok = Model().Apply(tx).ok;
    }
    else
        ok = Model().RemoveEmbed(id);

    if(!ok)
        return false;
    if(active_embed_id_ == id || active_table_id_ == id)
        ClearActiveObject();
    Refresh();
    return true;
}

String UiDoc::InsertTable(int columns, int rows, int header_rows)
{
    return Model().InsertTable(caret_pos_, columns, rows, header_rows);
}

void UiDoc::SetSearchQuery(const String& text)
{
    search_query_ = text;
    RecomputeSearch();
}

void UiDoc::SetSearchIgnoreCase(bool enabled)
{
    search_ignore_case_ = enabled;
    RecomputeSearch();
}

void UiDoc::SetSearchWholeWord(bool enabled)
{
    search_whole_word_ = enabled;
    RecomputeSearch();
}

bool UiDoc::IsWordChar(wchar ch) const
{
    return IsAlNum((int)ch) || ch == '_';
}

void UiDoc::RecomputeSearch()
{
    search_matches_.Clear();
    search_match_index_ = -1;
    if(search_query_.IsEmpty()) {
        WhenSearch(search_query_);
        Refresh();
        return;
    }

    WString text = Model().GetText();
    WString query = ToUnicode(search_query_, CHARSET_UTF8);
    if(search_ignore_case_) {
        text = ToLower(text);
        query = ToLower(query);
    }
    int from = 0;
    while(from <= text.GetCount() - query.GetCount()) {
        int at = text.Find(query, from);
        if(at < 0)
            break;
        int to = at + query.GetCount();
        bool word_ok = !search_whole_word_ ||
                       (at == 0 || !IsWordChar(text[at - 1])) &&
                       (to == text.GetCount() || !IsWordChar(text[to]));
        if(word_ok)
            search_matches_.Add(UiDocRange(at, to));
        from = max(at + 1, to);
    }
    WhenSearch(search_query_);
    Refresh();
}

bool UiDoc::FindNext()
{
    if(search_matches_.IsEmpty())
        return false;
    int next = 0;
    for(int i = 0; i < search_matches_.GetCount(); i++) {
        if(search_matches_[i].from > caret_pos_) {
            next = i;
            break;
        }
        if(i == search_matches_.GetCount() - 1)
            next = 0;
    }
    search_match_index_ = next;
    SetSelection(search_matches_[next]);
    return true;
}

bool UiDoc::FindPrev()
{
    if(search_matches_.IsEmpty())
        return false;
    int prev = search_matches_.GetCount() - 1;
    for(int i = search_matches_.GetCount() - 1; i >= 0; --i) {
        if(search_matches_[i].to < caret_pos_) {
            prev = i;
            break;
        }
    }
    search_match_index_ = prev;
    SetSelection(search_matches_[prev]);
    return true;
}

bool UiDoc::ReplaceCurrentSearch(const WString& replacement)
{
    if(search_match_index_ < 0 || search_match_index_ >= search_matches_.GetCount())
        return false;
    UiDocRange range = search_matches_[search_match_index_];
    Replace(range, replacement);
    RecomputeSearch();
    return true;
}

int UiDoc::ReplaceAllSearch(const WString& replacement)
{
    if(search_matches_.IsEmpty())
        return 0;
    UiDocCoreTransaction tx;
    tx.label = "Replace all";
    for(int i = search_matches_.GetCount() - 1; i >= 0; --i) {
        UiDocCoreChange change;
        change.type = UiDocCoreChange::ReplaceText;
        change.range = search_matches_[i];
        change.text = replacement;
        tx.changes.Add(pick(change));
    }
    int count = search_matches_.GetCount();
    if(!Model().Apply(tx).ok)
        return 0;
    RecomputeSearch();
    return count;
}

UiDoc& UiDoc::ClearAnnotationLanes()
{
    annotation_lanes_.Clear();
    Refresh();
    return *this;
}

UiDoc& UiDoc::AddAnnotationLane(const AnnotationLane& lane)
{
    if(lane.id.IsEmpty())
        return *this;
    for(int i = 0; i < annotation_lanes_.GetCount(); i++) {
        if(annotation_lanes_[i].id == lane.id) {
            annotation_lanes_[i] = clone(lane);
            Refresh();
            return *this;
        }
    }
    annotation_lanes_.Add(clone(lane));
    Refresh();
    return *this;
}

UiDoc& UiDoc::SetAnnotationLaneVisible(const String& id, bool visible)
{
    for(AnnotationLane& lane : annotation_lanes_)
        if(lane.id == id) {
            lane.visible = visible;
            Refresh();
            break;
        }
    return *this;
}

UiDoc& UiDoc::SetAnnotationLaneColor(const String& id, Color color)
{
    for(AnnotationLane& lane : annotation_lanes_)
        if(lane.id == id) {
            lane.color = color;
            Refresh();
            break;
        }
    return *this;
}

UiDoc& UiDoc::SetAnnotationLaneIcon(const String& id, const Image& icon)
{
    for(AnnotationLane& lane : annotation_lanes_)
        if(lane.id == id) {
            lane.icon = icon;
            Refresh();
            break;
        }
    return *this;
}

void UiDoc::SetGutterSide(GutterSide side)
{
    gutter_side_ = side;
    InvalidateAllLayout();
    RefreshLayout();
    Refresh();
}

void UiDoc::ShowLineNumbers(bool show)
{
    show_line_numbers_ = show;
    InvalidateAllLayout();
    RefreshLayout();
    Refresh();
}

void UiDoc::ShowMetadataMarkers(bool show)
{
    show_metadata_markers_ = show;
    InvalidateAllLayout();
    RefreshLayout();
    Refresh();
}

bool UiDoc::Undo()
{
    if(!Model().Undo())
        return false;
    anchor_pos_ = caret_pos_ = ClampPos(caret_pos_);
    ClearActiveObject();
    WhenSelection();
    return true;
}

bool UiDoc::Redo()
{
    if(!Model().Redo())
        return false;
    anchor_pos_ = caret_pos_ = ClampPos(caret_pos_);
    ClearActiveObject();
    WhenSelection();
    return true;
}

void UiDoc::Cut()
{
    Copy();
    DeleteSelection();
}

void UiDoc::Copy() const
{
    UiDocRange range = SelectionRange();
    if(!range.IsEmpty())
        WriteClipboardText(ToUtf8(Model().GetSlice(range)));
}

void UiDoc::Paste()
{
    String text = ReadClipboardText();
    if(!text.IsEmpty())
        InsertText(ToUnicode(text, CHARSET_UTF8));
}

void UiDoc::ClearActiveObject()
{
    active_table_id_.Clear();
    active_table_row_ = active_table_column_ = -1;
    active_table_anchor_pos_ = active_table_pos_ = 0;
    table_drag_selecting_ = false;
    active_embed_id_.Clear();
    image_dragging_ = false;
    image_resizing_ = false;
    image_drag_moved_ = false;
    image_drag_start_ = Point(0, 0);
    image_resize_start_size_ = Size(0, 0);
}

Size UiDoc::GetMinSize() const
{
    return Size(DPI(300), DPI(200));
}

void UiDoc::GotFocus()
{
    Refresh();
    Ctrl::GotFocus();
}

void UiDoc::LostFocus()
{
    Refresh();
    Ctrl::LostFocus();
}

}
