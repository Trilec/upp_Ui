// Compact command popups reuse the authoritative settings controls. Opening a
// page changes parenting only; it never clones settings or scans a directory.
#include "UiFileBrowser.h"

namespace Upp
{

namespace
{
bool FileBrowserOwnsPopupFocus(Ctrl *ctrl, const Ctrl *popup)
{
    // PopUp records its logical owner as the anchor control. Win32's HWND owner
    // loses that child identity, so walk native parents then logical owners.
    while(ctrl)
    {
        ctrl = ctrl->GetTopCtrl();
        if(ctrl == popup)
            return true;
        ctrl = ctrl->GetOwnerCtrl();
    }
    return false;
}
} // namespace

void UiFileBrowserOptionsPopup::DeactivateBy(Ctrl *next)
{
    if(FileBrowserOwnsPopupFocus(next, this))
        return;
    // Win32 may transfer focus while a new owned popup is still being created.
    // Recheck after registration rather than closing its parent mid-PopUp().
    SetTimeCallback(10,
                    [this]
                    {
                        if(FileBrowserOwnsPopupFocus(GetActiveCtrl(), this))
                            return;
                        auto notify = WhenDismiss;
                        notify();
                    });
}

bool UiFileBrowserOptionsPopup::Key(dword key, int count)
{
    if(key == K_ESCAPE)
    {
        auto notify = WhenDismiss;
        notify();
        return true;
    }
    return Ctrl::Key(key, count);
}

void UiFileBrowser::CloseOptions()
{
    CloseCreationColour();
    if(!options_open_)
        return;
    options_open_ = false; // Clear before native close/deactivation can reenter.
    options_popup_.KillTimeCallback();
    for(UiDropdown *dropdown :
        {&filter_action_, &filter_field_, &sequence_mode_, &frame_view_, &pattern_mode_, &sort_key_,
         &folder_placement_, &new_item_kind_, &new_image_fill_})
        if(dropdown->IsPopupOpen())
            dropdown->Key(K_ESCAPE, 1);
    options_popup_.Close();
    options_layout_.ClearItems();
    options_anchor_ = nullptr;
    RefreshModeVisuals();
}

void UiFileBrowser::OpenOptions(OptionsPage page, Ctrl &anchor)
{
    bool same = options_open_ && options_page_ == page;
    CloseOptions();
    if(same)
        return;
    options_page_ = page;
    options_anchor_ = &anchor;
    options_open_ = true;
    UiPanel *panel = page == OptionsPage::Filter     ? &filter_group_
                     : page == OptionsPage::Sequence ? &sequence_group_
                     : page == OptionsPage::View     ? &view_group_
                     : page == OptionsPage::Sort     ? &sort_group_
                                                     : &new_item_group_;
    options_layout_.Add(*panel).Expand(1);
    if(page == OptionsPage::NewItem)
    {
        new_item_name_.SetData("");
        new_item_error_.SetText("");
        ConfigureCreation();
    }
    PositionOptions();
    // The owning browser remains alive; children and callbacks are members.
    options_popup_.PopUp(this, true, true, false);
    if(page == OptionsPage::Filter)
        search_.SetFocus();
    else if(page == OptionsPage::NewItem)
        new_item_name_.SetFocus();
    else if(page == OptionsPage::Sequence)
        sequence_mode_.SetFocus();
    else if(page == OptionsPage::Sort)
        sort_key_.SetFocus();
    else
        details_button_.SetFocus();
    RefreshModeVisuals();
}

void UiFileBrowser::PositionOptions()
{
    if(!options_anchor_)
        return;
    Rect anchor = options_anchor_->GetScreenRect();
    Rect work = Ctrl::GetWorkArea(anchor.CenterPoint());
    Size size = options_page_ == OptionsPage::Filter     ? Size(SizePx(460), SizePx(236))
                : options_page_ == OptionsPage::Sequence ? Size(SizePx(260), SizePx(220))
                : options_page_ == OptionsPage::View     ? Size(SizePx(270), SizePx(168))
                : options_page_ == OptionsPage::Sort
                    ? Size(SizePx(230), SizePx(114))
                    : Size(SizePx(340), SizePx((int)new_item_kind_.GetData() >= 3 ? 264 : 196));
    size.cx = min(size.cx, work.GetWidth());
    size.cy = min(size.cy, work.GetHeight());
    int x = clamp(anchor.left, work.left, work.right - size.cx);
    int y = anchor.bottom + DPI(2);
    if(y + size.cy > work.bottom)
        y = anchor.top - size.cy - DPI(2);
    y = clamp(y, work.top, work.bottom - size.cy);
    options_popup_.SetRect(Rect(Point(x, y), size));
    options_popup_.Layout();
    if(colour_popup_.IsOpen())
        PositionCreationColour();
}

void UiFileBrowser::ConfigureCreation()
{
    // A type change replaces the field set; dismiss any picker anchored to it.
    // Keep the typed name and image defaults so switching types does not lose work.
    CloseCreationColour();
    bool image = (int)new_item_kind_.GetData() >= 3;
    new_item_layout_.PauseLayout().ClearItems();
    new_item_layout_.Add(new_item_title_).Fixed(SizePx(20));
    new_item_layout_.Add(new_item_kind_).Fixed(SizePx(26));
    new_item_layout_.Add(new_item_name_).Fixed(SizePx(26));
    if(image)
    {
        new_item_layout_.Add(new_image_size_row_).Fixed(SizePx(26));
        new_item_layout_.Add(new_image_fill_row_).Fixed(SizePx(26));
    }
    new_item_layout_.Add(new_item_error_).Expand(1);
    new_item_layout_.Add(create_item_button_).Fixed(SizePx(28));
    new_item_layout_.ResumeLayout();
    new_item_name_.SetPlaceholder((int)new_item_kind_.GetData() == 0 ? "Folder name" : "File name");
    new_item_error_.SetText("");
    RefreshCreationColour();
    if(options_open_)
        PositionOptions();
}

void UiFileBrowser::RefreshCreationColour()
{
    Color colour;
    if(!UiColorPickerMicro::ParseHex(AsString(new_image_colour_.GetData()), colour))
    {
        new_image_swatch_.SetIcon(Image());
        return;
    }
    ImageBuffer swatch(Size(DPI(20), DPI(20)));
    Fill(swatch.Begin(), colour, swatch.GetLength());
    new_image_swatch_.SetIcon(Image(swatch)).SetIconSize(Size(DPI(20), DPI(20)));
}

void UiFileBrowser::CreateItem()
{
    UiFileBrowserCreateRequest request;
    request.kind = (UiFileBrowserCreateKind)(int)new_item_kind_.GetData();
    request.name = AsString(new_item_name_.GetData());
    if(request.kind == UiFileBrowserCreateKind::Png || request.kind == UiFileBrowserCreateKind::Jpeg)
    {
        auto dimension = [](const Value &value)
        {
            String text = TrimBoth(AsString(value));
            if(text.IsEmpty() || text.GetCount() > 4)
                return 0;
            int result = 0;
            for(char c : text)
            {
                if(c < '0' || c > '9')
                    return 0;
                result = result * 10 + c - '0';
            }
            return result;
        };
        request.size = Size(dimension(new_image_width_.GetData()), dimension(new_image_height_.GetData()));
        if(!UiColorPickerMicro::ParseHex(AsString(new_image_colour_.GetData()), request.colour))
        {
            new_item_error_.SetText("Enter a fill colour as #RRGGBB.");
            return;
        }
        request.gradient = (int)new_image_fill_.GetData() == 1;
    }
    String path, error;
    if(!CreateFileBrowserItem(folder_, request, path, error))
    {
        new_item_error_.SetText(error);
        return;
    }
    CloseOptions();
    RefreshFolder();
    restore_path_ = path;
    restore_frame_ = Null;
}

void UiFileBrowser::ChooseCreationColour()
{
    if(colour_popup_.IsOpen())
    {
        CloseCreationColour();
        return;
    }
    Color initial(208, 208, 208);
    UiColorPickerMicro::ParseHex(AsString(new_image_colour_.GetData()), initial);
    colour_micro_.SetColor(initial);
    PositionCreationColour();
    // This second popup belongs to New, so its focus is part of the same family.
    colour_popup_.PopUp(&options_popup_, true, true, false);
    colour_micro_.SetFocus();
}

void UiFileBrowser::CloseCreationColour()
{
    colour_popup_.KillTimeCallback();
    if(colour_popup_.IsOpen())
        colour_popup_.Close();
}

void UiFileBrowser::PositionCreationColour()
{
    Rect anchor = new_image_swatch_.GetScreenRect(), work = Ctrl::GetWorkArea(anchor.CenterPoint());
    Size size = colour_micro_.GetMinSize();
    size.cx = min(size.cx, work.GetWidth());
    size.cy = min(size.cy, work.GetHeight());
    int x = clamp(anchor.left, work.left, work.right - size.cx), y = anchor.bottom + DPI(2);
    if(y + size.cy > work.bottom)
        y = anchor.top - size.cy - DPI(2);
    colour_popup_.SetRect(Rect(Point(x, clamp(y, work.top, work.bottom - size.cy)), size));
    colour_popup_.Layout();
}

} // namespace Upp
