// Native UiTag reference: edit semantic content and appearance, compare prepared
// tags, and generate reusable public-API C++ without a demo framework.
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

UiRole ParseRole(const String& value)
{
    if(value == "Subtle") return UiRole::Subtle;
    if(value == "Accent") return UiRole::Accent;
    if(value == "Alert") return UiRole::Alert;
    return UiRole::Standard;
}

UiTagVariant ParseVariant(const String& value)
{
    if(value == "Filled") return UiTagVariant::Filled;
    if(value == "Outline") return UiTagVariant::Outline;
    return UiTagVariant::Soft;
}

UiIconRenderMode ParseIconMode(const String& value)
{
    if(value == "PreserveColor") return UiIconRenderMode::PreserveColor;
    if(value == "Auto") return UiIconRenderMode::Auto;
    return UiIconRenderMode::MonoTint;
}

UiAlign ParseIconSide(const String& value)
{
    return value == "Right" ? UiAlign::RIGHT : UiAlign::LEFT;
}

Image ResolveDemoIcon(const String& value)
{
    if(value == "Info") return ICON_DESIGN_HELP_48();
    if(value == "Code") return ICON_DESIGN_CODE_BLOCKS_48();
    if(value == "Copy") return ICON_CONTENT_CONTENT_COPY_48();
    if(value == "Theme") return ICON_ACTION_DARK_MODE_48();
    return Image();
}

String IconCode(const String& value)
{
    if(value == "Info") return "ICON_DESIGN_HELP_48()";
    if(value == "Code") return "ICON_DESIGN_CODE_BLOCKS_48()";
    if(value == "Copy") return "ICON_CONTENT_CONTENT_COPY_48()";
    if(value == "Theme") return "ICON_ACTION_DARK_MODE_48()";
    return "Image()";
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

const char *StateKey(int st)
{
    static const char *key[4] = { "normal", "hot", "pressed", "disabled" };
    return key[clamp(st, 0, 3)];
}

const char *StateLabel(int st)
{
    static const char *label[4] = { "Normal", "Hot", "Pressed", "Disabled" };
    return label[clamp(st, 0, 3)];
}

const char *StateEnum(int st)
{
    static const char *name[4] = {
        "ST_NORMAL", "ST_HOT", "ST_PRESSED", "ST_DISABLED"
    };
    return name[clamp(st, 0, 3)];
}

String StateProperty(const char *prefix, int st, const char *suffix)
{
    return String(prefix) + "." + StateKey(st) + "." + suffix;
}

class TagPreviewCtrl : public Ctrl {
public:
    typedef TagPreviewCtrl CLASSNAME;

    TagPreviewCtrl()
    {
        BackPaint();
    }

    TagPreviewCtrl& SetMain(const UiTagData& data,
                            const UiTagStyle& style,
                            int max_width)
    {
        main_data_ = data;
        main_style_ = style;
        main_max_width_ = max(DPI(36), max_width);
        RefreshLayout();
        return *this;
    }

    Event<String, Value> WhenTagAction;

    void Layout() override
    {
        Prepare();
    }

    void Paint(Draw& w) override
    {
        Size size = GetSize();
        Color paper = SColorPaper();
        Color wash_a = Blend(paper, SColorHighlight(), 18);
        Color wash_b = Blend(paper, SColorText(), 10);

        w.DrawRect(size, paper);
        const int tile = DPI(42);
        for(int y = 0; y < size.cy; y += tile)
            for(int x = 0; x < size.cx; x += tile)
                if(((x / tile) + (y / tile)) & 1)
                    w.DrawRect(x, y,
                               min(tile, size.cx - x),
                               min(tile, size.cy - y),
                               ((x / tile) & 1) ? wash_a : wash_b);

        Font heading = StdFont().Bold();
        w.DrawText(DPI(18), DPI(14), "EDITABLE TAG", heading, SColorText());
        w.DrawText(DPI(18), DPI(112), "REFERENCE FORMS", heading, SColorText());
        w.DrawText(DPI(18), DPI(226), "DENSE PREPARED ROW — no child controls",
                   heading, SColorText());

        for(int i = 0; i < prepared_.GetCount(); i++) {
            const UiTagPresentation& tag = prepared_[i];
            StyledState state = ST_NORMAL;
            if(!tag.enabled)
                state = ST_DISABLED;
            else if(i == pressed_ && tag.IsInteractive())
                state = ST_PRESSED;
            else if(i == hot_ && tag.IsInteractive())
                state = ST_HOT;
            UiPaintTag(w, tag, state);
        }
    }

    void MouseMove(Point p, dword flags) override
    {
        int hit = Hit(p);
        if(hit >= 0 && !prepared_[hit].IsInteractive())
            hit = -1;
        if(hit != hot_) {
            hot_ = hit;
            Refresh();
        }
        Ctrl::MouseMove(p, flags);
    }

    void MouseLeave() override
    {
        if(pressed_ < 0) {
            hot_ = -1;
            Refresh();
        }
        Ctrl::MouseLeave();
    }

    void LeftDown(Point p, dword flags) override
    {
        int hit = Hit(p);
        if(hit < 0 || !prepared_[hit].enabled
           || !prepared_[hit].IsInteractive())
            return;

        pressed_ = hit;
        hot_ = hit;
        SetCapture();
        Refresh();
        Ctrl::LeftDown(p, flags);
    }

    void LeftUp(Point p, dword flags) override
    {
        if(pressed_ < 0)
            return;

        const int pressed = pressed_;
        const int hit = Hit(p);
        pressed_ = -1;

        if(HasCapture())
            ReleaseCapture();

        hot_ = hit >= 0 && prepared_[hit].IsInteractive() ? hit : -1;
        Refresh();

        if(hit == pressed && hit >= 0
           && prepared_[hit].enabled
           && prepared_[hit].IsInteractive()) {
            String id = prepared_[hit].id;
            Value value = prepared_[hit].value;
            WhenTagAction(id, value);
            return;
        }

        Ctrl::LeftUp(p, flags);
    }

    void CancelMode() override
    {
        pressed_ = -1;
        hot_ = -1;
        Refresh();
        // Capture teardown owns ReleaseCapture(); never recurse from CancelMode.
        Ctrl::CancelMode();
    }

private:
    void AddPrepared(const UiTagData& data,
                     const UiTagStyle& style,
                     Rect bounds)
    {
        UiTagPresentation p = UiPrepareTag(data, style, bounds);
        if(p.visible)
            prepared_.Add(pick(p));
    }

    void Prepare()
    {
        prepared_.Clear();
        hot_ = pressed_ = -1;

        Size size = GetSize();
        if(size.cx <= 0 || size.cy <= 0)
            return;

        Size main_natural =
            UiMeasureTag(main_data_, main_style_, main_max_width_);
        int main_w = min(main_max_width_, max(DPI(36), main_natural.cx));
        int main_h = max(DPI(20), main_natural.cy);
        AddPrepared(main_data_, main_style_,
                    RectC(DPI(18), DPI(48), main_w, main_h));

        int x = DPI(18);
        int y = DPI(146);
        const int gap = DPI(8);

        UiTagData passive("PASSIVE", UiRole::Subtle, UiTagVariant::Soft);
        UiTagStyle passive_style = UiResolveTagStyle(passive.role);
        Size ps = UiMeasureTag(passive, passive_style);
        AddPrepared(passive, passive_style, RectC(x, y, ps.cx, ps.cy));
        x += ps.cx + gap;

        UiTagData info("INFO", UiRole::Accent, UiTagVariant::Filled);
        info.icon = ICON_DESIGN_HELP_48();
        info.id = "sample-info";
        info.value = "Information tag";
        info.interactive = true;
        UiTagStyle info_style = UiResolveTagStyle(info.role);
        Size is = UiMeasureTag(info, info_style);
        AddPrepared(info, info_style, RectC(x, y, is.cx, is.cy));
        x += is.cx + gap;

        UiTagData icon_only;
        icon_only.icon = ICON_CONTENT_CONTENT_COPY_48();
        icon_only.role = UiRole::Standard;
        icon_only.variant = UiTagVariant::Outline;
        icon_only.id = "sample-icon";
        icon_only.value = "Icon-only tag";
        icon_only.interactive = true;
        UiTagStyle icon_style = UiResolveTagStyle(icon_only.role);
        icon_style.icon_size = Size(DPI(14), DPI(14));
        Size ios = UiMeasureTag(icon_only, icon_style);
        AddPrepared(icon_only, icon_style, RectC(x, y, ios.cx, ios.cy));
        x += ios.cx + gap;

        UiTagData disabled("DISABLED", UiRole::Alert, UiTagVariant::Soft);
        disabled.enabled = false;
        UiTagStyle disabled_style = UiResolveTagStyle(disabled.role);
        Size ds = UiMeasureTag(disabled, disabled_style);
        AddPrepared(disabled, disabled_style, RectC(x, y, ds.cx, ds.cy));

        UiTagData ellipsis(
            "THIS TAG IS DELIBERATELY CONSTRAINED",
            UiRole::Standard, UiTagVariant::Outline);
        UiTagStyle ellipsis_style = UiResolveTagStyle(ellipsis.role);
        AddPrepared(ellipsis, ellipsis_style,
                    RectC(DPI(18), DPI(184), DPI(150),
                          UiMeasureTag(ellipsis, ellipsis_style).cy));

        x = DPI(18);
        y = DPI(260);
        int row_h = 0;
        for(int i = 0; i < 28; i++) {
            UiRole role = (UiRole)(i % 4);
            UiTagData data(Format("T%02d", i + 1),
                           role, i % 3 == 0 ? UiTagVariant::Outline
                                           : UiTagVariant::Soft);
            UiTagStyle style = UiResolveTagStyle(role);
            Size ts = UiMeasureTag(data, style);
            if(x > DPI(18) && x + ts.cx > size.cx - DPI(18)) {
                x = DPI(18);
                y += row_h + DPI(5);
                row_h = 0;
            }
            if(y + ts.cy > size.cy - DPI(20))
                break;
            AddPrepared(data, style, RectC(x, y, ts.cx, ts.cy));
            x += ts.cx + DPI(5);
            row_h = max(row_h, ts.cy);
        }
    }

    int Hit(Point p) const
    {
        for(int i = prepared_.GetCount() - 1; i >= 0; i--)
            if(prepared_[i].bounds.Contains(p))
                return i;
        return -1;
    }

private:
    UiTagData main_data_;
    UiTagStyle main_style_;
    int main_max_width_ = DPI(220);

    Vector<UiTagPresentation> prepared_;
    int hot_ = -1;
    int pressed_ = -1;
};

class UiTagDemo : public TopWindow {
public:
    typedef UiTagDemo CLASSNAME;

    UiTagDemo()
    {
        Title("UiTag Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1240), DPI(780));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        RegisterPropertyEditorV1Editors(factory_);
        factory_.RegisterPicker("tag-demo-image",
            [=](Value& value, Ctrl *owner) { return PickImage(value, owner); });
        factory_.RegisterThumbnailProvider("tag-demo-image",
            [=](const Value& value) { return LoadImageValue(value); });

        BuildHeader();
        BuildPreview();
        BuildRightRail();
        BuildInspector();
        BuildOverrides();
        ConfigureEditors();
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
        const int rail_w = min(DPI(430), max(DPI(370), r.GetWidth() / 3));
        const int gap = DPI(12);
        const int preview_w = max(0, r.GetWidth() - rail_w - gap);

        preview_panel_.SetRect(r.left, top, preview_w, body_h);
        right_panel_.SetRect(r.left + preview_w + gap, top, rail_w, body_h);

        Size ps = preview_panel_.GetSize();
        const int caption_h = DPI(46);
        preview_.SetRect(DPI(8), DPI(8),
                         max(0, ps.cx - DPI(16)),
                         max(0, ps.cy - caption_h - DPI(12)));
        caption_.SetRect(0, max(0, ps.cy - caption_h), ps.cx, caption_h);

        Size rs = right_panel_.GetSize();
        tools_.SetRect(DPI(4), DPI(4), max(0, rs.cx - DPI(8)), DPI(36));
        pages_.SetRect(DPI(4), DPI(44),
                       max(0, rs.cx - DPI(8)),
                       max(0, rs.cy - DPI(48)));
    }

private:
    void BuildHeader()
    {
        Add(header_);
        header_.SetTitle("UiTag")
               .SetSubTitle("Ultralight prepared semantic marker: passive or host-interactive, text and/or icon")
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
             .Tip("About UiTag");
        exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Close demo");

        header_actions_.Add(theme_).Fixed(DPI(34));
        header_actions_.Add(help_).Fixed(DPI(34));
        header_actions_.Add(exit_).Fixed(DPI(34));
    }

    void BuildPreview()
    {
        Add(preview_panel_);
        preview_panel_.Add(preview_);
        preview_panel_.Add(caption_);
        caption_.SetText(
            "Click the editable tag when Interactive is enabled, or the INFO/icon-only reference tags. All tags are prepared records, not child controls.")
                .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    }

    void BuildRightRail()
    {
        Add(right_panel_);
        right_panel_.Add(tools_);
        right_panel_.Add(pages_);

        tools_.SetGap(DPI(4))
              .SetInset(Rect(DPI(2), 0, DPI(2), 0))
              .SetAlignItems(UiCrossAlign::Center);

        content_mode_.SetText("")
                     .SetIcon(ICON_DESIGN_TUNE_48())
                     .SetIconSize(DPI(17), DPI(17))
                     .SetIconSide(UiAlign::LEFT)
                     .SetCheckable().Tip("Inspector — tag data and behaviour");
        appearance_mode_.SetText("")
                        .SetIcon(ICON_DESIGN_FORMAT_PAINT_48())
                        .SetIconSize(DPI(17), DPI(17))
                        .SetIconSide(UiAlign::LEFT)
                        .SetCheckable().Tip("Theme overrides");
        code_mode_.SetText("")
                  .SetIcon(ICON_DESIGN_CODE_BLOCKS_48())
                  .SetIconSize(DPI(17), DPI(17))
                  .SetIconSide(UiAlign::LEFT)
                  .SetCheckable().Tip("Generated C++");

        tools_.Add(content_mode_).Fixed(DPI(38));
        tools_.Add(appearance_mode_).Fixed(DPI(38));
        tools_.Add(code_mode_).Fixed(DPI(38));
        tools_.AddSpacer(1).Expand(1);

        pages_.Add(content_page_, "content");
        pages_.Add(appearance_page_, "appearance");
        pages_.Add(code_page_, "code");

        content_page_.Add(content_.SizePos());
        appearance_page_.Add(appearance_.SizePos());

        code_page_.Add(code_);
        code_.HSizePos(DPI(6), DPI(6)).VSizePos(DPI(42), DPI(6));
        code_.SetReadOnly();

        code_page_.Add(copy_.RightPos(DPI(8), DPI(32))
                            .TopPos(DPI(6), DPI(30)));
        copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48())
             .SetIconSize(DPI(16), DPI(16))
             .Tip("Copy generated C++");
    }

    void BuildInspector()
    {
        content_model_.AddText(
            "text", "Text", "READY", "Content");
        content_model_.AddChoice(
            "icon", "Icon", "Info", "Content")
            .AddChoice("None", "None")
            .AddChoice("Info", "Info")
            .AddChoice("Code", "Code")
            .AddChoice("Copy", "Copy")
            .AddChoice("Theme", "Theme");

        content_model_.AddChoice(
            "role", "Role", "Accent", "Semantics")
            .AddChoice("Standard", "Standard")
            .AddChoice("Subtle", "Subtle")
            .AddChoice("Accent", "Accent")
            .AddChoice("Alert", "Alert");
        content_model_.AddChoice(
            "variant", "Variant", "Soft", "Semantics")
            .AddChoice("Soft", "Soft")
            .AddChoice("Filled", "Filled")
            .AddChoice("Outline", "Outline");

        content_model_.AddBoolean(
            "interactive", "Interactive", true, "Behaviour");
        content_model_.AddBoolean(
            "enabled", "Enabled", true, "Behaviour");

        content_model_.AddNumericInt(
            "max_width", "Max width", 230, 36, 520, 1, "Layout")
            .SetUnit("px");

        content_model_.SetGroupSubtitle(
            "Content", "text and icon are independently optional");
        content_model_.SetGroupSubtitle(
            "Semantics", "role and compact Soft/Filled/Outline presentation");
        content_model_.SetGroupSubtitle(
            "Behaviour", "interaction metadata only; the parent owns events/focus");
        content_model_.SetGroupSubtitle(
            "Layout", "bounded width proves preparation-time ellipsis");
        content_model_.StructureChanged();
    }

    void BuildOverrides()
    {
        UiTagStyle base = UiResolveTagStyle(UiRole::Accent);

        MarkOverride(appearance_model_.AddBoolean(
            "face.enabled", "Background", base.metrics.face_enabled, "Background"));
        MarkOverride(AddPropertyImage(
            appearance_model_, "face.image", "Image", String(),
            "tag-demo-image", "Background"));
        for(int st = 0; st < 4; st++) {
            MarkOverride(appearance_model_.AddColor(
                StateProperty("face", st, "color"), StateLabel(st),
                FaceColor(base.palette, (StyledState)st, SColorFace()),
                "Background colours"));
            MarkOverride(appearance_model_.AddNumericInt(
                StateProperty("face", st, "alpha"),
                String(StateLabel(st)) + " opacity",
                base.face_alpha[st], 0, 255, 1,
                "Background opacity"));
        }

        MarkOverride(appearance_model_.AddBoolean(
            "frame.enabled", "Frame", base.metrics.frame_enabled, "Frame"));
        MarkOverride(appearance_model_.AddNumericInt(
            "frame.width", "Width", base.metrics.frame_width,
            0, 12, 1, "Frame").SetUnit("px"));
        MarkOverride(appearance_model_.AddNumericInt(
            "radius", "Radius", base.metrics.radius,
            0, 32, 1, "Frame").SetUnit("px"));
        for(int st = 0; st < 4; st++) {
            MarkOverride(appearance_model_.AddColor(
                StateProperty("frame", st, "color"), StateLabel(st),
                base.palette.frame[st], "Frame colours"));
            MarkOverride(appearance_model_.AddNumericInt(
                StateProperty("frame", st, "alpha"),
                String(StateLabel(st)) + " opacity",
                base.frame_alpha[st], 0, 255, 1,
                "Frame opacity"));
        }

        MarkOverride(AddPropertyFont(
            appearance_model_, "font.face", "Font",
            base.font.GetFaceName(), "Typography"));
        MarkOverride(appearance_model_.AddNumericInt(
            "font.height", "Size", max(1, base.font.GetHeight()),
            6, 64, 1, "Typography").SetUnit("px"));
        MarkOverride(appearance_model_.AddBoolean(
            "font.bold", "Bold", base.font.IsBold(), "Typography"));
        MarkOverride(appearance_model_.AddBoolean(
            "font.italic", "Italic", base.font.IsItalic(), "Typography"));
        for(int st = 0; st < 4; st++)
            MarkOverride(appearance_model_.AddColor(
                StateProperty("ink", st, "color"), StateLabel(st),
                base.palette.ink[st], "Text colours"));

        MarkOverride(appearance_model_.AddNumericInt(
            "padding.x", "Horizontal", base.metrics.content_margin.left,
            0, 24, 1, "Padding").SetUnit("px"));
        MarkOverride(appearance_model_.AddNumericInt(
            "padding.y", "Vertical", base.metrics.content_margin.top,
            0, 16, 1, "Padding").SetUnit("px"));
        MarkOverride(appearance_model_.AddNumericInt(
            "content.gap", "Icon/text gap", base.content_gap,
            0, 20, 1, "Padding").SetUnit("px"));

        MarkOverride(appearance_model_.AddNumericInt(
            "icon.width", "Width", base.icon_size.cx,
            0, 64, 1, "Icon").SetUnit("px"));
        MarkOverride(appearance_model_.AddNumericInt(
            "icon.height", "Height", base.icon_size.cy,
            0, 64, 1, "Icon").SetUnit("px"));
        MarkOverride(appearance_model_.AddChoice(
            "icon.side", "Side",
            base.icon_side == UiAlign::RIGHT ? "Right" : "Left", "Icon")
            .AddChoice("Left", "Left")
            .AddChoice("Right", "Right"));
        MarkOverride(appearance_model_.AddChoice(
            "icon.mode", "Render mode", "MonoTint", "Icon")
            .AddChoice("MonoTint", "MonoTint")
            .AddChoice("PreserveColor", "PreserveColor")
            .AddChoice("Auto", "Auto"));
        for(int st = 0; st < 4; st++)
            MarkOverride(appearance_model_.AddColor(
                StateProperty("icon", st, "color"), StateLabel(st),
                UiResolveIconColor(base.palette, (StyledState)st),
                "Icon tint colours"));

        MarkOverride(appearance_model_.AddNumericInt(
            "soft.alpha", "Soft opacity cap", base.soft_face_alpha,
            0, 255, 1, "Variant"));

        appearance_model_.SetGroupSubtitle(
            "Background", "face visibility plus optional shared image UiFill");
        appearance_model_.SetGroupSubtitle(
            "Background colours", "Normal / Hot / Pressed / Disabled face colours");
        appearance_model_.SetGroupSubtitle(
            "Background opacity", "true prepared alpha over arbitrary underlying media");
        appearance_model_.SetGroupSubtitle(
            "Frame", "shared frame visibility, width and radius");
        appearance_model_.SetGroupSubtitle(
            "Frame colours", "state-aware frame colours");
        appearance_model_.SetGroupSubtitle(
            "Frame opacity", "independent prepared frame alpha by state");
        appearance_model_.SetGroupSubtitle(
            "Typography", "single-line font; wrapping/rich text belongs to UiLabel");
        appearance_model_.SetGroupSubtitle(
            "Text colours", "Normal / Hot / Pressed / Disabled ink");
        appearance_model_.SetGroupSubtitle(
            "Icon", "explicit box, side and aspect fit");
        appearance_model_.SetGroupSubtitle(
            "Icon tint colours", "used by MonoTint; PreserveColor leaves source colours");
        appearance_model_.SetGroupSubtitle(
            "Variant", "Soft caps face alpha; Outline suppresses the face");
        appearance_model_.StructureChanged();
    }

    void ConfigureEditors()
    {
        content_.SetFactory(&factory_);
        appearance_.SetFactory(&factory_);
        content_.SetModel(&content_model_);
        appearance_.SetModel(&appearance_model_);
        content_.SetLabelRatio(46);
        appearance_.SetLabelRatio(46);

        PropertyEditorStyle style = PropertyEditorStyle::System();
        style.show_group_summaries = true;
        content_.SetStyle(style);
        appearance_.SetStyle(style);
    }

    void ConnectEvents()
    {
        auto changed = [=](String, Value) { ApplyProjection(); };
        content_.WhenPreview = changed;
        content_.WhenCommit = changed;
        appearance_.WhenPreview = changed;
        appearance_.WhenCommit = changed;

        content_.WhenReset =
            [=](String id) { ResetProperty(content_model_, id); };
        appearance_.WhenReset =
            [=](String id) { ResetProperty(appearance_model_, id); };
        appearance_.WhenOverride =
            [=](String id, bool active) { SetOverrideActive(id, active); };

        content_mode_.WhenAction = [=] { SelectPage(0); };
        appearance_mode_.WhenAction = [=] { SelectPage(1); };
        code_mode_.WhenAction = [=] { SelectPage(2); };

        preview_.WhenTagAction = [=](String id, Value value) {
            caption_.SetText(
                "Host received interactive tag: " + id
                + "  payload=" + AsString(value));
        };

        theme_.WhenAction = [=] { ToggleTheme(); };
        help_.WhenAction = [=] {
            PromptOK(
                "UiTag reference demo\n\n"
                "UiTag is not a Ctrl. The preview host prepares and paints "
                "bounded tags, owns hover/pressed state, performs hit testing "
                "and routes id/value. Tags may be passive or interactive and "
                "may contain text, icon, or both.");
        };
        exit_.WhenAction = [=] { Break(); };
        copy_.WhenAction = [=] { WriteClipboardText(generated_); };
    }

    Value ContentValue(const String& id,
                       const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = content_model_.Find(id);
        return item ? item->value : fallback;
    }

    Value AppearanceValue(const String& id,
                          const Value& fallback = Value()) const
    {
        const PropertyEditorItem *item = appearance_model_.Find(id);
        return item ? item->value : fallback;
    }

    bool OverrideActive(const String& id) const
    {
        const PropertyEditorItem *item = appearance_model_.Find(id);
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
        PropertyEditorItem *item = appearance_model_.Find(id);
        if(!item || !item->overrideable)
            return;
        item->override_active = active;
        appearance_model_.StructureChanged();
        appearance_.RefreshModel();
        ApplyProjection();
    }

    void SyncInheritedOverrides(const UiTagStyle& style)
    {
        auto set = [&](const String& id, const Value& value) {
            if(!OverrideActive(id))
                appearance_model_.SetValue(id, value, false);
        };

        set("face.enabled", style.metrics.face_enabled);
        set("face.image", String());
        for(int st = 0; st < 4; st++) {
            set(StateProperty("face", st, "color"),
                FaceColor(style.palette, (StyledState)st, SColorFace()));
            set(StateProperty("face", st, "alpha"),
                style.face_alpha[st]);
        }

        set("frame.enabled", style.metrics.frame_enabled);
        set("frame.width", style.metrics.frame_width);
        set("radius", style.metrics.radius);
        for(int st = 0; st < 4; st++) {
            set(StateProperty("frame", st, "color"),
                style.palette.frame[st]);
            set(StateProperty("frame", st, "alpha"),
                style.frame_alpha[st]);
        }

        set("font.face", style.font.GetFaceName());
        set("font.height", max(1, style.font.GetHeight()));
        set("font.bold", style.font.IsBold());
        set("font.italic", style.font.IsItalic());
        for(int st = 0; st < 4; st++)
            set(StateProperty("ink", st, "color"),
                style.palette.ink[st]);

        set("padding.x", style.metrics.content_margin.left);
        set("padding.y", style.metrics.content_margin.top);
        set("content.gap", style.content_gap);

        set("icon.width", style.icon_size.cx);
        set("icon.height", style.icon_size.cy);
        set("icon.side",
            style.icon_side == UiAlign::RIGHT ? "Right" : "Left");
        String mode = style.icon_render_mode == UiIconRenderMode::PreserveColor
                    ? "PreserveColor"
                    : style.icon_render_mode == UiIconRenderMode::Auto
                    ? "Auto" : "MonoTint";
        set("icon.mode", mode);
        for(int st = 0; st < 4; st++)
            set(StateProperty("icon", st, "color"),
                UiResolveIconColor(style.palette, (StyledState)st));

        set("soft.alpha", style.soft_face_alpha);
        appearance_.RefreshModel();
    }

    void ApplyOverrides(UiTagStyle& style) const
    {
        if(OverrideActive("face.enabled"))
            style.metrics.face_enabled =
                (bool)AppearanceValue("face.enabled");
        for(int st = 0; st < 4; st++) {
            String color_id = StateProperty("face", st, "color");
            String alpha_id = StateProperty("face", st, "alpha");
            if(OverrideActive(color_id))
                style.palette.face[st] =
                    UiFill::Solid(Color(AppearanceValue(color_id)));
            if(OverrideActive(alpha_id))
                style.face_alpha[st] =
                    clamp((int)AppearanceValue(alpha_id), 0, 255);
        }

        if(OverrideActive("face.image")) {
            Image image = LoadImageValue(AppearanceValue("face.image"));
            if(!IsNull(image))
                for(int st = 0; st < 4; st++)
                    style.palette.face[st] = UiFill::ImageFill(image);
        }

        if(OverrideActive("frame.enabled"))
            style.metrics.frame_enabled =
                (bool)AppearanceValue("frame.enabled");
        if(OverrideActive("frame.width"))
            style.metrics.frame_width =
                max(0, (int)AppearanceValue("frame.width"));
        if(OverrideActive("radius"))
            style.metrics.radius =
                max(0, (int)AppearanceValue("radius"));
        for(int st = 0; st < 4; st++) {
            String color_id = StateProperty("frame", st, "color");
            String alpha_id = StateProperty("frame", st, "alpha");
            if(OverrideActive(color_id))
                style.palette.frame[st] =
                    Color(AppearanceValue(color_id));
            if(OverrideActive(alpha_id))
                style.frame_alpha[st] =
                    clamp((int)AppearanceValue(alpha_id), 0, 255);
        }

        Font font = style.font;
        if(OverrideActive("font.face"))
            font.FaceName(AsString(AppearanceValue("font.face")));
        if(OverrideActive("font.height"))
            font.Height(max(1, (int)AppearanceValue("font.height")));
        if(OverrideActive("font.bold"))
            font.Bold((bool)AppearanceValue("font.bold"));
        if(OverrideActive("font.italic"))
            font.Italic((bool)AppearanceValue("font.italic"));
        style.font = font;

        for(int st = 0; st < 4; st++) {
            String id = StateProperty("ink", st, "color");
            if(OverrideActive(id))
                style.palette.ink[st] =
                    Color(AppearanceValue(id));
        }

        int px = style.metrics.content_margin.left;
        int py = style.metrics.content_margin.top;
        if(OverrideActive("padding.x"))
            px = max(0, (int)AppearanceValue("padding.x"));
        if(OverrideActive("padding.y"))
            py = max(0, (int)AppearanceValue("padding.y"));
        style.metrics.content_margin = Rect(px, py, px, py);

        if(OverrideActive("content.gap"))
            style.content_gap =
                max(0, (int)AppearanceValue("content.gap"));
        if(OverrideActive("icon.width"))
            style.icon_size.cx =
                max(0, (int)AppearanceValue("icon.width"));
        if(OverrideActive("icon.height"))
            style.icon_size.cy =
                max(0, (int)AppearanceValue("icon.height"));
        if(OverrideActive("icon.side"))
            style.icon_side =
                ParseIconSide(AsString(AppearanceValue("icon.side")));
        if(OverrideActive("icon.mode"))
            style.icon_render_mode =
                ParseIconMode(AsString(AppearanceValue("icon.mode")));
        for(int st = 0; st < 4; st++) {
            String id = StateProperty("icon", st, "color");
            if(OverrideActive(id))
                style.palette.icon[st] =
                    Color(AppearanceValue(id));
        }

        if(OverrideActive("soft.alpha"))
            style.soft_face_alpha =
                clamp((int)AppearanceValue("soft.alpha"), 0, 255);
    }

    void ApplyProjection()
    {
        UiRole role =
            ParseRole(AsString(ContentValue("role", "Accent")));

        UiTagData data(
            AsString(ContentValue("text", "READY")),
            role,
            ParseVariant(AsString(ContentValue("variant", "Soft"))));
        data.id = "main-tag";
        data.value = "demo-payload";
        data.interactive =
            (bool)ContentValue("interactive", true);
        data.enabled =
            (bool)ContentValue("enabled", true);
        data.icon =
            ResolveDemoIcon(AsString(ContentValue("icon", "Info")));

        UiTagStyle style = UiResolveTagStyle(role);
        SyncInheritedOverrides(style);
        ApplyOverrides(style);

        preview_.SetMain(
            data, style,
            (int)ContentValue("max_width", 230));

        UpdateGeneratedCode(data, style);
        UpdateThemeIcon();
        RefreshLayout();
        Refresh();
    }

    void UpdateGeneratedCode(const UiTagData& data,
                             const UiTagStyle& style)
    {
        String role = AsString(ContentValue("role", "Accent"));
        String variant = AsString(ContentValue("variant", "Soft"));
        String icon = AsString(ContentValue("icon", "Info"));

        String out;
        out << "UiTagData data("
            << CppString(data.text)
            << ", UiRole::" << role
            << ", UiTagVariant::" << variant << ");\n"
            << "data.id = \"main-tag\";\n"
            << "data.value = \"demo-payload\";\n"
            << "data.interactive = "
            << (data.interactive ? "true" : "false") << ";\n"
            << "data.enabled = "
            << (data.enabled ? "true" : "false") << ";\n"
            << "data.icon = " << IconCode(icon) << ";\n\n"
            << "UiTagStyle style = UiResolveTagStyle(UiRole::"
            << role << ");\n";

        auto emit_bool = [&](const String& id, const String& target) {
            if(OverrideActive(id))
                out << target << " = "
                    << ((bool)AppearanceValue(id) ? "true" : "false")
                    << ";\n";
        };
        auto emit_int = [&](const String& id, const String& target) {
            if(OverrideActive(id))
                out << target << " = "
                    << (int)AppearanceValue(id) << ";\n";
        };
        auto emit_color = [&](const String& id, const String& target) {
            if(OverrideActive(id))
                out << target << " = "
                    << CppColor(Color(AppearanceValue(id))) << ";\n";
        };

        emit_bool("face.enabled", "style.metrics.face_enabled");
        for(int st = 0; st < 4; st++) {
            String color_id = StateProperty("face", st, "color");
            String alpha_id = StateProperty("face", st, "alpha");
            if(OverrideActive(color_id))
                out << "style.palette.face[" << StateEnum(st)
                    << "] = UiFill::Solid("
                    << CppColor(Color(AppearanceValue(color_id)))
                    << ");\n";
            emit_int(alpha_id,
                     String("style.face_alpha[") + StateEnum(st) + "]");
        }

        if(OverrideActive("face.image")
           && !AsString(AppearanceValue("face.image")).IsEmpty())
            out << "{\n"
                << "    Image face_image = StreamRaster::LoadFileAny("
                << CppString(AsString(AppearanceValue("face.image")))
                << ");\n"
                << "    for(int st = 0; st < 4; st++)\n"
                << "        style.palette.face[st] = UiFill::ImageFill(face_image);\n"
                << "}\n";

        emit_bool("frame.enabled", "style.metrics.frame_enabled");
        emit_int("frame.width", "style.metrics.frame_width");
        emit_int("radius", "style.metrics.radius");
        for(int st = 0; st < 4; st++) {
            emit_color(StateProperty("frame", st, "color"),
                       String("style.palette.frame[") + StateEnum(st) + "]");
            emit_int(StateProperty("frame", st, "alpha"),
                     String("style.frame_alpha[") + StateEnum(st) + "]");
        }

        if(OverrideActive("font.face"))
            out << "style.font.FaceName("
                << CppString(AsString(AppearanceValue("font.face"))) << ");\n";
        if(OverrideActive("font.height"))
            out << "style.font.Height("
                << (int)AppearanceValue("font.height") << ");\n";
        if(OverrideActive("font.bold"))
            out << "style.font.Bold("
                << ((bool)AppearanceValue("font.bold") ? "true" : "false")
                << ");\n";
        if(OverrideActive("font.italic"))
            out << "style.font.Italic("
                << ((bool)AppearanceValue("font.italic") ? "true" : "false")
                << ");\n";
        for(int st = 0; st < 4; st++)
            emit_color(StateProperty("ink", st, "color"),
                       String("style.palette.ink[") + StateEnum(st) + "]");

        if(OverrideActive("padding.x") || OverrideActive("padding.y")) {
            int px = style.metrics.content_margin.left;
            int py = style.metrics.content_margin.top;
            out << Format(
                "style.metrics.content_margin = Rect(%d, %d, %d, %d);\n",
                px, py, px, py);
        }
        emit_int("content.gap", "style.content_gap");

        if(OverrideActive("icon.width") || OverrideActive("icon.height"))
            out << Format("style.icon_size = Size(%d, %d);\n",
                          style.icon_size.cx, style.icon_size.cy);
        if(OverrideActive("icon.side"))
            out << "style.icon_side = UiAlign::"
                << (style.icon_side == UiAlign::RIGHT ? "RIGHT" : "LEFT")
                << ";\n";
        if(OverrideActive("icon.mode")) {
            String mode = AsString(AppearanceValue("icon.mode"));
            out << "style.icon_render_mode = UiIconRenderMode::"
                << mode << ";\n";
        }
        for(int st = 0; st < 4; st++)
            emit_color(StateProperty("icon", st, "color"),
                       String("style.palette.icon[") + StateEnum(st) + "]");

        emit_int("soft.alpha", "style.soft_face_alpha");

        out << "\nint max_width = "
            << (int)ContentValue("max_width", 230) << ";\n"
            << "Size natural = UiMeasureTag(data, style, max_width);\n"
            << "UiTagPresentation presentation = UiPrepareTag("
            << "data, style, RectC(0, 0, natural.cx, natural.cy));\n"
            << "// Host supplies ST_NORMAL/HOT/PRESSED/DISABLED and calls:\n"
            << "// UiPaintTag(draw, presentation, state);\n";

        generated_ = out;
        code_.SetData(generated_);
    }

    bool PickImage(Value& value, Ctrl *)
    {
        FileSel selector;
        selector.Type("Images", "*.png *.bmp *.jpg *.jpeg");
        if(!AsString(value).IsEmpty())
            selector.Set(AsString(value));
        if(!selector.ExecuteOpen("Choose tag background image"))
            return false;
        value = ~selector;
        return true;
    }

    Image LoadImageValue(const Value& value) const
    {
        String path = AsString(value);
        return path.IsEmpty() ? Image() : StreamRaster::LoadFileAny(path);
    }

    void SelectPage(int page)
    {
        page = minmax(page, 0, 2);
        pages_.SetActivePage(page);
        content_mode_.SetChecked(page == 0);
        appearance_mode_.SetChecked(page == 1);
        code_mode_.SetChecked(page == 2);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark
                     ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(context);
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
        preview_panel_.SetCustomStyle(surface);
        right_panel_.SetCustomStyle(surface);
        UiPanel::Style page_style = surface;
        page_style.transparent = true;
        page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
        for(UiPanel* panel : { &content_page_, &appearance_page_, &code_page_ })
            panel->SetCustomStyle(page_style);
        for(UiLabel* label : { &caption_ })
            label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        content_.SetPaletteMode(mode);
        appearance_.SetPaletteMode(mode);
        for(PropertyEditor* editor : { &content_, &appearance_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_, &help_, &exit_, &content_mode_, &appearance_mode_, &code_mode_, &copy_ }) {
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
    PropertyEditorModel content_model_, appearance_model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_, help_, exit_;

    UiPanel preview_panel_;
    TagPreviewCtrl preview_;
    UiLabel caption_;

    UiPanel right_panel_;
    UiBoxLayout tools_ { UiDirection::H };
    UiToolButton content_mode_, appearance_mode_, code_mode_;
    UiStack pages_;
    UiPanel content_page_, appearance_page_, code_page_;
    PropertyEditor content_, appearance_;
    UiMultiEdit code_;
    UiToolButton copy_;

    String generated_;
    Color window_face_ = SColorFace();
};

} // namespace

GUI_APP_MAIN
{
    UiTagDemo().Run();
}
