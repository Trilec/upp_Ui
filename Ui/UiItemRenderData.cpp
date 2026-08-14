#include <Ui/UiItemRender.h>
#include <Ui/UiTheme.h>

namespace Upp {

UiItemRenderData UiMakeItemRenderData(const UiModelColumn& column, bool enabled)
{
    UiItemRenderData data;
    data.title = column.text;
    data.icon = column.icon;
    data.icon_render_mode = column.icon_render_mode;
    data.enabled = enabled;
    data.custom_ink_color = column.ink;
    data.text_align = column.align;
    return data;
}

UiItemRenderData UiMakeItemRenderData(const UiTableCell& cell, const String& display)
{
    UiItemRenderData data;
    data.title = display;
    data.value = cell.value;
    data.icon = cell.icon;
    data.icon_render_mode = cell.icon_render_mode;
    data.enabled = cell.enabled;
    data.custom_ink_color = cell.use_custom_ink ? cell.ink : Null;
    data.use_custom_font = cell.use_custom_font;
    data.custom_font = cell.font;
    data.text_align = cell.align;
    if(cell.has_error)
        data.role = UiRole::Alert;
    else if(cell.has_warning)
        data.role = UiRole::Accent;
    return data;
}

UiItemRenderData UiMakeItemRenderData(const UiTableHeader& header, const String& display)
{
    UiItemRenderData data;
    data.title = display;
    data.data = header.data;
    data.icon = header.icon;
    data.icon_render_mode = header.icon_render_mode;
    data.enabled = header.enabled;
    data.emphasized = true;
    data.custom_ink_color = header.custom_ink_color;
    data.text_align = header.align;
    data.role = UiRole::Subtle;
    return data;
}

} // namespace Upp
