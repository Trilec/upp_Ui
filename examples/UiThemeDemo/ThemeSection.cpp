#include "UiThemeDemo.h"

const char* ThemeRoleName(UiRole role)
{
    static const char* names[] = {"Standard", "Subtle", "Accent", "Alert"};
    return names[int(role)];
}

Image ThemeSampleImage()
{
    ImageBuffer buffer(320, 180);
    for(int y = 0; y < 180; ++y)
        for(int x = 0; x < 320; ++x) {
            bool sun = (x - 245)*(x - 245) + (y - 48)*(y - 48) < 24*24;
            buffer[y][x] = sun ? Color(232, 173, 91) : Blend(Color(42, 64, 100), Color(91, 126, 130), (x + y)/2);
        }
    return Image(buffer);
}

namespace {
template<class T> bool Freeze(Ctrl& control)
{
    if(T* typed = dynamic_cast<T*>(&control)) {
        typed->ClearCustomStyle();
        auto style = typed->GetStyle();
        typed->SetCustomStyle(style);
        return true;
    }
    return false;
}
}

// Resolve only on a user theme/role change, never during Paint. Freeze composite
// children too, so restoring the application's shell context cannot recolour the
// other pane. No controls, models or edited values are reconstructed here.
void FreezeThemeTree(Ctrl& root, UiRole role)
{
    if(auto* p = dynamic_cast<UiDoc*>(&root)) p->SetCustomStyle(UiTheme::ResolveDoc());
    else if(auto* p = dynamic_cast<UiToolButton*>(&root)) p->SetCustomStyle(UiTheme::ResolveToolButton(role));
    else if(auto* p = dynamic_cast<UiButton*>(&root)) p->SetCustomStyle(UiTheme::ResolveButton(role));
    else if(auto* p = dynamic_cast<UiBaseEdit*>(&root)) p->SetCustomStyle(UiTheme::ResolveEdit(role));
    else if(auto* p = dynamic_cast<UiScrollPanel*>(&root)) {
        p->SetCustomStyle(UiTheme::ResolveScrollPanel(role));
        p->SetCustomScrollBarStyle(UiTheme::ResolveScrollBar());
    }
    else if(auto* p = dynamic_cast<UiGroupPanel*>(&root)) p->SetCustomStyle(UiTheme::ResolveGroupPanel(role));
    else if(auto* p = dynamic_cast<UiPanel*>(&root)) p->SetCustomStyle(UiTheme::ResolvePanel(role));
    else if(auto* p = dynamic_cast<UiLabel*>(&root)) p->SetCustomStyle(UiTheme::ResolveLabel(role, UiTextSize::Body));
    else if(auto* p = dynamic_cast<UiCheckBox*>(&root)) p->SetCustomStyle(UiTheme::ResolveCheckBox(role));
    else if(auto* p = dynamic_cast<UiRadioButton*>(&root)) p->SetCustomStyle(UiTheme::ResolveRadioButton(role));
    else if(auto* p = dynamic_cast<UiToggle*>(&root)) p->SetCustomStyle(UiTheme::ResolveToggle(role));
    else if(auto* p = dynamic_cast<UiTitleCard*>(&root)) p->SetCustomStyle(UiTheme::ResolveTitleCard(role));
    else if(auto* p = dynamic_cast<UiTab*>(&root)) p->SetCustomStyle(UiTheme::ResolveTab(role));
    else if(auto* p = dynamic_cast<UiDropdown*>(&root)) p->SetCustomStyle(UiTheme::ResolveDropdown(role));
    else if(auto* p = dynamic_cast<UiSlider*>(&root)) p->SetCustomStyle(UiTheme::ResolveSlider(role));
    else if(auto* p = dynamic_cast<UiRangeSlider*>(&root)) p->SetCustomStyle(UiTheme::ResolveSlider(role));
    else if(auto* p = dynamic_cast<UiRangeSegments*>(&root)) p->SetCustomStyle(UiTheme::ResolveRangeSegments(role));
    else if(auto* p = dynamic_cast<UiProgressBar*>(&root)) p->SetCustomStyle(UiTheme::ResolveProgressBar(role));
    else if(auto* p = dynamic_cast<UiSplitter*>(&root)) p->SetCustomStyle(UiTheme::ResolveSplitter(role));
    else if(auto* p = dynamic_cast<UiTree*>(&root)) p->SetCustomStyle(UiTheme::ResolveTree(UiTheme::GetContext(), role));
    else if(auto* p = dynamic_cast<UiList*>(&root)) p->SetCustomStyle(UiTheme::ResolveList(role));
    else if(auto* p = dynamic_cast<UiTable*>(&root)) p->SetCustomStyle(UiTheme::ResolveTable(role));
    else if(auto* p = dynamic_cast<UiMenu*>(&root)) p->SetCustomStyle(UiTheme::ResolveMenu(UiTheme::GetContext()));
    else if(auto* p = dynamic_cast<UiProgressRing*>(&root)) { p->SetRole(role); Freeze<UiProgressRing>(root); }
    else if(Freeze<UiBreadcrumbs>(root) || Freeze<UiMediaCard>(root) || Freeze<UiGallery>(root)
         || Freeze<UiDateTime>(root) || Freeze<UiColorMatrix>(root) || Freeze<UiColorPicker>(root)
         || Freeze<UiMatrixSelector>(root) || Freeze<UiChartRing>(root) || Freeze<UiAccordion>(root)
         || Freeze<UiScrollBar>(root) || Freeze<UiBezierCurveEditor>(root)) {}
    for(Ctrl* child = root.GetFirstChild(); child; child = child->GetNext())
        FreezeThemeTree(*child, role);
}

ThemeSection::ThemeSection(const String& title, const String& detail)
{
    Add(heading_); Add(detail_);
    heading_.SetText(title);
    detail_.SetText(detail);
}

void ThemeSection::AddSpecimen(const char* type, Ctrl& control, int height)
{
    UiLabel& label = names_.Add();
    label.SetText(type).SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    Add(label); Add(control);
    Row& row = rows_.Add(); row.control = &control; row.height = height; row.label = names_.GetCount()-1;
}

void ThemeSection::AddSpecimenPair(const char* first_name, Ctrl& first,
                                   const char* second_name, Ctrl& second,
                                   int height, bool labels_above)
{
    AddSpecimen(first_name,first,height);
    Row& row=rows_.Top();row.second=&second;row.labels_above=labels_above;
    UiLabel& label=names_.Add();label.SetText(second_name);
    Add(label);Add(second);
}

int ThemeSection::Arrange(int width)
{
    const int pad=DPI(12), gap=DPI(6);
    heading_.SetRect(pad,DPI(8),max(0,width-2*pad),DPI(24));
    detail_.SetRect(pad,DPI(33),max(0,width-2*pad),DPI(18));
    int y=DPI(58);
    for(const Row& row : rows_) {
        int cells=row.second ? 2 : 1;
        int cell_width=(width-2*pad-(cells-1)*DPI(14))/cells;
        bool above=row.labels_above || row.height>80;
        bool stack_pair=row.second && cell_width<DPI(250);
        if(stack_pair) cell_width=width-2*pad;
        int height=max(DPI(row.height),row.height<=44 ? row.control->GetMinSize().cy : 0);
        if(row.second && row.height<=44) height=max(height,row.second->GetMinSize().cy);
        for(int cell=0;cell<cells;++cell) {
            int x=pad+(stack_pair ? 0 : cell*(cell_width+DPI(14)));
            Ctrl& control=cell ? *row.second : *row.control;
            UiLabel& name=names_[row.label+cell];
            if(above) {
                name.SetRect(x,y,cell_width,DPI(20));
                control.SetRect(x,y+DPI(22),max(0,cell_width),height);
            }
            else {
                int label_width=min(DPI(row.second ? 88 : 114),cell_width/3);
                name.SetRect(x,y,label_width,height);
                control.SetRect(x+label_width+DPI(5),y,max(0,cell_width-label_width-DPI(5)),height);
            }
            if(stack_pair && cell==0) y+=height+gap+(above ? DPI(22) : 0);
        }
        y+=height+gap+(above ? DPI(22) : 0);
    }
    return y+DPI(6);
}

void ThemeSection::Apply(const UiThemeContext& context, UiRole role)
{
    FreezeThemeTree(*this, role);
    SetCustomStyle(UiTheme::ResolvePanel(context, UiRole::Standard));
    heading_.SetCustomStyle(UiTheme::ResolveLabel(context, UiLabelRole::Title));
    auto caption = UiTheme::ResolveLabel(context, UiLabelRole::Caption);
    detail_.SetCustomStyle(caption);
    for(UiLabel& name : names_) name.SetCustomStyle(caption);
}

void ThemeTags::Apply()
{
    for(int i = 0; i < 4; ++i) styles_[i] = UiResolveTagStyle(UiRole(i));
    Layout(); Refresh();
}
void ThemeTags::Layout()
{
    int width = max(1, GetSize().cx / 4);
    for(int i = 0; i < 4; ++i) {
        UiTagData data(ThemeRoleName(UiRole(i)), UiRole(i), UiTagVariant::Soft);
        prepared_[i] = UiPrepareTag(data, styles_[i], RectC(i*width, 0, width-DPI(6), GetSize().cy));
    }
}
void ThemeTags::Paint(Draw& draw)
{
    for(int i = 0; i < 4; ++i) UiPaintTag(draw, prepared_[i], ST_NORMAL);
}
