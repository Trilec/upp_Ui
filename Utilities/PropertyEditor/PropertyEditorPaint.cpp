#include "PropertyEditor.h"
#include <Ui/UiIcons.h>

namespace Upp {

void PropertyEditor::DrawGroupRow(Draw& w, int,
                                  const DisplayRow& row, const Rect& r)
{
    w.DrawRect(r, style_.group_background);
    int padding = style_.cell_padding;
    String mark = IsGroupOpen(row.group_id) ? "-" : "+";
    Font font = StdFont().Bold();
    int y = r.top + (r.GetHeight() - font.GetHeight()) / 2;

    w.DrawText(r.left + padding, y, mark, font, style_.group_ink);
    w.DrawText(r.left + padding + DPI(16), y, row.group_id,
               font, style_.group_ink);

    if(style_.show_group_summaries && model_) {
        int total = 0;
        int local = 0;
        for(int i = 0; i < model_->GetCount(); i++) {
            const PropertyEditorItem& item = (*model_)[i];
            if(!item.overrideable || item.group != row.group_id)
                continue;
            total++;
            if(item.override_active)
                local++;
        }
        if(total > 0) {
            const String summary = Format("%d of %d local", local, total);
            const int summary_width = GetTextSize(summary, StdFont()).cx;
            w.DrawText(r.right - summary_width - padding, y, summary,
                       StdFont(), style_.inherited_ink);
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
    Font font = StdFont();
    int text_y = r.top + (r.GetHeight() - font.GetHeight()) / 2;
    int indent = max(0, item.indent) * style_.indent_width;
    w.DrawText(r.left + style_.cell_padding + indent,
               text_y, item.label, font, label_ink);

    Rect value_rect = GetValueRect(display_index);
    if(!FindInlineEditor(display_index) &&
       (display_index != active_display_row_ || !active_editor_))
        DrawValueSummary(w, item, value_rect);

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
        w.DrawLine(divider_x, r.top, divider_x, r.bottom,
                   1, style_.divider);
        w.DrawLine(r.left, r.bottom - 1, r.right, r.bottom - 1,
                   1, style_.divider);
    }
}

void PropertyEditor::DrawValueSummary(Draw& w,
                                      const PropertyEditorItem& item,
                                      Rect value_rect) const
{
    Font font = StdFont();
    int y = value_rect.top + (value_rect.GetHeight() - font.GetHeight()) / 2;
    Color ink = item.enabled ? style_.value_ink : style_.disabled_ink;
    if(item.mixed)
        ink = style_.mixed_ink;
    else if(item.inherited)
        ink = style_.inherited_ink;

    if(item.inherited) {
        w.DrawText(value_rect.left, y, "Using theme", font, ink);
        return;
    }

    const auto DrawSwatch = [&](int x, Color color) {
        const int diameter = min(DPI(16),
                                 max(0, value_rect.GetHeight() - DPI(8)));
        Rect dot = RectC(x,
            value_rect.top + (value_rect.GetHeight() - diameter) / 2,
            diameter, diameter);
        if(diameter > 0)
            w.DrawEllipse(dot, color, 1, style_.frame);
        return diameter;
    };

    if(item.kind == PropertyEditorKind::Color &&
       !item.mixed && item.value.GetType() == COLOR_V) {
        DrawSwatch(value_rect.left, Color(item.value));
        return;
    }

    if(item.kind == PropertyEditorKind::ColorPalette &&
       !item.mixed && item.value.Is<ValueArray>()) {
        ValueArray colors = item.value;
        int x = value_rect.left;
        const int count = min(4, colors.GetCount());
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
    if(item.inherited)
        return "Using theme";

    switch(item.kind) {
    case PropertyEditorKind::Boolean:
        return (bool)item.value ? "On" : "Off";
    case PropertyEditorKind::Choice:
        for(const PropertyEditorChoice& choice : item.choices)
            if(choice.value == item.value)
                return choice.label;
        return AsString(item.value);
    case PropertyEditorKind::Color:
        return item.value.GetType() == COLOR_V ? String() : "<none>";
    case PropertyEditorKind::FillRecipe:
        if(item.value.Is<ValueMap>()) {
            ValueMap recipe = item.value;
            const int q = recipe.Find("mode");
            return q >= 0 ? AsString(recipe.GetValue(q)) : "None";
        }
        return "None";
    case PropertyEditorKind::Multiline:
        return PeFormatMultilineSummary(item.value);
    case PropertyEditorKind::Vector2:
        return PropertyEditorFormatVector(item.value, 2, item.decimals);
    case PropertyEditorKind::Vector3:
        return PropertyEditorFormatVector(item.value, 3, item.decimals);
    case PropertyEditorKind::Curve:
        return PropertyEditorFormatCurve(item.value);
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

}
