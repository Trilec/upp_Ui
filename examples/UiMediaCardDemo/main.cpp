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

UiAlign ParseSide(const String& s)
{
    if(s == "Top") return UiAlign::TOP;
    if(s == "Left") return UiAlign::LEFT;
    if(s == "Right") return UiAlign::RIGHT;
    if(s == "None") return UiAlign::DEFAULT;
    return UiAlign::BOTTOM;
}

Size ParseAspect(const String& s)
{
    if(s == "4:3") return Size(4, 3);
    if(s == "3:2") return Size(3, 2);
    if(s == "16:9") return Size(16, 9);
    return Size(1, 1);
}

String SideCode(const String& s)
{
    if(s == "Top") return "TOP";
    if(s == "Left") return "LEFT";
    if(s == "Right") return "RIGHT";
    if(s == "None") return "DEFAULT";
    return "BOTTOM";
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

Image MakePreviewImage()
{
    const Size size(960, 540);
    ImageBuffer out(size);

    for(int y = 0; y < size.cy; y++) {
        RGBA *row = out[y];
        for(int x = 0; x < size.cx; x++) {
            int r = 34 + x * 54 / size.cx + y * 16 / size.cy;
            int g = 48 + x * 34 / size.cx + y * 42 / size.cy;
            int b = 74 + (size.cx - x) * 58 / size.cx + y * 22 / size.cy;
            if(y > 330) {
                r = 27 + x * 16 / size.cx;
                g = 31 + x * 14 / size.cx;
                b = 39 + x * 18 / size.cx;
            }
            row[x] = RGBA(Color(clamp(r, 0, 255),
                                clamp(g, 0, 255),
                                clamp(b, 0, 255)));
            row[x].a = 255;
        }
    }

    for(int y = 130; y < 260; y++) {
        RGBA *row = out[y];
        for(int x = 670; x < 800; x++) {
            int dx = x - 735;
            int dy = y - 195;
            if(dx * dx + dy * dy < 58 * 58) {
                row[x] = RGBA(Color(221, 157, 86));
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
        SetRect(0, 0, DPI(1280), DPI(800));

        UiThemeContext ctx = UiTheme::GetContext();
        ctx.preset = UiThemePreset::Minimal;
        ctx.mode = UiThemeMode::Light;
        UiTheme::Set(ctx);

        RegisterPropertyEditorV1Editors(factory_);

        BuildHeader();
        BuildPreview();
        BuildRightRail();
        BuildInspector();
        BuildOverrides();
        ConfigureEditors();
        ConnectEvents();

        SelectPage(0);
        ApplyProjection();
    }

    void Layout() override
    {
        Rect r = GetSize();
        r.Deflate(DPI(12));

        header_.SetRect(r.left, r.top, r.GetWidth(), DPI(68));

        const int top = r.top + DPI(80);
        const int body_h = max(0, r.bottom - top);
        const int rail_w = min(DPI(430), max(DPI(360), r.GetWidth() / 3));
        const int gap = DPI(12);
        const int preview_w = max(0, r.GetWidth() - rail_w - gap);

        preview_.SetRect(r.left, top, preview_w, body_h);
        right_.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

        Size ps = preview_.GetSize();
        const int caption_h = DPI(52);
        Rect area = RectC(DPI(20), DPI(18),
                          max(0, ps.cx - DPI(40)),
                          max(0, ps.cy - caption_h - DPI(24)));

        const int wanted_w = max(DPI(150), (int)InspectorValue("width", 340));
        const int wanted_h = max(DPI(150), (int)InspectorValue("height", 390));
        const int sample_gap = DPI(18);
        const int sample_w = min(DPI(220), max(DPI(160), area.GetWidth() / 3));
        const int main_max_w = max(DPI(160), area.GetWidth() - sample_w - sample_gap);
        const int main_w = min(wanted_w, main_max_w);
        const int main_h = min(wanted_h, area.GetHeight());

        card_.SetRect(area.left,
                      area.top + max(0, (area.GetHeight() - main_h) / 2),
                      main_w, main_h);

        const int sample_x = area.left + main_w + sample_gap;
        empty_card_.SetRect(sample_x,
                            area.top + max(0, (area.GetHeight() - DPI(205)) / 2),
                            max(0, area.right - sample_x),
                            min(DPI(205), area.GetHeight()));

        caption_.SetRect(0, max(0, ps.cy - caption_h), ps.cx, caption_h);

        Size rs = right_.GetSize();
        tools_.SetRect(DPI(4), DPI(4), max(0, rs.cx - DPI(8)), DPI(36));
        pages_.SetRect(DPI(4), DPI(44),
                       max(0, rs.cx - DPI(8)),
                       max(0, rs.cy - DPI(48)));
    }

private:
    void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiMediaCard")
               .SetSubTitle("Media-first card with PropertyEditor, semantic badges and prepared image geometry")
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);

        header_actions_.SetGap(DPI(4)).SetInset(0)
                       .SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);

        theme_.SetIcon(ICON_ACTION_LIGHT_MODE_48())
              .SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
        help_.SetIcon(ICON_DESIGN_HELP_48())
             .SetIconSize(DPI(16), DPI(16)).Tip("About this demo");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48())
             .SetIconSize(DPI(16), DPI(16)).Tip("Close demo");

        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
    }

    void BuildPreview()
    {
        Add(preview_);
        preview_.Add(card_);
        preview_.Add(empty_card_);
        preview_.Add(caption_);

        caption_.SetText("Inspector drives the main card. The smaller card shows the same control in its empty reference state.")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildRightRail()
    {
        Add(right_);
        right_.Add(tools_);
        right_.Add(pages_);

        tools_.SetGap(DPI(4)).SetInset(Rect(DPI(2), 0, DPI(2), 0))
              .SetAlignItems(UiCrossAlign::Center);

        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetCheckable().Tip("Inspector");
        overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                       .SetIconSize(DPI(17), DPI(17))
                       .SetCheckable().Tip("Theme Overrides");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48())
                  .SetIconSize(DPI(17), DPI(17))
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
        code_.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
        code_.SetReadOnly();
        code_page_.Add(copy_.RightPos(DPI(8), DPI(32)).TopPos(DPI(6), DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(16), DPI(16)).Tip("Copy generated C++");
    }

    void BuildInspector()
    {
        inspector_model_.AddText("title", "Title", "SH030 take 03", "Content");
        inspector_model_.AddText("subtitle", "Subtitle", "1536 x 864 - Flux", "Content");
        inspector_model_.AddText("metadata", "Metadata", "AURORA / SQ020", "Content");
        inspector_model_.AddChoice("media", "Media", "Preview image", "Content")
                        .AddChoice("Preview image", "Preview image")
                        .AddChoice("Empty state", "Empty state");
        inspector_model_.AddText("empty_text", "Empty cue", "+", "Content");

        inspector_model_.AddChoice("role", "Role", "Standard", "Presentation")
                        .AddChoice("Standard", "Standard")
                        .AddChoice("Subtle", "Subtle")
                        .AddChoice("Accent", "Accent")
                        .AddChoice("Alert", "Alert");
        inspector_model_.AddChoice("fit", "Media fit", "Cover", "Presentation")
                        .AddChoice("Cover", "Cover")
                        .AddChoice("Contain", "Contain");
        inspector_model_.AddChoice("label_side", "Label side", "Bottom", "Presentation")
                        .AddChoice("Bottom", "Bottom")
                        .AddChoice("Top", "Top")
                        .AddChoice("Left", "Left")
                        .AddChoice("Right", "Right")
                        .AddChoice("None", "None");
        inspector_model_.AddChoice("aspect", "Media aspect", "1:1", "Presentation")
                        .AddChoice("1:1", "1:1")
                        .AddChoice("4:3", "4:3")
                        .AddChoice("3:2", "3:2")
                        .AddChoice("16:9", "16:9");
        inspector_model_.AddBoolean("badges", "Show badges", true, "Presentation");

        inspector_model_.AddNumericInt("width", "Preview width", 340, 150, 620, 1, "Layout").SetUnit("px");
        inspector_model_.AddNumericInt("height", "Preview height", 390, 150, 620, 1, "Layout").SetUnit("px");

        inspector_model_.AddBoolean("selected", "Selected", false, "Behaviour");
        inspector_model_.AddBoolean("selectable", "Selectable", true, "Behaviour");
        inspector_model_.AddBoolean("enabled", "Enabled", true, "Behaviour");

        inspector_model_.SetGroupSubtitle("Content", "display data only; file/import ownership remains outside UiMediaCard");
        inspector_model_.SetGroupSubtitle("Presentation", "authored composition layered over the current semantic theme role");
        inspector_model_.SetGroupSubtitle("Behaviour", "card-level interaction; badges can independently opt into actions");
        inspector_model_.StructureChanged();
    }

    void BuildOverrides()
    {
        UiMediaCard probe;
        UiMediaCard::Style base = probe.GetStyle();

        MarkOverride(override_model_.AddColor("card.face", "Face",
            FaceColor(base.palette, ST_NORMAL, Color(250,250,251)), "Card Surface"));
        MarkOverride(override_model_.AddColor("card.frame", "Frame",
            base.palette.frame[ST_NORMAL], "Card Surface"));
        MarkOverride(override_model_.AddNumericInt("card.radius", "Radius",
            base.metrics.radius, 0, 40, 1, "Card Surface").SetUnit("px"));

        MarkOverride(override_model_.AddColor("media.face", "Face",
            FaceColor(base.media_palette, ST_NORMAL, Color(234,237,241)), "Media Surface"));
        MarkOverride(override_model_.AddColor("media.frame", "Frame",
            base.media_palette.frame[ST_NORMAL], "Media Surface"));
        MarkOverride(override_model_.AddNumericInt("media.radius", "Radius",
            base.media_metrics.radius, 0, 40, 1, "Media Surface").SetUnit("px"));

        MarkOverride(override_model_.AddColor("title.ink", "Title ink",
            base.title_ink[ST_NORMAL], "Typography"));
        MarkOverride(override_model_.AddColor("subtitle.ink", "Subtitle ink",
            base.subtitle_ink[ST_NORMAL], "Typography"));
        MarkOverride(override_model_.AddColor("metadata.ink", "Metadata ink",
            base.metadata_ink[ST_NORMAL], "Typography"));
        MarkOverride(override_model_.AddNumericInt("title.height", "Title size",
            base.title_font.GetHeight(), 7, 32, 1, "Typography").SetUnit("px"));

        MarkOverride(override_model_.AddNumericInt("spacing.media_text", "Media / text",
            base.media_text_gap, 0, 30, 1, "Spacing").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt("badge.inset", "Badge inset",
            base.badge_inset, 0, 30, 1, "Badges").SetUnit("px"));
        MarkOverride(override_model_.AddNumericInt("badge.radius", "Badge radius",
            base.badge_style[(int)UiRole::Standard].metrics.radius,
            0, 20, 1, "Badges").SetUnit("px"));

        override_model_.SetGroupSubtitle("Card Surface", "outer themed interaction surface");
        override_model_.SetGroupSubtitle("Media Surface", "media well behind image, empty state and badges");
        override_model_.SetGroupSubtitle("Badges", "painted overlay presentations rather than child controls");
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

    void ConnectEvents()
    {
        auto changed = [=](String, Value) { ApplyProjection(); };
        inspector_.WhenPreview = changed;
        inspector_.WhenCommit = changed;
        overrides_.WhenPreview = changed;
        overrides_.WhenCommit = changed;

        inspector_.WhenReset = [=](String id) { ResetProperty(inspector_model_, id); };
        overrides_.WhenReset = [=](String id) { ResetProperty(override_model_, id); };
        overrides_.WhenOverride = [=](String id, bool active) { SetOverrideActive(id, active); };

        inspector_mode_.WhenAction = [=] { SelectPage(0); };
        overrides_mode_.WhenAction = [=] { SelectPage(1); };
        code_mode_.WhenAction = [=] { SelectPage(2); };

        theme_.WhenAction = [=] { ToggleTheme(); };
        help_.WhenAction = [=] {
            PromptOK("UiMediaCard reference demo\n\nInspector authors content, layout and interaction. Theme Overrides remain inherited until explicitly enabled. Code is regenerated from the same PropertyEditor state.");
        };
        exit_.WhenAction = [=] { Break(); };
        copy_.WhenAction = [=] { WriteClipboardText(generated_); };

        card_.WhenAction = [=] {
            caption_.SetText("Card action fired. Drag/release now follows U++ capture teardown without recursive CancelMode.");
        };
        card_.WhenBadgeAction = [=](String id, Value) {
            caption_.SetText("Badge action: " + id);
        };
    }

    Value InspectorValue(const String& id, const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = inspector_model_.Find(id);
        return item ? item->value : fallback;
    }

    Value OverrideValue(const String& id, const Value& fallback = Value()) const
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

        set("card.face", FaceColor(style.palette, ST_NORMAL, Color(250,250,251)));
        set("card.frame", style.palette.frame[ST_NORMAL]);
        set("card.radius", style.metrics.radius);
        set("media.face", FaceColor(style.media_palette, ST_NORMAL, Color(234,237,241)));
        set("media.frame", style.media_palette.frame[ST_NORMAL]);
        set("media.radius", style.media_metrics.radius);
        set("title.ink", style.title_ink[ST_NORMAL]);
        set("subtitle.ink", style.subtitle_ink[ST_NORMAL]);
        set("metadata.ink", style.metadata_ink[ST_NORMAL]);
        set("title.height", style.title_font.GetHeight());
        set("spacing.media_text", style.media_text_gap);
        set("badge.inset", style.badge_inset);
        set("badge.radius", style.badge_style[(int)UiRole::Standard].metrics.radius);
        overrides_.RefreshModel();
    }

    void ApplyOverrides(UiMediaCard::Style& style) const
    {
        auto face = [&](const char *id, StyledPalette& palette) {
            if(OverrideActive(id))
                palette.face[ST_NORMAL] = UiFill::Solid(Color(OverrideValue(id)));
        };
        auto color = [&](const char *id, Color& target) {
            if(OverrideActive(id))
                target = Color(OverrideValue(id));
        };

        face("card.face", style.palette);
        color("card.frame", style.palette.frame[ST_NORMAL]);
        if(OverrideActive("card.radius"))
            style.metrics.radius = max(0, (int)OverrideValue("card.radius"));

        face("media.face", style.media_palette);
        color("media.frame", style.media_palette.frame[ST_NORMAL]);
        if(OverrideActive("media.radius"))
            style.media_metrics.radius = max(0, (int)OverrideValue("media.radius"));

        color("title.ink", style.title_ink[ST_NORMAL]);
        color("subtitle.ink", style.subtitle_ink[ST_NORMAL]);
        color("metadata.ink", style.metadata_ink[ST_NORMAL]);
        if(OverrideActive("title.height"))
            style.title_font.Height(max(7, (int)OverrideValue("title.height")));

        if(OverrideActive("spacing.media_text"))
            style.media_text_gap = max(0, (int)OverrideValue("spacing.media_text"));
        if(OverrideActive("badge.inset"))
            style.badge_inset = max(0, (int)OverrideValue("badge.inset"));
        if(OverrideActive("badge.radius")) {
            int radius = max(0, (int)OverrideValue("badge.radius"));
            for(int i = 0; i < 4; i++)
                style.badge_style[i].metrics.radius = radius;
        }
    }

    void ConfigureBadges()
    {
        card_.ClearBadges();
        if(!(bool)InspectorValue("badges", true))
            return;

        UiBadgeData kind("IMAGE", UiRole::Subtle, UiBadgeVariant::Filled);
        kind.id = "kind";
        card_.AddBadge(kind, UiMediaBadgeAnchor::TopLeft);

        UiBadgeData ready("READY", UiRole::Accent, UiBadgeVariant::Soft);
        ready.id = "ready";
        ready.actionable = true;
        ready.value = "ready";
        card_.AddBadge(ready, UiMediaBadgeAnchor::TopRight);

        UiBadgeData take("take 03", UiRole::Standard, UiBadgeVariant::Soft);
        take.id = "take";
        card_.AddBadge(take, UiMediaBadgeAnchor::BottomRight);
    }

    void ConfigureEmptyExample()
    {
        empty_card_.ClearCustomStyle().SetRole(UiRole::Subtle);
        UiMediaCard::Style style = empty_card_.GetStyle();
        style.label_side = UiAlign::BOTTOM;
        style.media_fit = UiMediaFit::Contain;
        style.media_aspect = Size(1, 1);

        empty_card_.SetCustomStyle(style)
                   .ClearImage()
                   .SetEmptyCue("+")
                   .SetTitle("Reference 1")
                   .SetSubTitle("Drop or choose media")
                   .SetMetadata("")
                   .SetSelected(false)
                   .SetSelectable(true)
                   .ClearBadges();

        UiBadgeData kind("IMAGE", UiRole::Subtle, UiBadgeVariant::Outline);
        empty_card_.AddBadge(kind, UiMediaBadgeAnchor::TopLeft);
    }

    void ApplyProjection()
    {
        UiRole role = ParseRole(AsString(InspectorValue("role", "Standard")));

        card_.ClearCustomStyle().SetRole(role);
        UiMediaCard::Style style = card_.GetStyle();
        style.media_fit = ParseFit(AsString(InspectorValue("fit", "Cover")));
        style.label_side = ParseSide(AsString(InspectorValue("label_side", "Bottom")));
        style.media_aspect = ParseAspect(AsString(InspectorValue("aspect", "1:1")));

        SyncInheritedOverrides(style);
        ApplyOverrides(style);
        card_.SetCustomStyle(style);

        if(AsString(InspectorValue("media", "Preview image")) == "Empty state")
            card_.ClearImage();
        else
            card_.SetImage(preview_image_);

        card_.SetEmptyCue(AsString(InspectorValue("empty_text", "+")))
             .SetTitle(AsString(InspectorValue("title", "SH030 take 03")))
             .SetSubTitle(AsString(InspectorValue("subtitle", "1536 x 864 - Flux")))
             .SetMetadata(AsString(InspectorValue("metadata", "AURORA / SQ020")))
             .SetSelected((bool)InspectorValue("selected", false))
             .SetSelectable((bool)InspectorValue("selectable", true));

        card_.Enable((bool)InspectorValue("enabled", true));
        ConfigureBadges();
        ConfigureEmptyExample();

        UpdateGeneratedCode();
        UpdateThemeIcon();
        RefreshLayout();
        Refresh();
    }

    void UpdateGeneratedCode()
    {
        String role = AsString(InspectorValue("role", "Standard"));
        String fit = AsString(InspectorValue("fit", "Cover"));
        String side = AsString(InspectorValue("label_side", "Bottom"));
        Size aspect = ParseAspect(AsString(InspectorValue("aspect", "1:1")));

        String out;
        out << "UiMediaCard card;\n"
            << "card.SetRole(UiRole::" << role << ");\n"
            << "UiMediaCard::Style style = card.GetStyle();\n"
            << "style.media_fit = UiMediaFit::" << fit << ";\n"
            << "style.label_side = UiAlign::" << SideCode(side) << ";\n"
            << Format("style.media_aspect = Size(%d, %d);\n", aspect.cx, aspect.cy);

        if(OverrideActive("card.face"))
            out << "style.palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("card.face"))) << ");\n";
        if(OverrideActive("card.frame"))
            out << "style.palette.frame[ST_NORMAL] = " << CppColor(Color(OverrideValue("card.frame"))) << ";\n";
        if(OverrideActive("card.radius"))
            out << Format("style.metrics.radius = %d;\n", (int)OverrideValue("card.radius"));
        if(OverrideActive("media.face"))
            out << "style.media_palette.face[ST_NORMAL] = UiFill::Solid(" << CppColor(Color(OverrideValue("media.face"))) << ");\n";
        if(OverrideActive("media.frame"))
            out << "style.media_palette.frame[ST_NORMAL] = " << CppColor(Color(OverrideValue("media.frame"))) << ";\n";
        if(OverrideActive("media.radius"))
            out << Format("style.media_metrics.radius = %d;\n", (int)OverrideValue("media.radius"));
        if(OverrideActive("title.ink"))
            out << "style.title_ink[ST_NORMAL] = " << CppColor(Color(OverrideValue("title.ink"))) << ";\n";
        if(OverrideActive("subtitle.ink"))
            out << "style.subtitle_ink[ST_NORMAL] = " << CppColor(Color(OverrideValue("subtitle.ink"))) << ";\n";
        if(OverrideActive("metadata.ink"))
            out << "style.metadata_ink[ST_NORMAL] = " << CppColor(Color(OverrideValue("metadata.ink"))) << ";\n";
        if(OverrideActive("title.height"))
            out << Format("style.title_font.Height(%d);\n", (int)OverrideValue("title.height"));
        if(OverrideActive("spacing.media_text"))
            out << Format("style.media_text_gap = %d;\n", (int)OverrideValue("spacing.media_text"));
        if(OverrideActive("badge.inset"))
            out << Format("style.badge_inset = %d;\n", (int)OverrideValue("badge.inset"));
        if(OverrideActive("badge.radius"))
            out << Format("for(int i = 0; i < 4; i++) style.badge_style[i].metrics.radius = %d;\n",
                          (int)OverrideValue("badge.radius"));

        out << "card.SetCustomStyle(style)\n"
            << "    .SetTitle(" << CppString(AsString(InspectorValue("title", String()))) << ")\n"
            << "    .SetSubTitle(" << CppString(AsString(InspectorValue("subtitle", String()))) << ")\n"
            << "    .SetMetadata(" << CppString(AsString(InspectorValue("metadata", String()))) << ");\n";

        if(AsString(InspectorValue("media", "Preview image")) == "Empty state")
            out << "card.ClearImage().SetEmptyCue(" << CppString(AsString(InspectorValue("empty_text", "+"))) << ");\n";
        else
            out << "card.SetImage(image);\n";

        if((bool)InspectorValue("badges", true)) {
            out << "\nUiBadgeData kind(\"IMAGE\", UiRole::Subtle, UiBadgeVariant::Filled);\n"
                << "card.AddBadge(kind, UiMediaBadgeAnchor::TopLeft);\n"
                << "UiBadgeData state(\"READY\", UiRole::Accent, UiBadgeVariant::Soft);\n"
                << "card.AddBadge(state, UiMediaBadgeAnchor::TopRight);\n";
        }
        if((bool)InspectorValue("selected", false))
            out << "card.SetSelected();\n";
        if(!(bool)InspectorValue("selectable", true))
            out << "card.SetSelectable(false);\n";
        if(!(bool)InspectorValue("enabled", true))
            out << "card.Enable(false);\n";

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
        ctx.mode = ctx.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(ctx);
        ApplyProjection();
    }

    void UpdateThemeIcon()
    {
        theme_.SetIcon(UiTheme::GetContext().mode == UiThemeMode::Dark
                     ? ICON_ACTION_DARK_MODE_48()
                     : ICON_ACTION_LIGHT_MODE_48());
    }

private:
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_, help_, exit_;

    UiPanel preview_;
    UiMediaCard card_, empty_card_;
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

    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_, override_model_;
    String generated_;
};

} // namespace

GUI_APP_MAIN
{
    UiMediaCardDemo().Run();
}
