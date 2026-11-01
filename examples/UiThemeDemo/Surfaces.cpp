#include "UiThemeDemo.h"

SurfacesSection::SurfacesSection()
    : ThemeSection("Surfaces and composition", "Styled surfaces, tabs, expandable sections and splitters")
{
    title_.SetTitle("Project Aurora").SetSubTitle("A title card with a clear hierarchy");
    AddSpecimen("UiTitleCard",title_,64);
    panel_text_.SetText("UiPanel surface");panel_.Add(panel_text_.SizePos());
    group_text_.SetText("Related settings");group_.SetTitle("Settings").SetContent(group_text_);
    AddSpecimenPair("UiPanel",panel_,"UiGroupPanel",group_,88,true);
    for(int i=0;i<2;++i) {
        pages_[i].Add(tab_text_[i].SizePos());
        tab_text_[i].SetText(i ? "The second page retains its contents." : "Switch pages without rebuilding controls.");
        tabs_.Add(pages_[i],i ? "Details" : "Preview");
    }
    AddSpecimen("UiTab",tabs_,120);
    for(int i=0;i<2;++i) {
        int section=accordion_.AddSection(i ? "Delivery options" : "Review settings",i==0);
        accordion_text_[i].SetText(i ? "Choose the final output settings." : "Open or close either section.");
        accordion_.GetSectionContent(section).Add(accordion_text_[i].HSizePos(DPI(8),DPI(8)).TopPos(DPI(4),DPI(30)));
        accordion_.SetSectionBodyHeight(section,DPI(42));
    }
    AddSpecimen("UiAccordion",accordion_,136);
    splitter_.Set(split_panels_[0],split_panels_[1]);
    quad_.Set(quad_panels_[0],quad_panels_[1],quad_panels_[2],quad_panels_[3]);
    AddSpecimenPair("UiSplitter",splitter_,"UiQuadSplitter",quad_,110,true);
}
void SurfacesSection::Apply(const UiThemeContext& context, UiRole role)
{
    ThemeSection::Apply(context,role);
    // Reapply the composite's own header recipe after freezing descendants;
    // its chevron media reservation is structural, not a generic title style.
    auto style=UiTheme::ResolveAccordion(role);
    style.show_chevron=true;style.chevron_scale=true;style.chevron_size=DPI(14);
    style.show_drag_handle=false;style.header_height=DPI(30);style.body_min_height=DPI(42);
    accordion_.SetCustomStyle(style);
}
