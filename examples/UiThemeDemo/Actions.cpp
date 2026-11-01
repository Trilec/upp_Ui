#include "UiThemeDemo.h"

ActionsSection::ActionsSection()
    : ThemeSection("Actions and choices", "Four semantic roles, real selection and native popup behaviour")
{
    commands_.SetGap(DPI(6)); icons_.SetGap(DPI(8)); choices_.SetGap(DPI(8));
    for(int i = 0; i < 4; ++i) {
        buttons_[i].SetText(ThemeRoleName(UiRole(i)));
        commands_.Add(buttons_[i]).Expand();
        tools_[i].SetIcon(ICON_DESIGN_SETTINGS_48()).SetIconSize(DPI(18),DPI(18)).Tip(ThemeRoleName(UiRole(i)));
        icons_.Add(tools_[i]).Fixed(DPI(38));
    }
    icons_.Add(disabled_).Expand(); disabled_.SetText("Disabled").Disable();
    AddSpecimen("UiButton", commands_);
    AddSpecimen("UiToolButton", icons_);
    split_.SetText("Export"); split_.Add("Image", "image").Add("Document", "document");
    check_.SetText("Include").SetChecked();
    radio_[0].SetText("Draft").SetGroup(1).SetChecked(); radio_[1].SetText("Final").SetGroup(1);
    toggle_.SetOn(true);
    choices_.Add(check_).Expand(); choices_.Add(radio_[0]).Expand(); choices_.Add(radio_[1]).Expand(); choices_.Add(toggle_).Fixed(DPI(64)); choices_.AddSpacer(1).Fixed(DPI(6));
    AddSpecimen("Check / Radio / Toggle", choices_);
    dropdown_.Add("Draft quality",0).Add("Review quality",1).Add("Final quality",2).Select(1);
    AddSpecimen("UiDropdown", dropdown_);
    menu_.Model().AddChild(menu_.Model().Root(), UiMenuItem("Record an action", "record"));
    menu_.Model().AddChild(menu_.Model().Root(), UiMenuItem("Export the selection", "export"));
    menu_button_.SetText("Open command menu");
    menu_button_.WhenAction = [this] { menu_.PopUp(&menu_button_, menu_button_.GetScreenRect().BottomLeft()); };
    menu_.WhenAction = [this](UiMenuNodeRef, const UiMenuItem& item) { menu_button_.SetText(item.text); };
    AddSpecimenPair("UiSplitButton",split_,"UiMenu",menu_button_);
    path_.SetPath("Projects/Aurora/Sequence 020").SetCurrentIndex(2);
    AddSpecimen("UiBreadcrumbs", path_);
    label_.SetText("A readable label follows its semantic role."); AddSpecimen("UiLabel", label_);
    AddSpecimen("UiTag", tags_, 32);
}
void ActionsSection::Apply(const UiThemeContext& context, UiRole role)
{
    ThemeSection::Apply(context, role);
    for(int i = 0; i < 4; ++i) {
        buttons_[i].SetCustomStyle(UiTheme::ResolveButton(context, UiRole(i)));
        tools_[i].SetCustomStyle(UiTheme::ResolveToolButton(context, UiRole(i)));
    }
    menu_.SetCustomStyle(UiTheme::ResolveMenu(context));
    tags_.Apply();
}

FieldsSection::FieldsSection()
    : ThemeSection("Text and numeric entry", "Edit these values, then switch themes: your work stays in place")
{
    text_.SetTextUtf8("Project Aurora"); password_.SetTextUtf8("a sample password");
    AddSpecimenPair("UiLineEdit",text_,"UiPasswordEdit",password_);
    mask_.SetMask("##/##/####").SetTextUtf8("23/10/2026");
    date_.DateTimeMode().SetFormatStyle(UiDateTimeFormatStyle::Iso).SetClockFormat(UiClockFormat::Hour24).SetNow();
    AddSpecimenPair("UiMaskEdit",mask_,"UiDateTime",date_);
    integer_.MinMax(1,10).Step(1);integer_.SetValue(3);
    decimal_.MinMax(-10,10).Step(0.25).Precision(2);decimal_.SetValue(1.25);
    AddSpecimenPair("UiIntEdit",integer_,"UiFloatEdit",decimal_);
    notes_.SetTextUtf8("Light and Dark share the same theme recipes.\nYour text survives preset and role changes.");
    AddSpecimen("UiMultiEdit",notes_,76);
}
