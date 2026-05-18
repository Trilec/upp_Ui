/*
    UiLabelDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
#include "../BuilderDemoSupport.h"

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

struct RichSpanConfig : Moveable<RichSpanConfig> {
    String text;
    Color ink = Null;
    bool bold = false;
    bool italic = false;
    bool underline = false;
};

struct LabelConfig {
    String text = "System status";
    UiAlign align_h = UiAlign::LEFT;
    UiAlign align_v = UiAlign::CENTER;
    UiAlign icon_side = UiAlign::LEFT;
    int content_gap = DPI(8);
    int icon_size = DPI(18);
    int margin_x = DPI(10);
    int margin_y = DPI(6);
    int radius = DPI(8);
    int frame_width = 1;
    bool body_face = true;
    bool body_frame = true;
    bool selectable = true;
    bool underline = false;
    bool show_icon = true;
    bool rich_mode = false;
};

class UiLabelBuilder : public BuilderWindowBase {
public:
    typedef UiLabelBuilder CLASSNAME;

    UiLabelBuilder()
        : BuilderWindowBase("UiLabelDemo", "U++ UiLabel Builder", "Inspect plain and rich label content, icon layout, alignment, and partial text styling from one shell.")
    {
        Preview().Add(label_);

        AddStateRow(StateBox(), state_theme_row_, state_theme_label_, state_theme_value_, "Theme");
        AddStateRow(StateBox(), state_mode_row_, state_mode_label_, state_mode_value_, "Mode");
        AddStateRow(StateBox(), state_text_row_, state_text_label_, state_text_value_, "Text");
        AddStateRow(StateBox(), state_span_row_, state_span_label_, state_span_value_, "Spans");
        AddStateRow(StateBox(), state_surface_row_, state_surface_label_, state_surface_value_, "Surface");

        AddEditRow(PropsBox(), text_row_box_, text_label_, text_edit_, "Text");
        AddDropdownRow(PropsBox(), align_h_row_box_, align_h_label_, align_h_drop_, "Align H");
        AddDropdownRow(PropsBox(), align_v_row_box_, align_v_label_, align_v_drop_, "Align V");
        AddDropdownRow(PropsBox(), icon_side_row_box_, icon_side_label_, icon_side_drop_, "Icon Side");
        AddSliderRow(PropsBox(), gap_row_, "Content Gap", "8px");
        AddSliderRow(PropsBox(), icon_size_row_, "Icon Sz", "18px");
        AddSliderRow(PropsBox(), margin_x_row_, "Margin X", "10px");
        AddSliderRow(PropsBox(), margin_y_row_, "Margin Y", "6px");
        AddSliderRow(PropsBox(), radius_row_, "Radius", "8px");
        AddSliderRow(PropsBox(), frame_width_row_, "Frame W", "1px");
        AddToggleRow(PropsBox(), rich_mode_row_, "Rich Mode");
        AddToggleRow(PropsBox(), face_row_, "Body Face");
        AddToggleRow(PropsBox(), frame_row_, "Body Frame");
        AddToggleRow(PropsBox(), selectable_row_, "Selectable");
        AddToggleRow(PropsBox(), underline_row_, "Underline");
        AddToggleRow(PropsBox(), icon_row_, "Show Icon");

        AddEditRow(PropsBox(), span_text_row_box_, span_text_label_, span_text_edit_, "Span Text");
        AddToggleRow(PropsBox(), span_bold_row_, "Span Bold");
        AddToggleRow(PropsBox(), span_italic_row_, "Span Italic");
        AddToggleRow(PropsBox(), span_underline_row_, "Span Underline");
        AddToggleRow(PropsBox(), span_color_enabled_row_, "Span Ink");
        AddColorRow(PropsBox(), span_color_row_, "Ink Color");

        span_actions_row_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        span_actions_row_.Add(add_span_button_).Expand(1).MinHeight(DPI(28));
        span_actions_row_.Add(update_span_button_).Expand(1).MinHeight(DPI(28));
        span_actions_row_.Add(newline_button_).Expand(1).MinHeight(DPI(28));
        span_actions_row_.Add(delete_span_button_).Expand(1).MinHeight(DPI(28));
        span_actions_row_.Add(clear_spans_button_).Expand(1).MinHeight(DPI(28));
        PropsBox().Add(span_actions_row_).Fit();

        span_list_.NoHeader();
        span_list_.AddColumn("Span");
        span_list_.AddColumn("Style");
        span_list_.SetLineCy(DPI(22));
        span_list_.SetMinSize(Size(DPI(240), DPI(120)));
        PropsBox().Add(span_list_).Fit();

        const EnumOption aligns_h[] = {
            { "Left", (int)UiAlign::LEFT }, { "Center", (int)UiAlign::CENTER }, { "Right", (int)UiAlign::RIGHT }
        };
        const EnumOption aligns_v[] = {
            { "Top", (int)UiAlign::TOP }, { "Center", (int)UiAlign::CENTER }, { "Bottom", (int)UiAlign::BOTTOM }
        };
        const EnumOption sides[] = {
            { "Left", (int)UiAlign::LEFT }, { "Right", (int)UiAlign::RIGHT }, { "Top", (int)UiAlign::TOP }, { "Bottom", (int)UiAlign::BOTTOM }
        };

        PopulateDropdown(align_h_drop_, aligns_h, 3);
        PopulateDropdown(align_v_drop_, aligns_v, 3);
        PopulateDropdown(icon_side_drop_, sides, 4);

        text_edit_.SetData(cfg_.text);
        span_text_edit_.SetData(String("LIVE"));
        gap_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(cfg_.content_gap);
        icon_size_row_.Slider().SetRange(DPI(8), DPI(48)).SetStep(1).SetValue(cfg_.icon_size);
        margin_x_row_.Slider().SetRange(0, DPI(24)).SetStep(1).SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetRange(0, DPI(18)).SetStep(1).SetValue(cfg_.margin_y);
        radius_row_.Slider().SetRange(0, DPI(20)).SetStep(1).SetValue(cfg_.radius);
        frame_width_row_.Slider().SetRange(0, 4).SetStep(1).SetValue(cfg_.frame_width);
        span_color_row_.SetLabel("Ink Color").SetColorCount(1).ShowValue(false);
        span_color_row_.SetColor(0, Color(196, 59, 59));

        text_edit_.WhenAction = [=] { cfg_.text = text_edit_.GetText().ToString(); RefreshFromConfig(); };
        text_edit_.WhenChange = [=] { cfg_.text = text_edit_.GetText().ToString(); RefreshFromConfig(); };
        align_h_drop_.WhenSelect = [=](int) { cfg_.align_h = (UiAlign)(int)align_h_drop_.GetSelectedData(); RefreshFromConfig(); };
        align_v_drop_.WhenSelect = [=](int) { cfg_.align_v = (UiAlign)(int)align_v_drop_.GetSelectedData(); RefreshFromConfig(); };
        icon_side_drop_.WhenSelect = [=](int) { cfg_.icon_side = (UiAlign)(int)icon_side_drop_.GetSelectedData(); RefreshFromConfig(); };
        gap_row_.WhenAction = [=] { cfg_.content_gap = (int)gap_row_.Slider().GetValue(); RefreshFromConfig(); };
        icon_size_row_.WhenAction = [=] { cfg_.icon_size = (int)icon_size_row_.Slider().GetValue(); RefreshFromConfig(); };
        margin_x_row_.WhenAction = [=] { cfg_.margin_x = (int)margin_x_row_.Slider().GetValue(); RefreshFromConfig(); };
        margin_y_row_.WhenAction = [=] { cfg_.margin_y = (int)margin_y_row_.Slider().GetValue(); RefreshFromConfig(); };
        radius_row_.WhenAction = [=] { cfg_.radius = (int)radius_row_.Slider().GetValue(); RefreshFromConfig(); };
        frame_width_row_.WhenAction = [=] { cfg_.frame_width = (int)frame_width_row_.Slider().GetValue(); RefreshFromConfig(); };
        rich_mode_row_.Toggle().WhenAction = [=] { cfg_.rich_mode = rich_mode_row_.Toggle().IsOn(); RefreshFromConfig(); };
        face_row_.Toggle().WhenAction = [=] { cfg_.body_face = face_row_.Toggle().IsOn(); RefreshFromConfig(); };
        frame_row_.Toggle().WhenAction = [=] { cfg_.body_frame = frame_row_.Toggle().IsOn(); RefreshFromConfig(); };
        selectable_row_.Toggle().WhenAction = [=] { cfg_.selectable = selectable_row_.Toggle().IsOn(); RefreshFromConfig(); };
        underline_row_.Toggle().WhenAction = [=] { cfg_.underline = underline_row_.Toggle().IsOn(); RefreshFromConfig(); };
        icon_row_.Toggle().WhenAction = [=] { cfg_.show_icon = icon_row_.Toggle().IsOn(); RefreshFromConfig(); };
        span_bold_row_.Toggle().WhenAction = [=] { SyncSelectedSpanFromEditor(false); };
        span_italic_row_.Toggle().WhenAction = [=] { SyncSelectedSpanFromEditor(false); };
        span_underline_row_.Toggle().WhenAction = [=] { SyncSelectedSpanFromEditor(false); };
        span_color_enabled_row_.Toggle().WhenAction = [=] { SyncSelectedSpanFromEditor(false); };
        span_color_row_.WhenAction = [=] { SyncSelectedSpanFromEditor(false); };
        span_text_edit_.WhenAction = [=] { SyncSelectedSpanFromEditor(false); };
        span_text_edit_.WhenChange = [=] { SyncSelectedSpanFromEditor(false); };
        add_span_button_.WhenAction = [=] { AddSpanFromEditor(); };
        update_span_button_.WhenAction = [=] { SyncSelectedSpanFromEditor(true); };
        newline_button_.WhenAction = [=] { AddNewlineSpan(); };
        delete_span_button_.WhenAction = [=] { DeleteSelectedSpan(); };
        clear_spans_button_.WhenAction = [=] { spans_.Clear(); selected_span_ = -1; RefreshFromConfig(); };
        span_list_.WhenSel = [=] { selected_span_ = span_list_.GetCursor(); LoadSelectedSpanToEditor(); };

        add_span_button_.SetText("Add Span");
        update_span_button_.SetText("Update");
        newline_button_.SetText("New Line");
        delete_span_button_.SetText("Delete");
        clear_spans_button_.SetText("Clear");

        BuildDefaultRichSpans();
        label_.NoWantFocus();
        FinishInit();
        RefreshFromConfig();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        UiLabel::Style body = MakeBodyLabelStyle(Palette());
        UiLabel::Style value = MakeValueLabelStyle(Palette());
        UiBaseEdit::Style edit = MakeEditStyle(Palette());
        UiDropdown::Style dd = MakeDropdownStyle(Palette());
        UiButton::Style btn = MakeSmallButtonStyle(Palette());

        state_theme_label_.SetCustomStyle(body); state_theme_value_.SetCustomStyle(value);
        state_mode_label_.SetCustomStyle(body); state_mode_value_.SetCustomStyle(value);
        state_text_label_.SetCustomStyle(body); state_text_value_.SetCustomStyle(value);
        state_span_label_.SetCustomStyle(body); state_span_value_.SetCustomStyle(value);
        state_surface_label_.SetCustomStyle(body); state_surface_value_.SetCustomStyle(value);
        text_label_.SetCustomStyle(body); align_h_label_.SetCustomStyle(body); align_v_label_.SetCustomStyle(body); icon_side_label_.SetCustomStyle(body);
        span_text_label_.SetCustomStyle(body);
        text_edit_.SetCustomStyle(edit);
        span_text_edit_.SetCustomStyle(edit);
        align_h_drop_.SetCustomStyle(dd); align_v_drop_.SetCustomStyle(dd); icon_side_drop_.SetCustomStyle(dd);
        gap_row_.SetLabelStyle(body).SetValueStyle(value);
        icon_size_row_.SetLabelStyle(body).SetValueStyle(value);
        margin_x_row_.SetLabelStyle(body).SetValueStyle(value);
        margin_y_row_.SetLabelStyle(body).SetValueStyle(value);
        radius_row_.SetLabelStyle(body).SetValueStyle(value);
        frame_width_row_.SetLabelStyle(body).SetValueStyle(value);
        rich_mode_row_.SetLabelStyle(body);
        face_row_.SetLabelStyle(body);
        frame_row_.SetLabelStyle(body);
        selectable_row_.SetLabelStyle(body);
        underline_row_.SetLabelStyle(body);
        icon_row_.SetLabelStyle(body);
        span_bold_row_.SetLabelStyle(body);
        span_italic_row_.SetLabelStyle(body);
        span_underline_row_.SetLabelStyle(body);
        span_color_enabled_row_.SetLabelStyle(body);
        span_color_row_.SetLabelStyle(body);
        add_span_button_.SetCustomStyle(btn);
        update_span_button_.SetCustomStyle(btn);
        newline_button_.SetCustomStyle(btn);
        delete_span_button_.SetCustomStyle(btn);
        clear_spans_button_.SetCustomStyle(btn);
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        Size sz = label_.GetMinSize();
        sz.cx = min(sz.cx + DPI(32), canvas.GetWidth());
        sz.cy = min(sz.cy + DPI(18), canvas.GetHeight());
        int x = canvas.left + (canvas.GetWidth() - sz.cx) / 2;
        int y = canvas.top + (canvas.GetHeight() - sz.cy) / 2;
        label_.SetRect(x, y, sz.cx, sz.cy);
    }

private:
    struct EnumOption { const char* label; int value; };

    void AddColorRow(UiBoxLayout& target, UiCompositeColor& row, const char* name)
    {
        row.SetLabel(name).SetColorCount(1).ShowValue(false);
        target.Add(row).Fit();
    }

    void PopulateDropdown(UiDropdown& drop, const EnumOption* opts, int count)
    {
        drop.UseInternalModel();
        drop.Clear();
        for(int i = 0; i < count; i++)
            drop.Add(opts[i].label, opts[i].value);
    }

    void BuildDefaultRichSpans()
    {
        spans_.Clear();
        RichSpanConfig a; a.text = "System"; a.ink = Color(44, 99, 212); a.bold = true; spans_.Add(a);
        RichSpanConfig b; b.text = " status "; spans_.Add(b);
        RichSpanConfig c; c.text = "LIVE"; c.ink = Color(201, 55, 72); c.bold = true; spans_.Add(c);
    }

    RichSpanConfig SpanFromEditor() const
    {
        RichSpanConfig s;
        s.text = span_text_edit_.GetText().ToString();
        s.bold = span_bold_row_.Toggle().IsOn();
        s.italic = span_italic_row_.Toggle().IsOn();
        s.underline = span_underline_row_.Toggle().IsOn();
        s.ink = span_color_enabled_row_.Toggle().IsOn() ? span_color_row_.GetColor(0) : Null;
        return s;
    }

    void LoadSelectedSpanToEditor()
    {
        if(selected_span_ < 0 || selected_span_ >= spans_.GetCount())
            return;
        const RichSpanConfig& s = spans_[selected_span_];
        span_text_edit_.SetData(s.text == "\n" ? String() : s.text);
        span_bold_row_.Toggle().SetOn(s.bold);
        span_italic_row_.Toggle().SetOn(s.italic);
        span_underline_row_.Toggle().SetOn(s.underline);
        span_color_enabled_row_.Toggle().SetOn(!IsNull(s.ink));
        span_color_row_.SetColor(0, IsNull(s.ink) ? Palette().blue : s.ink);
    }

    void AddSpanFromEditor()
    {
        RichSpanConfig s = SpanFromEditor();
        if(s.text.IsEmpty())
            return;
        spans_.Add(s);
        selected_span_ = spans_.GetCount() - 1;
        RefreshFromConfig();
    }

    void AddNewlineSpan()
    {
        RichSpanConfig s;
        s.text = "\n";
        spans_.Add(s);
        selected_span_ = spans_.GetCount() - 1;
        RefreshFromConfig();
    }

    void DeleteSelectedSpan()
    {
        if(selected_span_ < 0 || selected_span_ >= spans_.GetCount())
            return;
        spans_.Remove(selected_span_);
        if(selected_span_ >= spans_.GetCount())
            selected_span_ = spans_.GetCount() - 1;
        RefreshFromConfig();
    }

    void SyncSelectedSpanFromEditor(bool commit)
    {
        if(selected_span_ < 0 || selected_span_ >= spans_.GetCount())
            return;
        spans_[selected_span_] = SpanFromEditor();
        if(commit)
            RefreshFromConfig();
    }

    String BuildRichPlainText() const
    {
        String out;
        for(const RichSpanConfig& s : spans_)
            out << s.text;
        return out;
    }

    String SpanStyleCode(const RichSpanConfig& s) const
    {
        String out;
        if(!IsNull(s.ink)) out << "ink ";
        if(s.bold) out << "bold ";
        if(s.italic) out << "italic ";
        if(s.underline) out << "underline ";
        return out.IsEmpty() ? String("plain") : TrimBoth(out);
    }

    void RefreshSpanList()
    {
        span_list_.Clear();
        for(int i = 0; i < spans_.GetCount(); i++) {
            const RichSpanConfig& s = spans_[i];
            String label = s.text == "\n" ? String("<newline>") : s.text;
            span_list_.Add(label, SpanStyleCode(s));
        }
        if(selected_span_ >= 0 && selected_span_ < span_list_.GetCount())
            span_list_.SetCursor(selected_span_);
    }

    void ApplyRichContent()
    {
        label_.ClearSpans().EnableRich(true);
        for(const RichSpanConfig& s : spans_) {
            if(s.text == "\n")
                label_.AddNewlineSpan();
            else
                label_.AddTextSpan(s.text, s.ink, s.bold, s.italic, s.underline);
        }
    }

    void RefreshFromConfig()
    {
        UiLabel::Style style = UiTheme::ResolveLabel(UiLabelRole::Body);
        style.align_h = cfg_.align_h;
        style.align_v = cfg_.align_v;
        style.metrics.radius = cfg_.radius;
        style.metrics.frame_width = cfg_.frame_width;
        style.metrics.face_enabled = cfg_.body_face;
        style.metrics.frame_enabled = cfg_.body_frame;
        style.metrics.content_margin = Rect(cfg_.margin_x, cfg_.margin_y, cfg_.margin_x, cfg_.margin_y);
        style.content_gap = cfg_.content_gap;
        for(int i = 0; i < 4; ++i) {
            style.palette.face[i] = UiFill::Solid(Palette().segment_face);
            style.palette.frame[i] = Palette().segment_frame;
        }

        label_.SetCustomStyle(style)
              .SetIcon(cfg_.show_icon ? ICON_ACTION_SEARCH_48() : Image())
              .SetIconRenderMode(UiIconRenderMode::PreserveColor)
              .SetIconSide(cfg_.icon_side)
              .SetIconSize(cfg_.icon_size, cfg_.icon_size)
              .SetSelectable(cfg_.selectable)
              .SetUnderline(cfg_.underline);

        if(cfg_.rich_mode)
            ApplyRichContent();
        else
            label_.EnableRich(false).SetText(cfg_.text);

        align_h_drop_.SelectByData((int)cfg_.align_h);
        align_v_drop_.SelectByData((int)cfg_.align_v);
        icon_side_drop_.SelectByData((int)cfg_.icon_side);
        text_edit_.SetData(cfg_.text);
        gap_row_.Slider().SetValue(cfg_.content_gap);
        icon_size_row_.Slider().SetValue(cfg_.icon_size);
        margin_x_row_.Slider().SetValue(cfg_.margin_x);
        margin_y_row_.Slider().SetValue(cfg_.margin_y);
        radius_row_.Slider().SetValue(cfg_.radius);
        frame_width_row_.Slider().SetValue(cfg_.frame_width);
        rich_mode_row_.Toggle().SetOn(cfg_.rich_mode);
        face_row_.Toggle().SetOn(cfg_.body_face);
        frame_row_.Toggle().SetOn(cfg_.body_frame);
        selectable_row_.Toggle().SetOn(cfg_.selectable);
        underline_row_.Toggle().SetOn(cfg_.underline);
        icon_row_.Toggle().SetOn(cfg_.show_icon);

        gap_row_.SetValueText(AsString(cfg_.content_gap) + "px");
        icon_size_row_.SetValueText(AsString(cfg_.icon_size) + "px");
        margin_x_row_.SetValueText(AsString(cfg_.margin_x) + "px");
        margin_y_row_.SetValueText(AsString(cfg_.margin_y) + "px");
        radius_row_.SetValueText(AsString(cfg_.radius) + "px");
        frame_width_row_.SetValueText(AsString(cfg_.frame_width) + "px");

        RefreshSpanList();
        if(selected_span_ >= 0 && selected_span_ < spans_.GetCount())
            LoadSelectedSpanToEditor();

        state_theme_value_.SetText(Palette().dark ? "Dark" : "Light");
        state_mode_value_.SetText(cfg_.rich_mode ? "Rich" : "Plain");
        state_text_value_.SetText(cfg_.rich_mode ? BuildRichPlainText() : cfg_.text);
        state_span_value_.SetText(AsString(spans_.GetCount()));
        state_surface_value_.SetText(AsString(cfg_.body_face ? "Face" : "No Face") + " / " + AsString(cfg_.body_frame ? "Frame" : "No Frame"));

        SetUsageCode(BuildUsageCode());
        Refresh();
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiLabel label;\n";
        code << "UiLabel::Style style = UiTheme::ResolveLabel(UiLabelRole::Body);\n";
        code << "style.align_h = UiAlign::" << (cfg_.align_h == UiAlign::CENTER ? "CENTER" : cfg_.align_h == UiAlign::RIGHT ? "RIGHT" : "LEFT") << ";\n";
        code << "style.align_v = UiAlign::" << (cfg_.align_v == UiAlign::TOP ? "TOP" : cfg_.align_v == UiAlign::BOTTOM ? "BOTTOM" : "CENTER") << ";\n";
        code << "style.metrics.content_margin = Rect(" << cfg_.margin_x << ", " << cfg_.margin_y << ", " << cfg_.margin_x << ", " << cfg_.margin_y << ");\n";
        code << "style.metrics.radius = " << cfg_.radius << ";\n";
        code << "style.metrics.frame_width = " << cfg_.frame_width << ";\n";
        code << "style.metrics.face_enabled = " << (cfg_.body_face ? "true" : "false") << ";\n";
        code << "style.metrics.frame_enabled = " << (cfg_.body_frame ? "true" : "false") << ";\n";
        code << "style.content_gap = " << cfg_.content_gap << ";\n";
        code << "label.SetCustomStyle(style)\n";
        if(cfg_.show_icon)
            code << "     .SetIcon(ICON_ACTION_SEARCH_48())\n";
        code << "     .SetIconSide(UiAlign::" << (cfg_.icon_side == UiAlign::RIGHT ? "RIGHT" : cfg_.icon_side == UiAlign::TOP ? "TOP" : cfg_.icon_side == UiAlign::BOTTOM ? "BOTTOM" : "LEFT") << ")\n";
        code << "     .SetIconSize(" << cfg_.icon_size << ", " << cfg_.icon_size << ")\n";
        code << "     .SetSelectable(" << (cfg_.selectable ? "true" : "false") << ")";
        if(cfg_.rich_mode) {
            code << "\n     .EnableRich(true);\n";
            for(const RichSpanConfig& s : spans_) {
                if(s.text == "\n")
                    code << "label.AddNewlineSpan();\n";
                else {
                    code << "label.AddTextSpan(" << QuoteCpp(s.text) << ", ";
                    if(IsNull(s.ink))
                        code << "Null";
                    else
                        code << "Color(" << s.ink.GetR() << ", " << s.ink.GetG() << ", " << s.ink.GetB() << ")";
                    code << ", " << (s.bold ? "true" : "false") << ", " << (s.italic ? "true" : "false") << ", " << (s.underline ? "true" : "false") << ");\n";
                }
            }
        }
        else {
            code << "\n     .SetText(" << QuoteCpp(cfg_.text) << ")";
            if(cfg_.underline)
                code << "\n     .SetUnderline(true);\n";
            else
                code << ";\n";
        }
        return code;
    }

    LabelConfig cfg_;
    Vector<RichSpanConfig> spans_;
    int selected_span_ = -1;
    UiLabel label_;

    UiBoxLayout state_theme_row_ { UiBoxLayout::Direction::H }, state_mode_row_ { UiBoxLayout::Direction::H }, state_text_row_ { UiBoxLayout::Direction::H }, state_span_row_ { UiBoxLayout::Direction::H }, state_surface_row_ { UiBoxLayout::Direction::H };
    UiLabel state_theme_label_, state_theme_value_, state_mode_label_, state_mode_value_, state_text_label_, state_text_value_, state_span_label_, state_span_value_, state_surface_label_, state_surface_value_;

    UiBoxLayout text_row_box_ { UiBoxLayout::Direction::H }, align_h_row_box_ { UiBoxLayout::Direction::H }, align_v_row_box_ { UiBoxLayout::Direction::H }, icon_side_row_box_ { UiBoxLayout::Direction::H }, span_text_row_box_ { UiBoxLayout::Direction::H }, span_actions_row_ { UiBoxLayout::Direction::H };
    UiLabel text_label_, align_h_label_, align_v_label_, icon_side_label_, span_text_label_;
    UiLineEdit text_edit_, span_text_edit_;
    UiDropdown align_h_drop_, align_v_drop_, icon_side_drop_;
    UiCompositeSlider gap_row_, icon_size_row_, margin_x_row_, margin_y_row_, radius_row_, frame_width_row_;
    UiCompositeToggle rich_mode_row_, face_row_, frame_row_, selectable_row_, underline_row_, icon_row_, span_bold_row_, span_italic_row_, span_underline_row_, span_color_enabled_row_;
    UiCompositeColor span_color_row_;
    UiButton add_span_button_, update_span_button_, newline_button_, delete_span_button_, clear_spans_button_;
    ArrayCtrl span_list_;
};

}

GUI_APP_MAIN
{
    UiLabelBuilder demo;
    demo.Run();
}