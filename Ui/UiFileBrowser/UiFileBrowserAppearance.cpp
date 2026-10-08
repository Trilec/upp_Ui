#include "UiFileBrowserAppearance.h"

namespace Upp
{
namespace UiFileBrowserAppearance
{

Font ModernFont()
{
    for(const char *name : {"Segoe UI Variable", "Segoe UI", "Inter"})
    {
        int face = Font::FindFaceNameIndex(name);
        if(face > 0)
            return Font(face, DPI(13));
    }
    return GetStdFont().Height(DPI(13));
}

Color TypeInk(const UiFileBrowserEntry &entry)
{
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    if(entry.IsSequence() && !entry.complete)
        return dark ? Color(224, 176, 96) : Color(137, 91, 24);
    using Type = UiFileBrowserFileType;
    switch(UiFileBrowserModel::GetFileType(entry))
    {
    case Type::Folder:
        return dark ? Color(114, 185, 234) : Color(29, 119, 181);
    case Type::Image:
        return dark ? Color(124, 194, 182) : Color(44, 126, 109);
    case Type::Video:
        return dark ? Color(184, 153, 220) : Color(118, 82, 162);
    case Type::Audio:
        return dark ? Color(217, 151, 179) : Color(151, 77, 114);
    case Type::Executable:
        return dark ? Color(151, 202, 154) : Color(64, 129, 70);
    case Type::Text:
        return dark ? Color(222, 177, 130) : Color(157, 103, 49);
    case Type::Script:
        return dark ? Color(134, 183, 223) : Color(57, 117, 160);
    case Type::Document:
        return dark ? Color(162, 174, 224) : Color(89, 102, 164);
    case Type::Spreadsheet:
        return dark ? Color(146, 201, 172) : Color(54, 130, 94);
    case Type::Presentation:
        return dark ? Color(223, 156, 140) : Color(158, 88, 71);
    case Type::Archive:
        return dark ? Color(215, 190, 132) : Color(142, 112, 43);
    case Type::Code:
        return dark ? Color(128, 199, 209) : Color(43, 128, 139);
    case Type::Config:
        return dark ? Color(163, 182, 195) : Color(85, 108, 126);
    case Type::Data:
        return dark ? Color(141, 189, 199) : Color(61, 115, 131);
    case Type::Font:
        return dark ? Color(210, 172, 213) : Color(142, 98, 151);
    case Type::Model:
        return dark ? Color(181, 188, 139) : Color(105, 117, 63);
    case Type::Material:
        return dark ? Color(203, 177, 151) : Color(137, 107, 79);
    case Type::Project:
        return dark ? Color(174, 158, 213) : Color(109, 87, 159);
    case Type::Shortcut:
        return dark ? Color(149, 172, 210) : Color(77, 104, 147);
    case Type::Other:
    default:
        return dark ? Color(152, 162, 171) : Color(95, 105, 114);
    }
}

Color NameInk(const UiFileBrowserEntry &entry)
{
    if(!entry.name.StartsWith("."))
        return Null;
    auto style = UiTheme::ResolveList();
    return Blend(style.ink, style.muted_ink, 120);
}

Image TintGlyph(const Image &glyph, Color tint)
{
    ImageBuffer buffer(glyph.GetSize());
    const RGBA *src = glyph.Begin();
    RGBA *dst = buffer.Begin();
    for(int i = 0; i < glyph.GetLength(); ++i)
    {
        int alpha = src[i].a;
        dst[i].r = byte(tint.GetR() * alpha / 255);
        dst[i].g = byte(tint.GetG() * alpha / 255);
        dst[i].b = byte(tint.GetB() * alpha / 255);
        dst[i].a = byte(alpha);
    }
    return Image(buffer);
}

UiModelItem BrowserItem(const String &text, const Value &data, const Image &icon)
{
    UiModelItem item(text, data);
    item.icon = icon;
    item.icon_render_mode = UiIconRenderMode::MonoTint;
    if(icon == ICON_DESIGN_FOLDER_48())
    {
        // Folder color is part of the glyph, so selection must not mono-tint it.
        UiFileBrowserEntry folder;
        folder.kind = UiFileBrowserEntryKind::Folder;
        item.icon = FileIcon(folder);
        item.icon_render_mode = UiIconRenderMode::PreserveColor;
    }
    return item;
}
Image FileIcon(const UiFileBrowserEntry &entry)
{
    using Type = UiFileBrowserFileType;
    Type type = UiFileBrowserModel::GetFileType(entry);
    bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    int variant = entry.IsSequence() ? (entry.complete ? 1 : 2) : 0;
    // Bounded cache: two palettes, twenty families, three sequence states.
    static Image cache[2][(int)Type::Other + 1][3];
    Image &icon = cache[dark][(int)type][variant];
    if(icon.IsEmpty())
    {
        Image glyph = ICON_DESIGN_DESCRIPTION_48();
        if(entry.IsSequence())
            glyph = ICON_DESIGN_STACK_48();
        else if(type == Type::Folder)
            glyph = ICON_DESIGN_FOLDER_48();
        else if(type == Type::Image)
            glyph = ICON_DESIGN_IMAGE_48();
        else if(type == Type::Video)
            glyph = ICON_MEDIA_PLAY_48();
        icon = TintGlyph(glyph, TypeInk(entry));
    }
    return icon;
}

} // namespace UiFileBrowserAppearance
} // namespace Upp
