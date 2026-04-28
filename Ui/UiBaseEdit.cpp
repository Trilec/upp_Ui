#include "UiBaseEdit.h"
#include <Ui/UiTheme.h>

namespace Upp {

#define LTIMING(x) // RTIMING(x)

// --------------------------------------------------------------------
// Style
// --------------------------------------------------------------------
const UiBaseEdit::Style& UiBaseEdit::StyleDefault()
{
    static Style s;
    ONCELOCK {
        const Color face_normal    = Color(255, 255, 255);
        const Color face_pressed   = Color(248, 250, 252);
        const Color frame_normal   = Color(209, 213, 219);
        const Color frame_hot      = Color(148, 163, 184);
        const Color frame_pressed  = Color(100, 116, 139);
        const Color frame_disabled = Color(226, 232, 240);
        const Color text_primary   = Color(17, 24, 39);
        const Color text_muted     = Color(148, 163, 184);

        s.palette.face[ST_NORMAL]   = UiFill::Solid(face_normal);
        s.palette.face[ST_HOT]      = UiFill::Solid(face_normal);
        s.palette.face[ST_PRESSED]  = UiFill::Solid(face_pressed);
        s.palette.face[ST_DISABLED] = UiFill::Solid(Color(248, 250, 252));

        s.palette.frame[ST_NORMAL]   = frame_normal;
        s.palette.frame[ST_HOT]      = frame_hot;
        s.palette.frame[ST_PRESSED]  = frame_pressed;
        s.palette.frame[ST_DISABLED] = frame_disabled;

        s.palette.ink[ST_NORMAL]   = text_primary;
        s.palette.ink[ST_HOT]      = text_primary;
        s.palette.ink[ST_PRESSED]  = text_primary;
        s.palette.ink[ST_DISABLED] = text_muted;

        s.metrics.radius = 0;
        s.metrics.frame_width = DPI(1);
        s.metrics.frame_enabled = true;
        s.metrics.face_enabled = true;
        s.metrics.dashed = false;
        s.metrics.high_contrast = false;
        s.metrics.use_text_font = false;
        s.metrics.text_font = StdFont();
        s.metrics.content_margin = Rect(DPI(10), DPI(7), DPI(10), DPI(7));
        s.metrics.shadow = StyledShadow();
        s.metrics.highlight = StyledHighlight();

        s.skin = StyledSkin();
        s.font = StdFont();
        s.text_align = UiAlign::LEFT;

        s.caret_color = text_primary;
        s.caret_width = DPI(1);
        s.block_caret = false;
        s.selection_color = Color(219, 234, 254);
        s.selection_ink = text_primary;
        s.placeholder_ink = Color(148, 163, 184);
        s.whitespace_color = Color(148, 163, 184);
        s.tab_char_color = Color(148, 163, 184);
        s.tab_size = 4;
        s.show_tabs = false;
        s.show_spaces = false;
        s.show_line_endings = false;
        s.show_readonly_bg = true;
    }
    return s;
}

void UiBaseEdit::InvalidateStyleCache()
{
    theme_revision_ = 0;
    text_rect_ = Rect(0, 0, 0, 0);
    InvalidateTextMetricsCache();
}

UiBaseEdit::Style& UiBaseEdit::StyleEdit()
{
    if(!has_style_override_) {
        style_ = GetEffectiveStyle();
        has_style_override_ = true;
    }
    InvalidateStyleCache();
    return style_;
}

void UiBaseEdit::SyncThemeStyle()
{
    if(has_style_override_)
        return;

    const uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;

    themed_style_ = UiTheme::ResolveEdit();
    theme_revision_ = revision;
    text_rect_ = Rect(0, 0, 0, 0);
    InvalidateTextMetricsCache();

    Font fnt = themed_style_.metrics.use_text_font
               ? themed_style_.metrics.text_font
               : themed_style_.font;
    if(IsNull(fnt))
        fnt = StdFont();

    font_size_ = GetTextSize("A", fnt);
    if(font_size_.cx <= 0) font_size_.cx = 8;
    if(font_size_.cy <= 0) font_size_.cy = 12;
    caret_height_ = GetVisualLineHeight();
    sb_.SetLine(caret_height_);
}

const UiBaseEdit::Style& UiBaseEdit::GetEffectiveStyle() const
{
    if(has_style_override_)
        return style_;

    const_cast<UiBaseEdit*>(this)->SyncThemeStyle();
    return themed_style_;
}

UiBaseEdit& UiBaseEdit::SetStyle(const Style& s)
{
    style_ = Style(s);
    has_style_override_ = true;
    OnStyleChanged();
    return *this;
}

UiBaseEdit& UiBaseEdit::ClearStyleOverride()
{
    if(!has_style_override_)
        return *this;

    has_style_override_ = false;
    style_ = StyleDefault();
    InvalidateStyleCache();
    OnStyleChanged();
    return *this;
}

void UiBaseEdit::OnStyleChanged()
{
    BackPaint();
    InvalidateTextMetricsCache();
    text_rect_ = Rect(0, 0, 0, 0);
    SyncFont();
    Layout();
    Refresh();
}


// --------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------

UiBaseEdit::UiBaseEdit()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
{
    WantFocus();
    BackPaint();

    AddFrame(sb_);
    sb_.WhenScroll << [=] { Scroll(); WhenScroll(); };
    sb_.SetLine(DPI(16));
    scroller_.Set(sb_);
    
    WhenBar = [=](Bar& bar) {
        bar.Add(IsUndo(), t_("Undo"), CtrlImg::undo(), [=] { Undo(); }).Key(K_CTRL_Z);
        bar.Add(IsRedo(), t_("Redo"), CtrlImg::redo(), [=] { Redo(); }).Key(K_CTRL_Y);
        bar.Separator();
        bar.Add(IsSelection() && IsEditable(), t_("Cut"), CtrlImg::cut(), [=] { Cut(); }).Key(K_CTRL_X);
        bar.Add(IsSelection(), t_("Copy"), CtrlImg::copy(), [=] { Copy(); }).Key(K_CTRL_C);
        bar.Add(IsEditable() && IsClipboardAvailableText(), t_("Paste"), CtrlImg::paste(), [=] { Paste(); }).Key(K_CTRL_V);
        bar.Add(IsSelection() && IsEditable(), t_("Delete"), CtrlImg::remove(), [=] { RemoveSelection(); }).Key(K_DELETE);
        bar.Separator();
        bar.Add(t_("Select All"), [=] { SelectAll(); }).Key(K_CTRL_A);
    };

    SyncThemeStyle();
    SyncFont();
    Clear();
}
// --------------------------------------------------------------------
// Data Model (Undo)
// --------------------------------------------------------------------

void UiBaseEdit::UndoRec::SetText(const WString& text)
{
    data = FastCompress(ToUtf8(text));
}

WString UiBaseEdit::UndoRec::GetText() const
{
    return ToUtf32(FastDecompress(data));
}

// --------------------------------------------------------------------
// Data Model (Core)
// --------------------------------------------------------------------

void UiBaseEdit::Clear()
{
    LTIMING("UiBaseEdit::Clear");
    lin_.Clear();
    lin_.Add();
    total_wchars_ = 0;
    InvalidateTextMetricsCache();
    
    undo_.Clear();
    redo_.Clear();
    dirty_ = 0;
    undoserial_ = 0;
    
    anchor_ = -1;
    cursor_ = 0;
    gcolumn_ = 0;
    
    SyncSb();
    PlaceCaret();
    Refresh();
    WhenSelection();
}

void UiBaseEdit::SetText(const WString& s)
{
    Clear();
    Insert0(0, s);
    ClearDirty();
    undo_.Clear();
    redo_.Clear();
    SetCursor(0);
}

WString UiBaseEdit::GetText() const
{
    return GetW(0, total_wchars_);
}


void UiBaseEdit::SetData(const Value& v)
{
    if(v.Is<WString>())
        SetText((WString)v);
    else if(v.Is<String>())
        SetText(ToUtf32((String)v));
    else
        SetText(WString(v));
}

Value UiBaseEdit::GetData() const
{
    return GetText();
}

int UiBaseEdit::GetChar(int64 pos) const
{
    if(pos < 0 || pos >= total_wchars_)
        return 0;

    int line_idx = GetLine(pos);
    if(line_idx < 0 || line_idx >= GetLineCount())
        return 0;

    int64 line_start = GetPos(line_idx, 0);
    int64 offset     = pos - line_start;

    int line_len = GetLineLength(line_idx);

    // Inside the line: real character
    if(offset >= 0 && offset < line_len) {
        WString wline = GetWLine(line_idx);
        return wline[(int)offset];
    }

    // Just after the line: conceptual '\n' between this and the next line.
    if(line_idx < GetLineCount() - 1 && offset == line_len)
        return '\n';

    return 0;
}


WString UiBaseEdit::GetW(int64 pos, int64 size) const
{
    LTIMING("UiBaseEdit::GetW");

    if(size <= 0 || pos < 0 || pos >= total_wchars_)
        return WString();

    int64 end = pos + size;
    if(end > total_wchars_)
        end = total_wchars_;

    WString w;

    int line_idx = GetLine(pos);
    if(line_idx < 0)
        return WString();

    int64 line_start = GetPos(line_idx, 0);
    int64 offset     = pos - line_start;

    while(line_idx < GetLineCount() && pos < end) {
        int line_len = GetLineLength(line_idx);

        // 1) Characters within this line
        if(offset < line_len) {
            int64 remaining_in_line = line_len - offset;
            int64 remaining_total   = end - pos;
            int   take              = (int)min(remaining_in_line, remaining_total);

            if(take > 0) {
                WString wline = GetWLine(line_idx);
                w.Cat(wline.Mid((int)offset, take));
                pos    += take;
                offset += take;
            }
        }

        if(pos >= end)
            break;

        // 2) Newline between this line and the next (conceptual '\n')
        if(line_idx < GetLineCount() - 1) {
            if(pos < end) {
                w.Cat('\n');
                pos++;
            }
        }

        line_idx++;
        offset = 0;
    }

    return w;
}


void UiBaseEdit::SetLine(int i, const WString& w)
{
    if(i >= 0 && i < lin_.GetCount()) {
        lin_[i].text = ToUtf8(w);
        lin_[i].len  = w.GetCount();
        line_metrics_dirty_ = true;
    }
}

WString UiBaseEdit::BuildFullText() const
{
    // Current internal model already treats '\n' between logical lines,
    // so this is equivalent to returning the whole buffer as a single WString.
    return GetW(0, total_wchars_);
}


// --------------------------------------------------------------------
// Data Model (Internal Insert/Remove)
// --------------------------------------------------------------------
void UiBaseEdit::Insert0(int64 pos, const WString& text)
{
    LTIMING("UiBaseEdit::Insert0");
    if(text.IsEmpty())
        return;

    int line_idx = GetLine(pos);

    int64 line_start = GetPos(line_idx, 0);
    int   offset     = (int)(pos - line_start);

    WString line = GetWLine(line_idx);

    WString prefix = line.Mid(0, offset);
    WString suffix = line.Mid(offset);

    Vector<WString> new_lines = Split(text, '\n', false);

    SetLine(line_idx, prefix + new_lines[0]);

    if(new_lines.GetCount() > 1) {
        LineInsert(line_idx + 1, new_lines.GetCount() - 1);

        for(int i = 1; i < new_lines.GetCount() - 1; i++)
            SetLine(line_idx + i, new_lines[i]);

        int last_idx = line_idx + new_lines.GetCount() - 1;
        SetLine(last_idx, new_lines.Top() + suffix);
    }
    else {
        SetLine(line_idx, GetWLine(line_idx) + suffix);
    }

    total_wchars_ += text.GetCount();
    SyncSb();
    Refresh(); // always repaint after modifying buffer
}

void UiBaseEdit::Remove0(int64 pos, int64 size)
{
    LTIMING("UiBaseEdit::Remove0");
    if(size <= 0)
        return;

    int line1 = GetLine(pos);

    int64 start1  = GetPos(line1, 0);
    int   offset1 = (int)(pos - start1);

    WString line   = GetWLine(line1);
    WString prefix = line.Mid(0, offset1);

    int64 end_pos = pos + size;
    int   line2   = GetLine(end_pos);

    int64 start2  = GetPos(line2, 0);
    int   offset2 = (int)(end_pos - start2);

    WString suffix;
    if(line2 < GetLineCount())
        suffix = GetWLine(line2).Mid(offset2);

    SetLine(line1, prefix + suffix);

    if(line1 < line2)
        LineRemove(line1 + 1, line2 - line1);

    total_wchars_ -= size;
    SyncSb();
    Refresh(); // always repaint after modifying buffer
}

int64 UiBaseEdit::InsertU(int64 pos, const WString& text, bool typing)
{
    Insert0(pos, text);
    if(undo_steps_ > 0) {
        if(typing && !undo_.IsEmpty() && text.Find('\n') < 0) {
            UndoRec& u = undo_.Tail();
            if(u.typing && u.pos + u.size == pos) {
                u.size += text.GetCount();
                return text.GetCount();
            }
        }
        UndoRec& u = undo_.AddTail();
        incundoserial_ = true;
        IncDirty();
        u.serial = undoserial_;
        u.pos    = pos;
        u.size   = text.GetCount();
        u.typing = typing;
    }
    return text.GetCount();
}

void UiBaseEdit::RemoveU(int64 pos, int64 size)
{
    if(pos + size > total_wchars_)
        size = total_wchars_ - pos;
    if(size <= 0) return;
    
    if(undo_steps_ > 0) {
        UndoRec& u = undo_.AddTail();
        incundoserial_ = true;
        IncDirty();
        u.serial = undoserial_;
        u.pos    = pos;
        u.size   = 0;
        u.SetText(GetW(pos, size));
        u.typing = false;
    }
    Remove0(pos, size);
}

void UiBaseEdit::Undodo()
{
    while(undo_.GetCount() > undo_steps_)
        undo_.DropHead();
    redo_.Clear();
}

void UiBaseEdit::NextUndo()
{
    undoserial_ += incundoserial_;
    incundoserial_ = false;
}

void UiBaseEdit::IncDirty()
{
    dirty_++;
}

void UiBaseEdit::DecDirty()
{
    dirty_--;
}

void UiBaseEdit::Undo()
{
    if(undo_.IsEmpty() || IsReadOnly()) return;
    undo_op_ = true;
    int   s  = undo_.Tail().serial;
    int64 nc = -1;
    
    while(undo_.GetCount() && undo_.Tail().serial == s) {
        const UndoRec& u = undo_.Tail();
        UndoRec&       r = redo_.AddTail();
        r.serial = s;
        r.typing = false;
        r.pos    = u.pos;
        nc       = u.pos;
        
        if(u.size > 0) { // Insert -> Remove
            r.size = 0;
            r.SetText(GetW(u.pos, u.size));
            Remove0(u.pos, u.size);
        }
        else { // Remove -> Insert
            WString text = u.GetText();
            r.size = text.GetCount();
            Insert0(u.pos, text);
            nc += r.size;
        }
        undo_.DropTail();
        DecDirty();
    }
    
    ClearSelection();
    if(nc >= 0) PlaceCaret(nc, false);
    WhenChange();
    undo_op_ = false;
}

void UiBaseEdit::Redo()
{
    if(redo_.IsEmpty() || IsReadOnly()) return;
    NextUndo();
    int   s  = redo_.Tail().serial;
    int64 nc = -1;
    
    while(redo_.GetCount() && redo_.Tail().serial == s) {
        const UndoRec& r = redo_.Tail();
        
        if(r.size > 0) { // Insert -> Remove
            nc = r.pos;
            RemoveU(r.pos, r.size);
        }
        else { // Remove -> Insert
            WString text = r.GetText();
            nc = r.pos + text.GetCount();
            InsertU(r.pos, text, false);
        }
        redo_.DropTail();
        IncDirty();
    }
    
    ClearSelection();
    if(nc >= 0) PlaceCaret(nc, false);
    WhenChange();
}

// --------------------------------------------------------------------
// Selection
// --------------------------------------------------------------------

void UiBaseEdit::SetSelection(int64 l, int64 h)
{
    l = minmax(l, (int64)0, total_wchars_);
    h = minmax(h, (int64)0, total_wchars_);
    PlaceCaret(l, false);
    PlaceCaret(h, true);
    WhenSelection();
}

bool UiBaseEdit::GetSelection(int64& l, int64& h) const
{
    if(!IsSelection()) {
        l = h = cursor_;
        return false;
    }
    l = min(anchor_, cursor_);
    h = max(anchor_, cursor_);
    return true;
}

WString UiBaseEdit::GetSelectionW() const
{
    int64 l, h;
    if(GetSelection(l, h))
        return GetW(l, h - l);
    return WString();
}

void UiBaseEdit::ClearSelection()
{
    if(anchor_ >= 0) {
        anchor_ = -1;
        Refresh();
        WhenSelection();
    }
}

bool UiBaseEdit::RemoveSelection()
{
    if(IsReadOnly() || !IsSelection()) return false;
    int64 l, h;
    GetSelection(l, h);
    NextUndo();
    RemoveU(l, h - l);
    ClearSelection();
    PlaceCaret(l, false);
    WhenChange();
    return true;
}

void UiBaseEdit::SelectAll()
{
    SetSelection(0, total_wchars_);
}

// --------------------------------------------------------------------
// Clipboard
// --------------------------------------------------------------------

void UiBaseEdit::Cut()
{
    if(IsReadOnly()) return;
    Copy();
    RemoveSelection();
}

void UiBaseEdit::Copy()
{
    WString w = GetSelectionW();
    if(w.IsEmpty() && !IsSelection() && GetLineCount() == 1) {
        w = GetWLine(GetLine(cursor_));
        if(accepts_newlines_) w.Cat('\n');
    }
    ClearClipboard();
    AppendClipboardUnicodeText(w);
    AppendClipboardText(w.ToString());
}

void UiBaseEdit::Paste()
{
    if(IsReadOnly()) return;
    WString w = ReadClipboardUnicodeText();
    if(w.IsEmpty())
        w = ReadClipboardText().ToWString();
    if(w.IsEmpty()) return;

    if(!accepts_newlines_) {
        w.Replace("\n", "");
        w.Replace("\r", "");
    }

    RemoveSelection();
    NextUndo();
    int64 n = InsertU(cursor_, w, false);
    PlaceCaret(cursor_ + n, false);
    WhenChange();
}

// --------------------------------------------------------------------
// View / Layout
// --------------------------------------------------------------------
void UiBaseEdit::InvalidateTextMetricsCache()
{
    line_metrics_dirty_ = true;
    placeholder_width_dirty_ = true;
}

void UiBaseEdit::EnsureTextMetricsCache() const
{
    const Style& style = GetEffectiveStyle();
    if(!line_metrics_dirty_ && !placeholder_width_dirty_)
        return;

    Font fnt = style.font;
    if(IsNull(fnt))
        fnt = StdFont();

    space_width_cache_ = GetTextSize(" ", fnt).cx;
    if(space_width_cache_ <= 0)
        space_width_cache_ = 1;
    tab_width_cache_ = space_width_cache_ * max(style.tab_size, 1);

    VectorMap<int, int> glyph_width_cache;

    line_metrics_cache_.SetCount(GetLineCount());

    for(int i = 0; i < GetLineCount(); i++) {
        LineMetricsCache& mc = line_metrics_cache_[i];
        mc.char_widths.Clear();
        mc.line_px = 0;

        WString s = GetDisplayLine(i);
        int px = 0;
        mc.char_widths.SetCount(s.GetCount(), 0);
        for(int j = 0; j < s.GetCount(); j++) {
            int cw;
            if(s[j] == '\t') {
                int adv = tab_width_cache_ - (px % tab_width_cache_);
                cw = adv > 0 ? adv : tab_width_cache_;
            }
            else {
                int key = (int)s[j];
                int fi = glyph_width_cache.Find(key);
                if(fi >= 0)
                    cw = glyph_width_cache[fi];
                else {
                    WString one;
                    one.Cat(s[j]);
                    cw = GetTextSize(one, fnt).cx;
                    if(cw <= 0)
                        cw = 1;
                    glyph_width_cache.Add(key, cw);
                }
            }
            if(cw <= 0)
                cw = 1;
            mc.char_widths[j] = cw;
            px += cw;
        }
        mc.line_px = px;
    }

    if(placeholder_width_dirty_)
        placeholder_width_cache_ = placeholder_text_.IsEmpty() ? 0 : GetTextSize(placeholder_text_, fnt).cx;

    line_metrics_dirty_ = false;
    placeholder_width_dirty_ = false;
}

void UiBaseEdit::SyncFont()
{
    const Style& style = GetEffectiveStyle();
    LTIMING("UiBaseEdit::SyncFont");

    // Resolve font using StyledMetrics, same as UiLabel.
    Font fnt = style.metrics.use_text_font
               ? style.metrics.text_font
               : style.font;

    if(IsNull(fnt))
        fnt = StdFont();

    font_size_ = GetTextSize("A", fnt);
    if(font_size_.cx <= 0) font_size_.cx = 8;
    if(font_size_.cy <= 0) font_size_.cy = 12;

    InvalidateTextMetricsCache();
    caret_height_ = GetVisualLineHeight();
    sb_.SetLine(caret_height_);
    SyncSb();
    PlaceCaret();
    Refresh();
}


int UiBaseEdit::GetVisualLineHeight() const
{
    return max(DPI(12), font_size_.cy);
}

int UiBaseEdit::GetSingleLineYOffset() const
{
    if(accepts_newlines_)
        return 0;

    if(GetLineCount() <= 0 || caret_height_ <= 0)
        return 0;

    Rect text_r = GetTextRect();
    int  view_h = text_r.GetHeight();
    if(view_h <= caret_height_)
        return 0;

    return (view_h - caret_height_) / 2;
}

void UiBaseEdit::Layout()
{
    LTIMING("UiBaseEdit::Layout");

    text_rect_ = Rect(0, 0, 0, 0);
    LayoutSides();

    Rect tr = GetTextRect();
    if(tr.IsEmpty())
        tr = GetViewRect();

    Size page;
    page.cx = max(1, tr.Width());                         // X = pixels
    page.cy = max(1, tr.Height() / max(1, caret_height_)); // Y = lines

    if(!accepts_newlines_)
        page.cy = 1;

    sb_.SetPage(page);
    SyncSb();
}

void UiBaseEdit::SyncSb()
{
    LTIMING("UiBaseEdit::SyncSb");
    if(!IsVisible())
        return;

    EnsureTextMetricsCache();

    int max_len_px = 0;
    for(int i = 0; i < line_metrics_cache_.GetCount(); i++)
        max_len_px = max(max_len_px, line_metrics_cache_[i].line_px);

    // Small right breathing room so caret isn't glued to edge.
    max_len_px += DPI(4);

    Size page  = sb_.GetPage();
    Size total = Size(max(1, max_len_px), max(1, GetLineCount()));

    sb_.SetTotal(total);
    sb_.SetPage(page);

    // Clamp scroll position when content shrinks (fixes "delete doesn't move back").
    Point cur = GetScrollPos();

    int maxx = max(0, total.cx - page.cx); // pixels
    int maxy = max(0, total.cy - page.cy); // lines

    Point clamped;
    clamped.x = minmax(cur.x, 0, maxx);
    clamped.y = minmax(cur.y, 0, maxy);

    if(clamped != cur) {
        sb_.Set(clamped);
    }
}

void UiBaseEdit::Scroll()
{
    scroller_.Scroll(*this, GetScrollPos());
    PlaceCaret();
    Refresh();
}

Rect UiBaseEdit::GetViewRect() const
{
    const Style& style = GetEffectiveStyle();
    // GetView() already subtracts U++ frames (e.g. scrollbars added via AddFrame).
    // Then apply styled frame + inset + padding consistently.
    return UiStyledInnerRect(GetView(), style.metrics, style.skin);
}

Rect UiBaseEdit::GetTextRect() const
{
    if(sides_.IsEmpty())
        return GetViewRect();

    return text_rect_.IsEmpty() ? GetViewRect() : text_rect_;
}


Point UiBaseEdit::GetContentArea() const
{
    const Style& style = GetEffectiveStyle();
    Rect ci = UiNonNegativeThickness(style.skin.content_inset);
    Rect cp = UiNonNegativeThickness(style.metrics.content_margin);
    int fw  = UiResolvedFrameWidth(style.metrics, style.skin);
    return Point(fw + ci.left + cp.left,
                 fw + ci.top + cp.top);
}

// --------------------------------------------------------------------
// Flank layout & helpers
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Side layout & helpers
// --------------------------------------------------------------------

UiBaseEdit::SideItem* UiBaseEdit::FindSideById(int id)
{
    if(id <= 0) return nullptr;
    for(int i = 0; i < sides_.GetCount(); ++i) {
        if(sides_[i].id == id)
            return &sides_[i];
    }
    return nullptr;
}

void UiBaseEdit::LayoutSides()
{
    const Style& style = GetEffectiveStyle();
    // Base rect for chrome/side controls:
    // - GetView() subtracts U++ frames like scrollbars
    // - We remove only the styled frame here (NOT text padding)
    Rect chrome = GetView();
    int  fw     = max(0, style.metrics.frame_width);
    chrome.Deflate(fw, fw);

    text_rect_ = Rect(0, 0, 0, 0);

    if(chrome.IsEmpty()) {
        // Still hide invalid/non-visible children if needed
        for(int i = 0; i < sides_.GetCount(); ++i) {
            SideItem& s = sides_[i];
            if(s.ctrl && s.ctrl->GetParent() == this)
                s.ctrl->Hide();
        }
        return;
    }

    if(sides_.IsEmpty()) {
        Rect cp = UiNonNegativeThickness(style.metrics.content_margin);
        text_rect_ = UiApplyThicknessRect(chrome, cp);
        return;
    }

    Rect remaining = chrome;

    Vector<int> left_idx, right_idx, top_idx, bottom_idx;

    // Collect and validate sides
    for(int i = 0; i < sides_.GetCount(); ++i) {
        SideItem& s = sides_[i];

        if(!s.ctrl || s.ctrl->GetParent() != this) {
            if(s.ctrl && s.ctrl->GetParent() == this)
                s.ctrl->Hide();
            continue;
        }

        if(!s.visible) {
            s.ctrl->Hide();
            continue;
        }

        switch(s.side) {
        case UiAlign::LEFT:   left_idx.Add(i);   break;
        case UiAlign::RIGHT:  right_idx.Add(i);  break;
        case UiAlign::TOP:    top_idx.Add(i);    break;
        case UiAlign::BOTTOM: bottom_idx.Add(i); break;
        default:
            break;
        }
    }

    auto ClampPos = [](int v) -> int { return v < 0 ? 0 : v; };

    // ---- Top / Bottom ----
    auto layout_side_tb = [&](Vector<int>& idx, bool top_side) {
        if(idx.IsEmpty() || remaining.IsEmpty())
            return;

        UiDirection dir = sides_[idx[0]].dir;

        // Default strip height if control provides no fixed/min height.
        int default_h = max(1, max(caret_height_, font_size_.cy + DPI(4)));
        default_h = min(default_h, remaining.GetHeight());

        if(dir == UiDirection::H) {
            // Horizontal row at top/bottom
            Vector<Size> sz;
            sz.SetCount(idx.GetCount());

            int total_w = 0;
            int strip_h = 0;

            for(int k = 0; k < idx.GetCount(); ++k) {
                SideItem& s = sides_[idx[k]];
                Ctrl*     c = s.ctrl;
                Size      ms = c ? c->GetMinSize() : Size(0, 0);

                int h = (s.fixed.cy > 0) ? s.fixed.cy
                      : (ms.cy > 0)      ? ms.cy
                                         : default_h;
                h = min(h, remaining.GetHeight());

                int w = (s.fixed.cx > 0) ? s.fixed.cx
                      : (ms.cx > 0)      ? ms.cx
                                         : h; // square fallback

                w = min(w, remaining.GetWidth());

                sz[k] = Size(ClampPos(w), ClampPos(h));
                total_w += sz[k].cx;
                strip_h = max(strip_h, sz[k].cy);
            }

            strip_h = min(strip_h, remaining.GetHeight());

            int x = remaining.left;
            int y = top_side ? remaining.top : (remaining.bottom - strip_h);

            int reserve_edge = top_side ? remaining.top : remaining.bottom;

            for(int k = 0; k < idx.GetCount(); ++k) {
                SideItem& s  = sides_[idx[k]];
                Size      ss = sz[k];

                int cy = y + (strip_h - ss.cy) / 2;
                Rect rr = RectC(x, cy, ss.cx, ss.cy);

                s.rect = rr;
                s.ctrl->SetRect(rr);

                x += ss.cx;

                if(!s.overlay) {
                    if(top_side)
                        reserve_edge = max(reserve_edge, rr.bottom);
                    else
                        reserve_edge = min(reserve_edge, rr.top);
                }
            }

            if(top_side)
                remaining.top = min(remaining.bottom, reserve_edge);
            else
                remaining.bottom = max(remaining.top, reserve_edge);
        }
        else {
            // Vertical stack at top/bottom
            int count = idx.GetCount();
            if(count <= 0)
                return;

            Vector<int> heights;
            heights.SetCount(count);

            int total_h = 0;
            for(int k = 0; k < count; ++k) {
                SideItem& s = sides_[idx[k]];
                Ctrl*     c = s.ctrl;
                Size      ms = c ? c->GetMinSize() : Size(0, 0);

                int h = (s.fixed.cy > 0) ? s.fixed.cy
                      : (ms.cy > 0)      ? ms.cy
                                         : default_h;
                h = min(h, remaining.GetHeight());

                heights[k] = ClampPos(h);
                total_h += heights[k];
            }

            total_h = min(total_h, remaining.GetHeight());

            int x = remaining.left;
            int y = top_side ? remaining.top : (remaining.bottom - total_h);

            int reserve_edge = top_side ? remaining.top : remaining.bottom;

            for(int k = 0; k < count; ++k) {
                SideItem& s  = sides_[idx[k]];
                Ctrl*     c  = s.ctrl;
                Size      ms = c ? c->GetMinSize() : Size(0, 0);

                int h = heights[k];
                int w = (s.fixed.cx > 0) ? s.fixed.cx
                      : (ms.cx > 0)      ? ms.cx
                                         : h;

                w = min(w, remaining.GetWidth());

                Rect rr = RectC(x, y, ClampPos(w), h);

                s.rect = rr;
                s.ctrl->SetRect(rr);

                y += h;

                if(!s.overlay) {
                    if(top_side)
                        reserve_edge = max(reserve_edge, rr.bottom);
                    else
                        reserve_edge = min(reserve_edge, rr.top);
                }
            }

            if(top_side)
                remaining.top = min(remaining.bottom, reserve_edge);
            else
                remaining.bottom = max(remaining.top, reserve_edge);
        }
    };

    // Do top/bottom first so left/right fill the remaining height
    layout_side_tb(top_idx, true);
    layout_side_tb(bottom_idx, false);

    // ---- Left / Right ----
    auto layout_side_lr = [&](Vector<int>& idx, bool left_side) {
        if(idx.IsEmpty() || remaining.IsEmpty())
            return;

        UiDirection dir = sides_[idx[0]].dir;

        int strip_h = remaining.GetHeight();
        if(strip_h <= 0)
            return;

        if(dir == UiDirection::H) {
            // Horizontal row hugging left/right, filling the available height (chrome, not padded text)
            Vector<Size> sz;
            sz.SetCount(idx.GetCount());

            int total_w = 0;

            for(int k = 0; k < idx.GetCount(); ++k) {
                SideItem& s  = sides_[idx[k]];
                Ctrl*     c  = s.ctrl;
                Size      ms = c ? c->GetMinSize() : Size(0, 0);

                // Height: fill unless fixed.cy explicitly requests a smaller control
                int h = (s.fixed.cy > 0) ? min(s.fixed.cy, strip_h) : strip_h;

                // Width: prefer fixed.cx, else minsize.cx, else square from height
                int w = (s.fixed.cx > 0) ? s.fixed.cx
                      : (ms.cx > 0)      ? ms.cx
                                         : h;

                w = min(w, remaining.GetWidth());

                sz[k] = Size(ClampPos(w), ClampPos(h));
                total_w += sz[k].cx;
            }

            int x = left_side ? remaining.left : (remaining.right - total_w);

            // Reserve boundary based on actual non-overlay geometry (handles overlay mixed with non-overlay)
            int reserve_edge = left_side ? remaining.left : remaining.right;

            for(int k = 0; k < idx.GetCount(); ++k) {
                SideItem& s  = sides_[idx[k]];
                Size      ss = sz[k];

                int y = remaining.top + (strip_h - ss.cy) / 2;
                Rect rr = RectC(x, y, ss.cx, ss.cy);

                s.rect = rr;
                s.ctrl->SetRect(rr);

                x += ss.cx;

                if(!s.overlay) {
                    if(left_side)
                        reserve_edge = max(reserve_edge, rr.right);
                    else
                        reserve_edge = min(reserve_edge, rr.left);
                }
            }

            if(left_side)
                remaining.left = min(remaining.right, reserve_edge);
            else
                remaining.right = max(remaining.left, reserve_edge);
        }
        else {
            // Vertical stack along left/right edge
            int count = idx.GetCount();
            if(count <= 0)
                return;

            // Compute each cell height; width derived from fixed/min/square
            Vector<int> cell_h;
            cell_h.SetCount(count);

            int y = remaining.top;
            int reserve_edge = left_side ? remaining.left : remaining.right;

            // Determine strip width as max of requested widths
            int strip_w = 0;

            for(int k = 0; k < count; ++k) {
                SideItem& s = sides_[idx[k]];
                Ctrl*     c = s.ctrl;
                Size      ms = c ? c->GetMinSize() : Size(0, 0);

                int h = (s.fixed.cy > 0) ? s.fixed.cy
                      : (ms.cy > 0)      ? ms.cy
                                         : max(1, strip_h / count);
                h = min(h, remaining.bottom - y);
                h = ClampPos(h);

                cell_h[k] = h;

                int w = (s.fixed.cx > 0) ? s.fixed.cx
                      : (ms.cx > 0)      ? ms.cx
                                         : h;

                strip_w = max(strip_w, min(w, remaining.GetWidth()));
            }

            // Place items
            y = remaining.top;
            for(int k = 0; k < count; ++k) {
                SideItem& s = sides_[idx[k]];

                int h = cell_h[k];
                if(h <= 0)
                    continue;

                int x = left_side ? remaining.left : (remaining.right - strip_w);

                Rect rr = RectC(x, y, strip_w, h);

                s.rect = rr;
                s.ctrl->SetRect(rr);

                y += h;

                if(!s.overlay) {
                    if(left_side)
                        reserve_edge = max(reserve_edge, rr.right);
                    else
                        reserve_edge = min(reserve_edge, rr.left);
                }
            }

            if(left_side)
                remaining.left = min(remaining.right, reserve_edge);
            else
                remaining.right = max(remaining.left, reserve_edge);
        }
    };

    layout_side_lr(left_idx, true);
    layout_side_lr(right_idx, false);

    // Finally: compute text rect from remaining chrome area, then apply styled content padding.
    Rect cp = UiNonNegativeThickness(style.metrics.content_margin);
    Rect tr = UiApplyThicknessRect(remaining, cp);

    // Clamp degenerate
    if(tr.right < tr.left)   tr.right = tr.left;
    if(tr.bottom < tr.top)   tr.bottom = tr.top;

    text_rect_ = tr;
}


// --------------------------------------------------------------------
// Side public API
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Side public API
// --------------------------------------------------------------------

SideHandle UiBaseEdit::AddToSide(Ctrl& c, UiAlign side, Size fixed, UiDirection dir)
{
    SideItem si;
    si.ctrl    = &c;
    si.id      = next_side_id_++;
    si.side    = side;
    si.dir     = dir;
    si.fixed   = fixed;
    si.visible = true;
    si.overlay = false;
    sides_.Add(pick(si));

    Add(c);
    Layout();
    Refresh();
    return SideHandle(this, sides_.Top().id);
}

void UiBaseEdit::RemoveSide(int id)
{
    for(int i = 0; i < sides_.GetCount(); ++i) {
        if(sides_[i].id == id) {
            if(sides_[i].ctrl && sides_[i].ctrl->GetParent() == this)
                sides_[i].ctrl->Hide();
            sides_.Remove(i);
            text_rect_ = Rect(0, 0, 0, 0);
            Layout();
            Refresh();
            return;
        }
    }
}

void UiBaseEdit::ClearSides()
{
    for(int i = 0; i < sides_.GetCount(); ++i) {
        if(sides_[i].ctrl && sides_[i].ctrl->GetParent() == this)
            sides_[i].ctrl->Hide();
    }
    sides_.Clear();
    text_rect_ = Rect(0, 0, 0, 0);
    Layout();
    Refresh();
}

void UiBaseEdit::SetSideVisible(int id, bool vis)
{
    SideItem* s = FindSideById(id);
    if(!s) return;
    if(s->visible == vis) return;
    s->visible = vis;
    if(s->ctrl) {
        if(vis) s->ctrl->Show();
        else    s->ctrl->Hide();
    }
    text_rect_ = Rect(0, 0, 0, 0);
    Layout();
    Refresh();
}

void UiBaseEdit::SetSideOverlay(int id, bool overlay)
{
    SideItem* s = FindSideById(id);
    if(!s) return;
    if(s->overlay == overlay) return;
    s->overlay = overlay;
    text_rect_ = Rect(0, 0, 0, 0);
    Layout();
    Refresh();
}

void UiBaseEdit::SetSideFixedSize(int id, Size sz)
{
    SideItem* s = FindSideById(id);
    if(!s) return;
    if(s->fixed == sz) return;
    s->fixed = sz;
    text_rect_ = Rect(0, 0, 0, 0);
    Layout();
    Refresh();
}

void UiBaseEdit::SetSideDirection(int id, UiDirection dir)
{
    SideItem* s = FindSideById(id);
    if(!s) return;
    if(s->dir == dir) return;
    s->dir = dir;
    text_rect_ = Rect(0, 0, 0, 0);
    Layout();
    Refresh();
}

bool UiBaseEdit::HasSide(int id) const
{
    if(id <= 0) return false;
    for(int i = 0; i < sides_.GetCount(); ++i) {
        if(sides_[i].id == id)
            return true;
    }
    return false;
}

int UiBaseEdit::GetSideId(Ctrl& c) const
{
    for(int i = 0; i < sides_.GetCount(); ++i) {
        if(sides_[i].ctrl == &c)
            return sides_[i].id;
    }
    return -1;
}

SideHandle UiBaseEdit::GetSideHandle(Ctrl& c) const
{
    int id = GetSideId(c);
    if(id <= 0)
        return SideHandle();
    return SideHandle(const_cast<UiBaseEdit*>(this), id);
}


// --------------------------------------------------------------------
// Caret & Position
// --------------------------------------------------------------------

int UiBaseEdit::GetLine(int64 pos) const
{
    LTIMING("UiBaseEdit::GetLine");
    int64 current_pos = 0;
    for(int i = 0; i < GetLineCount(); i++) {
        int len = GetLineLength(i) + 1;
        if(pos < current_pos + len || i == GetLineCount() - 1)
            return i;
        current_pos += len;
    }
    return GetLineCount() - 1;
}

int64 UiBaseEdit::GetPos(int line, int col) const
{
    LTIMING("UiBaseEdit::GetPos(line, col)");
    line = minmax(line, 0, GetLineCount() - 1);
    
    int64 pos = 0;
    for(int i = 0; i < line; i++)
        pos += GetLineLength(i) + 1;
    
    col = minmax(col, 0, GetLineLength(line));
    return pos + col;
}

Point UiBaseEdit::GetColumnLine(int64 pos) const
{
    LTIMING("UiBaseEdit::GetColumnLine");
    int   line_idx       = GetLine(pos);
    int64 line_start_pos = GetPos(line_idx, 0);
    return Point(int(pos - line_start_pos), line_idx);
}

void UiBaseEdit::PlaceCaret(int64 new_cursor, bool sel)
{
    LTIMING("UiBaseEdit::PlaceCaret(pos, sel)");
    new_cursor = minmax(new_cursor, (int64)0, total_wchars_);
    
    if(!sel)
        anchor_ = -1;
    else if(anchor_ < 0)
        anchor_ = cursor_;
        
    bool selection_changed = false;

    if(cursor_ != new_cursor || (sel && anchor_ < 0) || (!sel && anchor_ >= 0)) {
        cursor_ = new_cursor;
        Point cl = GetColumnLine(cursor_);
        gcolumn_ = cl.x;
        PlaceCaret();          // update pixel position
        selection_changed = true;
    }

    if(selection_changed) {
        Refresh();             // repaint selection + caret
        WhenSelection();
    }

    ScrollToCaret();
}


void UiBaseEdit::PlaceCaret()
{
    const Style& style = GetEffectiveStyle();
    LTIMING("UiBaseEdit::PlaceCaret()");

    if(GetLineCount() <= 0) {
        caret_pos_ = Point(0, 0);
        return;
    }

    Rect text_r = GetTextRect();
    Point spos  = GetScrollPos();

    Point cl       = GetColumnLine(cursor_);
    int   line_idx = cl.y;
    int   col      = cl.x;

    EnsureTextMetricsCache();

    int prefix_px = 0;
    if(line_idx >= 0 && line_idx < line_metrics_cache_.GetCount()) {
        const LineMetricsCache& mc = line_metrics_cache_[line_idx];
        int n = minmax(col, 0, mc.char_widths.GetCount());
        for(int i = 0; i < n; i++)
            prefix_px += mc.char_widths[i];
    }

    int view_w        = text_r.GetWidth();
    int line_px_width = (line_idx >= 0 && line_idx < line_metrics_cache_.GetCount())
                        ? line_metrics_cache_[line_idx].line_px
                        : 0;

    int start_x = text_r.left - spos.x; // X scroll is pixels now
    switch(style.text_align) {
    case UiAlign::CENTER:
        if(line_px_width < view_w)
            start_x += (view_w - line_px_width) / 2;
        break;
    case UiAlign::RIGHT:
        if(line_px_width < view_w)
            start_x += (view_w - line_px_width);
        break;
    case UiAlign::LEFT:
    default:
        break;
    }

    int yoff      = GetSingleLineYOffset();

    caret_pos_.x = start_x + prefix_px;
    caret_pos_.y = text_r.top + yoff
                   + line_idx * caret_height_
                   - (spos.y * caret_height_);

    Refresh();
}


void UiBaseEdit::ScrollToCaret()
{
    if(!HasFocus() || !IsVisible())
        return;

    if(GetLineCount() <= 0)
        return;

    Point p = GetColumnLine(cursor_);

    EnsureTextMetricsCache();
    int x_pixels = 0;
    if(p.y >= 0 && p.y < line_metrics_cache_.GetCount()) {
        const LineMetricsCache& mc = line_metrics_cache_[p.y];
        int n = minmax(p.x, 0, mc.char_widths.GetCount());
        for(int i = 0; i < n; i++)
            x_pixels += mc.char_widths[i];
    }
    sb_.ScrollInto(Point(x_pixels, p.y));
}

int64 UiBaseEdit::GetMousePos(Point p) const
{
    const Style& style = GetEffectiveStyle();
    LTIMING("UiBaseEdit::GetMousePos");
    if(caret_height_ == 0)
        return 0;

    Rect text_r = GetTextRect();
    int  view_w = text_r.GetWidth();

    Point spos = GetScrollPos();

    p -= text_r.TopLeft();
    p.x += spos.x;                 // X scroll is pixels
    p.y += spos.y * caret_height_; // Y scroll is lines

    int line_idx = minmax(p.y / caret_height_, 0, GetLineCount() - 1);

    WString line     = GetDisplayLine(line_idx);
    int     line_len = line.GetLength();
    EnsureTextMetricsCache();
    int     line_px  = (line_idx >= 0 && line_idx < line_metrics_cache_.GetCount())
                       ? line_metrics_cache_[line_idx].line_px
                       : 0;

    int align_offset = 0;
    switch(style.text_align) {
    case UiAlign::CENTER:
        if(line_px < view_w)
            align_offset = (view_w - line_px) / 2;
        break;
    case UiAlign::RIGHT:
        if(line_px < view_w)
            align_offset = (view_w - line_px);
        break;
    case UiAlign::LEFT:
    default:
        break;
    }

    p.x -= align_offset;

    int x = 0;
    for(int i = 0; i < line_len; i++) {
        int char_w = space_width_cache_;
        if(line_idx >= 0 && line_idx < line_metrics_cache_.GetCount() &&
           i >= 0 && i < line_metrics_cache_[line_idx].char_widths.GetCount())
            char_w = line_metrics_cache_[line_idx].char_widths[i];

        if(p.x < x + char_w / 2)
            return GetPos(line_idx, i);

        x += char_w;
    }

    return GetPos(line_idx, line_len);
}


Rect UiBaseEdit::GetCaretRect(int64 pos) const
{
    const Style& style = GetEffectiveStyle();
    LTIMING("UiBaseEdit::GetCaretRect");

    Point cl   = GetColumnLine(pos);
    int   line = cl.y;
    if(line < 0 || line >= GetLineCount())
        return Rect(0, 0, 0, 0);

    Rect text_r = GetTextRect();

    int view_w    = text_r.GetWidth();
    int line_px   = (line >= 0 && line < line_metrics_cache_.GetCount())
                    ? line_metrics_cache_[line].line_px
                    : 0;
    int align_off = 0;

    switch(style.text_align) {
    case UiAlign::CENTER:
        if(line_px < view_w)
            align_off = (view_w - line_px) / 2;
        break;
    case UiAlign::RIGHT:
        if(line_px < view_w)
            align_off = (view_w - line_px);
        break;
    case UiAlign::LEFT:
    default:
        break;
    }

    int prefix_px = 0;
    if(line >= 0 && line < line_metrics_cache_.GetCount()) {
        const LineMetricsCache& mc = line_metrics_cache_[line];
        int n = minmax(cl.x, 0, mc.char_widths.GetCount());
        for(int i = 0; i < n; i++)
            prefix_px += mc.char_widths[i];
    }

    Point spos = GetScrollPos();
    int   yoff = GetSingleLineYOffset();

    int base_x = text_r.left - spos.x; // X scroll is pixels
    int base_y = text_r.top  + yoff - (spos.y * caret_height_);

    int caret_x = base_x + align_off + prefix_px;
    int caret_y = base_y + line * caret_height_;

    int cx = style.block_caret ? font_size_.cx : max(style.caret_width, 1);

    return Rect(caret_x, caret_y, caret_x + cx, caret_y + caret_height_);
}


Rect UiBaseEdit::GetCaret() const
{
    return GetCaretRect(cursor_);
}

// --------------------------------------------------------------------
// Painting
// --------------------------------------------------------------------
void UiBaseEdit::UpdateVisualState()
{
    bool enabled = IsEnabled();
    bool hot     = mouse_over_ || has_focus_;
    visual_state_ = ResolveStyledState(enabled, hot, pressed_);
}


void UiBaseEdit::Paint(Draw& w)
{
    const Style& style = GetEffectiveStyle();
    LTIMING("UiBaseEdit::Paint");

    Rect r = GetSize();
    if(r.IsEmpty())
        return;

    bool enabled   = IsEnabled();
    bool has_focus = HasFocus();

    UpdateVisualState();
    StyledState st = visual_state_;

    StyledPalette paint_palette = style.palette;
    const StyledMetrics& m      = style.metrics;
    const StyledSkin& skin      = style.skin;

    if(IsReadOnly() && style.show_readonly_bg) {
        UiFill ro = UiFill::Solid(SColorFace());
        paint_palette.face[ST_NORMAL]  = ro;
        paint_palette.face[ST_HOT]     = ro;
        paint_palette.face[ST_PRESSED] = ro;
    }

    Rect outer = r;

    if(WhenPaintBackground)
        WhenPaintBackground(w, outer, paint_palette, m, skin, st, has_focus);
    else
        UiPaintStyledBackground(w, outer, paint_palette, m, skin, st, has_focus);

    Rect text_r = GetTextRect();
    if(text_r.IsEmpty())
        return;

    Point spos = GetScrollPos();
    int   yoff = GetSingleLineYOffset();

    Font fnt = style.font;
    if(IsNull(fnt))
        fnt = StdFont();

    w.Clip(text_r);

    int base_x = text_r.left - spos.x;               // pixels
    int base_y = text_r.top  + yoff - (spos.y * caret_height_);

    if(IsEmpty() && !placeholder_text_.IsEmpty() && !HasFocus()) {
        int view_w = text_r.GetWidth();
        int ph_w   = placeholder_width_cache_;

        int baseline_y = text_r.top + yoff + (caret_height_ - font_size_.cy) / 2;

        int px = text_r.left;
        switch(style.text_align) {
        case UiAlign::CENTER:
            if(ph_w < view_w)
                px += (view_w - ph_w) / 2;
            break;
        case UiAlign::RIGHT:
            if(ph_w < view_w)
                px += (view_w - ph_w);
            break;
        case UiAlign::LEFT:
        default:
            break;
        }

        w.DrawText(px, baseline_y, placeholder_text_, fnt, style.placeholder_ink);
    }

    int first_line = spos.y;
    int last_line  = min(first_line + sb_.GetPage().cy + 1, GetLineCount() - 1);

    for(int i = first_line; i <= last_line; i++)
        PaintLine(w, i, base_x, base_y + i * caret_height_, text_r);

    w.End();

    bool has_text   = !IsEmpty();
    bool show_caret = HasFocus()
                      && IsShowEnabled()
                      && drop_cursor_ < 0
                      && has_text;

    if(show_caret) {
        Rect cr = GetCaretRect(cursor_);

        if(!style.block_caret && !overwrite_) {
            int glyph_h = font_size_.cy;
            if(glyph_h <= 0)
                glyph_h = caret_height_;
            int top = cr.top + (caret_height_ - glyph_h) / 2;
            cr.top    = top;
            cr.bottom = top + glyph_h;
        }

        if(style.block_caret || overwrite_) {
            Rect underline = cr;
            underline.top = underline.bottom - DPI(2);
            w.DrawRect(underline, style.caret_color);
        }
        else {
            if(cr.Width() <= 0)
                cr.right = cr.left + max(style.caret_width, DPI(1));
            w.DrawRect(cr, style.caret_color);
        }
    }

    if(drop_cursor_ >= 0) {
        Rect cr = GetCaretRect(drop_cursor_);
        int glyph_h = font_size_.cy;
        if(glyph_h <= 0)
            glyph_h = caret_height_;
        int top = cr.top + (caret_height_ - glyph_h) / 2;
        cr.top    = top;
        cr.bottom = top + glyph_h;

        w.DrawRect(cr, SColorHighlight());
    }

    if(WhenPaintForeground)
        WhenPaintForeground(w, outer, paint_palette, m, skin, st, has_focus);
    else
        UiPaintStyledForeground(w, outer, paint_palette, m, skin, st, has_focus);
}


void UiBaseEdit::PaintLine(Draw& w, int i, int x, int y, const Rect& clip) const
{
    const Style& style = GetEffectiveStyle();
    LTIMING("UiBaseEdit::PaintLine");

    WString text = GetDisplayLine(i);
    int     len  = text.GetLength();
    int64   line_pos = GetPos(i, 0);

    int line_height = GetVisualLineHeight();

    int line_px_width = (i >= 0 && i < line_metrics_cache_.GetCount())
                        ? line_metrics_cache_[i].line_px
                        : 0;
    int view_w        = clip.GetWidth();

    int px = x;
    switch(style.text_align) {
    case UiAlign::CENTER:
        if(line_px_width < view_w)
            px += (view_w - line_px_width) / 2;
        break;
    case UiAlign::RIGHT:
        if(line_px_width < view_w)
            px += (view_w - line_px_width);
        break;
    case UiAlign::LEFT:
    default:
        break;
    }

    int text_y = y + (line_height - font_size_.cy) / 2;

    int64 sel_l, sel_h;
    bool  has_sel = GetSelection(sel_l, sel_h);

    int s1 = -1, s2 = -1;
    if(has_sel) {
        s1 = int(max((int64)0, sel_l - line_pos));
        s2 = int(min((int64)len, sel_h - line_pos));
    }

    bool enabled = IsEnabled();
    StyledState ink_state = enabled ? ST_NORMAL : ST_DISABLED;

    for(int j = 0; j <= len; j++) {
        wchar chr    = (j < len) ? text[j] : 0;
        int   char_w = space_width_cache_;

        if(j < len) {
            if(i >= 0 && i < line_metrics_cache_.GetCount() &&
               j >= 0 && j < line_metrics_cache_[i].char_widths.GetCount()) {
                char_w = line_metrics_cache_[i].char_widths[j];
            }
        }

        bool is_sel = (has_sel && j >= s1 && j < s2);

        if(is_sel)
            w.DrawRect(px, y, char_w, line_height, style.selection_color);

        if(j < len) {
            Color ink = is_sel
                        ? style.selection_ink
                        : style.palette.ink[ink_state];

            if(chr == ' ' && style.show_spaces) {
                w.DrawText(px + (char_w - font_size_.cx) / 2, text_y, ".", style.font, style.whitespace_color);
            }
            else if(chr == '\t' && style.show_tabs) {
                w.DrawText(px + (char_w - font_size_.cx) / 2, text_y, "?", style.font, style.tab_char_color);
            }
            else if(chr != '\t') {
                w.DrawText(px, text_y, text.Mid(j, 1), style.font, ink);
            }
        }
        else if(style.show_line_endings && accepts_newlines_ && i < GetLineCount() - 1) {
            w.DrawText(px, text_y, "?", style.font, style.whitespace_color);
        }

        px += char_w;
    }
}



// --------------------------------------------------------------------
// Input & Events
// --------------------------------------------------------------------
void UiBaseEdit::CancelMode()
{
    pressed_     = false;
    drop_cursor_ = -1;

    UpdateVisualState();
    Refresh();
}

void UiBaseEdit::MouseEnter(Point p, dword flags)
{
    mouse_over_ = true;
    UpdateVisualState();
    Refresh();
}

void UiBaseEdit::MouseLeave()
{
    mouse_over_ = false;
    UpdateVisualState();
    Refresh();
}

void UiBaseEdit::MouseMove(Point p, dword flags)
{
    Ctrl::MouseMove(p, flags);

    if(HasCapture() && (flags & K_MOUSELEFT)) {
        int64 pos = GetMousePos(p);
        PlaceCaret(pos, true);
    }
}

void UiBaseEdit::GotFocus()
{
    has_focus_ = true;
    UpdateVisualState();
    Refresh();
}

void UiBaseEdit::LostFocus()
{
    has_focus_ = false;
    UpdateVisualState();
    Refresh();
}


void UiBaseEdit::LeftDown(Point p, dword flags)
{
    if(!IsShowEnabled() || !HasMouse()) return;
    if(click_focus_ && !HasFocus())
        SetFocus();
    
    int64 pos = GetMousePos(p);
    PlaceCaret(pos, flags & K_SHIFT);
    SetCapture();
    pressed_ = true;
}

void UiBaseEdit::LeftDrag(Point p, dword flags)
{
    if(!HasCapture()) return;
    int64 pos = GetMousePos(p);
    PlaceCaret(pos, true);
}

void UiBaseEdit::LeftUp(Point p, dword flags)
{
    pressed_ = false;
    ReleaseCapture();
}

void UiBaseEdit::LeftDouble(Point p, dword flags)
{
    // Standard behaviour:
    //  - Double-click selects the "word" under the mouse.
    //  - Word = run of non-whitespace characters on the same line.
    if(!IsShowEnabled() || !HasMouse())
        return;

    int64 pos  = GetMousePos(p);
    int   line = GetLine(pos);
    if(line < 0 || line >= GetLineCount())
        return;

    int64 line_start = GetPos(line, 0);
    WString wline    = GetDisplayLine(line);

    int col = (int)(pos - line_start);
    col = minmax(col, 0, wline.GetCount());

    auto is_sep = [](wchar ch) -> bool {
        return ch == 0 || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
    };

    int left  = col;
    int right = col;

    while(left > 0 && !is_sep(wline[left - 1]))
        --left;
    while(right < wline.GetCount() && !is_sep(wline[right]))
        ++right;

    int64 sel_l = GetPos(line, left);
    int64 sel_h = GetPos(line, right);

    SetSelection(sel_l, sel_h);
}

void UiBaseEdit::LeftTriple(Point p, dword flags)
{
    // Triple-click: select the entire logical line under the mouse.
    // (More in line with "standard" editors.)
    if(!IsShowEnabled() || !HasMouse())
        return;

    int64 pos  = GetMousePos(p);
    int   line = GetLine(pos);
    if(line < 0 || line >= GetLineCount())
        return;

    int64 sel_l = GetPos(line, 0);
    int64 sel_h = GetPos(line, GetLineLength(line));

    SetSelection(sel_l, sel_h);
}


void UiBaseEdit::RightDown(Point p, dword flags)
{
    if(!HasFocus())
        SetFocus();
        
    int64 pos = GetMousePos(p);
    if(!IsSelection() || pos < min(cursor_, anchor_) || pos > max(cursor_, anchor_))
        PlaceCaret(pos, false);
        
    MenuBar::Execute(WhenBar);
}

void UiBaseEdit::MouseWheel(Point p, int zdelta, dword flags)
{
    sb_.WheelY(zdelta);
}

void UiBaseEdit::HorzMouseWheel(Point p, int zdelta, dword flags)
{
    sb_.WheelX(zdelta);
}

Image UiBaseEdit::CursorImage(Point p, dword flags)
{
    return IsReadOnly() ? Image::Arrow() : Image::IBeam();
}

bool UiBaseEdit::Key(dword key, int count)
{
    if(!IsShowEnabled()) return false;
    
    bool sel = key & K_SHIFT;
    NextUndo();
    
    switch(key & ~K_SHIFT) {
    case K_LEFT:
        PlaceCaret(cursor_ - 1, sel);
        return true;
    case K_RIGHT:
        PlaceCaret(cursor_ + 1, sel);
        return true;
    case K_UP:
        PlaceCaret(GetPos(GetColumnLine(cursor_).y - 1, gcolumn_), sel);
        return true;
    case K_DOWN:
        PlaceCaret(GetPos(GetColumnLine(cursor_).y + 1, gcolumn_), sel);
        return true;
    case K_HOME:
        PlaceCaret(GetPos(GetColumnLine(cursor_).y, 0), sel);
        return true;
    case K_END:
        PlaceCaret(GetPos(GetColumnLine(cursor_).y, GetLineLength(GetLine(cursor_))), sel);
        return true;
        
    case K_PAGEUP:
        PlaceCaret(GetPos(max(0, GetColumnLine(cursor_).y - sb_.GetPage().cy), gcolumn_), sel);
        return true;
    case K_PAGEDOWN:
        PlaceCaret(GetPos(min(GetLineCount() - 1, GetColumnLine(cursor_).y + sb_.GetPage().cy), gcolumn_), sel);
        return true;

    case K_CTRL_LEFT:
    case K_CTRL_RIGHT:
        return true;
        
    case K_CTRL_A:
        SelectAll();
        return true;
    case K_CTRL_C:
    case K_CTRL_INSERT:
        Copy();
        return true;
    }
    
    if(IsReadOnly()) return false;
    
    switch(key & ~K_SHIFT) {
    case K_BACKSPACE:
        if(!RemoveSelection()) {
            if(cursor_ > 0) {
                RemoveU(cursor_ - 1, 1);
                PlaceCaret(cursor_ - 1, false);
            }
        }
        WhenChange();
        return true;
    case K_DELETE:
        if(!RemoveSelection()) {
            if(cursor_ < total_wchars_) {
                RemoveU(cursor_, 1);
                PlaceCaret(cursor_, false);
            }
        }
        WhenChange();
        return true;
        
    case K_ENTER:
        if(accepts_newlines_) {
            RemoveSelection();
            InsertU(cursor_, "\n", true);
            PlaceCaret(cursor_ + 1, false);
            WhenChange();
        } else {
            WhenAction();
        }
        return true;
        
    case K_TAB:
        if(accepts_tabs_) {
            RemoveSelection();
            InsertU(cursor_, "\t", true);
            PlaceCaret(cursor_ + 1, false);
            WhenChange();
            return true;
        }
        return false;

    case K_CTRL_X:
    case K_SHIFT_DELETE:
        Cut();
        WhenChange();
        return true;
        
    case K_CTRL_V:
    case K_SHIFT_INSERT:
        Paste();
        WhenChange();
        return true;
        
    case K_CTRL_Z:
        Undo();
        return true;
    case K_CTRL_Y:
        Redo();
        return true;
    case K_ESCAPE:
        ClearSelection();
        return true;
    }

    if(key >= 32 && key < 65536) {
        if(overwrite_ && !IsSelection() && cursor_ < total_wchars_ && GetChar(cursor_) != '\n')
            RemoveU(cursor_, count);
        else
            RemoveSelection();
        
        InsertU(cursor_, WString(key, count), true);
        PlaceCaret(cursor_ + count, false);
        WhenChange();
        return true;
    }

    return false;
}

// --------------------------------------------------------------------
// Drag & Drop
// --------------------------------------------------------------------

void UiBaseEdit::DragAndDrop(Point p, PasteClip& d)
{
    if(IsReadOnly() || !accepts_drop_ || !AcceptText(d)) {
        drop_cursor_ = -1;
        Refresh();
        return;
    }
    
    d.SetAction(DND_COPY);
    drop_cursor_ = GetMousePos(p);
    Refresh();
}

void UiBaseEdit::DragRepeat(Point p)
{
    int64 pos = GetMousePos(p);
    if(pos != drop_cursor_) {
        drop_cursor_ = pos;
        Refresh();
    }
}

void UiBaseEdit::DragLeave()
{
    if(drop_cursor_ >= 0) {
        drop_cursor_ = -1;
        Refresh();
    }
}

void UiBaseEdit::Drop(Point p, PasteClip& d)
{
    if(IsReadOnly() || !accepts_drop_ || !AcceptText(d)) {
        drop_cursor_ = -1;
        Refresh();
        return;
    }
    
    String  s    = GetString(d);
    WString text = WString(s);

    if(!accepts_newlines_) {
        text.Replace("\n", "");
        text.Replace("\r", "");
    }
    
    int64 pos = GetMousePos(p);
    
    NextUndo();
    int64 n = InsertU(pos, text, false);
    PlaceCaret(pos + n, false);
    WhenChange();
    
    drop_cursor_ = -1;
    Refresh();
}

// --------------------------------------------------------------------
// Misc Overrides
// --------------------------------------------------------------------

Size UiBaseEdit::GetMinSize() const
{
    const Style& style = GetEffectiveStyle();
    return UiStyledOuterSizeFromContent(Size(font_size_.cx * 4,
                                            GetVisualLineHeight()),
                                       style.metrics,
                                       style.skin);
}

// --------------------------------------------------------------------
// SideHandle implementation
// --------------------------------------------------------------------

SideHandle::SideHandle(UiBaseEdit* parent, int id)
    : parent_(parent)
    , id_(id)
{
}

bool SideHandle::IsValid() const
{
    return parent_ && id_ > 0 && parent_->HasSide(id_);
}

SideHandle& SideHandle::Visible(bool b)
{
    if(IsValid())
        parent_->SetSideVisible(id_, b);
    return *this;
}

SideHandle& SideHandle::Overlay(bool b)
{
    if(IsValid())
        parent_->SetSideOverlay(id_, b);
    return *this;
}

SideHandle& SideHandle::FixedSize(Size sz)
{
    if(IsValid())
        parent_->SetSideFixedSize(id_, sz);
    return *this;
}

SideHandle& SideHandle::Direction(UiDirection d)
{
    if(IsValid())
        parent_->SetSideDirection(id_, d);
    return *this;
}

Ctrl* SideHandle::TryGetCtrl() const
{
    if(!IsValid())
        return nullptr;
    UiBaseEdit::SideItem* s = parent_->FindSideById(id_);
    return s ? s->ctrl : nullptr;
}

SideHandle SideHandle::AddToSide(Ctrl& c,
                                 UiAlign side,
                                 Size fixed,
                                 UiDirection dir)
{
    if(!parent_)
        return SideHandle();
    return parent_->AddToSide(c, side, fixed, dir);
}




} // namespace Upp
