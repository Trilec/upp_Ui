#ifndef _Ui_UiDoc_UiDocMetadataPrivate_h_
#define _Ui_UiDoc_UiDocMetadataPrivate_h_

#include "UiDoc.h"

namespace Upp {

inline String UiDocNormalizeMetadataType(String type)
{
    type = TrimBoth(type);
    if(type.IsEmpty())
        type = "note";
    if(!type.StartsWith("metadata."))
        type = "metadata." + type;
    return type;
}

inline bool UiDocIsMetadataAnnotation(const UiDocAnnotation& annotation)
{
    return annotation.type.StartsWith("metadata.");
}

inline String UiDocMetadataTitle(const UiDocAnnotation& annotation)
{
    int q = annotation.payload.Find("title");
    if(q >= 0 && !IsNull(annotation.payload[q])) {
        String title = AsString(annotation.payload[q]);
        if(!title.IsEmpty())
            return title;
    }
    String type = annotation.type;
    if(type.StartsWith("metadata."))
        type = type.Mid(9);
    type.Replace("_", " ");
    type.Replace(".", " ");
    return type.IsEmpty() ? String("Metadata") : ToUpper(type.Left(1)) + type.Mid(1);
}

inline String UiDocMetadataBody(const UiDocAnnotation& annotation)
{
    int q = annotation.payload.Find("text");
    return q >= 0 && !IsNull(annotation.payload[q]) ? AsString(annotation.payload[q]) : String();
}

inline Vector<String> UiDocMetadataWrapLines(const String& text, const Font& font, int width)
{
    Vector<String> out;
    width = max(DPI(24), width);
    WString source = ToUnicode(text, CHARSET_UTF8);
    WString line;

    auto Flush = [&]() {
        out.Add(ToUtf8(line));
        line.Clear();
    };

    for(int i = 0; i < source.GetCount(); i++) {
        wchar ch = source[i];
        if(ch == '\n') {
            Flush();
            continue;
        }

        WString next = line;
        next.Cat(ch);
        if(!line.IsEmpty() && GetTextSize(ToUtf8(next), font).cx > width) {
            Flush();
            if(ch != ' ')
                line.Cat(ch);
        }
        else
            line.Cat(ch);
    }

    if(!line.IsEmpty() || out.IsEmpty())
        Flush();
    return out;
}

inline int UiDocMetadataCardHeight(const UiDocAnnotation& annotation, int width)
{
    Font title_font = SansSerifZ(DPI(9)).Bold();
    Font body_font = SansSerifZ(DPI(9));
    int icon = DPI(12);
    int pad = DPI(7);
    int header = max(icon, title_font.GetHeight());
    int body_width = max(DPI(32), width - 2 * pad);
    Vector<String> lines = UiDocMetadataWrapLines(UiDocMetadataBody(annotation), body_font, body_width);
    int body_height = max(body_font.GetHeight(), lines.GetCount() * max(DPI(12), body_font.GetHeight() + DPI(2)));
    return pad + header + DPI(4) + body_height + pad;
}

}

#endif
