#ifndef _Ui_UiBaseEdit_h_
#define _Ui_UiBaseEdit_h_

/*
    UiBaseEdit
    ========== 

    Purpose
    - Shared styled text-edit foundation for the Ui edit control family.

    Intent
    - Centralize text model, caret/selection logic, scrolling, side-control
      composition, and the shared edit style contract in one place.

    Thread context
    - GUI thread only.

    Usage
    - Derive specialized edits such as UiLineEdit, UiMaskEdit, and
      UiPasswordEdit from this base instead of re-implementing edit behavior.

    Changelog
    - 2026-03: added release-standard file documentation.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h> // For ChStyle, ScrollBars
#include <Ui/UiStyle.h>      // Styling engine (UiAlign, UiDirection, StyledPalette, etc.)
#include <Ui/UiDraw.h>       // Drawing helpers (UiPaintFaceFrameDash, UiDraw9Slice, etc.)

namespace Upp {

// ============================================================================
//  UiBaseEdit + SideHandle
// ============================================================================
//
//  UiBaseEdit is the foundational base class for all text editing controls
//  in the Ui framework (UiLineEdit, UiPasswordEdit, UiMultiEdit, UiMaskEdit, etc.).
//
//  It handles:
//  - Core text model (lines, encoding).
//  - Rendering, caret management, scrolling.
//  - Undo/redo and clipboard operations.
//  - Styling via UiStyle (palette, metrics, skin, font).
//  - Optional side controls (buttons/icons/etc.) on each edge.
//
//  Side controls:
//  --------------
//  - Side controls are ordinary child controls (buttons, labels, etc.)
//    attached to the sides of the edit and auto-laid-out.
//  - You never compute their rectangles manually; just AddToSide(...).
//  - Useful for search icons, clear buttons, password eye/submit arrow,
//    spinner up/down buttons, etc.
//
//  SideHandle:
//  -----------
//  - SideHandle is the only safe way to refer to an attached side item
//    long-term. It stores a (UiBaseEdit*, id) pair.
//  - A SideHandle automatically becomes invalid if the side is removed.
//  - Always call IsValid() if you store it long-term.
//  - TryGetCtrl() returns a raw Ctrl* or nullptr if no longer valid.
// ============================================================================

class UiBaseEdit; // forward

// ---------------------------------------------------------------------------
// SideHandle - lightweight proxy for UiBaseEdit ?side? items
// ---------------------------------------------------------------------------
class SideHandle {
public:
    SideHandle() = default;
    SideHandle(UiBaseEdit* parent, int id);

    // Validation
    bool IsValid() const;

    // Fluent config (chainable)
    SideHandle& Visible(bool b = true);
    SideHandle& Hidden(bool b = true) { return Visible(!b); }
    SideHandle& Overlay(bool b = true);
    SideHandle& FixedSize(Size sz);
    SideHandle& Direction(UiDirection d);

    // Safe access
    int   GetId() const { return id_; }
    Ctrl* TryGetCtrl() const;     // nullptr if invalid

    // Chaining convenience: allows
    //   edit.SetPlaceholder("...")
    //       .AddToSide(icon, UiAlign::LEFT, Size(20,20)).Overlay(true)
    //       .AddToSide(clear_btn, UiAlign::RIGHT, Size(24,24)).Overlay(true);
    SideHandle AddToSide(Ctrl& c,
                         UiAlign side,
                         Size fixed = Size(0, 0),
                         UiDirection dir = UiDirection::H);

private:
    UiBaseEdit* parent_ = nullptr;
    int         id_     = 0;
};

// ============================================================================
//  UiBaseEdit
// ============================================================================

class UiBaseEdit : public Ctrl, public CtrlStyled<UiBaseEdit> {
public:
    typedef UiBaseEdit CLASSNAME;
    friend class SideHandle;

    // ------------------------------------------------------------------------
    // Style Definition
    // ------------------------------------------------------------------------
    struct Style : ChStyle<Style> {
        StyledPalette palette;        // Face, Frame, Ink
        StyledMetrics metrics;        // Radius, FrameWidth
        StyledSkin    skin;           // 9-slice skin (if any)

        Font    font        = StdFont(); // Default font

        UiAlign text_align  = UiAlign::LEFT; // LEFT / CENTER / RIGHT for text

        Color caret_color    = SColorText();
        int   caret_width    = DPI(1);
        bool  block_caret    = false;
        
        Color selection_color = SColorHighlight();
        Color selection_ink   = SColorHighlightText();
        Color placeholder_ink = SColorDisabled(); // Ink for placeholder
        
        Color whitespace_color = Blend(SColorLight, SColorHighlight);
        Color tab_char_color   = Blend(SColorLight, SColorHighlight, 150);
        
        int  tab_size          = 4;
        bool show_tabs         = false;
        bool show_spaces       = false;
        bool show_line_endings = false;
        bool show_readonly_bg  = true; // Use Paper_Readonly color

        void Serialize(Stream& s)
        {
            s % palette % metrics % skin
              % font
              % text_align
              % caret_color % caret_width % block_caret
              % selection_color % selection_ink % placeholder_ink
              % whitespace_color % tab_char_color
              % tab_size % show_tabs % show_spaces
              % show_line_endings % show_readonly_bg;
        }
    };

    static const Style& StyleDefault();

protected:
    // ------------------------------------------------------------------------
    // Internal Data Model (TextCtrl)
    // ------------------------------------------------------------------------
    struct Ln : Moveable<Ln> {
        int    len;  // Length in wchars
        String text; // Text in Utf8
        Ln() { len = 0; }
    };
    
    struct UndoRec {
        int    serial;
        int64  pos;
        int64  size;
        String data; // Compressed WString
        bool   typing;
        
        void   SetText(const WString& text);
        WString GetText() const;
    };

    Vector<Ln>       lin_;
    int64            total_wchars_ = 0; // Total wchar count
    int              undo_steps_   = 1000;
    int              undoserial_   = 0;
    bool             incundoserial_ = false;
    BiArray<UndoRec> undo_;
    BiArray<UndoRec> redo_;
    bool             undo_op_      = false;
    int              dirty_        = 0;

    // ------------------------------------------------------------------------
    // Internal View & State
    // ------------------------------------------------------------------------
    Style       style_;
    mutable Style themed_style_;
    mutable uint64 theme_revision_ = 0;
    bool        has_style_override_ = false;
    ScrollBars  sb_;
    Scroller    scroller_;
    Size        font_size_ {0, 0};   // Cached font size
    int64       cursor_      = 0;
    int64       anchor_      = -1;
    Point       caret_pos_ {0, 0};   // On-screen pixel position
    int         caret_height_ = 0;
    int         gcolumn_      = 0;   // Desired X position for Up/Down movement
    int64       drop_cursor_  = -1;  // Caret position for D&D

    bool        mouse_over_   = false;
    bool        pressed_      = false;
    bool        has_focus_    = false;
    StyledState visual_state_ = ST_NORMAL;

    // Behavior Flags
    String      placeholder_text_;
    bool        accepts_newlines_ = true;
    bool        accepts_tabs_     = true;
    bool        accepts_drop_     = true;
    bool        overwrite_        = false;
    bool        click_focus_      = true;

    // ------------------------------------------------------------------------
    // Side items (generic side controls)
    // ------------------------------------------------------------------------
    struct SideItem : Moveable<SideItem> {
        Ctrl*       ctrl    = nullptr;          // child control
        int         id      = -1;               // stable id returned from AddToSide
        UiAlign     side    = UiAlign::RIGHT;   // LEFT / RIGHT / TOP / BOTTOM
        UiDirection dir     = UiDirection::H;   // H (row) or V (stack)
        Size        fixed   = Size(0, 0);       // optional fixed size
        Rect        rect;                       // computed layout rect
        bool        visible = true;             // participates in layout
        bool        overlay = false;            // if true, does NOT reserve text space
    };

    Vector<SideItem> sides_;
    int              next_side_id_ = 1;
    mutable Rect     text_rect_;               // cached text rect inside sides

    struct LineMetricsCache : Moveable<LineMetricsCache> {
        Vector<int> char_widths;
        int         line_px = 0;
    };
    mutable Vector<LineMetricsCache> line_metrics_cache_;
    mutable bool    line_metrics_dirty_ = true;
    mutable int     space_width_cache_ = 1;
    mutable int     tab_width_cache_ = 4;
    mutable int     placeholder_width_cache_ = 0;
    mutable bool    placeholder_width_dirty_ = true;

    // ------------------------------------------------------------------------
    // Internal Helpers
    // ------------------------------------------------------------------------
    
    // Visual text for painting / hit testing.
    // Derived classes (like UiPasswordEdit) override this to mask text.
    virtual WString GetDisplayLine(int i) const { return GetWLine(i); }
    
    int          GetVisualLineHeight() const;
    void         InvalidateStyleCache();
    Style&       StyleEdit();
    void         SyncThemeStyle();
    const Style& GetEffectiveStyle() const;
    Size         GetFontCellSize() const { return font_size_; }

    // Core Data Ops
    void    Insert0(int64 pos, const WString& text);
    void    Remove0(int64 pos, int64 size);
    int64   InsertU(int64 pos, const WString& text, bool typing);
    void    RemoveU(int64 pos, int64 size);
    void    Undodo();
    void    NextUndo();
    void    IncDirty();
    void    DecDirty();
 
    // Core View/Layout Ops
    void    UpdateVisualState();
    void    SyncFont();
    void    SyncSb();
    void    Scroll();
    void    PlaceCaret(int64 new_cursor, bool sel);
    void    PlaceCaret();
    Rect    GetCaretRect(int64 pos) const;
    void    ScrollToCaret();
    Point   GetScrollPos() const { return sb_; }
    int64   GetMousePos(Point p) const;
    Point   GetColumnLine(int64 pos) const; // (col, line_idx)
    int64   GetPos(int line, int col = 0) const;
    int64   GetPos(int line) const;
    int     GetLine(int64 pos) const;
    
    // Build full logical text (lines + '\n' between them).
    WString BuildFullText() const;
    
    void    LayoutSides();          // compute side rects + text_rect_
    SideItem* FindSideById(int id);
    void    InvalidateTextMetricsCache();
    void    EnsureTextMetricsCache() const;
    
    // Line Accessors
    int           GetLineCount() const           { return lin_.GetCount(); }
    const String& GetUtf8Line(int i) const       { return lin_[i].text; }
    WString       GetWLine(int i) const          { return ToUtf32(GetUtf8Line(i)); }
    int           GetLineLength(int i) const     { return lin_[i].len; }
    void          SetLine(int i, const WString& w);
    void          LineInsert(int i, int n)       { lin_.InsertN(i, n); }
    void          LineRemove(int i, int n)       { lin_.Remove(i, n); }

    WString GetW(int64 pos, int64 size) const;
    int     GetChar(int64 pos) const;
    
    // Paint Helpers
    void    PaintLine(Draw& w, int i, int x, int y, const Rect& clip) const;
    Point   GetContentArea() const;
    Rect    GetViewRect() const;   // inner area after frame + padding
    Rect    GetTextRect() const;   // inner area after sides
    int     GetSingleLineYOffset() const; // vertical centering for single-line mode

public:
    UiBaseEdit();
    virtual ~UiBaseEdit() {}

    // ------------------------------------------------------------------------
    // Configuration & Behavior
    // ------------------------------------------------------------------------
    
    UiBaseEdit& SetPlaceholder(const String& s) {
        placeholder_text_ = s;
        Font fnt = GetEffectiveStyle().font;
        if(IsNull(fnt))
            fnt = StdFont();
        placeholder_width_cache_ = placeholder_text_.IsEmpty() ? 0 : GetTextSize(placeholder_text_, fnt).cx;
        placeholder_width_dirty_ = false;
        Refresh();
        return *this;
    }
    
    UiBaseEdit& SetAcceptsNewlines(bool b) { accepts_newlines_ = b; return *this; }
    bool        AcceptsNewlines() const    { return accepts_newlines_; }
    
    UiBaseEdit& SetAcceptsTabs(bool b)     { accepts_tabs_ = b; return *this; }
    bool        AcceptsTabs() const        { return accepts_tabs_; }
    
    UiBaseEdit& SetAcceptsDrop(bool b)     { accepts_drop_ = b; return *this; }
    bool        AcceptsDrop() const        { return accepts_drop_; }
    
    UiBaseEdit& SetOverwriteMode(bool b)   { overwrite_ = b; PlaceCaret(); return *this; }
    bool        IsOverwriteMode() const    { return overwrite_; }
    
    UiBaseEdit& ClickFocus(bool b = true)  { click_focus_ = b; return *this; }
    
    void        SetTip(const String& tip)  { Ctrl::Tip(tip); }

    // ------------------------------------------------------------------------
    // Layout & Alignment
    // ------------------------------------------------------------------------
    
    UiBaseEdit& SetTextAlign(UiAlign a) { StyleEdit().text_align = a; OnStyleChanged(); return *this; }
    UiAlign     GetTextAlign() const    { return GetEffectiveStyle().text_align; }

    // ------------------------------------------------------------------------
    // Side Controls (left/right/top/bottom)
    // ------------------------------------------------------------------------
    //
    //  SideHandle AddToSide(Ctrl& c,
    //                       UiAlign side,
    //                       Size fixed      = Size(0,0),
    //                       UiDirection dir = UiDirection::H)
    //
    //  - side: LEFT/RIGHT/TOP/BOTTOM attachment.
    //  - fixed: optional fixed size; if (0,0), size comes from child minsize
    //           and/or inferred from band height.
    //  - dir:
    //      * for LEFT/RIGHT:
    //          H = horizontal row (left/right, same vertical band as text)
    //          V = vertical stack (left/right, fills band)
    //      * for TOP/BOTTOM:
    //          H = horizontal row at that edge
    //          V = vertical stack at that edge
    //
    //  Returns a SideHandle that can be used to configure visibility/overlay,
    //  and can safely be stored long-term (check IsValid()).
    //
    SideHandle AddToSide(Ctrl& c,
                         UiAlign side,
                         Size fixed      = Size(0, 0),
                         UiDirection dir = UiDirection::H);

    void RemoveSide(int id);
    void ClearSides();

    void SetSideVisible(int id, bool vis);
    void SetSideOverlay(int id, bool overlay);
    void SetSideFixedSize(int id, Size sz);
    void SetSideDirection(int id, UiDirection dir);

    // Lookup helpers
    SideHandle GetSideHandle(Ctrl& c) const; // invalid handle if not found
    int        GetSideId(Ctrl& c) const;     // -1 if not found
    bool       HasSide(int id) const;

    // ------------------------------------------------------------------------
    // Styling Interface (CtrlStyled)
    // ------------------------------------------------------------------------
    
    UiBaseEdit&     SetStyle(const Style& s);
    UiBaseEdit&     ClearStyleOverride();
    bool            HasStyleOverride() const { return has_style_override_; }
    const Style&    GetStyle() const { return GetEffectiveStyle(); }
    
    StyledPalette&  StyledPaletteRef()  { return StyleEdit().palette; }
    StyledMetrics&  StyledMetricsRef()  { return StyleEdit().metrics; }
    StyledSkin&     StyledSkinRef()     { return StyleEdit().skin;    }
    void            OnStyleChanged();

    // ------------------------------------------------------------------------
    // Data Access
    // ------------------------------------------------------------------------
    
    virtual void  SetData(const Value& v) override;
    virtual Value GetData() const override;
    
    void    SetText(const WString& s);
    WString GetText() const;
    // Explicit UTF-8 text API for app-level string handling.
    // Prefer these over GetData().ToString() for textual controls.
    UiBaseEdit& SetTextUtf8(const String& s) { SetText(ToUtf32(s)); return *this; }
    String      GetTextUtf8() const          { return ToUtf8(GetText()); }
    void    Clear();
    bool    IsEmpty() const { return total_wchars_ == 0; }
    bool    IsDirty() const { return dirty_; }
    void    ClearDirty()    { dirty_ = 0; }

    // ------------------------------------------------------------------------
    // Selection & Caret
    // ------------------------------------------------------------------------
    
    void    SetSelection(int64 l = 0, int64 h = INT_MAX);
    bool    GetSelection(int64& l, int64& h) const;
    WString GetSelectionW() const;
    void    ClearSelection();
    bool    RemoveSelection();
    bool    IsSelection() const { return anchor_ >= 0 && anchor_ != cursor_; }

    int64   GetCursor() const { return cursor_; }
    void    SetCursor(int64 c) { PlaceCaret(c, false); }
    
    // ------------------------------------------------------------------------
    // Undo / Redo & Clipboard
    // ------------------------------------------------------------------------
    
    void    Undo();
    void    Redo();
    bool    IsUndo() const { return undo_.GetCount(); }
    bool    IsRedo() const { return redo_.GetCount(); }

    void    Cut();
    void    Copy();
    void    Paste();
    void    SelectAll();

    // ------------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------------
    
    // Paint hooks - mirror UiLabel / UiButton signatures
    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintBackground;

    Event<Draw&, const Rect&,
          const StyledPalette&, const StyledMetrics&, const StyledSkin&,
          StyledState, bool> WhenPaintForeground;

    Event<>   WhenAction;    // Fired on Enter (for LineEdit) or special event
    Event<>   WhenChange;    // Fired on any text modification
    Event<>   WhenSelection;   // Fired on selection change
    Event<>   WhenScroll;
    Event<Bar&> WhenBar;     // Context menu

    // ------------------------------------------------------------------------
    // Ctrl Overrides
    // ------------------------------------------------------------------------
    
    virtual void  Paint(Draw& w) override;
    virtual bool  Key(dword key, int count) override;
    virtual void  LeftDown(Point p, dword flags) override;
    virtual void  LeftUp(Point p, dword flags) override;
    virtual void  LeftDrag(Point p, dword flags) override;
    virtual void  LeftDouble(Point p, dword flags) override;
    virtual void  LeftTriple(Point p, dword flags) override;
    virtual void  RightDown(Point p, dword flags) override;
    virtual void  MouseMove(Point p, dword flags) override;
    virtual void  MouseWheel(Point p, int zdelta, dword flags) override;
    virtual void  HorzMouseWheel(Point p, int zdelta, dword flags) override;
    virtual Image CursorImage(Point p, dword flags) override;
    virtual void  GotFocus() override;
    virtual void  LostFocus() override;
    virtual void  MouseEnter(Point p, dword flags) override;
    virtual void  MouseLeave() override;
    virtual void  Layout() override;
    virtual void  CancelMode() override;
    virtual Size  GetMinSize() const override;
    virtual Rect  GetCaret() const override; // For IME

    // Drag & Drop
    virtual void  DragAndDrop(Point p, PasteClip& d) override;
    virtual void  DragRepeat(Point p) override;
    virtual void  DragLeave() override;
    virtual void  Drop(Point p, PasteClip& d);  // not 'override': Ctrl has Drop(PasteClip&) signature
};

} // namespace Upp

#endif
