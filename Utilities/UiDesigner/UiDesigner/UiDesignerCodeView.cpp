#include "UiDesignerWidgets.h"
#include <Ui/UiIcons.h>

namespace Upp {

UiDesignerCodeView::UiDesignerCodeView()
{
    Add(edit_);
    Add(copy_);
    Add(fullscreen_);

    edit_.SetReadOnly();

    copy_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    copy_.SetText("")
         .SetIcon(ICON_CONTENT_CONTENT_COPY_48())
         .SetIconSize(DPI(18), DPI(18))
         .SetContentInset(DPI(3))
         .SetContentGap(DPI(0))
         .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
         .SetIconScaleToContent(false)
         .NoWantFocus();
    copy_.Tip("Copy all generated code");
    copy_.WhenAction = [=] { CopyAll(); };

    fullscreen_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Subtle));
    fullscreen_.SetText("")
               .SetIcon(ICON_DESIGN_UNFOLD_MORE_48())
               .SetIconSize(DPI(18), DPI(18))
               .SetContentInset(DPI(3))
               .SetContentGap(DPI(0))
               .SetAlign(UiAlign::CENTER, UiAlign::CENTER)
               .SetIconScaleToContent(false)
               .NoWantFocus();
    fullscreen_.Tip("Open generated code in a full-screen dialog");
    fullscreen_.WhenAction = [=] { ShowFullscreen(); };
}

void UiDesignerCodeView::SetCode(const String& code)
{
    edit_.SetData(code);
}

String UiDesignerCodeView::GetCode() const
{
    return AsString(edit_.GetData());
}

void UiDesignerCodeView::Layout()
{
    const int toolbar_height = DPI(40);
    const int button_size = DPI(30);
    const int y = max(0, (toolbar_height - button_size) / 2);
    copy_.SetRect(DPI(6), y, button_size, button_size);
    fullscreen_.SetRect(DPI(42), y, button_size, button_size);
    edit_.SetRect(0, toolbar_height, GetSize().cx,
                  max(0, GetSize().cy - toolbar_height));
}

void UiDesignerCodeView::CopyAll()
{
    WriteClipboardText(GetCode());
}

void UiDesignerCodeView::ShowFullscreen()
{
    TopWindow dialog;
    UiMultiEdit code;
    code.SetReadOnly();
    code.SetData(GetCode());
    dialog.Title("Generated code").Sizeable().Zoomable();
    dialog.Add(code.SizePos());
    dialog.FullScreen();
    dialog.Run();
}

}
