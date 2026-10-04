#include "UiColorProbe.h"
#include <Ui/UiIcons.h>
#include <cmath>
namespace Upp {
UiToolButton::Style UiColorProbe::IconButton::ResolveThemeStyle() const
{
    Style style = UiToolButton::ResolveThemeStyle();
    style.palette.face[ST_NORMAL] = UiFill::None();
    style.palette.frame[ST_NORMAL] = Null;
    style.metrics.frame_enabled = false;
    style.metrics.content_margin = Rect(0,0,0,0);
    return style;
}
void UiColorProbe::Channel::Paint(Draw& w)
{
    w.DrawRect(GetSize(), Blend(SColorFace(), tint, 24));
    UiLabel::Paint(w);
}
void UiColorProbe::Swatch::Paint(Draw& w)
{
    w.DrawRect(GetSize(), IsNull(color) ? SColorFace() : color);
}
UiColorProbe::UiColorProbe()
{
    Add(swatch_); Add(mode_button_); Add(area_button_); Add(format_button_); Add(copy_button_);
    Add(accessory_); accessory_.Hide();
    accessory_.SetSizing(UIDIRECT_EXPAND,UIDIRECT_EXPAND);
    const Color tints[] = {Color(220,70,70), Color(55,180,85), Color(65,130,240), Color(140,140,140)};
    for(int i = 0; i < 4; ++i) {
        Add(channels_[i]); channels_[i].tint = tints[i];
        channels_[i].SetSelectable().SetAlignH(UiAlign::CENTER);
    }
    for(UiToolButton* button : {&mode_button_, &area_button_, &format_button_, &copy_button_}) {
        button->SetIconSize(DPI(12), DPI(12));
        button->SetIconRenderMode(UiIconRenderMode::MonoTint);
    }
    format_button_.SetIcon(ICON_NAVIGATION_OUTLINED_ARROW_DROP_DOWN_48());
    format_button_.SetIconSize(DPI(8),DPI(8));
    mode_button_.SetIcon(ICON_SAMPLE_POINT_48()).SetCheckable();
    area_button_.SetIcon(ICON_SAMPLE_AREA_48()).SetCheckable();
    copy_button_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).Tip("Copy values or display colour");
    copy_button_.WhenAction = [this] { OpenCopy(); };
    mode_button_.WhenAction = [this] {
        SetMode(Mode::Point); auto notify = WhenOptions; notify(mode_,space_);
    };
    area_button_.WhenAction = [this] {
        SetMode(Mode::Area); auto notify = WhenOptions; notify(mode_,space_);
    };
    mode_button_.WhenContext = area_button_.WhenContext = [this] { OpenOptions(); };
    format_button_.WhenAction = [this] { OpenFormats(); };
    options_.WhenAction = [this](UiMenuNodeRef, const UiMenuItem& item) {
        int id = (int)item.data;
        if(id < 2) SetMode(id == 0 ? Mode::Point : Mode::Area);
        else SetSpace(id == 2 ? Space::Source : Space::Display);
        auto notify = WhenOptions; notify(mode_, space_);
    };
    formats_.WhenAction = [this](UiMenuNodeRef, const UiMenuItem& item) {
        int id = (int)item.data;
        if(id == 0) SetFormat(Format::Float);
        else if(id == -1) SetFormat(Format::Hex);
        else { SetBitDepth(id); SetFormat(Format::Integer); }
        auto notify = WhenFormat; notify(format_, bits_);
    };
    copy_menu_.WhenAction = [this](UiMenuNodeRef, const UiMenuItem& item) {
        WriteClipboardText((int)item.data == 0 ? GetSampleText() : GetDisplayHex());
    };
    ShowAlpha(false); ShowCopy(false); UpdateButtons(); UpdateReadout();
}
UiColorProbe& UiColorProbe::SetSample(const UiColorSample& sample)
{
    sample_ = sample; swatch_.color = sample.valid ? sample.swatch : Color(Null);
    swatch_.Refresh(); UpdateReadout(); return *this;
}
UiColorProbe& UiColorProbe::SetMode(Mode mode) { mode_ = mode; UpdateButtons(); return *this; }
UiColorProbe& UiColorProbe::SetSpace(Space space) { space_ = space; UpdateButtons(); return *this; }
UiColorProbe& UiColorProbe::SetPrecision(int digits) { precision_ = minmax(digits,0,9); UpdateReadout(); return *this; }
UiColorProbe& UiColorProbe::SetFormat(Format format) { format_ = format; UpdateReadout(); return *this; }
UiColorProbe& UiColorProbe::SetBitDepth(int bits) { bits_ = minmax(bits,1,16); UpdateReadout(); return *this; }
UiColorProbe& UiColorProbe::ShowAlpha(bool on) { alpha_ = on; channels_[3].Show(on); RefreshLayout(); return *this; }
UiColorProbe& UiColorProbe::ShowSwatch(bool on) { swatch_shown_ = on; swatch_.Show(on); RefreshLayout(); return *this; }
UiColorProbe& UiColorProbe::ShowCopy(bool on) { copy_shown_ = on; copy_button_.Show(on); RefreshLayout(); return *this; }
UiColorProbe& UiColorProbe::SetIconColor(Color color)
{
    mode_button_.SetIconColor(color); area_button_.SetIconColor(color);
    format_button_.SetIconColor(color); copy_button_.SetIconColor(color); return *this;
}
String UiColorProbe::GetSampleText() const
{
    if(!sample_.valid) return String();
    String text;
    for(int i = 0; i < (alpha_ ? 4 : 3); ++i) {
        if(i && format_ != Format::Hex) text << " ";
        text << GetChannelText(i);
    }
    return text;
}
String UiColorProbe::GetDisplayHex() const
{
    if(!sample_.valid || IsNull(sample_.swatch)) return String();
    return Upp::Format("#%02X%02X%02X", sample_.swatch.GetR(), sample_.swatch.GetG(), sample_.swatch.GetB());
}
String UiColorProbe::GetChannelText(int channel) const
{
    if(channel < 0 || channel > 3 || !sample_.valid) return "—";
    const double values[] = {sample_.r,sample_.g,sample_.b,sample_.a};
    double value = values[channel];
    if(std::isnan(value)) return "NaN";
    if(std::isinf(value)) return value < 0 ? "-Inf" : "+Inf";
    if(format_ == Format::Float) {
        if(std::fabs(value) >= 1000 || (value != 0 && std::fabs(value) < std::pow(10.0,-precision_)))
            return Upp::Format("%.*g",max(2,precision_+1),value);
        return FormatDoubleFix(value,precision_);
    }
    int maximum = format_ == Format::Hex ? 255 : (1 << bits_) - 1;
    int integer = (int)std::round(minmax(value,0.0,1.0)*maximum);
    if(format_ == Format::Hex) return Upp::Format(channel == 0 ? "#%02X" : "%02X",integer);
    return AsString(integer);
}
void UiColorProbe::UpdateReadout()
{
    const char* names[] = {"R","G","B","A"};
    const double values[] = {sample_.r,sample_.g,sample_.b,sample_.a};
    for(int i = 0; i < 4; ++i) {
        channels_[i].SetText(GetChannelText(i));
        channels_[i].Tip(String(names[i])+" raw: "+AsString(values[i])+"  "+sample_.space+"  "+sample_.quality);
    }
    format_button_.Tip(format_ == Format::Float ? "Number format: raw float" : format_ == Format::Hex ?
        "Number format: RGB hex (8-bit display)" : Upp::Format("Number format: %d-bit integer (0–1 mapped to full scale)",bits_));
}
void UiColorProbe::UpdateButtons()
{
    mode_button_.SetChecked(mode_ == Mode::Point);
    area_button_.SetChecked(mode_ == Mode::Area);
    String space = space_ == Space::Source ? "Source" : "Display";
    mode_button_.Tip("Point sample / "+space+" — right-click or Shift+F10 for sampling space");
    area_button_.Tip("Rectangle sample / "+space+" — right-click or Shift+F10 for sampling space");
}
void UiColorProbe::OpenOptions()
{
    options_.ClearModel();
    const char* labels[] = {"Point sample","Area sample","Source values","Display values"};
    for(int i = 0; i < 4; ++i) {
        UiMenuItem item(labels[i],i); item.radio = item.checkable = true;
        item.checked = i == (mode_ == Mode::Point ? 0 : 1) || i == (space_ == Space::Source ? 2 : 3);
        item.separator_before = i == 2;
        options_.Model().AddChild(options_.Model().Root(),item);
    }
    options_.PopUp(&mode_button_,mode_button_.GetScreenRect().BottomLeft());
}
void UiColorProbe::OpenFormats()
{
    formats_.ClearModel();
    const int ids[] = {0,5,8,10,12,16,-1};
    for(int id : ids) {
        UiMenuItem item(id == 0 ? "Float (raw; normalized 0–1)" : id == -1 ? "Hex #RRGGBB / AA" : Upp::Format("%d-bit integer",id),id);
        item.radio = item.checkable = true;
        item.checked = id == 0 ? format_ == Format::Float : id == -1 ? format_ == Format::Hex : format_ == Format::Integer && bits_ == id;
        formats_.Model().AddChild(formats_.Model().Root(),item);
    }
    formats_.PopUp(&format_button_,format_button_.GetScreenRect().BottomLeft());
}
void UiColorProbe::OpenCopy()
{
    copy_menu_.ClearModel();
    copy_menu_.Model().AddChild(copy_menu_.Model().Root(),UiMenuItem("Copy displayed values",0,sample_.valid));
    copy_menu_.Model().AddChild(copy_menu_.Model().Root(),UiMenuItem("Copy display swatch as hex",1,sample_.valid));
    copy_menu_.PopUp(&copy_button_,copy_button_.GetScreenRect().BottomLeft());
}
UiColorProbe& UiColorProbe::SetControlsSide(UiAlign side)
{
    if(side == UiAlign::LEFT || side == UiAlign::RIGHT || side == UiAlign::TOP || side == UiAlign::BOTTOM) {
        controls_side_ = side;
        accessory_.Show(side == UiAlign::TOP || side == UiAlign::BOTTOM);
        RefreshLayout();
    }
    return *this;
}
UiColorProbe& UiColorProbe::SetRowHeight(int pixels)
{
    row_height_ = max(DPI(16),pixels); RefreshLayout(); return *this;
}
UiColorProbe& UiColorProbe::SetGap(int pixels)
{
    gap_ = max(0,pixels); RefreshLayout(); return *this;
}
Size UiColorProbe::GetMinSize() const
{
    bool row = controls_side_ == UiAlign::TOP || controls_side_ == UiAlign::BOTTOM;
    int count = alpha_ ? 4 : 3;
    int tool_count = copy_shown_ ? 3 : 2;
    int tool_width = tool_count*DPI(20)+(tool_count-1)*gap_;
    int value_width = count*DPI(32)+count*gap_+DPI(14);
    if(swatch_shown_) value_width += row_height_+gap_;
    return Size(row ? max(value_width,tool_width) : value_width+gap_+tool_width,
                row ? row_height_*2+gap_ : row_height_);
}
void UiColorProbe::Layout()
{
    Rect values(GetSize()), toolbar = values;
    int tool_count = copy_shown_ ? 3 : 2;
    int gap = min(gap_,max(0,values.GetWidth()/(2*(tool_count+5))));
    int button = min(DPI(20),max(0,(values.GetWidth()-(tool_count-1)*gap)/(tool_count+2)));
    int count = alpha_ ? 4 : 3, tools = button*tool_count+(tool_count-1)*gap;
    bool row = controls_side_ == UiAlign::TOP || controls_side_ == UiAlign::BOTTOM;
    if(row) {
        int height = min(row_height_,values.GetHeight()/2);
        int row_gap = min(gap,max(0,values.GetHeight()-height));
        if(controls_side_ == UiAlign::TOP) { toolbar.bottom = height; values.top = height+row_gap; }
        else { toolbar.top = max(0,values.bottom-height); values.bottom = max(values.top,toolbar.top-row_gap); }
        accessory_.SetRect(RectC(toolbar.left,toolbar.top,max(0,toolbar.GetWidth()-tools-gap),toolbar.GetHeight()));
        toolbar.left = max(toolbar.left,toolbar.right-tools);
    }
    else if(controls_side_ == UiAlign::LEFT) { toolbar.right = tools; values.left = min(values.right,toolbar.right+gap); }
    else { toolbar.left = max(0,values.right-tools); values.right = max(values.left,toolbar.left-gap); }
    mode_button_.SetRect(toolbar.left,toolbar.top,button,toolbar.GetHeight());
    area_button_.SetRect(toolbar.left+button+gap,toolbar.top,button,toolbar.GetHeight());
    copy_button_.SetRect(toolbar.left+2*(button+gap),toolbar.top,copy_shown_ ? button : 0,toolbar.GetHeight());
    int swatch = swatch_shown_ ? min(values.GetHeight(),values.GetWidth()) : 0;
    int start = min(values.right,values.left+swatch+(swatch ? gap : 0));
    int arrow = min(DPI(14),max(0,values.right-start));
    int finish = max(start,values.right-arrow);
    swatch_.SetRect(values.left,values.top,swatch,values.GetHeight());
    int available = max(0,finish-start-count*gap);
    for(int i = 0; i < count; ++i) {
        int left = min(finish,start+available*i/count+i*gap);
        int right = min(finish,start+available*(i+1)/count+i*gap);
        channels_[i].SetRect(left,values.top,max(0,right-left),values.GetHeight());
    }
    format_button_.SetRect(finish,values.top,arrow,values.GetHeight());
}
}
