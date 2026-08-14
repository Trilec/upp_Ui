#include <Ui/Ui.h>

using namespace Upp;

class UiGalleryDemo : public TopWindow {
public:
    typedef UiGalleryDemo CLASSNAME;

    UiGalleryDemo()
    {
        Title("Ui model rendering — List + Gallery");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1240), DPI(760));

        root_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(10));
        header_.SetTitle("Ui model rendering")
               .SetSubTitle("One 10,000-item model · shared renderers · bounded visible pools")
               .SetHeaderMode(UiGroupPanel::Inside)
               .SetInset(Rect(DPI(10), DPI(7), DPI(10), DPI(7)));

        actions_.SetDirection(UiDirection::H).SetGap(DPI(5)).SetInset(0)
                .SetAlignItems(UiCrossAlign::Center);
        status_.SetText("Preparing model...");
        btn_first_.SetText("First");
        btn_last_.SetText("Last");
        btn_basic_.SetText("Basic");
        btn_image_.SetText("Image");
        btn_zoom_out_.SetText("Zoom -");
        btn_zoom_in_.SetText("Zoom +");
        btn_theme_.SetText("Dark");
        actions_.Add(status_).Expand(1);
        actions_.Add(btn_first_).Fixed(DPI(58));
        actions_.Add(btn_last_).Fixed(DPI(58));
        actions_.Add(btn_basic_).Fixed(DPI(62));
        actions_.Add(btn_image_).Fixed(DPI(62));
        actions_.Add(btn_zoom_out_).Fixed(DPI(68));
        actions_.Add(btn_zoom_in_).Fixed(DPI(68));
        actions_.Add(btn_theme_).Fixed(DPI(58));
        header_.SetContent(actions_);

        views_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0);
        list_.SetSelectionMode(UILISTSEL_MULTI);
        gallery_.SetSelectionMode(UIGALLERYSEL_MULTI)
                .SetItemSize(Size(DPI(104), DPI(108)))
                .SetGap(DPI(7))
                .SetInset(DPI(8))
                .SetOverscanRows(2)
                .SetZoomRange(0.55, 2.0, 1.12);
        views_.Add(list_).Expand(1).MinMain(DPI(280));
        views_.Add(gallery_).Expand(2).MinMain(DPI(480));

        root_.Add(header_).Fit();
        root_.Add(views_).Expand(1);
        Add(root_.SizePos());

        BuildDemoImages();
        BuildModel();
        UseImageRender();
        Wire();
        UpdateStatus();
    }

private:
    UiBoxLayout root_;
    UiGroupPanel header_;
    UiBoxLayout actions_;
    UiBoxLayout views_;
    UiLabel status_;
    UiButton btn_first_;
    UiButton btn_last_;
    UiButton btn_basic_;
    UiButton btn_image_;
    UiButton btn_zoom_out_;
    UiButton btn_zoom_in_;
    UiButton btn_theme_;
    UiList list_;
    UiGallery gallery_;
    UiListModel model_;
    Vector<Image> demo_images_;
    bool dark_ = false;
    bool image_render_ = true;

    static Color SeedColor(int seed, int offset)
    {
        return Color(48 + (seed * 37 + offset * 31) % 176,
                     48 + (seed * 59 + offset * 47) % 176,
                     48 + (seed * 83 + offset * 19) % 176);
    }

    void BuildDemoImages()
    {
        demo_images_.Reserve(64);
        for(int i = 0; i < 64; i++) {
            ImageDraw draw(36, 36);
            Color background = SeedColor(i, 0);
            Color accent = SeedColor(i, 1);
            Color line = SeedColor(i, 2);
            draw.DrawRect(0, 0, 36, 36, background);

            switch(i % 3) {
            case 0:
                draw.DrawEllipse(7, 7, 22, 22, accent);
                break;
            case 1:
                draw.DrawRect(7, 7, 22, 22, accent);
                break;
            default:
                draw.DrawEllipse(5, 9, 26, 18, accent);
                break;
            }

            if(i & 1)
                draw.DrawLine(5, 30, 30, 5, 2, line);
            else
                draw.DrawLine(5, 18, 30, 18, 2, line);
            demo_images_.Add(draw);
        }
    }

    void BuildModel()
    {
        Vector<UiModelItem> items;
        items.Reserve(10000);
        for(int i = 0; i < 10000; i++) {
            UiModelItem item(Format("Item %d", i + 1), i);
            item.description = Format("Seed %02d", i % demo_images_.GetCount());
            item.right_text = Format("#%d", i + 1);
            item.image = demo_images_[i % demo_images_.GetCount()];
            item.icon = item.image;
            item.icon_render_mode = UiIconRenderMode::PreserveColor;
            if(i % 97 == 0) {
                item.has_metadata = true;
                item.metadata_color = SeedColor(i, 3);
            }
            items.Add(pick(item));
        }
        model_.AddRange(items);
        list_.SetModel(model_);
        gallery_.SetModel(model_);
    }

    void UseBasicRender()
    {
        UiItemRenderBasic render;
        list_.SetItemRender(render);
        gallery_.SetItemRender(render);
        image_render_ = false;
        UpdateStatus();
    }

    void UseImageRender()
    {
        UiItemRenderImage render;
        list_.SetItemRender(render);
        gallery_.SetItemRender(render);
        image_render_ = true;
        UpdateStatus();
    }

    void GoTo(int index)
    {
        if(index < 0 || index >= model_.GetCount())
            return;
        list_.SetCursor(index);
        gallery_.SetCursor(index);
        UpdateStatus();
    }

    void ToggleTheme()
    {
        dark_ = !dark_;
        UiTheme::Set(dark_ ? UiThemeMode::Dark : UiThemeMode::Light);
        btn_theme_.SetText(dark_ ? "Light" : "Dark");
        // Renderer theme changes may affect fonts/metrics, so prepare them via
        // the normal layout path rather than asking Paint to recalculate.
        list_.RefreshLayout();
        gallery_.RefreshLayout();
        root_.RefreshLayout();
        Refresh();
    }

    void Wire()
    {
        btn_first_.WhenAction = [=] { GoTo(0); };
        btn_last_.WhenAction = [=] { GoTo(model_.GetCount() - 1); };
        btn_basic_.WhenAction = [=] { UseBasicRender(); };
        btn_image_.WhenAction = [=] { UseImageRender(); };
        btn_zoom_out_.WhenAction = [=] { gallery_.ZoomBy(1.0 / 1.12); UpdateStatus(); };
        btn_zoom_in_.WhenAction = [=] { gallery_.ZoomBy(1.12); UpdateStatus(); };
        btn_theme_.WhenAction = [=] { ToggleTheme(); UpdateStatus(); };
        list_.WhenSelection = [=] { UpdateStatus(); };
        gallery_.WhenSelection = [=] { UpdateStatus(); };
        gallery_.WhenVisibleRange = [=](int, int) { UpdateStatus(); };
    }

    void UpdateStatus()
    {
        UiVisibleRange r = gallery_.GetVisibleRange(true);
        String text;
        text << model_.GetCount() << " items · "
             << (image_render_ ? "Image" : "Basic") << " render · "
             << gallery_.GetColumnCount() << " cols · zoom "
             << Format("%.0f%%", gallery_.GetZoom() * 100.0);
        if(!r.IsEmpty())
            text << " · range " << r.first + 1 << "–" << r.last + 1;
        text << " · pools L" << list_.GetLiveItemRenderCount()
             << "/G" << gallery_.GetLiveItemRenderCount()
             << " · selected L" << list_.GetSelectionCount()
             << "/G" << gallery_.GetSelectionCount();
        status_.SetText(text);
    }
};

GUI_APP_MAIN
{
    UiGalleryDemo().Run();
}
