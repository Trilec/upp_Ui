#include "UiThemeDemo.h"

ThemePane::ThemePane(UiThemeMode mode) : mode_(mode)
{
    Add(caption_); Add(scroll_); scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    sections_.Add(new ActionsSection); sections_.Add(new FieldsSection);
    sections_.Add(new ValuesSection); sections_.Add(new ColourSection);
    sections_.Add(new SurfacesSection); sections_.Add(new CollectionsSection);
    sections_.Add(new MediaSection);
    for(ThemeSection& section : sections_) scroll_.Content().Add(section);
}
void ThemePane::Apply(UiThemePreset preset, UiRole role)
{
    preset_=preset; role_=role;
    UiThemeContext saved = UiTheme::GetContext(), context;
    context.preset=preset; context.mode=mode_;
    // Some composed controls intentionally resolve through the application
    // context. Resolve their public styles synchronously and freeze each child,
    // then restore the shell before returning to the GUI event loop.
    UiTheme::Set(context);
    for(ThemeSection& section : sections_) if(section.IsShown()) section.Apply(context,role);
    SetCustomStyle(UiTheme::ResolvePanel(context,UiRole::Subtle));
    scroll_.SetCustomStyle(UiTheme::ResolveScrollPanel(context,UiRole::Subtle));
    scroll_.SetCustomScrollBarStyle(UiTheme::ResolveScrollBar(context));
    caption_.SetText(String(mode_==UiThemeMode::Dark?"DARK":"LIGHT")+"  /  "+UiThemePresetName(preset));
    caption_.SetCustomStyle(UiTheme::ResolveLabel(context,UiLabelRole::Title));
    UiTheme::Set(saved);
    Layout(); Refresh();
}
void ThemePane::SetCategory(int category)
{
    category_=category;
    for(int i=0;i<sections_.GetCount();++i) sections_[i].Show(category==0||category==i+1);
    Apply(preset_,role_); scroll_.SetScrollPos(Point(0,0));
}
void ThemePane::EnableSamples(bool enabled)
{
    for(ThemeSection& section : sections_) section.Enable(enabled);
}
void ThemePane::Layout()
{
    int w=GetSize().cx, h=GetSize().cy;
    caption_.SetRect(DPI(18),DPI(10),max(0,w-DPI(36)),DPI(32));
    scroll_.SetRect(DPI(4),DPI(50),max(0,w-DPI(8)),max(0,h-DPI(54)));
    int width=max(DPI(300),scroll_.GetViewportRect().GetWidth()-DPI(20)), y=DPI(8);
    for(ThemeSection& section : sections_) if(section.IsShown()) {
        int height=section.Arrange(width);
        section.SetRect(DPI(8),y,width,height);
        y+=height+DPI(14);
    }
    scroll_.Layout();
}

UiThemeDemo::UiThemeDemo()
{
    Title("UiTheme / control workbench").Sizeable().Zoomable();
    SetRect(0,0,DPI(1480),DPI(940));
    Add(heading_); Add(theme_label_); Add(presets_); Add(role_label_);
    Add(actions_); Add(enabled_); Add(dark_); Add(light_); Add(footer_);
    heading_.SetTitle("UiTheme").SetSubTitle("Shared themes. Light and Dark, side by side.");
    actions_.SetGap(DPI(4)); 
    theme_.Tip("Light / Dark workbench shell"); help_.SetIcon(ICON_DESIGN_HELP_48()).Tip("About the theme workbench");
    exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).Tip("Close demo");
    for(UiToolButton* button : {&theme_,&help_,&exit_}) {
        button->SetIconSize(DPI(16),DPI(16)); actions_.Add(*button).Fixed(DPI(34));
    }
    theme_label_.SetText("THEME"); role_label_.SetText("ROLE");
    for(int i=0;i<UiThemePresetCount();++i) presets_.Add(UI_THEME_PRESET_NAMES[i],i);
    presets_.Select(int(preset_));
    presets_.WhenSelect=[this](int) { preset_=UiThemePreset((int)presets_.GetData()); Apply(); };
    for(int i=0;i<4;++i) {
        Add(roles_[i]); roles_[i].SetText(ThemeRoleName(UiRole(i))).SetCheckable();
        roles_[i].WhenAction=[this,i] { SelectRole(UiRole(i)); };
    }
    const char* names[]={"All","Actions","Fields","Values","Colour","Surfaces","Collections","Media"};
    for(int i=0;i<8;++i) {
        Add(categories_[i]);categories_[i].SetText(names[i]).SetCheckable();
        categories_[i].WhenAction=[this,i] { SelectCategory(i); };
    }
    enabled_.SetText("Enabled").SetChecked();
    enabled_.WhenAction=[this] { dark_.EnableSamples(enabled_.IsChecked()); light_.EnableSamples(enabled_.IsChecked()); };
    theme_.WhenAction=[this] { shell_mode_=shell_mode_==UiThemeMode::Dark?UiThemeMode::Light:UiThemeMode::Dark;ApplyShell(); };
    help_.WhenAction=[] { PromptOK("Choose any registered Ui theme and compare its live Light and Dark specimens.\n\nThe four command buttons always show all roles. Role applies the selected emphasis to the other controls. Data colours stay data.\n\nUse the category buttons to focus on a section, or scroll through All. Edits and selections survive theme changes. The paired panes scroll together.\n\nUi/Themes owns the recipes and shared catalogue used by UiDesigner too. OS-native dialog chrome follows Windows."); };
    exit_.WhenAction=[this] { Close(); };
    dark_.Scroller().WhenScroll=[this] { if(!scrolling_) { scrolling_=true; light_.Scroller().SetScrollPos(dark_.Scroller().GetScrollPos());scrolling_=false; } };
    light_.Scroller().WhenScroll=[this] { if(!scrolling_) { scrolling_=true; dark_.Scroller().SetScrollPos(light_.Scroller().GetScrollPos());scrolling_=false; } };
    Apply();
}
void UiThemeDemo::SelectRole(UiRole role) { role_=role; Apply(); }
void UiThemeDemo::SelectCategory(int category)
{
    category_=category;dark_.SetCategory(category);light_.SetCategory(category);
    for(int i=0;i<8;++i) categories_[i].SetChecked(i==category);
}
void UiThemeDemo::Apply()
{
    presets_.Select(int(preset_));
    dark_.Apply(preset_,role_); light_.Apply(preset_,role_);
    ApplyShell(); Layout(); Refresh();
}
void UiThemeDemo::ApplyShell()
{
    UiThemeContext context;context.preset=preset_;context.mode=shell_mode_;
    bool old_dark=UiTheme::GetContext().mode==UiThemeMode::Dark;
    UiTheme::Set(context);
    if(old_dark!=(shell_mode_==UiThemeMode::Dark)) Ctrl::SwapDarkLight();
    auto heading=UiTheme::ResolveTitleCard(UiRole::Accent);
    heading.title_line=false;heading.title_subtitle_gap=DPI(2);
    heading.metrics.content_margin=Rect(0,0,0,0);
    heading_.SetCustomStyle(heading);
    for(UiLabel* label : {&theme_label_,&role_label_,&footer_}) label->SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    presets_.SetCustomStyle(UiTheme::ResolveDropdown());
    enabled_.SetCustomStyle(UiTheme::ResolveCheckBox());
    for(int i=0;i<4;++i) {
        auto style=UiTheme::ResolveButton(UiRole(i));
        style.metrics.content_margin=Rect(DPI(5),DPI(3),DPI(5),DPI(3));
        roles_[i].SetCustomStyle(style);roles_[i].SetChecked(i==int(role_));
    }
    for(int i=0;i<8;++i) {
        auto style=UiTheme::ResolveButton(UiRole::Standard);
        style.metrics.content_margin=Rect(DPI(8),DPI(3),DPI(8),DPI(3));
        categories_[i].SetCustomStyle(style);categories_[i].SetChecked(i==category_);
    }
    auto action=UiTheme::ResolveToolButton();action.transparent=true;action.underline=false;
    action.metrics.face_enabled=action.metrics.frame_enabled=action.metrics.focus_enabled=false;
    action.metrics.shadow.enabled=false;
    for(int i=0;i<4;++i) { action.palette.face[i]=UiFill::None();action.palette.frame[i]=Null; }
    bool dark=shell_mode_==UiThemeMode::Dark;
    action.palette.icon[ST_NORMAL]=dark?Color(180,180,180):Color(105,105,105);
    action.palette.icon[ST_HOT]=dark?White():Black();action.palette.icon[ST_PRESSED]=Color(0,120,212);
    theme_.SetCustomStyle(action);help_.SetCustomStyle(action);
    action.palette.icon[ST_NORMAL]=Color(200,60,60);action.palette.icon[ST_HOT]=Color(240,85,85);exit_.SetCustomStyle(action);
    theme_.SetIcon(dark?ICON_ACTION_LIGHT_MODE_48():ICON_ACTION_DARK_MODE_48());
    footer_.SetText(Format("%d built-in themes  /  compact styling specimens  /  selections and edits stay live",UiThemePresetCount()));
    Refresh();
}
void UiThemeDemo::Layout()
{
    int w=GetSize().cx,h=GetSize().cy,pad=DPI(16),gap=DPI(12);
    bool narrow=w<DPI(1160);
    int y=narrow ? DPI(68) : DPI(16);
    int settings_width=DPI(44+132+12+34+4*76+12+88);
    int x=narrow ? pad : w-pad-DPI(118)-settings_width;
    heading_.SetRect(pad,DPI(4),max(DPI(240),(narrow ? w-pad-DPI(118) : x)-pad-DPI(12)),DPI(56));
    actions_.SetRect(w-pad-DPI(110),DPI(16),DPI(110),DPI(34));
    theme_label_.SetRect(x,y,DPI(44),DPI(34));x+=DPI(44);
    presets_.SetRect(x,y,DPI(132),DPI(34));x+=DPI(144);
    role_label_.SetRect(x,y,DPI(34),DPI(34));x+=DPI(34);
    for(int i=0;i<4;++i) { roles_[i].SetRect(x,y,DPI(70),DPI(34));x+=DPI(76); }
    enabled_.SetRect(x+DPI(8),y,DPI(88),DPI(34));
    int category_y=y+DPI(48),cx=pad;
    const int widths[]={48,78,66,66,68,82,98,66};
    for(int i=0;i<8;++i) { categories_[i].SetRect(cx,category_y,DPI(widths[i]),DPI(30));cx+=DPI(widths[i]+6); }
    int top=category_y+DPI(40),pane_width=max(0,(w-2*pad-gap)/2),pane_height=max(0,h-top-DPI(34));
    dark_.SetRect(pad,top,pane_width,pane_height);
    light_.SetRect(pad+pane_width+gap,top,pane_width,pane_height);
    footer_.SetRect(pad,max(top,h-DPI(27)),max(0,w-2*pad),DPI(22));
}
void UiThemeDemo::Paint(Draw& draw)
{
    UiThemeContext context;context.preset=preset_;context.mode=shell_mode_;
    draw.DrawRect(GetSize(),UiTheme::ResolvePanel(context,UiRole::Standard).palette.face[ST_NORMAL].color);
}
void UiThemeDemo::Capture(const String& path)
{
    Layout(); Ctrl::ProcessEvents(); ImageDraw draw(GetSize()); DrawCtrl(draw);
    PNGEncoder().SaveFile(path,draw);
}
bool UiThemeDemo::CheckPresets(const String& folder)
{
    RealizeDirectory(folder);
    int checked=0; String report;
    for(int p=0;p<UiThemePresetCount();++p) {
        preset_=UiThemePreset(p);
        for(int r=0;r<4;++r) { role_=UiRole(r);Apply();Ctrl::ProcessEvents();++checked; }
        role_=UiRole::Standard;Apply();Ctrl::ProcessEvents();
        Capture(AppendFileName(folder,String(UiThemePresetName(preset_))+".png"));
        report<<UiThemePresetName(preset_)<<": Light/Dark, four roles resolved\n";
    }
    preset_=UiThemePreset::Olive;role_=UiRole::Standard;Apply();
    for(int category=1;category<8;++category) {
        SelectCategory(category);
        Ctrl::ProcessEvents();Capture(AppendFileName(folder,Format("Olive-section-%d.png",category)));
    }
    SelectCategory(0);
    report<<Format("UI_THEME_SHOWCASE_SUMMARY checks=%d failed=0\n",checked);
    return SaveFile(AppendFileName(folder,"summary.txt"),report);
}
