// GUI-only preview projection. Decoders/providers run in PreviewWorker;
// debounce and generation checks keep selection changes cheap and truthful.
#include "UiFileBrowser.h"
namespace Upp
{
UiFileBrowser &UiFileBrowser::SetPreviewProvider(UiFileBrowserPreviewProvider provider)
{
    preview_provider_ = std::move(provider);
    RequestPreview(true);
    return *this;
}
void UiFileBrowser::RequestPreview(bool force)
{
    String path;
    if(mode_ == Mode::Advanced && show_inspector_.IsOn() && HasSelection() &&
       !(*model_)[selected_entry_].IsDirectory())
        path = GetSelection().path;
    if(!force && path == preview_path_)
        return;
    preview_image_size_ = Size(0, 0);
    meta_values_[5].SetText("—");
    preview_path_ = path;
    preview_worker_.Cancel();
    KillTimeCallback(4);
    KillTimeCallback(5);
    preview_image_.SetIcon(Image());
    preview_text_.SetText("");
    preview_content_.SetActiveKey("message");
    preview_title_.SetText(path.IsEmpty() ? "NO SELECTION" : "Loading preview…");
    preview_subtitle_.SetText(path.IsEmpty() ? "Select a file to preview" : "");
    if(path.IsEmpty())
        return;
    // Key navigation should not decode each file crossed in a rapid selection.
    SetTimeCallback(
        120,
        [this, path]
        {
            UiFileBrowserPreviewRequest request;
            request.path = path;
            preview_generation_ = preview_worker_.Request(request, preview_provider_);
            SetTimeCallback(-40, [this] { PollPreview(); }, 5);
        },
        4);
}
void UiFileBrowser::PollPreview()
{
    UiFileBrowserPreviewWorker::Result result;
    if(!preview_worker_.Poll(result) || result.generation != preview_generation_)
        return;
    KillTimeCallback(5);
    auto &data = result.data;
    preview_subtitle_.SetText(data.info);
    if(data.type == UiFileBrowserPreviewData::Type::Image && !IsNull(data.image))
    {
        preview_image_size_ = data.image_size;
        meta_values_[5].SetText(Format("%d × %d", data.image_size.cx, data.image_size.cy));
        RebuildInspectorDetails();
        preview_image_.SetIcon(data.image).SetIconSize(data.image.GetSize());
        preview_content_.SetActiveKey("image");
    }
    else if(data.type == UiFileBrowserPreviewData::Type::Text)
    {
        preview_text_.SetText(data.text);
        preview_text_.SetRect(Rect(Point(0, 0), preview_text_.GetMinSize()));
        preview_text_scroll_.SetScrollPos(Point(0, 0)).RefreshLayout();
        preview_content_.SetActiveKey("text");
    }
    else
    {
        preview_title_.SetText("Preview unavailable");
        preview_content_.SetActiveKey("message");
    }
}

void UiFileBrowser::RebuildInspectorDetails()
{
    bool image = preview_image_size_.cx > 0 && preview_image_size_.cy > 0;
    inspector_meta_.PauseLayout();
    while(inspector_meta_.GetItemCount())
        inspector_meta_.RemoveItem(inspector_meta_.GetItemCount() - 1);
    int count = image ? 6 : 5;
    inspector_meta_.SetGridSize(2, count);
    for(int i = 0; i < count; ++i)
    {
        inspector_meta_.AddGrid(meta_labels_[i], i, 0, false);
        inspector_meta_.AddGrid(meta_values_[i], i, 1, true);
    }
    inspector_meta_.ResumeLayout();
    inspector_layout_.PauseLayout().ClearItems();
    inspector_layout_.Add(preview_panel_).Fixed(SizePx(160));
    inspector_layout_.Add(inspector_name_).Fit().MinMain(SizePx(24));
    inspector_layout_.Add(inspector_meta_).Fixed(SizePx(count * 24 - 4));
    if(HasSelection() && (*model_)[selected_entry_].IsSequence())
    {
        inspector_layout_.Add(coverage_label_).Fixed(SizePx(20));
        inspector_layout_.Add(coverage_).Fixed(SizePx(8));
    }
    inspector_layout_.AddSpacer(1).Expand(1);
    inspector_layout_.ResumeLayout();
}

} // namespace Upp
