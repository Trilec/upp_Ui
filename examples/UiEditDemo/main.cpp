// Self-contained UiEditDemo reference: one authored model drives the preview and public-API C++ recipe.
#include <CtrlLib/CtrlLib.h>
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>

using namespace Upp;

namespace {

enum EditSample : int {
    EDIT_LINE = 0,
    EDIT_PASSWORD,
    EDIT_MASK,
    EDIT_MULTI,
    EDIT_COUNT,
};

String SampleName(EditSample sample)
{
    switch(sample) {
    case EDIT_PASSWORD: return "Password";
    case EDIT_MASK:     return "Mask";
    case EDIT_MULTI:    return "Multi-line";
    default:            return "Single-line";
    }
}

String CppBool(bool value) { return value ? "true" : "false"; }
String CppColor(Color c)
{
    return IsNull(c) ? String("Null")
                     : Format("Color(%d, %d, %d)", c.GetR(), c.GetG(), c.GetB());
}
String CppString(const String& value)
{
    String out = "\"";
    for(int i = 0; i < value.GetCount(); i++) {
        int c = value[i];
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else out.Cat(c);
    }
    return out << '"';
}

UiAlign ParseTextAlign(const String& value)
{
    if(value == "Center") return UiAlign::CENTER;
    if(value == "Right") return UiAlign::RIGHT;
    return UiAlign::LEFT;
}
String AlignCode(const String& value)
{
    if(value == "Center") return "UiAlign::CENTER";
    if(value == "Right") return "UiAlign::RIGHT";
    return "UiAlign::LEFT";
}

struct EditConfig {
    String text;
    String placeholder;
    bool enabled = true;
    bool read_only = false;
    bool accepts_drop = true;
    bool overwrite = false;
    String text_align = "Left";

    Color face = White();
    Color frame = Color(203, 213, 225);
    Color ink = Color(30, 41, 59);
    Color placeholder_ink = Color(148, 163, 184);
    int frame_width = 1;
    int radius = 7;
    int font_height = 13;
    int margin_x = 10;
    int margin_y = 6;

    Color caret = Color(30, 41, 59);
    int caret_width = 1;
    bool block_caret = false;
    Color selection_face = Color(219, 234, 254);
    Color selection_ink = Color(30, 41, 59);

    bool underline_enabled = false;
    int underline_width = 1;
    Color underline = Color(100, 116, 139);

    int tab_size = 4;
    bool show_tabs = false;
    bool show_spaces = false;
    bool show_line_endings = false;

    String password_char = "Bullet";
    bool password_plain = false;
    bool password_eye = true;

    String mask = "##/##/####";
    String mask_prompt = "_";
    String mask_validator = "Date";
    String mask_formatter = "None";
    bool mask_show_error = true;
    bool mask_flash = true;

    bool multi_accept_tabs = true;
    bool multi_left_icon = false, multi_clear_action = false;
};

class UiEditDemoWindow : public TopWindow {
public:
    typedef UiEditDemoWindow CLASSNAME;

    UiEditDemoWindow()
    {
        Title("Ui Edit Family Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1280), DPI(820));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);
        RegisterPropertyEditorV1Editors(factory_);
        SeedConfigs();

        Add(header_);
        Add(preview_panel_);
        Add(rail_panel_);

        header_.SetTitle("Ui edit family")
               .SetSubTitle("UiLineEdit, UiPasswordEdit, UiMaskEdit and UiMultiEdit share UiBaseEdit styling")
               .SetMedia(ICON_EDITOR_NOTES_48())
               .SetMediaAutoFit(true)
               .ShowTitleLine(false)
               .SetContentInset(DPI(8))
               .SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        header_actions_.AddSpacer(1).Expand(1);
        theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
        exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close demo");
        header_actions_.Add(theme_button_).Fixed(DPI(34));
        help_button_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Demo help");
        header_actions_.Add(help_button_).Fixed(DPI(34));
        header_actions_.Add(exit_button_).Fixed(DPI(34));

        preview_panel_.Add(selector_);
        selector_.SetGap(DPI(6)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        line_select_.SetText("Single-line").SetCheckable();
        password_select_.SetText("Password").SetCheckable();
        mask_select_.SetText("Mask").SetCheckable();
        multi_select_.SetText("Multi-line").SetCheckable();
        selector_.Add(line_select_).Expand(1);
        selector_.Add(password_select_).Expand(1);
        selector_.Add(mask_select_).Expand(1);
        selector_.Add(multi_select_).Expand(1);

        preview_panel_.Add(line_label_);
        preview_panel_.Add(password_label_);
        preview_panel_.Add(mask_label_);
        preview_panel_.Add(multi_label_);
        preview_panel_.Add(line_);
        preview_panel_.Add(password_);
        preview_panel_.Add(mask_);
        preview_panel_.Add(multi_);
        preview_panel_.Add(status_);
        line_label_.SetText("UiLineEdit");
        password_label_.SetText("UiPasswordEdit");
        mask_label_.SetText("UiMaskEdit");
        multi_label_.SetText("UiMultiEdit");
        status_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

        rail_panel_.Add(view_bar_);
        rail_panel_.Add(properties_);
        rail_panel_.Add(code_mode_);
        rail_panel_.Add(code_);
        view_bar_.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        props_button_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable().SetChecked(true);
        code_button_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable();
        view_bar_.Add(props_button_).Fixed(DPI(38));
        props_button_.Tip("Inspector"); code_button_.Tip("Generated code"); overrides_button_.Tip("Theme Overrides");
        overrides_button_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).SetCheckable();
        view_bar_.Add(overrides_button_).Fixed(DPI(38));
        view_bar_.Add(code_button_).Fixed(DPI(38));
        view_bar_.AddSpacer(1).Expand(1);

        code_mode_.UseInternalModel().Clear()
                  .Add("Usage", "usage")
                  .Add("Current changes", "changes")
                  .Add("Full explicit", "explicit");
        code_mode_.SelectByData("changes");
        code_.SetEditable(false);
        code_.SetAcceptsTabs(true);

        properties_.SetFactory(&factory_);
        properties_.SetModel(&model_);
        properties_.SetLabelRatio(38);
        PropertyEditorStyle pe_style = PropertyEditorStyle::System();
        pe_style.show_group_summaries = true;
        properties_.SetStyle(pe_style);

        Connect();
        SelectSample(EDIT_LINE);
        ApplyTheme();
        ApplyAllSamples();
        SetCodeView(false);
    }

    void Paint(Draw& draw) override { draw.DrawRect(GetSize(), window_face_); }

    virtual void Layout() override
    {
        Rect client = GetSize();
        const int pad = DPI(12), gap = DPI(10), header_h = DPI(72);
        const int rail_w = min(DPI(465), max(DPI(365), client.GetWidth() * 38 / 100));
        header_.SetRect(pad, pad, max(0, client.GetWidth() - pad * 2), header_h);
        const int top = pad + header_h + gap;
        const int body_h = max(0, client.GetHeight() - top - pad);
        const int preview_w = max(0, client.GetWidth() - pad * 3 - rail_w);
        preview_panel_.SetRect(pad, top, preview_w, body_h);
        rail_panel_.SetRect(pad + preview_w + gap, top, rail_w, body_h);

        Rect pr = preview_panel_.GetSize();
        const int inset = DPI(24);
        selector_.SetRect(inset, DPI(18), max(0, pr.GetWidth() - inset * 2), DPI(34));
        const int label_h = DPI(24), edit_h = DPI(36);
        int y = selected_==EDIT_MULTI ? DPI(78) : max(DPI(78),pr.GetHeight()/2-DPI(40));
        const int edit_w = max(DPI(200), pr.GetWidth() - inset * 2);
        for(UiLabel* label:{&line_label_,&password_label_,&mask_label_,&multi_label_})
            label->SetRect(inset,y,edit_w,label_h);
        y += label_h;
        UiBaseEdit* edits[]={&line_,&password_,&mask_};
        for(UiBaseEdit* edit:edits) edit->SetRect(inset,y,edit_w,edit_h);
        const int multi_h = max(DPI(120), pr.GetHeight() - y - DPI(70));
        multi_.SetRect(inset, y, edit_w, multi_h);
        status_.SetRect(inset, max(0, pr.bottom - DPI(42)), edit_w, DPI(26));

        Rect rr = rail_panel_.GetSize();
        view_bar_.SetRect(DPI(8), DPI(8), max(0, rr.GetWidth() - DPI(16)), DPI(32));
        const int content_y = DPI(48);
        properties_.SetRect(DPI(8), content_y, max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - content_y - DPI(8)));
        code_mode_.SetRect(DPI(8), content_y, max(0, rr.GetWidth() - DPI(16)), DPI(32));
        code_.SetRect(DPI(8), content_y + DPI(40), max(0, rr.GetWidth() - DPI(16)), max(0, rr.GetHeight() - content_y - DPI(48)));
    }

    bool TestSelectors(const String& output)
    {
        String failures;
        int checks=0;
        auto check=[&](bool ok,const char* name) { ++checks; if(!ok) failures << name << "\n"; };
        UiButton* buttons[]={&line_select_,&password_select_,&mask_select_,&multi_select_};
        const char* types[]={"UiLineEdit edit;","UiPasswordEdit edit;","UiMaskEdit edit;","UiMultiEdit edit;"};
        String text[EDIT_COUNT];
        for(int i=0;i<EDIT_COUNT;i++) text[i]=cfg_[i].text;
        for(int theme=0;theme<2;theme++) {
            if(theme) ToggleTheme();
            for(int repeat=0;repeat<8;repeat++) for(int type=0;type<EDIT_COUNT;type++) {
                buttons[type]->SetFocus();
                check(buttons[type]->Key(K_SPACE,1),"Native selector keyboard action");
                for(int pump=0;pump<4;pump++) ProcessEvents();
                check(selected_==type,"Selected concrete edit");
                check(line_.IsShown()==(type==EDIT_LINE) && password_.IsShown()==(type==EDIT_PASSWORD)
                   && mask_.IsShown()==(type==EDIT_MASK) && multi_.IsShown()==(type==EDIT_MULTI),"Exactly one edit preview visible");
                check(cfg_[type].text==text[type],"Configuration retained across selection");
                check(code_.GetTextUtf8().Find(types[type])>=0,"Generated code owns selected concrete edit");
                ImageDraw image(GetSize()); DrawCtrl(image);
            }
        }
        SaveFile(output,Format("%d checks\n",checks)+(failures.IsEmpty()?"PASS\n":failures));
        return failures.IsEmpty();
    }

    void ExportGenerated(const String& directory)
    {
        RealizeDirectory(directory);
        SelectSample(EDIT_LINE);
        code_mode_.SelectByData("usage"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_LINE_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_LINE_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_LINE_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        if(selected_ == EDIT_MULTI) { model_.SetValue("multi_left_icon", true, false); model_.SetValue("multi_clear_action", true, false); }
        if(selected_ == EDIT_MASK) model_.SetValue("mask_prompt", "'", false);
        PullConfig(selected_); ApplySample(selected_);
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_LINE_authored.cpp"), code_.GetTextUtf8());
        }
        SelectSample(EDIT_PASSWORD);
        code_mode_.SelectByData("usage"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_PASSWORD_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_PASSWORD_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_PASSWORD_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        if(selected_ == EDIT_MULTI) { model_.SetValue("multi_left_icon", true, false); model_.SetValue("multi_clear_action", true, false); }
        if(selected_ == EDIT_MASK) model_.SetValue("mask_prompt", "'", false);
        PullConfig(selected_); ApplySample(selected_);
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_PASSWORD_authored.cpp"), code_.GetTextUtf8());
        }
        SelectSample(EDIT_MASK);
        code_mode_.SelectByData("usage"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MASK_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MASK_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MASK_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        if(selected_ == EDIT_MULTI) { model_.SetValue("multi_left_icon", true, false); model_.SetValue("multi_clear_action", true, false); }
        if(selected_ == EDIT_MASK) model_.SetValue("mask_prompt", "'", false);
        PullConfig(selected_); ApplySample(selected_);
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MASK_authored.cpp"), code_.GetTextUtf8());
        }
        SelectSample(EDIT_MULTI);
        code_mode_.SelectByData("usage"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MULTI_usage.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MULTI_changes.cpp"), code_.GetTextUtf8());
        code_mode_.SelectByData("explicit"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MULTI_explicit.cpp"), code_.GetTextUtf8());
        {
        if(PropertyEditorItem* text = model_.Find("text")) model_.SetValue("text", String("Quoted \"title\"\t\r\nC:\\media"), false);
        static const char* colors[] = { "face", "body_face", "track_color", "track_face", "tab_face" };
        for(const char* id : colors) if(model_.Find(id)) { model_.SetValue(id, Color(88, 99, 111), false); break; }
        if(selected_ == EDIT_MULTI) { model_.SetValue("multi_left_icon", true, false); model_.SetValue("multi_clear_action", true, false); }
        if(selected_ == EDIT_MASK) model_.SetValue("mask_prompt", "'", false);
        PullConfig(selected_); ApplySample(selected_);
        code_mode_.SelectByData("changes"); UpdateCode();
        SaveFile(AppendFileName(directory, "UiEditDemo_EDIT_MULTI_authored.cpp"), code_.GetTextUtf8());
        }
    }

private:
    PropertyEditorItem& Resettable(PropertyEditorItem& item)
    {
        item.SetDefault(item.value);
        return item;
    }

    Value Get(const char *id) const
    {
        const PropertyEditorItem *item = model_.Find(id);
        return item ? item->value : Value();
    }

    bool Changed(const char *id) const
    {
        const PropertyEditorItem *item = model_.Find(id);
        return item && item->value != item->default_value;
    }

    void SeedConfigs()
    {
        cfg_[EDIT_LINE].text = "Edit me";
        cfg_[EDIT_LINE].placeholder = "Single-line text";
        cfg_[EDIT_PASSWORD].text = "correct horse battery staple";
        cfg_[EDIT_PASSWORD].placeholder = "Password";
        cfg_[EDIT_MASK].text = "12/31/2026";
        cfg_[EDIT_MASK].placeholder = "MM/DD/YYYY";
        cfg_[EDIT_MULTI].text = "First line\nSecond line\nThird line";
        cfg_[EDIT_MULTI].placeholder = "Multi-line notes";
    }

    void BuildModel(EditSample sample)
    {
        const EditConfig& cfg = cfg_[sample];
        model_.Clear(false);

        if(sample == EDIT_MULTI)
            Resettable(model_.AddMultiline("text", "Text", cfg.text, "Content").SetExpandedRowSpan(3));
        else
            Resettable(model_.AddText("text", "Text", cfg.text, "Content"));
        Resettable(model_.AddText("placeholder", "Placeholder", cfg.placeholder, "Content"));

        Resettable(model_.AddBoolean("enabled", "Enabled", cfg.enabled, "Behaviour"));
        Resettable(model_.AddBoolean("read_only", "Read only", cfg.read_only, "Behaviour"));
        Resettable(model_.AddBoolean("accepts_drop", "Accept drop", cfg.accepts_drop, "Behaviour"));
        Resettable(model_.AddBoolean("overwrite", "Overwrite mode", cfg.overwrite, "Behaviour"));
        Resettable(model_.AddChoice("text_align", "Text alignment", cfg.text_align, "Behaviour")
            .AddChoice("Left", "Left").AddChoice("Center", "Center").AddChoice("Right", "Right"));

        Resettable(model_.AddColor("face", "Face", cfg.face, "Face"));
        Resettable(model_.AddColor("frame", "Frame", cfg.frame, "Frame"));
        Resettable(model_.AddNumericInt("frame_width", "Frame width", cfg.frame_width, 0, 12, 1, "Frame").SetUnit("px"));
        Resettable(model_.AddNumericInt("radius", "Radius", cfg.radius, 0, 40, 1, "Frame").SetUnit("px"));
        Resettable(model_.AddColor("ink", "Ink", cfg.ink, "Ink"));
        Resettable(model_.AddColor("placeholder_ink", "Placeholder", cfg.placeholder_ink, "Ink"));
        Resettable(model_.AddNumericInt("font_height", "Font height", cfg.font_height, 8, 40, 1, "Typography").SetUnit("px"));
        Resettable(model_.AddNumericInt("margin_x", "Horizontal", cfg.margin_x, 0, 40, 1, "Content Margin").SetUnit("px"));
        Resettable(model_.AddNumericInt("margin_y", "Vertical", cfg.margin_y, 0, 32, 1, "Content Margin").SetUnit("px"));

        Resettable(model_.AddColor("caret", "Caret colour", cfg.caret, "Editing"));
        Resettable(model_.AddNumericInt("caret_width", "Caret width", cfg.caret_width, 1, 8, 1, "Editing").SetUnit("px"));
        Resettable(model_.AddBoolean("block_caret", "Block caret", cfg.block_caret, "Editing"));
        Resettable(model_.AddColor("selection_face", "Selection face", cfg.selection_face, "Editing"));
        Resettable(model_.AddColor("selection_ink", "Selection ink", cfg.selection_ink, "Editing"));

        Resettable(model_.AddBoolean("underline_enabled", "Enabled", cfg.underline_enabled, "Underline"));
        Resettable(model_.AddNumericInt("underline_width", "Width", cfg.underline_width, 1, 8, 1, "Underline").SetUnit("px"));
        Resettable(model_.AddColor("underline", "Colour", cfg.underline, "Underline"));

        if(sample == EDIT_PASSWORD) {
            Resettable(model_.AddChoice("password_char", "Mask character", cfg.password_char, "Password")
                .AddChoice("Bullet", "Bullet •").AddChoice("Asterisk", "Asterisk *")
                .AddChoice("MiddleDot", "Middle dot ·"));
            Resettable(model_.AddBoolean("password_plain", "Show plain text", cfg.password_plain, "Password"));
            Resettable(model_.AddBoolean("password_eye", "Visibility button", cfg.password_eye, "Password"));
        }
        else if(sample == EDIT_MASK) {
            Resettable(model_.AddText("mask", "Mask", cfg.mask, "Mask"));
            Resettable(model_.AddText("mask_prompt", "Prompt character", cfg.mask_prompt, "Mask"));
            Resettable(model_.AddChoice("mask_validator", "Validator", cfg.mask_validator, "Mask")
                .AddChoice("None", "None").AddChoice("Date", "Date")
                .AddChoice("Time", "Time").AddChoice("NonEmpty", "Non-empty")
                .AddChoice("Alnum", "Alnum + underscore"));
            Resettable(model_.AddChoice("mask_formatter", "Formatter", cfg.mask_formatter, "Mask")
                .AddChoice("None", "None").AddChoice("Uppercase", "Uppercase")
                .AddChoice("Lowercase", "Lowercase").AddChoice("TitleCase", "Title case")
                .AddChoice("Username", "Username").AddChoice("SafeAlnum", "Safe alnum"));
            Resettable(model_.AddBoolean("mask_flash", "Flash validation on commit", cfg.mask_flash, "Mask"));
            Resettable(model_.AddBoolean("mask_show_error", "Show invalid state", cfg.mask_show_error, "Mask"));
        }
        else if(sample == EDIT_MULTI) {
            Resettable(model_.AddBoolean("multi_left_icon", "Leading icon", cfg.multi_left_icon, "Side controls"));
            Resettable(model_.AddBoolean("multi_clear_action", "Clear action", cfg.multi_clear_action, "Side controls"));
            Resettable(model_.AddBoolean("multi_accept_tabs", "Accept tabs", cfg.multi_accept_tabs, "Whitespace"));
            Resettable(model_.AddNumericInt("tab_size", "Tab size", cfg.tab_size, 1, 12, 1, "Whitespace"));
            Resettable(model_.AddBoolean("show_tabs", "Show tabs", cfg.show_tabs, "Whitespace"));
            Resettable(model_.AddBoolean("show_spaces", "Show spaces", cfg.show_spaces, "Whitespace"));
            Resettable(model_.AddBoolean("show_line_endings", "Show line endings", cfg.show_line_endings, "Whitespace"));
        }

        model_.SetGroupSubtitle("Content", SampleName(sample) + " sample content");
        model_.SetGroupSubtitle("Behaviour", "shared UiBaseEdit behaviour");
        model_.SetGroupSubtitle("Editing", "caret and selection");
        if(sample == EDIT_MULTI)
            model_.SetGroupSubtitle("Whitespace", "multi-line whitespace rendering");
        const EditConfig defaults;
        if(PropertyEditorItem* item = model_.Find("block_caret")) item->default_value = defaults.block_caret;
        if(PropertyEditorItem* item = model_.Find("caret")) item->default_value = defaults.caret;
        if(PropertyEditorItem* item = model_.Find("caret_width")) item->default_value = defaults.caret_width;
        if(PropertyEditorItem* item = model_.Find("face")) item->default_value = defaults.face;
        if(PropertyEditorItem* item = model_.Find("font_height")) item->default_value = defaults.font_height;
        if(PropertyEditorItem* item = model_.Find("frame")) item->default_value = defaults.frame;
        if(PropertyEditorItem* item = model_.Find("frame_width")) item->default_value = defaults.frame_width;
        if(PropertyEditorItem* item = model_.Find("ink")) item->default_value = defaults.ink;
        if(PropertyEditorItem* item = model_.Find("margin_x")) item->default_value = defaults.margin_x;
        if(PropertyEditorItem* item = model_.Find("margin_y")) item->default_value = defaults.margin_y;
        if(PropertyEditorItem* item = model_.Find("placeholder_ink")) item->default_value = defaults.placeholder_ink;
        if(PropertyEditorItem* item = model_.Find("radius")) item->default_value = defaults.radius;
        if(PropertyEditorItem* item = model_.Find("selection_face")) item->default_value = defaults.selection_face;
        if(PropertyEditorItem* item = model_.Find("selection_ink")) item->default_value = defaults.selection_ink;
        if(PropertyEditorItem* item = model_.Find("show_line_endings")) item->default_value = defaults.show_line_endings;
        if(PropertyEditorItem* item = model_.Find("show_spaces")) item->default_value = defaults.show_spaces;
        if(PropertyEditorItem* item = model_.Find("show_tabs")) item->default_value = defaults.show_tabs;
        if(PropertyEditorItem* item = model_.Find("tab_size")) item->default_value = defaults.tab_size;
        if(PropertyEditorItem* item = model_.Find("text_align")) item->default_value = defaults.text_align;
        if(PropertyEditorItem* item = model_.Find("underline")) item->default_value = defaults.underline;
        if(PropertyEditorItem* item = model_.Find("underline_enabled")) item->default_value = defaults.underline_enabled;
        if(PropertyEditorItem* item = model_.Find("underline_width")) item->default_value = defaults.underline_width;
        model_.StructureChanged();
        properties_.RefreshModel();
    }

    void PullConfig(EditSample sample)
    {
        EditConfig& cfg = cfg_[sample];
        cfg.text = AsString(Get("text"));
        cfg.placeholder = AsString(Get("placeholder"));
        cfg.enabled = (bool)Get("enabled");
        cfg.read_only = (bool)Get("read_only");
        cfg.accepts_drop = (bool)Get("accepts_drop");
        cfg.overwrite = (bool)Get("overwrite");
        cfg.text_align = AsString(Get("text_align"));
        cfg.face = Color(Get("face"));
        cfg.frame = Color(Get("frame"));
        cfg.frame_width = (int)Get("frame_width");
        cfg.radius = (int)Get("radius");
        cfg.ink = Color(Get("ink"));
        cfg.placeholder_ink = Color(Get("placeholder_ink"));
        cfg.font_height = (int)Get("font_height");
        cfg.margin_x = (int)Get("margin_x");
        cfg.margin_y = (int)Get("margin_y");
        cfg.caret = Color(Get("caret"));
        cfg.caret_width = (int)Get("caret_width");
        cfg.block_caret = (bool)Get("block_caret");
        cfg.selection_face = Color(Get("selection_face"));
        cfg.selection_ink = Color(Get("selection_ink"));
        cfg.underline_enabled = (bool)Get("underline_enabled");
        cfg.underline_width = (int)Get("underline_width");
        cfg.underline = Color(Get("underline"));

        if(sample == EDIT_PASSWORD) {
            cfg.password_char = AsString(Get("password_char"));
            cfg.password_plain = (bool)Get("password_plain");
            cfg.password_eye = (bool)Get("password_eye");
        }
        else if(sample == EDIT_MASK) {
            cfg.mask = AsString(Get("mask"));
            cfg.mask_prompt = AsString(Get("mask_prompt"));
            cfg.mask_validator = AsString(Get("mask_validator"));
            cfg.mask_formatter = AsString(Get("mask_formatter"));
            cfg.mask_flash = (bool)Get("mask_flash");
            cfg.mask_show_error = (bool)Get("mask_show_error");
        }
        else if(sample == EDIT_MULTI) {
            cfg.multi_left_icon = (bool)Get("multi_left_icon");
            cfg.multi_clear_action = (bool)Get("multi_clear_action");
            cfg.multi_accept_tabs = (bool)Get("multi_accept_tabs");
            cfg.tab_size = (int)Get("tab_size");
            cfg.show_tabs = (bool)Get("show_tabs");
            cfg.show_spaces = (bool)Get("show_spaces");
            cfg.show_line_endings = (bool)Get("show_line_endings");
        }
    }

    UiBaseEdit::Style MakeStyle(const EditConfig& cfg) const
    {
        const EditConfig defaults;
        UiBaseEdit::Style style = UiTheme::ResolveEdit(UiRole::Standard);
        for(int i = 0; i < 4; i++) {
            if(cfg.face != defaults.face) style.palette.face[i] = UiFill::Solid(cfg.face);
            if(cfg.frame != defaults.frame) style.palette.frame[i] = cfg.frame;
            if(cfg.ink != defaults.ink) style.palette.ink[i] = cfg.ink;
            if(cfg.underline != defaults.underline) style.underline[i] = cfg.underline;
        }
        style.metrics.face_enabled = true;
        if(cfg.frame_width != defaults.frame_width) style.metrics.frame_enabled = cfg.frame_width > 0;
        if(cfg.frame_width != defaults.frame_width) style.metrics.frame_width = DPI(cfg.frame_width);
        if(cfg.radius != defaults.radius) style.metrics.radius = DPI(cfg.radius);
        if(cfg.margin_x != defaults.margin_x || cfg.margin_y != defaults.margin_y) style.metrics.content_margin = Rect(DPI(cfg.margin_x), DPI(cfg.margin_y), DPI(cfg.margin_x), DPI(cfg.margin_y));
        if(cfg.font_height != defaults.font_height) style.font.Height(cfg.font_height);
        if(cfg.text_align != defaults.text_align) style.text_align = ParseTextAlign(cfg.text_align);
        if(cfg.placeholder_ink != defaults.placeholder_ink) style.placeholder_ink = cfg.placeholder_ink;
        if(cfg.caret != defaults.caret) style.caret_color = cfg.caret;
        if(cfg.caret_width != defaults.caret_width) style.caret_width = DPI(cfg.caret_width);
        if(cfg.block_caret != defaults.block_caret) style.block_caret = cfg.block_caret;
        if(cfg.selection_face != defaults.selection_face) style.selection_color = cfg.selection_face;
        if(cfg.selection_ink != defaults.selection_ink) style.selection_ink = cfg.selection_ink;
        if(cfg.underline_enabled != defaults.underline_enabled) style.underline_enabled = cfg.underline_enabled;
        if(cfg.underline_width != defaults.underline_width) style.underline_width = DPI(cfg.underline_width);
        if(cfg.tab_size != defaults.tab_size) style.tab_size = cfg.tab_size;
        if(cfg.show_tabs != defaults.show_tabs) style.show_tabs = cfg.show_tabs;
        if(cfg.show_spaces != defaults.show_spaces) style.show_spaces = cfg.show_spaces;
        if(cfg.show_line_endings != defaults.show_line_endings) style.show_line_endings = cfg.show_line_endings;
        return style;
    }

    void ApplyCommon(UiBaseEdit& edit, const EditConfig& cfg)
    {
        edit.SetCustomStyle(MakeStyle(cfg));
        edit.SetPlaceholder(cfg.placeholder);
        edit.SetAcceptsDrop(cfg.accepts_drop);
        edit.SetOverwriteMode(cfg.overwrite);
        edit.SetTextAlign(ParseTextAlign(cfg.text_align));
        edit.SetEditable(!cfg.read_only);
        edit.Enable(cfg.enabled);
    }

    void ApplySample(EditSample sample)
    {
        EditConfig& cfg = cfg_[sample];
        switch(sample) {
        case EDIT_PASSWORD: {
            ApplyCommon(password_, cfg);
            wchar mask_char = cfg.password_char == "Asterisk" ? '*' :
                              cfg.password_char == "MiddleDot" ? 0x00B7 : 0x2022;
            password_.SetPasswordChar(mask_char)
                     .EnableVisibilityIcon(cfg.password_eye)
                     .SetPlainTextVisible(cfg.password_plain);
            if(password_.GetTextUtf8() != cfg.text)
                password_.SetTextUtf8(cfg.text);
            break;
        }
        case EDIT_MASK: {
            ApplyCommon(mask_, cfg);
            const char prompt = cfg.mask_prompt.IsEmpty() ? '_' : cfg.mask_prompt[0];
            mask_.SetMask(cfg.mask, prompt);
            Function<bool(const String&)> validator;
            if(cfg.mask_validator == "Date") validator = UiMaskEdit::DateValidator();
            else if(cfg.mask_validator == "Time") validator = UiMaskEdit::TimeValidator();
            else if(cfg.mask_validator == "NonEmpty") validator = UiMaskEdit::NonEmptyValidator();
            else if(cfg.mask_validator == "Alnum") validator = UiMaskEdit::AlnumUnderscoreValidator(true);
            mask_.SetValidator(validator);
            Function<String(const String&)> formatter;
            if(cfg.mask_formatter == "Uppercase") formatter = UiMaskEdit::UppercaseFormatter();
            else if(cfg.mask_formatter == "Lowercase") formatter = UiMaskEdit::LowercaseFormatter();
            else if(cfg.mask_formatter == "TitleCase") formatter = UiMaskEdit::TitleCaseFormatter();
            else if(cfg.mask_formatter == "Username") formatter = UiMaskEdit::UsernameFormatter();
            else if(cfg.mask_formatter == "SafeAlnum") formatter = UiMaskEdit::SafeAlnumFormatter();
            mask_.SetFormatter(formatter);
            if(mask_.GetTextUtf8() != cfg.text)
                mask_.SetTextUtf8(cfg.text);
            mask_.ShowError(cfg.mask_show_error && !mask_.IsValid());
            break;
        }
        case EDIT_MULTI:
            ApplyCommon(multi_, cfg);
            multi_.SetAcceptsTabs(cfg.multi_accept_tabs);
            multi_icon_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(16), DPI(16));
            multi_clear_.SetIcon(ICON_DESIGN_DELETE_48()).SetIconSize(DPI(16), DPI(16));
            if(multi_.GetSideId(multi_icon_) < 0) multi_.AddToSide(multi_icon_, UiAlign::LEFT, Size(DPI(24), DPI(24))).Overlay(false);
            if(multi_.GetSideId(multi_clear_) < 0) multi_.AddToSide(multi_clear_, UiAlign::RIGHT, Size(DPI(24), DPI(24))).Overlay(false);
            multi_.GetSideHandle(multi_icon_).Visible(cfg.multi_left_icon);
            multi_.GetSideHandle(multi_clear_).Visible(cfg.multi_clear_action);
            if(multi_.GetTextUtf8() != cfg.text)
                multi_.SetTextUtf8(cfg.text);
            break;
        default:
            ApplyCommon(line_, cfg);
            if(line_.GetTextUtf8() != cfg.text)
                line_.SetTextUtf8(cfg.text);
            break;
        }
    }

    void ApplyAllSamples()
    {
        for(int i = 0; i < EDIT_COUNT; i++)
            ApplySample((EditSample)i);
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
        Refresh();
    }

    void SelectSample(EditSample sample)
    {
        selected_ = sample;
        line_.Show(sample==EDIT_LINE); line_label_.Show(sample==EDIT_LINE);
        password_.Show(sample==EDIT_PASSWORD); password_label_.Show(sample==EDIT_PASSWORD);
        mask_.Show(sample==EDIT_MASK); mask_label_.Show(sample==EDIT_MASK);
        multi_.Show(sample==EDIT_MULTI); multi_label_.Show(sample==EDIT_MULTI);
        line_select_.SetChecked(sample == EDIT_LINE);
        password_select_.SetChecked(sample == EDIT_PASSWORD);
        mask_select_.SetChecked(sample == EDIT_MASK);
        multi_select_.SetChecked(sample == EDIT_MULTI);
        BuildModel(sample);
        UpdatePropertyPage();
        ApplyTheme();
        UpdateStatus();
        UpdateCode();
        RefreshLayout();
    }

    void Connect()
    {
        line_select_.WhenAction = [=] { SelectSample(EDIT_LINE); };
        password_select_.WhenAction = [=] { SelectSample(EDIT_PASSWORD); };
        mask_select_.WhenAction = [=] { SelectSample(EDIT_MASK); };
        multi_select_.WhenAction = [=] { SelectSample(EDIT_MULTI); };
        props_button_.WhenAction = [=] { SelectStylePage(false); };
        code_button_.WhenAction = [=] { SetCodeView(true); };
        code_mode_.WhenAction = [=] { UpdateCode(); };
        theme_button_.WhenAction = [=] { ToggleTheme(); };
        help_button_.WhenAction = [=] { PromptOK("UiEditDemo: select a preview type, edit its Inspector or Theme Overrides, then copy the selected control from Generated Code."); };
        overrides_button_.WhenAction = [=] { SelectStylePage(true); };
        exit_button_.WhenAction = [=] { Close(); };

        multi_clear_.WhenAction = [=] { multi_.SetTextUtf8(""); CaptureText(EDIT_MULTI, multi_); };
        mask_.WhenAction = [=] {
            const bool valid = mask_.IsValid();
            mask_.ShowError(!valid);
            if(cfg_[EDIT_MASK].mask_flash) { if(valid) mask_.FlashSuccess(); else mask_.FlashError(); }
            UpdateStatus();
        };
        properties_.WhenPreview = [=](String, Value) {
            PullConfig(selected_);
            ApplySample(selected_);
            UpdateStatus();
            UpdateCode();
        };
        properties_.WhenCommit = [=](String, Value) {
            PullConfig(selected_);
            ApplySample(selected_);
            UpdateStatus();
            UpdateCode();
        };
        properties_.WhenReset = [=](String id) {
            PropertyEditorItem *item = model_.Find(id);
            if(item && item->resettable) {
                model_.SetValue(id, item->default_value);
                PullConfig(selected_);
                properties_.RefreshModel();
                ApplySample(selected_);
                UpdateStatus();
                UpdateCode();
            }
        };

        line_.WhenChange = [=] { CaptureText(EDIT_LINE, line_); };
        password_.WhenChange = [=] { CaptureText(EDIT_PASSWORD, password_); };
        mask_.WhenChange = [=] { CaptureText(EDIT_MASK, mask_); };
        multi_.WhenChange = [=] { CaptureText(EDIT_MULTI, multi_); };
    }

    void CaptureText(EditSample sample, UiBaseEdit& edit)
    {
        cfg_[sample].text = edit.GetTextUtf8();
        if(selected_ == sample && model_.Find("text")) {
            model_.SetValue("text", cfg_[sample].text, false);
            properties_.RefreshValue("text");
        }
        if(sample == EDIT_MASK)
            mask_.ShowError(cfg_[EDIT_MASK].mask_show_error && !mask_.IsValid());
        UpdateCode();
    }

    void UpdateStatus()
    {
        String detail = SampleName(selected_);
        if(selected_ == EDIT_MASK)
            detail << (mask_.IsValid() ? " · valid" : " · invalid");
        else if(selected_ == EDIT_PASSWORD)
            detail << (password_.IsPlainTextVisible() ? " · visible" : " · masked");
        else if(selected_ == EDIT_MULTI)
            detail << Format(" · %d chars", multi_.GetTextUtf8().GetCount());
        status_.SetText("Selected: " + detail);
    }

    void SetCodeView(bool on)
    {
        code_view_ = on;
        if(on) overrides_view_ = false;
        UpdatePropertyPage();
        props_button_.SetChecked(!on && !overrides_view_);
        overrides_button_.SetChecked(!on && overrides_view_);
        code_button_.SetChecked(on);
        properties_.Show(!on);
        code_mode_.Show(on);
        code_.Show(on);
        ApplyTheme();
        if(on) UpdateCode();
    }

    bool AnyStyleChange() const
    {
        static const char *ids[] = {
            "face", "frame", "ink", "placeholder_ink", "frame_width", "radius",
            "font_height", "margin_x", "margin_y", "text_align", "caret", "caret_width",
            "block_caret", "selection_face", "selection_ink", "underline_enabled",
            "underline_width", "underline"
        };
        for(const char *id : ids)
            if(Changed(id)) return true;
        if(selected_ == EDIT_MULTI)
            return Changed("tab_size") || Changed("show_tabs") || Changed("show_spaces") || Changed("show_line_endings");
        return false;
    }

    void EmitStyle(String& out, const EditConfig& cfg, bool explicit_style) const
    {
        out << "\n// Optional local design block shared by every UiBaseEdit-derived control.\n";
        out << "UiBaseEdit::Style style = UiTheme::ResolveEdit(UiRole::Standard);\n";
        out << "style.metrics.frame_width = DPI(" << cfg.frame_width << ");\n"
            << "style.metrics.radius = DPI(" << cfg.radius << ");\n"
            << "style.metrics.content_margin = Rect(DPI(" << cfg.margin_x << "), DPI(" << cfg.margin_y
            << "), DPI(" << cfg.margin_x << "), DPI(" << cfg.margin_y << "));\n"
            << "style.font.Height(" << cfg.font_height << ");\n"
            << "style.text_align = " << AlignCode(cfg.text_align) << ";\n"
            << "style.placeholder_ink = " << CppColor(cfg.placeholder_ink) << ";\n"
            << "style.caret_color = " << CppColor(cfg.caret) << ";\n"
            << "style.caret_width = DPI(" << cfg.caret_width << ");\n"
            << "style.block_caret = " << CppBool(cfg.block_caret) << ";\n"
            << "style.selection_color = " << CppColor(cfg.selection_face) << ";\n"
            << "style.selection_ink = " << CppColor(cfg.selection_ink) << ";\n"
            << "style.underline_enabled = " << CppBool(cfg.underline_enabled) << ";\n"
            << "style.underline_width = DPI(" << cfg.underline_width << ");\n";
        if(selected_ == EDIT_MULTI || explicit_style) {
            out << "style.tab_size = " << cfg.tab_size << ";\n"
                << "style.show_tabs = " << CppBool(cfg.show_tabs) << ";\n"
                << "style.show_spaces = " << CppBool(cfg.show_spaces) << ";\n"
                << "style.show_line_endings = " << CppBool(cfg.show_line_endings) << ";\n";
        }
        {
            out << "for(int state = 0; state < 4; ++state) {\n"
                << "    style.palette.face[state] = UiFill::Solid(" << CppColor(cfg.face) << ");\n"
                << "    style.palette.frame[state] = " << CppColor(cfg.frame) << ";\n"
                << "    style.palette.ink[state] = " << CppColor(cfg.ink) << ";\n"
                << "    style.underline[state] = " << CppColor(cfg.underline) << ";\n"
                << "}\n";
        }
        out << "edit.SetCustomStyle(style);\n";
    }

    void EmitSpecificSetup(String& out, const EditConfig& cfg) const
    {
        if(selected_ == EDIT_PASSWORD) {
            String mask = cfg.password_char == "Asterisk" ? "'*'" :
                          cfg.password_char == "MiddleDot" ? "0x00B7" : "0x2022";
            out << "edit.SetPasswordChar(" << mask << ")\n"
                << "    .EnableVisibilityIcon(" << CppBool(cfg.password_eye) << ")\n"
                << "    .SetPlainTextVisible(" << CppBool(cfg.password_plain) << ");\n";
        }
        else if(selected_ == EDIT_MASK) {
            char prompt = cfg.mask_prompt.IsEmpty() ? '_' : cfg.mask_prompt[0];
            out << "edit.SetMask(" << CppString(cfg.mask) << ", " << (int)(byte)prompt << ");\n";
            if(cfg.mask_validator == "Date") out << "edit.SetValidator(UiMaskEdit::DateValidator());\n";
            else if(cfg.mask_validator == "Time") out << "edit.SetValidator(UiMaskEdit::TimeValidator());\n";
            else if(cfg.mask_validator == "NonEmpty") out << "edit.SetValidator(UiMaskEdit::NonEmptyValidator());\n";
            else if(cfg.mask_validator == "Alnum") out << "edit.SetValidator(UiMaskEdit::AlnumUnderscoreValidator(true));\n";
            if(cfg.mask_formatter == "Uppercase") out << "edit.SetFormatter(UiMaskEdit::UppercaseFormatter());\n";
            else if(cfg.mask_formatter == "Lowercase") out << "edit.SetFormatter(UiMaskEdit::LowercaseFormatter());\n";
            else if(cfg.mask_formatter == "TitleCase") out << "edit.SetFormatter(UiMaskEdit::TitleCaseFormatter());\n";
            else if(cfg.mask_formatter == "Username") out << "edit.SetFormatter(UiMaskEdit::UsernameFormatter());\n";
            else if(cfg.mask_formatter == "SafeAlnum") out << "edit.SetFormatter(UiMaskEdit::SafeAlnumFormatter());\n";
            if(cfg.mask_show_error)
                out << "edit.ShowError(!edit.IsValid());\n";
        }
        else if(selected_ == EDIT_MULTI)
        {
            out << "edit.SetAcceptsTabs(" << CppBool(cfg.multi_accept_tabs) << ");\n";
            if(cfg.multi_left_icon) out << "UiToolButton icon;\nicon.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(16), DPI(16));\nedit.AddToSide(icon, UiAlign::LEFT, Size(DPI(24), DPI(24))).Overlay(false);\n";
            if(cfg.multi_clear_action) out << "UiToolButton clear;\nclear.SetIcon(ICON_DESIGN_DELETE_48()).SetIconSize(DPI(16), DPI(16));\nedit.AddToSide(clear, UiAlign::RIGHT, Size(DPI(24), DPI(24))).Overlay(false);\nclear.WhenAction = [&] { edit.SetTextUtf8(\"\"); };\n";
        }
    }

    String AuthoredStyleCode(const String& source) const
    {
        String result;
        for(const String& line : Split(source, '\n', false)) {
            bool keep = true;
            if(TrimLeft(line).StartsWith("style.palette.face")) keep = cfg_[selected_].face != EditConfig().face;
            if(TrimLeft(line).StartsWith("style.palette.frame")) keep = cfg_[selected_].frame != EditConfig().frame;
            if(TrimLeft(line).StartsWith("style.palette.ink")) keep = cfg_[selected_].ink != EditConfig().ink;
            if(TrimLeft(line).StartsWith("style.underline")) keep = cfg_[selected_].underline != EditConfig().underline;
            if(TrimLeft(line).StartsWith("style.metrics.frame_enabled")) keep = cfg_[selected_].frame_width != EditConfig().frame_width;
            if(TrimLeft(line).StartsWith("style.metrics.frame_width")) keep = cfg_[selected_].frame_width != EditConfig().frame_width;
            if(TrimLeft(line).StartsWith("style.metrics.radius")) keep = cfg_[selected_].radius != EditConfig().radius;
            if(TrimLeft(line).StartsWith("style.metrics.content_margin")) keep = cfg_[selected_].margin_x != EditConfig().margin_x || cfg_[selected_].margin_y != EditConfig().margin_y;
            if(TrimLeft(line).StartsWith("style.font.Height")) keep = cfg_[selected_].font_height != EditConfig().font_height;
            if(TrimLeft(line).StartsWith("style.text_align")) keep = cfg_[selected_].text_align != EditConfig().text_align;
            if(TrimLeft(line).StartsWith("style.placeholder_ink")) keep = cfg_[selected_].placeholder_ink != EditConfig().placeholder_ink;
            if(TrimLeft(line).StartsWith("style.caret_color")) keep = cfg_[selected_].caret != EditConfig().caret;
            if(TrimLeft(line).StartsWith("style.caret_width")) keep = cfg_[selected_].caret_width != EditConfig().caret_width;
            if(TrimLeft(line).StartsWith("style.block_caret")) keep = cfg_[selected_].block_caret != EditConfig().block_caret;
            if(TrimLeft(line).StartsWith("style.selection_color")) keep = cfg_[selected_].selection_face != EditConfig().selection_face;
            if(TrimLeft(line).StartsWith("style.selection_ink")) keep = cfg_[selected_].selection_ink != EditConfig().selection_ink;
            if(TrimLeft(line).StartsWith("style.underline_enabled")) keep = cfg_[selected_].underline_enabled != EditConfig().underline_enabled;
            if(TrimLeft(line).StartsWith("style.underline_width")) keep = cfg_[selected_].underline_width != EditConfig().underline_width;
            if(TrimLeft(line).StartsWith("style.tab_size")) keep = cfg_[selected_].tab_size != EditConfig().tab_size;
            if(TrimLeft(line).StartsWith("style.show_tabs")) keep = cfg_[selected_].show_tabs != EditConfig().show_tabs;
            if(TrimLeft(line).StartsWith("style.show_spaces")) keep = cfg_[selected_].show_spaces != EditConfig().show_spaces;
            if(TrimLeft(line).StartsWith("style.show_line_endings")) keep = cfg_[selected_].show_line_endings != EditConfig().show_line_endings;
            if(keep) result << line << "\n";
        }
        Vector<String> lines = Split(result, '\n', false);
        bool authored = false;
        for(const String& line : lines) if(TrimLeft(line).StartsWith("style.")) authored = true;
        result.Clear();
        for(int i = 0; i < lines.GetCount(); i++) {
            String trimmed = TrimLeft(lines[i]);
            if(trimmed.StartsWith("for(int state") && i + 1 < lines.GetCount() && TrimBoth(lines[i + 1]) == "}") { i++; continue; }
            if(!authored && (lines[i].Find("::Style style =") >= 0 || lines[i].Find(".SetCustomStyle(style)") >= 0)) continue;
            result << lines[i] << "\n";
        }
        return result;
    }

    void UpdateCode()
    {
        const EditConfig& cfg = cfg_[selected_];
        String mode = AsString(code_mode_.GetSelectedData());
        String type = selected_ == EDIT_PASSWORD ? "UiPasswordEdit" :
                      selected_ == EDIT_MASK ? "UiMaskEdit" :
                      selected_ == EDIT_MULTI ? "UiMultiEdit" : "UiLineEdit";
        String out = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        out << type << " edit;\n\n";
        out << "// Common UiBaseEdit content and behaviour.\n";
        out << "edit.SetTextUtf8(" << CppString(cfg.text) << ");\n"
            << "edit.SetPlaceholder(" << CppString(cfg.placeholder) << ");\n"
            << "edit.SetAcceptsDrop(" << CppBool(cfg.accepts_drop) << ");\n"
            << "edit.SetOverwriteMode(" << CppBool(cfg.overwrite) << ");\n"
            << "edit.SetTextAlign(" << AlignCode(cfg.text_align) << ");\n"
            << "edit.SetEditable(" << CppBool(!cfg.read_only) << ");\n"
            << "edit.Enable(" << CppBool(cfg.enabled) << ");\n";
        EmitSpecificSetup(out, cfg);

        if(mode == "changes") {
            if(AnyStyleChange()) EmitStyle(out, cfg, false);
            else out << "\n// No local design changes: the active UiTheme supplies the style.\n";
        }
        else if(mode == "explicit")
            EmitStyle(out, cfg, true);
        else
            out << "\n// Usage mode deliberately relies on UiTheme for visual styling.\n";

        if(selected_ == EDIT_MASK)
        {
            out << "\nedit.WhenChange = [&] { edit.ShowError(!edit.IsValid()); };\n";
            if(cfg.mask_flash) out << "edit.WhenAction = [&] { if(edit.IsValid()) edit.FlashSuccess(); else edit.FlashError(); };\n";
        }
        else
            out << "\nedit.WhenChange = [&] { String text = edit.GetTextUtf8(); /* react */ };\n";
        if(mode == "changes") out = AuthoredStyleCode(out);
        const String preamble = "#include <Ui/Ui.h>\n\nusing namespace Upp;\n\n";
        if(out.StartsWith(preamble)) {
            String body = out.Mid(preamble.GetCount());
            String members, setup;
            for(const String& line : Split(body, '\n', false)) {
                String declaration = TrimBoth(line);
                bool member = declaration.StartsWith("Ui") && declaration.EndsWith(";")
                           && declaration.Find("::") < 0 && declaration.Find('(') < 0
                           && declaration.Find('=') < 0 && declaration.Find('.') < 0;
                if(member) members << "    " << declaration << "\n";
                else setup << "        " << line << "\n";
            }
            // Borrowed side controls outlive the edit that hosts them.
            members.Replace("    " + type + " edit;\n", "");
            members << "    " << type << " edit;\n";
            out = preamble + "class ControlExample : public ParentCtrl {\n" + members
                + "public:\n    ControlExample() {\n" + setup;
            out << "        Add(edit.SizePos());\n";
            out << "    }\n};\n";
        }
        code_.SetTextUtf8(out);
    }

    void ToggleTheme()
    {
        UiThemeContext context = UiTheme::GetContext();
        context.mode = context.mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark;
        UiTheme::Set(context);
        Ctrl::SwapDarkLight();
        ApplyTheme();
        ApplyAllSamples();
    }

    bool IsStyleProperty(const String& id) const
    {
        static const char* ids[] = { "block_caret", "caret", "caret_width", "face", "font_height", "frame", "frame_width", "ink", "margin_x", "margin_y", "placeholder_ink", "radius", "selection_face", "selection_ink", "show_line_endings", "show_spaces", "show_tabs", "tab_size", "text_align", "underline", "underline_enabled", "underline_width" };
        for(const char* name : ids) if(id == name) return true;
        return false;
    }

    void UpdatePropertyPage()
    {
        for(const PropertyEditorItem& item : model_.GetItems())
            model_.SetVisible(item.id, IsStyleProperty(item.id) == overrides_view_, false);
        model_.StructureChanged();
    }

    void SelectStylePage(bool style)
    {
        overrides_view_ = style;
        SetCodeView(false);
    }

    void ApplyTheme()
    {
        const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
        theme_button_.SetIcon(dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
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
        rail_panel_.SetCustomStyle(surface);


        const auto mode = dark
                        ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
        properties_.SetPaletteMode(mode);

        for(PropertyEditor* editor : { &properties_ }) {
            PropertyEditorStyle editor_style = editor->GetStyle();
            editor_style.show_frame = false;
            editor_style.background = panel_face;
            editor_style.show_group_summaries = true;
            editor->SetStyle(editor_style);
        }
        for(UiToolButton* button : { &theme_button_, &help_button_, &exit_button_, &props_button_, &overrides_button_, &code_button_, &multi_icon_, &multi_clear_ }) {
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
        UiToolButton::Style exit_style = exit_button_.GetStyle();
        exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
        exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
        exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
        exit_button_.SetCustomStyle(exit_style);
        line_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        password_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        mask_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        multi_label_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        status_.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
        code_mode_.SetCustomStyle(UiTheme::ResolveDropdown(UiRole::Standard));
        UiButton *selectors[] = { &line_select_, &password_select_, &mask_select_, &multi_select_ };
        for(int i = 0; i < EDIT_COUNT; i++)
            selectors[i]->SetCustomStyle(UiTheme::ResolveButton(i == selected_ ? UiRole::Accent : UiRole::Subtle));
        Refresh();
    }

private:
    bool overrides_view_ = false;
    Color window_face_ = SColorFace();
    EditConfig cfg_[EDIT_COUNT];
    EditSample selected_ = EDIT_LINE;

    PropertyEditorFactory factory_;
    PropertyEditorModel model_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ { UiDirection::H };
    UiToolButton theme_button_, help_button_, exit_button_;
    UiPanel preview_panel_, rail_panel_;
    UiBoxLayout selector_ { UiDirection::H };
    UiButton line_select_, password_select_, mask_select_, multi_select_;
    UiLabel line_label_, password_label_, mask_label_, multi_label_, status_;
    UiLineEdit line_;
    UiPasswordEdit password_;
    UiMaskEdit mask_;
    UiToolButton multi_icon_, multi_clear_;
    UiMultiEdit multi_;

    UiBoxLayout view_bar_ { UiDirection::H };
    UiToolButton props_button_, overrides_button_, code_button_;
    PropertyEditor properties_;
    UiDropdown code_mode_;
    UiMultiEdit code_;
    bool code_view_ = false;
};

} // namespace

GUI_APP_MAIN
{
    UiEditDemoWindow demo;
    const Vector<String>& args = CommandLine();
    if(args.GetCount() == 2 && args[0] == "--export-generated") demo.ExportGenerated(args[1]);
    else if(args.GetCount()==2 && args[0]=="--selector-test") {
        demo.Open(); Ctrl::ProcessEvents();
        bool passed=demo.TestSelectors(args[1]); demo.Close(); SetExitCode(passed?0:1);
    }
    else demo.Run();
}
