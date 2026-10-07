// Native UiMediaCard reference: edit content and theme overrides, compare live
// cards with pooled Gallery rendering, and generate public-API usage C++.
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>

using namespace Upp;

namespace {

PropertyEditorItem& MarkOverride(PropertyEditorItem& item)
{
    item.overrideable = true;
    item.override_active = false;
    item.SetDefault(item.value);
    return item;
}

UiRole ParseRole(const String& s)
{
    if(s == "Subtle") return UiRole::Subtle;
    if(s == "Accent") return UiRole::Accent;
    if(s == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

UiMediaFit ParseFit(const String& s)
{
    return s == "Contain" ? UiMediaFit::Contain : UiMediaFit::Cover;
}

Size ParseAspect(const String& s)
{
    if(s == "4:3") return Size(4, 3);
    if(s == "3:2") return Size(3, 2);
    if(s == "16:9") return Size(16, 9);
    return Size(1, 1);
}

void ParseOverlayPosition(const String& value, UiAlign& h, UiAlign& v)
{
    h = UiAlign::CENTER;
    v = UiAlign::CENTER;

    if(value.Find("Left") >= 0) h = UiAlign::LEFT;
    if(value.Find("Right") >= 0) h = UiAlign::RIGHT;
    if(value.Find("Top") >= 0) v = UiAlign::TOP;
    if(value.Find("Bottom") >= 0) v = UiAlign::BOTTOM;
}

String AlignCode(UiAlign align)
{
    if(align == UiAlign::LEFT) return "LEFT";
    if(align == UiAlign::RIGHT) return "RIGHT";
    if(align == UiAlign::TOP) return "TOP";
    if(align == UiAlign::BOTTOM) return "BOTTOM";
    return "CENTER";
}

String CppString(const String& s)
{
    String out = "\"";
    for(int i = 0; i < s.GetCount(); i++) {
        int c = s[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    return out << '"';
}

String CppColor(Color c)
{
    if(IsNull(c))
        return "Null";
    return Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}

Color FaceColor(const StyledPalette& p, StyledState st, Color fallback)
{
    const UiFill& fill = p.face[st];
    return fill.IsSolid() && !IsNull(fill.color) ? fill.color : fallback;
}

Image MakePreviewImage(int seed = 0)
{
    const Size size(960, 540);
    ImageBuffer out(size);

    for(int y = 0; y < size.cy; y++) {
        RGBA *row = out[y];
        for(int x = 0; x < size.cx; x++) {
            int r = 30 + (x * (48 + seed * 3)) / size.cx + y * 14 / size.cy;
            int g = 44 + x * 34 / size.cx + y * (38 + seed * 2) / size.cy;
            int b = 70 + (size.cx - x) * (54 + seed * 2) / size.cx + y * 20 / size.cy;

            if(y > 330) {
                r = 25 + x * 18 / size.cx;
                g = 30 + x * 14 / size.cx;
                b = 38 + x * 20 / size.cx;
            }

            row[x] = RGBA(Color(clamp(r, 0, 255),
                                clamp(g, 0, 255),
                                clamp(b, 0, 255)));
            row[x].a = 255;
        }
    }

    int cx = 700 - seed * 17;
    for(int y = 130; y < 260; y++) {
        RGBA *row = out[y];
        for(int x = max(0, cx - 70); x < min(size.cx, cx + 70); x++) {
            int dx = x - cx;
            int dy = y - 195;
            if(dx * dx + dy * dy < 58 * 58) {
                row[x] = RGBA(Color(220, 150 + seed * 4, 82 + seed * 3));
                row[x].a = 255;
            }
        }
    }

    return Image(out);
}

class UiMediaCardDemo : public TopWindow {
public:
    typedef UiMediaCardDemo CLASSNAME;

    UiMediaCardDemo()
    {
        Title("UiMediaCard Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1320), DPI(840));

        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = UiThemeMode::Light;
        UiTheme::Set(ctx);

        RegisterPropertyEditorEditors(factory_);

        BuildHeader();
        BuildPreview();
        BuildRightRail();
        BuildInspector();
        BuildOverrides();
        ConfigureEditors();
        BuildGallerySample();
        ConnectEvents();

        SelectPage(0);
        ApplyTheme();
        ApplyProjection();
    }

    void Paint(Draw& w) override
    {
        w.DrawRect(GetSize(), window_face_);
    }

    void Layout() override
    {
        Rect r = GetSize();
        r.Deflate(DPI(12));

        header_.SetRect(r.left, r.top, r.GetWidth(), DPI(68));

        const int top = r.top + DPI(80);
        const int body_h = max(0, r.bottom - top);
        const int rail_w = min(DPI(440), max(DPI(370), r.GetWidth() / 3));
        const int gap = DPI(12);
        const int preview_w = max(0, r.GetWidth() - rail_w - gap);

        preview_.SetRect(r.left, top, preview_w, body_h);
        right_.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

        Size ps = preview_.GetSize();
        const int caption_h = DPI(48);
        const int gallery_h = min(DPI(166), max(DPI(126), ps.cy / 4));
        const int gallery_gap = DPI(10);
        const int label_h = DPI(18);

        Rect top_area = RectC(DPI(18), DPI(16) + label_h,
                              max(0, ps.cx - DPI(36)),
                              max(0, ps.cy - caption_h - gallery_h
                                           - gallery_gap - DPI(22) - label_h));

        const int wanted_w = max(DPI(170), (int)InspectorValue("width", 360));
        const int wanted_h = max(DPI(180), (int)InspectorValue("height", 430));
        const int sample_gap = DPI(18);
        const int sample_w =
            min(DPI(240), max(DPI(175), top_area.GetWidth() / 3));
        const int main_max_w =
            max(DPI(170), top_area.GetWidth() - sample_w - sample_gap);
        const int main_w = min(wanted_w, main_max_w);
        const int main_h = min(wanted_h, top_area.GetHeight());

        editable_label_.SetRect(top_area.left, DPI(14),
                                main_w, label_h);
        card_.SetRect(top_area.left,
                      top_area.top + max(0, (top_area.GetHeight() - main_h) / 2),
                      main_w, main_h);

        const int sample_x = top_area.left + main_w + sample_gap;
        const int actual_sample_w = max(0, top_area.right - sample_x);
        samples_label_.SetRect(sample_x, DPI(14),
                               actual_sample_w, label_h);

        const int small_gap = DPI(10);
        const int small_h =
            max(DPI(115), (top_area.GetHeight() - small_gap) / 2);

        empty_card_.SetRect(sample_x, top_area.top,
                            actual_sample_w, small_h);
        full_card_.SetRect(sample_x, top_area.top + small_h + small_gap,
                           actual_sample_w,
                           max(0, top_area.bottom
                                  - (top_area.top + small_h + small_gap)));

        const int gallery_top =
            max(0, ps.cy - caption_h - gallery_h);
        gallery_label_.SetRect(DPI(18), gallery_top,
                               max(0, ps.cx - DPI(36)), label_h);
        gallery_.SetRect(DPI(18), gallery_top + label_h,
                         max(0, ps.cx - DPI(36)),
                         max(0, gallery_h - label_h - DPI(6)));

        caption_.SetRect(0, max(0, ps.cy - caption_h),
                         ps.cx, caption_h);

        Size rs = right_.GetSize();
        tools_.SetRect(DPI(4), DPI(4),
                       max(0, rs.cx - DPI(8)), DPI(36));
        pages_.SetRect(DPI(4), DPI(44),
                       max(0, rs.cx - DPI(8)),
                       max(0, rs.cy - DPI(48)));
    }

private:
    void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiMediaCard")
               .SetSubTitle("Header + media + non-consuming tags/overlay + footer; live Ctrl and pooled renderer share one presentation")
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);

        header_actions_.SetGap(DPI(4)).SetInset(0)
                       .SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);

        theme_.SetIcon(ICON_ACTION_LIGHT_MODE_48())
              .SetIconSize(DPI(16), DPI(16))
              .Tip("Toggle light/dark theme");
        help_.SetIcon(ICON_DESIGN_HELP_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("About this demo");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Close demo");

        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
    }

    void BuildPreview()
    {
        Add(preview_);
        preview_.Add(editable_label_);
        preview_.Add(samples_label_);
        preview_.Add(gallery_label_);
        preview_.Add(card_);
        preview_.Add(empty_card_);
        preview_.Add(full_card_);
        preview_.Add(gallery_);
        preview_.Add(caption_);

        editable_label_.SetText("EDITABLE CARD — controlled by Content / Appearance")
                       .SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        samples_label_.SetText("STATIC COMPARISONS")
                      .SetAlign(UiAlign::LEFT, UiAlign::CENTER);
        gallery_label_.SetText("GALLERY RENDERER — UiMediaCardRender")
                      .SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        gallery_.SetItemSize(Size(DPI(132), DPI(136)))
                .SetGap(DPI(7))
                .SetInset(DPI(5))
                .SetOverscanRows(1);

        caption_.SetText(
            "Large left card is inspector-driven: click it to choose an image or drag an image/file onto it. Small right cards are static appearance examples.")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildRightRail()
    {
        Add(right_);
        right_.Add(tools_);
        right_.Add(pages_);

        tools_.SetGap(DPI(4))
              .SetInset(Rect(DPI(2), 0, DPI(2), 0))
              .SetAlignItems(UiCrossAlign::Center);

        inspector_mode_.SetText("")
                       .SetIcon(ICON_DESIGN_TUNE_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetIconSide(UiAlign::LEFT)
                       .SetCheckable().Tip("Inspector — content and structure");
        overrides_mode_.SetText("")
                       .SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetIconSide(UiAlign::LEFT)
                       .SetCheckable().Tip("Theme overrides");
        code_mode_.SetText("")
                  .SetIcon(ICON_DESIGN_CODE_BLOCKS_48())
                  .SetIconSize(DPI(17), DPI(17))
                  .SetIconSide(UiAlign::LEFT)
                  .SetCheckable().Tip("Generated C++");

        tools_.Add(inspector_mode_).Fixed(DPI(38));
        tools_.Add(overrides_mode_).Fixed(DPI(38));
        tools_.Add(code_mode_).Fixed(DPI(38));
        tools_.AddSpacer(1).Expand(1);

        pages_.Add(inspector_page_, "inspector");
        pages_.Add(overrides_page_, "overrides");
        pages_.Add(code_page_, "code");

        inspector_page_.Add(inspector_.SizePos());
        overrides_page_.Add(overrides_.SizePos());

        code_page_.Add(code_);
        code_.HSizePos(DPI(6), DPI(6))
             .VSizePos(DPI(42), DPI(6));
        code_.SetReadOnly();

        code_page_.Add(copy_.RightPos(DPI(8), DPI(32))
                            .TopPos(DPI(6), DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Copy generated C++");
    }

    void BuildInspector()
    {
        // Keep the two structural bands impossible to miss: Header is always
        // above Media and Footer is always below it.
        inspector_model_.AddBoolean(
            "header.show", "Header at top", true, "Structure");
        inspector_model_.AddBoolean(
            "footer.show", "Footer at bottom", true, "Structure");

        inspector_model_.AddText(
            "header.title", "Title", "Image processing", "Header — top");
        inspector_model_.AddText(
            "header.subtitle", "Subtitle", "Harbour / convert EXR", "Header — top");
        inspector_model_.AddText(
            "header.metadata", "Metadata", "", "Header — top");

        inspector_model_.AddChoice(
            "media", "Content", "Preview image", "Media")
            .AddChoice("Preview image", "Preview image")
            .AddChoice("Empty state", "Empty state");
        inspector_model_.AddText(
            "empty_text", "Empty cue", "+", "Media");
        inspector_model_.AddChoice(
            "fit", "Image fit", "Cover", "Media")
            .AddChoice("Cover", "Cover")
            .AddChoice("Contain", "Contain");
        inspector_model_.AddChoice(
            "aspect", "Media aspect", "16:9", "Media")
            .AddChoice("1:1", "1:1")
            .AddChoice("4:3", "4:3")
            .AddChoice("3:2", "3:2")
            .AddChoice("16:9", "16:9");

        inspector_model_.AddBoolean(
            "tags.top", "Top tags", true, "Tags / Overlay");
        inspector_model_.AddBoolean(
            "tags.bottom", "Bottom tags", true, "Tags / Overlay");
        inspector_model_.AddBoolean(
            "overlay.show", "Show overlay", false, "Tags / Overlay");
        inspector_model_.AddText(
            "overlay.text", "Overlay text", "PROCESSING 68%", "Tags / Overlay");
        inspector_model_.AddChoice(
            "overlay.position", "Overlay position", "Center", "Tags / Overlay")
            .AddChoice("Top Left", "Top Left")
            .AddChoice("Top Center", "Top Center")
            .AddChoice("Top Right", "Top Right")
            .AddChoice("Center Left", "Center Left")
            .AddChoice("Center", "Center")
            .AddChoice("Center Right", "Center Right")
            .AddChoice("Bottom Left", "Bottom Left")
            .AddChoice("Bottom Center", "Bottom Center")
            .AddChoice("Bottom Right", "Bottom Right");

        inspector_model_.AddText(
            "footer.title", "Title", "EXR / JPEG", "Footer — bottom");
        inspector_model_.AddText(
            "footer.subtitle", "Subtitle", "v012", "Footer — bottom");
        inspector_model_.AddText(
            "footer.metadata", "Metadata", "1536 x 864", "Footer — bottom");

        inspector_model_.AddChoice(
            "role", "Theme role", "Standard", "Presentation")
            .AddChoice("Standard", "Standard")
            .AddChoice("Subtle", "Subtle")
            .AddChoice("Accent", "Accent")
            .AddChoice("Alert", "Alert");

        inspector_model_.AddNumericInt(
            "width", "Preview width", 360, 170, 660, 1, "Layout")
            .SetUnit("px");
        inspector_model_.AddNumericInt(
            "height", "Preview height", 430, 180, 660, 1, "Layout")
            .SetUnit("px");

        inspector_model_.AddBoolean(
            "selected", "Selected", false, "Behaviour");
        inspector_model_.AddBoolean(
            "selectable", "Body clickable", true, "Behaviour");
        inspector_model_.AddBoolean(
            "enabled", "Enabled", true, "Behaviour");

        inspector_model_.SetGroupSubtitle(
            "Structure", "Header is above Media; Footer is below Media");
        inspector_model_.SetGroupSubtitle(
            "Header — top", "optional text band above the media region");
        inspector_model_.SetGroupSubtitle(
            "Media", "click the large card to choose an image, or drag an image/file onto it");
        inspector_model_.SetGroupSubtitle(
            "Tags / Overlay", "paint over media and never consume media geometry");
        inspector_model_.SetGroupSubtitle(
            "Footer — bottom", "optional text band below the media region");
        inspector_model_.SetGroupSubtitle(
            "Presentation", "semantic theme role before explicit Appearance overrides");
        inspector_model_.StructureChanged();
    }

    void BuildOverrides()
    {
        UiMediaCard probe;
        UiMediaCard::Style base = probe.GetStyle();

        MarkOverride(override_model_.AddBoolean(
            "card.background", "Background",
            base.metrics.face_enabled, "Card Surface"));
        MarkOverride(override_model_.AddBoolean(
            "card.border", "Border",
            base.metrics.frame_enabled, "Card Surface"));
        MarkOverride(override_model_.AddColor(
            "card.face", "Face",
            FaceColor(base.palette, ST_NORMAL, Color(250,250,251)),
            "Card Surface"));
        MarkOverride(override_model_.AddColor(
            "card.frame", "Frame",
            base.palette.frame[ST_NORMAL], "Card Surface"));
        MarkOverride(override_model_.AddNumericInt(
            "card.radius", "Radius",
            base.metrics.radius, 0, 40, 1, "Card Surface").SetUnit("px"));

        MarkOverride(override_model_.AddBoolean(
            "media.background", "Background",
            base.media_metrics.face_enabled, "Media Surface"));
        MarkOverride(override_model_.AddBoolean(
            "media.border", "Border",
            base.media_metrics.frame_enabled, "Media Surface"));
        MarkOverride(override_model_.AddColor(
            "media.face", "Face",
            FaceColor(base.media_palette, ST_NORMAL, Color(234,237,241)),
            "Media Surface"));
        MarkOverride(override_model_.AddColor(
            "media.frame", "Frame",
            base.media_palette.frame[ST_NORMAL], "Media Surface"));
        MarkOverride(override_model_.AddNumericInt(
            "media.radius", "Radius",
            base.media_metrics.radius, 0, 40, 1, "Media Surface").SetUnit("px"));

        MarkOverride(override_model_.AddBoolean(
            "header.background", "Background",
            base.header_style.metrics.face_enabled, "Header Surface"));
        MarkOverride(override_model_.AddBoolean(
            "header.border", "Border",
            base.header_style.metrics.frame_enabled, "Header Surface"));
        MarkOverride(override_model_.AddColor(
            "header.face", "Face",
            FaceColor(base.header_style.palette, ST_NORMAL, Color(250,250,251)),
            "Header Surface"));
        MarkOverride(override_model_.AddColor(
            "header.frame", "Frame",
            base.header_style.palette.frame[ST_NORMAL], "Header Surface"));

        MarkOverride(override_model_.AddBoolean(
            "footer.background", "Background",
            base.footer_style.metrics.face_enabled, "Footer Surface"));
        MarkOverride(override_model_.AddBoolean(
            "footer.border", "Border",
            base.footer_style.metrics.frame_enabled, "Footer Surface"));
        MarkOverride(override_model_.AddColor(
            "footer.face", "Face",
            FaceColor(base.footer_style.palette, ST_NORMAL, Color(250,250,251)),
            "Footer Surface"));
        MarkOverride(override_model_.AddColor(
            "footer.frame", "Frame",
            base.footer_style.palette.frame[ST_NORMAL], "Footer Surface"));

        MarkOverride(override_model_.AddNumericInt(
            "section.gap", "Section gap",
            base.section_gap, 0, 30, 1, "Spacing").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt(
            "tag.inset", "Tag inset",
            base.tag_inset, 0, 30, 1, "Tags").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt(
            "tag.gap", "Tag gap",
            base.tag_gap, 0, 20, 1, "Tags").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt(
            "tag.radius", "Tag radius",
            base.tag_style[(int)UiRole::Standard].metrics.radius,
            0, 20, 1, "Tags").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt(
            "overlay.radius", "Overlay radius",
            base.overlay_style[(int)UiRole::Standard].metrics.radius,
            0, 20, 1, "Overlay").SetUnit("px"));

        override_model_.SetGroupSubtitle(
            "Card Surface", "whole card face/frame; transparent and frameless by default");
        override_model_.SetGroupSubtitle(
            "Media Surface", "independent media face/frame; bordered by default");
        override_model_.SetGroupSubtitle(
            "Header Surface", "optional band surface; text-only by default");
        override_model_.SetGroupSubtitle(
            "Footer Surface", "optional band surface; text-only by default");
        override_model_.StructureChanged();
    }

    void ConfigureEditors()
    {
        inspector_.SetFactory(&factory_);
        overrides_.SetFactory(&factory_);
        inspector_.SetModel(&inspector_model_);
        overrides_.SetModel(&override_model_);
        inspector_.SetLabelRatio(46);
        overrides_.SetLabelRatio(46);

        PropertyEditorStyle style = PropertyEditorStyle::System();
        style.show_group_summaries = true;
        inspector_.SetStyle(style);
        overrides_.SetStyle(style);
    }

    void BuildGallerySample()
    {
        Vector<UiModelItem> items;
        for(int i = 0; i < 12; i++) {
            UiModelItem item(Format("Take %02d", i + 1), i);
            item.description = i % 3 == 0 ? "Image create" : "Image rework";
            item.right_text = Format("v%03d", 10 + i);
            item.image = MakePreviewImage(i % 5);
            item.icon = item.image;
            item.data = i;
            items.Add(pick(item));
        }

        gallery_model_.AddRange(items);
        gallery_.SetModel(gallery_model_);

        gallery_render_.SetResolver(
            [=](const UiItemRenderData& item, UiMediaCardData& card) {
                UiTagData kind("IMAGE", UiRole::Subtle, UiTagVariant::Filled);
                card.top_tags.Add().tag = kind;
                card.top_tags.Top().align = UiAlign::LEFT;

                UiTagData ready("READY", UiRole::Accent, UiTagVariant::Soft);
                card.top_tags.Add().tag = ready;
                card.top_tags.Top().align = UiAlign::RIGHT;

                if(AsString(item.title).EndsWith("03")) {
                    UiTagData loading("LOADING", UiRole::Accent, UiTagVariant::Soft);
                    card.overlay.content = loading;
                    card.overlay.visible = true;
                }
            });

        gallery_.SetItemRender(gallery_render_);
    }

    void UseDemoImage(const Image& image, const String& source)
    {
        if(IsNull(image) || image.IsEmpty()) {
            Exclamation("The selected item is not a supported image.");
            return;
        }

        preview_image_ = image;
        inspector_model_.SetValue("media", "Preview image");
        inspector_.RefreshModel();
        ApplyProjection();

        caption_.SetText(source.IsEmpty()
            ? "Loaded image into the editable card."
            : "Loaded image into the editable card: " + source);
    }

    void ChooseDemoImage()
    {
        FileSel selector;
        selector.Type("Images", "*.png *.bmp *.jpg *.jpeg");
        if(!selector.ExecuteOpen("Choose media for UiMediaCard"))
            return;

        String path = ~selector;
        UseDemoImage(StreamRaster::LoadFileAny(path), GetFileName(path));
    }

    void HandleDemoDrop(PasteClip& clip)
    {
        if(IsAvailableImage(clip)) {
            AcceptImage(clip);
            clip.SetAction(DND_COPY);
            if(clip.IsPaste())
                UseDemoImage(GetImage(clip), "dropped image");
            return;
        }

        if(IsAvailableFiles(clip)) {
            AcceptFiles(clip);
            clip.SetAction(DND_COPY);
            if(clip.IsPaste()) {
                Vector<String> files = GetFiles(clip);
                if(!files.IsEmpty())
                    UseDemoImage(StreamRaster::LoadFileAny(files[0]),
                                 GetFileName(files[0]));
            }
            return;
        }

        clip.Reject();
    }

    void ConnectEvents()
    {
        auto changed = [=](String, Value) { ApplyProjection(); };
        inspector_.WhenPreview = changed;
        inspector_.WhenCommit = changed;
        overrides_.WhenPreview = changed;
        overrides_.WhenCommit = changed;

        inspector_.WhenReset =
            [=](String id) { ResetProperty(inspector_model_, id); };
        overrides_.WhenReset =
            [=](String id) { ResetProperty(override_model_, id); };
        overrides_.WhenOverride =
            [=](String id, bool active) { SetOverrideActive(id, active); };

        inspector_mode_.WhenAction = [=] { SelectPage(0); };
        overrides_mode_.WhenAction = [=] { SelectPage(1); };
        code_mode_.WhenAction = [=] { SelectPage(2); };

        theme_.WhenAction = [=] { ToggleTheme(); };
        help_.WhenAction = [=] {
            PromptOK(
                "UiMediaCard reference demo\n\n"
                "CONTENT exposes Structure first: Header is the optional top "
                "text band and Footer is the optional bottom text band. "
                "APPEARANCE owns Card, Media, Header and Footer surfaces.\n\n"
                "Only the large left card is interactive in this demo. Click it "
                "to choose an image or drop an image/file onto it. The two small "
                "right cards are deliberately static comparisons. The Gallery "
                "below demonstrates the pooled UiMediaCardRender path.");
        };
        exit_.WhenAction = [=] { Break(); };
        copy_.WhenAction = [=] { WriteClipboardText(generated_); };

        card_.WhenAction = [=] { ChooseDemoImage(); };
        card_.WhenDrop = [=](PasteClip& clip) { HandleDemoDrop(clip); };
        card_.WhenTagAction = [=](String id, Value) {
            caption_.SetText("Tag action: " + id);
        };
    }

    Value InspectorValue(const String& id,
                         const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = inspector_model_.Find(id);
        return item ? item->value : fallback;
    }

    Value OverrideValue(const String& id,
                        const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = override_model_.Find(id);
        return item ? item->value : fallback;
    }

    bool OverrideActive(const String& id) const
    {
        const PropertyEditorItem *item = override_model_.Find(id);
        return item && item->override_active;
    }

    void ResetProperty(PropertyEditorModel& model, const String& id)
    {
        PropertyEditorItem *item = model.Find(id);
        if(!item || !item->resettable)
            return;
        model.SetValue(id, item->default_value);
        ApplyProjection();
    }

    void SetOverrideActive(const String& id, bool active)
    {
        PropertyEditorItem *item = override_model_.Find(id);
        if(!item || !item->overrideable)
            return;
        item->override_active = active;
        override_model_.StructureChanged();
        overrides_.RefreshModel();
        ApplyProjection();
    }

    void SyncInheritedOverrides(const UiMediaCard::Style& style)
    {
        auto set = [&](const char *id, const Value& value) {
            if(!OverrideActive(id))
                override_model_.SetValue(id, value, false);
        };

        set("card.background", style.metrics.face_enabled);
        set("card.border", style.metrics.frame_enabled);
        set("card.face",
            FaceColor(style.palette, ST_NORMAL, Color(250,250,251)));
        set("card.frame", style.palette.frame[ST_NORMAL]);
        set("card.radius", style.metrics.radius);

        set("media.background", style.media_metrics.face_enabled);
        set("media.border", style.media_metrics.frame_enabled);
        set("media.face",
            FaceColor(style.media_palette, ST_NORMAL, Color(234,237,241)));
        set("media.frame", style.media_palette.frame[ST_NORMAL]);
        set("media.radius", style.media_metrics.radius);

        set("header.background", style.header_style.metrics.face_enabled);
        set("header.border", style.header_style.metrics.frame_enabled);
        set("header.face",
            FaceColor(style.header_style.palette, ST_NORMAL, Color(250,250,251)));
        set("header.frame", style.header_style.palette.frame[ST_NORMAL]);

        set("footer.background", style.footer_style.metrics.face_enabled);
        set("footer.border", style.footer_style.metrics.frame_enabled);
        set("footer.face",
            FaceColor(style.footer_style.palette, ST_NORMAL, Color(250,250,251)));
        set("footer.frame", style.footer_style.palette.frame[ST_NORMAL]);

        set("section.gap", style.section_gap);
        set("tag.inset", style.tag_inset);
        set("tag.gap", style.tag_gap);
        set("tag.radius",
            style.tag_style[(int)UiRole::Standard].metrics.radius);
        set("overlay.radius",
            style.overlay_style[(int)UiRole::Standard].metrics.radius);

        overrides_.RefreshModel();
    }

    void ApplyOverrides(UiMediaCard::Style& style) const
    {
        auto face = [&](const char *id, StyledPalette& palette) {
            if(OverrideActive(id))
                palette.face[ST_NORMAL] =
                    UiFill::Solid(Color(OverrideValue(id)));
        };
        auto color = [&](const char *id, Color& target) {
            if(OverrideActive(id))
                target = Color(OverrideValue(id));
        };

        if(OverrideActive("card.background"))
            style.metrics.face_enabled =
                (bool)OverrideValue("card.background");
        if(OverrideActive("card.border"))
            style.metrics.frame_enabled =
                (bool)OverrideValue("card.border");
        face("card.face", style.palette);
        color("card.frame", style.palette.frame[ST_NORMAL]);
        if(OverrideActive("card.radius"))
            style.metrics.radius =
                max(0, (int)OverrideValue("card.radius"));

        if(OverrideActive("media.background"))
            style.media_metrics.face_enabled =
                (bool)OverrideValue("media.background");
        if(OverrideActive("media.border"))
            style.media_metrics.frame_enabled =
                (bool)OverrideValue("media.border");
        face("media.face", style.media_palette);
        color("media.frame", style.media_palette.frame[ST_NORMAL]);
        if(OverrideActive("media.radius"))
            style.media_metrics.radius =
                max(0, (int)OverrideValue("media.radius"));

        if(OverrideActive("header.background"))
            style.header_style.metrics.face_enabled =
                (bool)OverrideValue("header.background");
        if(OverrideActive("header.border"))
            style.header_style.metrics.frame_enabled =
                (bool)OverrideValue("header.border");
        face("header.face", style.header_style.palette);
        color("header.frame", style.header_style.palette.frame[ST_NORMAL]);

        if(OverrideActive("footer.background"))
            style.footer_style.metrics.face_enabled =
                (bool)OverrideValue("footer.background");
        if(OverrideActive("footer.border"))
            style.footer_style.metrics.frame_enabled =
                (bool)OverrideValue("footer.border");
        face("footer.face", style.footer_style.palette);
        color("footer.frame", style.footer_style.palette.frame[ST_NORMAL]);

        if(OverrideActive("section.gap"))
            style.section_gap =
                max(0, (int)OverrideValue("section.gap"));
        if(OverrideActive("tag.inset"))
            style.tag_inset =
                max(0, (int)OverrideValue("tag.inset"));
        if(OverrideActive("tag.gap"))
            style.tag_gap =
                max(0, (int)OverrideValue("tag.gap"));
        if(OverrideActive("tag.radius")) {
            int radius = max(0, (int)OverrideValue("tag.radius"));
            for(int i = 0; i < 4; i++)
                style.tag_style[i].metrics.radius = radius;
        }
        if(OverrideActive("overlay.radius")) {
            int radius = max(0, (int)OverrideValue("overlay.radius"));
            for(int i = 0; i < 4; i++)
                style.overlay_style[i].metrics.radius = radius;
        }
    }

    void ConfigureTagsAndOverlay()
    {
        card_.ClearTags().ClearOverlay();

        if((bool)InspectorValue("tags.top", true)) {
            UiTagData kind("IMAGE", UiRole::Subtle, UiTagVariant::Filled);
            kind.id = "kind";
            card_.AddTopTag(kind, UiAlign::LEFT);

            UiTagData ready("READY", UiRole::Accent, UiTagVariant::Soft);
            ready.id = "ready";
            ready.interactive = true;
            ready.value = "ready";
            card_.AddTopTag(ready, UiAlign::RIGHT);
        }

        if((bool)InspectorValue("tags.bottom", true)) {
            UiTagData take("take 03", UiRole::Standard, UiTagVariant::Soft);
            take.id = "take";
            card_.AddBottomTag(take, UiAlign::RIGHT);
        }

        if((bool)InspectorValue("overlay.show", false)) {
            UiAlign h, v;
            ParseOverlayPosition(
                AsString(InspectorValue("overlay.position", "Center")),
                h, v);

            UiTagData overlay(
                AsString(InspectorValue("overlay.text", "PROCESSING 68%")),
                UiRole::Accent, UiTagVariant::Filled);
            overlay.id = "overlay";
            card_.SetOverlay(overlay, h, v);
        }
    }

    void ConfigureSamples()
    {
        // These two cards are static comparison swatches, not editable inputs.
        empty_card_.ClearCustomStyle().SetRole(UiRole::Subtle);
        UiMediaCard::Style empty_style = empty_card_.GetStyle();
        empty_style.media_aspect = Size(1, 1);

        empty_card_.SetCustomStyle(empty_style)
                   .ClearImage()
                   .ClearHeader()
                   .SetFooter("Empty-state sample", "Static — media border only")
                   .SetEmptyCue("+")
                   .SetSelectable(false)
                   .ClearTags()
                   .ClearOverlay();

        UiTagData image("IMAGE", UiRole::Subtle, UiTagVariant::Outline);
        empty_card_.AddTopTag(image, UiAlign::LEFT);

        full_card_.ClearCustomStyle().SetRole(UiRole::Accent);
        UiMediaCard::Style full_style = full_card_.GetStyle();
        full_style.media_aspect = Size(16, 9);
        full_style.metrics.face_enabled = true;
        full_style.metrics.frame_enabled = true;
        full_style.header_style.metrics.face_enabled = true;

        full_card_.SetCustomStyle(full_style)
                  .SetImage(MakePreviewImage(3))
                  .SetHeader("Full-surface sample", "Static — header face enabled")
                  .SetFooter("Media + footer", "Card face / border enabled")
                  .SetSelectable(false)
                  .ClearTags()
                  .ClearOverlay();

        UiTagData state("READY", UiRole::Accent, UiTagVariant::Soft);
        full_card_.AddTopTag(state, UiAlign::RIGHT);
    }

    void ApplyProjection()
    {
        UiRole role =
            ParseRole(AsString(InspectorValue("role", "Standard")));

        card_.ClearCustomStyle().SetRole(role);
        UiMediaCard::Style style = card_.GetStyle();

        style.media_fit =
            ParseFit(AsString(InspectorValue("fit", "Cover")));
        style.media_aspect =
            ParseAspect(AsString(InspectorValue("aspect", "16:9")));

        SyncInheritedOverrides(style);
        ApplyOverrides(style);
        card_.SetCustomStyle(style);

        if(AsString(InspectorValue("media", "Preview image"))
           == "Empty state")
            card_.ClearImage();
        else
            card_.SetImage(preview_image_);

        card_.SetEmptyCue(
                 AsString(InspectorValue("empty_text", "+")))
             .SetSelected(
                 (bool)InspectorValue("selected", false))
             .SetSelectable(
                 (bool)InspectorValue("selectable", true));

        if((bool)InspectorValue("header.show", true))
            card_.SetHeader(
                AsString(InspectorValue("header.title", "Image processing")),
                AsString(InspectorValue("header.subtitle", "Harbour / convert EXR")),
                AsString(InspectorValue("header.metadata", "")));
        else
            card_.ClearHeader();

        if((bool)InspectorValue("footer.show", true))
            card_.SetFooter(
                AsString(InspectorValue("footer.title", "EXR / JPEG")),
                AsString(InspectorValue("footer.subtitle", "v012")),
                AsString(InspectorValue("footer.metadata", "1536 x 864")));
        else
            card_.ClearFooter();

        card_.Enable((bool)InspectorValue("enabled", true));

        ConfigureTagsAndOverlay();
        ConfigureSamples();

        UpdateGeneratedCode();
        UpdateThemeIcon();

        gallery_.RefreshLayout();
        RefreshLayout();
        Refresh();
    }

    void UpdateGeneratedCode()
    {
        String role = AsString(InspectorValue("role", "Standard"));
        String fit = AsString(InspectorValue("fit", "Cover"));
        Size aspect =
            ParseAspect(AsString(InspectorValue("aspect", "16:9")));

        String out;
        out << "UiMediaCard card;\n"
            << "card.SetRole(UiRole::" << role << ");\n"
            << "UiMediaCard::Style style = card.GetStyle();\n"
            << "style.media_fit = UiMediaFit::" << fit << ";\n"
            << Format("style.media_aspect = Size(%d, %d);\n",
                      aspect.cx, aspect.cy);

        auto emit_bool = [&](const char *id, const char *target) {
            if(OverrideActive(id))
                out << target << " = "
                    << ((bool)OverrideValue(id) ? "true" : "false")
                    << ";\n";
        };
        auto emit_color = [&](const char *id, const char *target) {
            if(OverrideActive(id))
                out << target << " = "
                    << CppColor(Color(OverrideValue(id))) << ";\n";
        };
        auto emit_face = [&](const char *id, const char *target) {
            if(OverrideActive(id))
                out << target << " = UiFill::Solid("
                    << CppColor(Color(OverrideValue(id))) << ");\n";
        };
        auto emit_int = [&](const char *id, const char *target) {
            if(OverrideActive(id))
                out << target << " = "
                    << (int)OverrideValue(id) << ";\n";
        };

        emit_bool("card.background", "style.metrics.face_enabled");
        emit_bool("card.border", "style.metrics.frame_enabled");
        emit_face("card.face", "style.palette.face[ST_NORMAL]");
        emit_color("card.frame", "style.palette.frame[ST_NORMAL]");
        emit_int("card.radius", "style.metrics.radius");

        emit_bool("media.background", "style.media_metrics.face_enabled");
        emit_bool("media.border", "style.media_metrics.frame_enabled");
        emit_face("media.face", "style.media_palette.face[ST_NORMAL]");
        emit_color("media.frame", "style.media_palette.frame[ST_NORMAL]");
        emit_int("media.radius", "style.media_metrics.radius");

        emit_bool("header.background", "style.header_style.metrics.face_enabled");
        emit_bool("header.border", "style.header_style.metrics.frame_enabled");
        emit_face("header.face", "style.header_style.palette.face[ST_NORMAL]");
        emit_color("header.frame", "style.header_style.palette.frame[ST_NORMAL]");

        emit_bool("footer.background", "style.footer_style.metrics.face_enabled");
        emit_bool("footer.border", "style.footer_style.metrics.frame_enabled");
        emit_face("footer.face", "style.footer_style.palette.face[ST_NORMAL]");
        emit_color("footer.frame", "style.footer_style.palette.frame[ST_NORMAL]");

        emit_int("section.gap", "style.section_gap");
        emit_int("tag.inset", "style.tag_inset");
        emit_int("tag.gap", "style.tag_gap");

        if(OverrideActive("tag.radius"))
            out << "for(int i = 0; i < 4; i++) "
                << "style.tag_style[i].metrics.radius = "
                << (int)OverrideValue("tag.radius") << ";\n";
        if(OverrideActive("overlay.radius"))
            out << "for(int i = 0; i < 4; i++) "
                << "style.overlay_style[i].metrics.radius = "
                << (int)OverrideValue("overlay.radius") << ";\n";

        out << "card.SetCustomStyle(style);\n";

        if((bool)InspectorValue("header.show", true))
            out << "card.SetHeader("
                << CppString(AsString(InspectorValue("header.title", ""))) << ", "
                << CppString(AsString(InspectorValue("header.subtitle", ""))) << ", "
                << CppString(AsString(InspectorValue("header.metadata", ""))) << ");\n";
        else
            out << "card.ClearHeader();\n";

        if(AsString(InspectorValue("media", "Preview image")) == "Empty state")
            out << "card.ClearImage().SetEmptyCue("
                << CppString(AsString(InspectorValue("empty_text", "+"))) << ");\n";
        else
            out << "card.SetImage(image).SetEmptyCue("
                << CppString(AsString(InspectorValue("empty_text", "+"))) << ");\n";

        out << "card.ClearTags().ClearOverlay();\n";

        if((bool)InspectorValue("tags.top", true)) {
            out << "UiTagData kind(\"IMAGE\", UiRole::Subtle, UiTagVariant::Filled);\n"
                << "kind.id = \"kind\";\n"
                << "card.AddTopTag(kind, UiAlign::LEFT);\n"
                << "UiTagData ready(\"READY\", UiRole::Accent, UiTagVariant::Soft);\n"
                << "ready.id = \"ready\";\n"
                << "ready.interactive = true;\n"
                << "ready.value = \"ready\";\n"
                << "card.AddTopTag(ready, UiAlign::RIGHT);\n";
        }

        if((bool)InspectorValue("tags.bottom", true))
            out << "UiTagData take(\"take 03\", UiRole::Standard, UiTagVariant::Soft);\n"
                << "take.id = \"take\";\n"
                << "card.AddBottomTag(take, UiAlign::RIGHT);\n";

        if((bool)InspectorValue("overlay.show", false)) {
            UiAlign h, v;
            ParseOverlayPosition(
                AsString(InspectorValue("overlay.position", "Center")), h, v);
            out << "UiTagData overlay("
                << CppString(AsString(InspectorValue("overlay.text", "")))
                << ", UiRole::Accent, UiTagVariant::Filled);\n"
                << "overlay.id = \"overlay\";\n"
                << "card.SetOverlay(overlay, UiAlign::"
                << AlignCode(h) << ", UiAlign::" << AlignCode(v) << ");\n";
        }

        if((bool)InspectorValue("footer.show", true))
            out << "card.SetFooter("
                << CppString(AsString(InspectorValue("footer.title", ""))) << ", "
                << CppString(AsString(InspectorValue("footer.subtitle", ""))) << ", "
                << CppString(AsString(InspectorValue("footer.metadata", ""))) << ");\n";
        else
            out << "card.ClearFooter();\n";

        out << "card.SetSelected("
            << ((bool)InspectorValue("selected", false) ? "true" : "false")
            << ");\n"
            << "card.SetSelectable("
            << ((bool)InspectorValue("selectable", true) ? "true" : "false")
            << ");\n"
            << "card.Enable("
            << ((bool)InspectorValue("enabled", true) ? "true" : "false")
            << ");\n";

        generated_ = out;
        code_.SetData(generated_);
    }

    void SelectPage(int page)
    {
        page = minmax(page, 0, 2);
        pages_.SetActivePage(page);
        inspector_mode_.SetChecked(page == 0);
        overrides_mode_.SetChecked(page == 1);
        code_mode_.SetChecked(page == 2);
    }

    void ToggleTheme()
    {
        UiThemeContext ctx = UiTheme::GetContext();
        ctx.mode = ctx.mode == UiThemeMode::Dark
                 ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(ctx);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyProjection();
    }

    void ApplyTheme()
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        window_face_ = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
        header_.SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
        UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
        const Color panel_face = dark ? Color(18, 18, 18) : Color(245, 245, 245);
        surface.transparent = false;
        surface.metrics.face_enabled = true;
        surface.metrics.frame_enabled = true;
        surface.metrics.frame_width = DPI(1);
        surface.metrics.radius = DPI(8);
        surface.metrics.shadow.enabled = false;
        surface.metrics.focus_enabled = false;
        for(int state = 0; state < 4; state++) {
            surface.palette.face[state] = UiFill::Solid(panel_face);
            surface.palette.frame[state] = dark ? Color(48, 48, 48) : Color(220, 220, 220);
        }
        preview_.SetCustomStyle(surface);
        right_.SetCustomStyle(surface);
        UiPanel::Style page_style = surface;
        page_style.transparent = true;
        page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
        for(UiPanel* panel : { &inspector_page_, &overrides_page_, &code_page_ })
            panel->SetCustomStyle(page_style);
        for(UiLabel* label : { &editable_label_, &samples_label_, &gallery_label_, &caption_ })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        inspector_.SetPaletteMode(mode);
        overrides_.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &inspector_, &overrides_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_, &help_, &exit_, &inspector_mode_, &overrides_mode_, &code_mode_, &copy_ }) {
            UiToolButton::Style style = UiTheme::ResolveToolButton(UiRole::Standard);
            style.transparent = true;
            style.metrics.face_enabled = style.metrics.frame_enabled = false;
            style.metrics.focus_enabled = false;
            style.metrics.shadow.enabled = false;
            style.underline = false;
            for(int state = 0; state < 4; state++) {
                style.palette.face[state] = UiFill::None();
                style.palette.frame[state] = Null;
            }
            const Color neutral = dark ? Color(180, 180, 180) : Color(110, 110, 110);
            style.palette.icon[ST_NORMAL] = neutral;
            style.palette.icon[ST_HOT] = dark ? White() : Color(32, 32, 32);
            style.palette.icon[ST_PRESSED] = Color(0, 120, 212);
            style.palette.icon[ST_DISABLED] = Blend(neutral, panel_face, 150);
            button->SetCustomStyle(style);
        }
        UiToolButton::Style exit_style = exit_.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        exit_.SetCustomStyle(exit_style);
        Refresh();
    }

    void UpdateThemeIcon()
    {
        theme_.SetIcon(UiTheme::GetContext().mode == UiThemeMode::Dark
                     ? ICON_ACTION_LIGHT_MODE_48()
                     : ICON_ACTION_DARK_MODE_48());
    }

private:
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_, override_model_;
    UiListModel gallery_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_, help_, exit_;

    UiPanel preview_;
    UiLabel editable_label_, samples_label_, gallery_label_;
    UiMediaCard card_, empty_card_, full_card_;
    UiGallery gallery_;
    UiMediaCardRender gallery_render_;
    UiLabel caption_;
    Image preview_image_ = MakePreviewImage();

    UiPanel right_;
    UiBoxLayout tools_ { UiDirection::H };
    UiToolButton inspector_mode_, overrides_mode_, code_mode_;
    UiStack pages_;
    UiPanel inspector_page_, overrides_page_, code_page_;
    PropertyEditor inspector_, overrides_;
    UiMultiEdit code_;
    UiToolButton copy_;

    String generated_;
    Color window_face_ = SColorFace();
};

} // namespace

GUI_APP_MAIN
{
    UiMediaCardDemo().Run();
}
