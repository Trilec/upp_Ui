#include <Ui/Ui.h>

using namespace Upp;

class UiGalleryDemo : public TopWindow {
public:
    typedef UiGalleryDemo CLASSNAME;

    UiGalleryDemo()
    {
        Title("UiGallery — 100,000 item model view");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1120), DPI(760));

        root_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(10));
        header_.SetTitle("UiGallery")
               .SetSubTitle("Uniform model-view tiles · 100,000 logical items")
               .SetHeaderMode(UiGroupPanel::Inside)
               .SetInset(Rect(DPI(10), DPI(7), DPI(10), DPI(7)));

        actions_.SetDirection(UiDirection::H).SetGap(DPI(6)).SetInset(0)
                .SetAlignItems(UiCrossAlign::Center);
        status_.SetText("Preparing model...");
        btn_first_.SetText("First");
        btn_last_.SetText("Last");
        smaller_.SetText("Smaller tiles");
        larger_.SetText("Larger tiles");
        actions_.Add(status_).Expand(1);
        actions_.Add(btn_first_).Fixed(DPI(64));
        actions_.Add(btn_last_).Fixed(DPI(64));
        actions_.Add(smaller_).Fixed(DPI(104));
        actions_.Add(larger_).Fixed(DPI(104));
        header_.SetContent(actions_);

        gallery_.SetSelectionMode(UIGALLERYSEL_MULTI)
                .SetItemSize(Size(DPI(92), DPI(88)))
                .SetGap(DPI(7))
                .SetInset(DPI(8))
                .SetOverscanRows(2);

        root_.Add(header_).Fit();
        root_.Add(gallery_).Expand(1);
        Add(root_.SizePos());

        BuildModel();
        Wire();
        UpdateStatus();
    }

private:
    UiBoxLayout root_;
    UiGroupPanel header_;
    UiBoxLayout actions_;
    UiLabel status_;
    UiButton btn_first_;
    UiButton btn_last_;
    UiButton smaller_;
    UiButton larger_;
    UiGallery gallery_;
    UiListModel model_;

    void BuildModel()
    {
        Vector<UiModelItem> items;
        items.Reserve(100000);
        for(int i = 0; i < 100000; i++) {
            UiModelItem item(Format("Item %d", i + 1), i);
            item.description = Format("%d", i);
            if(i % 97 == 0) {
                item.has_metadata = true;
                item.metadata_color = Color(65, 167, 248);
            }
            items.Add(pick(item));
        }
        model_.AddRange(items);
        gallery_.SetModel(model_);
    }

    void Wire()
    {
        btn_first_.WhenAction = [=] { gallery_.SetCursor(0); UpdateStatus(); };
        btn_last_.WhenAction = [=] { gallery_.SetCursor(model_.GetCount() - 1); UpdateStatus(); };
        smaller_.WhenAction = [=] {
            Size s = gallery_.GetItemSize();
            gallery_.SetItemSize(Size(max(DPI(52), s.cx - DPI(10)),
                                      max(DPI(56), s.cy - DPI(8))));
            UpdateStatus();
        };
        larger_.WhenAction = [=] {
            Size s = gallery_.GetItemSize();
            gallery_.SetItemSize(Size(min(DPI(180), s.cx + DPI(10)),
                                      min(DPI(170), s.cy + DPI(8))));
            UpdateStatus();
        };
        gallery_.WhenSelection = [=] { UpdateStatus(); };
        gallery_.WhenVisibleRange = [=](int, int) { UpdateStatus(); };
    }

    void UpdateStatus()
    {
        UiVisibleRange r = gallery_.GetVisibleRange(true);
        String text;
        text << model_.GetCount() << " items · " << gallery_.GetColumnCount() << " columns";
        if(!r.IsEmpty())
            text << " · useful range " << r.first + 1 << "–" << r.last + 1;
        text << " · selected " << gallery_.GetSelectionCount();
        status_.SetText(text);
    }
};

GUI_APP_MAIN
{
    UiGalleryDemo().Run();
}
