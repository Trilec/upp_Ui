#include "PropertyEditor.h"
#include <Ui/UiIcons.h>

namespace Upp {

static String PeFormatMultilineSummaryPaint(const Value& value)
{
    String s = AsString(value);
    s.Replace("\r", "");
    int line_count = 1;
    for(int i = 0; i < s.GetCount(); i++)
        if(s[i] == '\n')
            line_count++;
    String first = s;
    int nl = first.Find('\n');
    if(nl >= 0)
        first = first.Left(nl);
    first = TrimBoth(first);
    if(first.IsEmpty())
        first = "<empty>";
    if(line_count > 1)
        return Format("%s (%d lines)", first, line_count);
    return first;
}

void PropertyEditor::DrawGroupRow(Draw& w, int display_index,
                                  const DisplayRow& row, const Rect& r)
{
    const Color group_background = row.group_depth > 0
        ? LtColor(style_.group_background, 10)
        : style_.group_background;
    w.DrawRect(r, group_background);
    const int padding = style_.cell_padding;
    const int indent = row.group_depth * style_.indent_width;
    const String mark = IsGroupOpen(row.group_id) ? "-" : "+";
    const Font& title_font = style_.group_font;
    const int title_y = r.top + (r.GetHeight() - title_font.GetHeight()) / 2;
    const int marker_x = r.left + padding + indent;
    const int title_left = marker_x + DPI(16);
    const String title = row.group_label.IsEmpty() ? row.group_id : row.group_label;
    const int title_right = title_left + GetTextSize(title, title_font).cx;

    w.DrawText(marker_x, title_y, mark, title_font, style_.group_ink);
    w.DrawText(title_left, title_y, title, title_font, style_.group_ink);

    int right = r.right - padding;
    String action = GetGroupAction(row.group_id);
    if(!action.IsEmpty()) {
        Rect ar = GetGroupActionRect(display_index);
        int ay = ar.top + (ar.GetHeight() - style_.group_subtitle_font.GetHeight()) / 2;
        w.DrawText(ar.left + DPI(8), ay, action,
                   style_.group_subtitle_font, style_.group_ink);
        right = ar.left - DPI(4);
    }

    if(style_.show_group_summaries && row.override_total > 0) {
            const String summary = Format("%d of %d local",
                                          row.override_local, row.override_total);
            const int summary_width = GetTextSize(summary, style_.group_subtitle_font).cx;
            if(right - summary_width > title_right + DPI(12)) {
                w.DrawText(right - summary_width,
                           r.top + (r.GetHeight() - style_.group_subtitle_font.GetHeight()) / 2,
                           summary, style_.group_subtitle_font, style_.inherited_ink);
                right -= summary_width + DPI(10);
            }
    }

    if(model_) {
        const String subtitle = model_->GetGroupSubtitle(row.group_id);
        if(!subtitle.IsEmpty()) {
            const int subtitle_left = title_right + DPI(14);
            const int available = max(0, right - subtitle_left);
            if(available >= DPI(32)) {
                const int text_width = min(available,
                    GetTextSize(subtitle, style_.group_subtitle_font).cx);
                const int subtitle_x = right - text_width;
                const int subtitle_y = r.top +
                    (r.GetHeight() - style_.group_subtitle_font.GetHeight()) / 2;
                DrawTextEllipsis(w, subtitle_x, subtitle_y,
                                 text_width, subtitle, "...",
                                 style_.group_subtitle_font, style_.inherited_ink);
            }
        }
    }

    if(style_.show_dividers)
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1,
                   1, style_.divider);
}

void PropertyEditor::DrawPropertyRow(Draw& w, int display_index,
                                     const DisplayRow& row,
                                     const PropertyEditorItem& item,
                                     const Rect& r)
{
    Color paper = style_.background;
    if(style_.alternate_rows)
        paper = (row.property_ordinal & 1) ? style_.row_odd : style_.row_even;
    if(display_index == hover_display_row_)
        paper = style_.row_hover;
    if(display_index == selected_display_row_)
        paper = style_.row_selected;
    w.DrawRect(r, paper);

    int label_cx = GetLabelColumnWidth(r);
    int divider_x = r.left + label_cx;
    Color label_ink = item.enabled ? style_.label_ink : style_.disabled_ink;
    Font font = style_.label_font;
    int text_y = r.top + (min(style_.row_height, r.GetHeight()) - font.GetHeight()) / 2;

    int indent = max(0, row.group_depth + item.indent) * style_.indent_width;
    w.DrawText(r.left + style_.cell_padding + indent,
               text_y, item.label, font, label_ink);

    Rect value_rect = GetValueRect(display_index);
    bool has_inline = FindInlineEditor(display_index) != nullptr;
    bool has_active = display_index == active_display_row_ && active_editor_;
    if(!has_inline && !has_active)
        DrawValueSummary(w, item, value_rect);

    if(display_index == color_drop_display_row_ && !value_rect.IsEmpty())
        DrawFatFrame(w, value_rect.Deflated(DPI(1)), SColorHighlight(), DPI(2));

    if(item.resettable && !item.overrideable) {
        Rect reset = GetResetRect(display_index);
        if(!style_.reset_icon.IsEmpty()) {
            const int size = min(DPI(16), reset.GetHeight() - DPI(6));
            Rect icon(reset.left + (reset.GetWidth() - size) / 2,
                     reset.top + (reset.GetHeight() - size) / 2,
                     reset.left + (reset.GetWidth() - size) / 2 + size,
                     reset.top + (reset.GetHeight() - size) / 2 + size);
            if(size > 0)
                w.DrawImage(icon, style_.reset_icon);
        }
    }

    if(item.overrideable) {
        Rect override = GetOverrideRect(display_index);
        const Image icon = item.override_active
            ? ICON_ACTION_CHECK_CIRCLE_48()
            : ICON_DESIGN_CIRCLE_48();
        const int size = min(DPI(16), override.GetHeight() - DPI(6));
        Rect icon_rect(override.left + (override.GetWidth() - size) / 2,
                       override.top + (override.GetHeight() - size) / 2,
                       override.left + (override.GetWidth() - size) / 2 + size,
                       override.top + (override.GetHeight() - size) / 2 + size);
        if(size > 0)
            w.DrawImage(icon_rect, icon);
    }

    if(!item.validation_error.IsEmpty()) {
        int x = item.resettable || item.overrideable
            ? GetOverrideRect(display_index).left - DPI(14)
            : r.right - DPI(16);
        w.DrawText(x, text_y, "!", font.Bold(), style_.error_ink);
    }

    if(style_.show_dividers) {
        w.DrawLine(divider_x, r.top, divider_x, r.bottom, 1, style_.divider);
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1,
                   1, style_.divider);
    }
}

void PropertyEditor::DrawValueSummary(Draw& w,
                                      const PropertyEditorItem& item,
                                      Rect value_rect) const
{
    const Font& font = style_.value_font;
    int line_h = min(style_.row_height, value_rect.GetHeight());
    int y = value_rect.top + (line_h - font.GetHeight()) / 2;
    Color ink = item.enabled ? style_.value_ink : style_.disabled_ink;
    if(item.mixed)
        ink = style_.mixed_ink;
    else if(item.inherited)
        ink = style_.inherited_ink;

    const auto DrawSwatch = [&](int x, Color color) {
        const int side = min(DPI(19), max(0, line_h - DPI(5)));
        Rect swatch = RectC(x,
            value_rect.top + (line_h - side) / 2,
            side, side);
        if(side > 0) {
            w.DrawRect(swatch, color);
            DrawFrame(w, swatch, style_.frame);
        }
        return side;
    };

    if(item.kind == PropertyEditorKind::Color &&
       !item.mixed && item.value.GetType() == COLOR_V) {
        Color color = Color(item.value);
        int d = DrawSwatch(value_rect.left, color);
        String hex = Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());
        w.DrawText(value_rect.left + d + DPI(7), y, hex, font, ink);
        return;
    }

    if(item.kind == PropertyEditorKind::ColorPalette &&
       !item.mixed && item.value.Is<ValueArray>()) {
        ValueArray colors = item.value;
        int x = value_rect.left;
        const int count = min(8, colors.GetCount());
        for(int i = 0; i < count; i++) {
            Color color = colors[i].Is<Color>()
                ? Color(colors[i]) : Color(128, 128, 128);
            x += DrawSwatch(x, color) + DPI(3);
        }
        return;
    }

    w.DrawText(value_rect.left, y, FormatValueSummary(item), font, ink);
}

String PropertyEditor::FormatValueSummary(const PropertyEditorItem& item) const
{
    if(!item.validation_error.IsEmpty())
        return item.validation_error;
    if(item.mixed)
        return "<multiple values>";
    switch(item.kind) {
    case PropertyEditorKind::Boolean:
        if(item.boolean_presentation == PropertyBooleanPresentation::TrueFalse)
            return (bool)item.value ? "True" : "False";
        return (bool)item.value ? "On" : "Off";
    case PropertyEditorKind::Choice:
        for(const PropertyEditorChoice& choice : item.choices)
            if(choice.value == item.value)
                return choice.label;
        return AsString(item.value);
    case PropertyEditorKind::Color:
        if(item.value.GetType() == COLOR_V) {
            Color c = Color(item.value);
            return Format("#%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
        }
        return "<none>";
    case PropertyEditorKind::FillRecipe:
        if(item.value.Is<ValueMap>()) {
            ValueMap recipe = item.value;
            const int q = recipe.Find("mode");
            return q >= 0 ? AsString(recipe.GetValue(q)) : "None";
        }
        return "None";
    case PropertyEditorKind::Multiline:
        return PeFormatMultilineSummaryPaint(item.value);
    case PropertyEditorKind::Vector2:
        return PropertyEditorFormatVector(item.value, 2, item.decimals);
    case PropertyEditorKind::Vector3:
        return PropertyEditorFormatVector(item.value, 3, item.decimals);
    case PropertyEditorKind::Curve:
        return item.editor_variant == "bezier"
            ? PropertyEditorFormatBezierCurve(item.value)
            : PropertyEditorFormatCurve(item.value);
    case PropertyEditorKind::Integer:
    case PropertyEditorKind::SliderInt:
    case PropertyEditorKind::NumericInt:
        return AsString((int)item.value) +
               (item.unit.IsEmpty() ? "" : " " + item.unit);
    case PropertyEditorKind::Double:
    case PropertyEditorKind::SliderDouble:
    case PropertyEditorKind::NumericDouble:
        return Format("%.*f", max(0, item.decimals), (double)item.value) +
               (item.unit.IsEmpty() ? "" : " " + item.unit);
    default:
        return AsString(item.value);
    }
}

void PropertyEditor::ModelStructureChanged(PropertyEditorModel *source)
{
    if(source != model_)
        return;
    if(dispatching_editor_callback_) {
        structure_refresh_pending_ = true;
        if(!structure_refresh_posted_) {
            structure_refresh_posted_ = true;
            Ptr<PropertyEditor> self = this;
            PostCallback([self] {
                if(!self)
                    return;
                self->structure_refresh_posted_ = false;
                if(!self->structure_refresh_pending_)
                    return;
                self->structure_refresh_pending_ = false;
                self->selected_display_row_ = -1;
                self->RebuildRows();
            });
        }
        return;
    }
    selected_display_row_ = -1;
    RebuildRows();
}

void PropertyEditor::ModelValueChanged(PropertyEditorModel *source,
                                       const String& id)
{
    if(source != model_)
        return;
    if(applying_editor_preview_ &&
       (id == active_property_id_ || id == inline_preview_property_id_))
        return;
    RefreshValue(id);
}

void PropertyEditor::ModelGroupMetadataChanged(PropertyEditorModel *source)
{
    if(source != model_)
        return;
    Refresh();
}

} // namespace Upp
