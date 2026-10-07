// Self-contained media-control designer. The host owns samples, frames and playback;
// the selected concrete control supplies the preview and generated public-API recipe.
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>
#include <plugin/png/png.h>
#include <climits>
#include <limits>
using namespace Upp;

namespace {
String MediaCppString(const String& text) {
    String out = "\"";
    for(byte c : text) {
        if(c == '\\') out << "\\\\";
        else if(c == '"') out << "\\\"";
        else if(c == '\n') out << "\\n";
        else if(c == '\r') out << "\\r";
        else if(c == '\t') out << "\\t";
        else if(c < 32) out << Format("\\%03o", (int)c);
        else out.Cat(c);
    }
    return out + '"';
}
String MediaCppColor(Color color) { return IsNull(color) ? String("Null") : Format("Color(%d, %d, %d)",color.GetR(),color.GetG(),color.GetB()); }
String MediaBool(bool value) { return value ? "true" : "false"; }
String MediaFrameCode(int64 value) { return value == INT64_MIN ? String("(-9223372036854775807LL - 1LL)") : AsString(value) + "LL"; }
UiAlign MediaSide(String side) { return side == "Left" ? UiAlign::LEFT : side == "Right" ? UiAlign::RIGHT : side == "Top" ? UiAlign::TOP : UiAlign::BOTTOM; }
String MediaSideCode(String side) { return "UiAlign::" + ToUpper(side); }
}

class MediaDemo : public TopWindow {
    PropertyEditorModel model_, override_model_;
    PropertyEditorFactory factory_;
    UiTitleCard header_;
    UiBoxLayout header_actions_ {UiDirection::H}, tools_ {UiDirection::H};
    UiToolButton theme_, help_, exit_, inspector_mode_, overrides_mode_, code_mode_, copy_;
    UiButton select_probe_, select_playback_, select_hdr_;
    UiBoxLayout selector_ {UiDirection::H};
    UiPanel preview_, right_;
    PropertyEditor inspector_, overrides_;
    UiMultiEdit code_;
    UiLabel probe_header_, hint_, events_;
    UiColorProbe probe_;
    UiPlaybackBar playback_;
    UiRangeSlider hdr_;
    int event_count_ = 0, page_ = 0;
    bool dark_ = false;
    bool control_refresh_posted_ = false;
    Color window_face_ = SColorFace();
    Value Get(const char* id) const { const auto* item = model_.Find(id); return item ? item->value : Value(); }
    Value Override(const char* id) const { const auto* item = override_model_.Find(id); return item ? item->value : Value(); }
    bool Active(const char* id) const { const auto* item = override_model_.Find(id); return item && item->override_active; }
    String Selected() const { return AsString(Get("control")); }
    void Log(const String& text) { events_.SetText(AsString(++event_count_) + ": " + text); }
    PropertyEditorItem& Authored(PropertyEditorItem& item) { item.SetDefault(item.value); return item; }
    PropertyEditorItem& Inherited(PropertyEditorItem& item) { item.SetDefault(item.value); item.overrideable = true; item.override_active = false; return item; }
    void BuildModels() {
        Authored(model_.AddChoice("control","Preview control","Probe","Preview").AddChoice("Probe","UiColorProbe").AddChoice("Playback","UiPlaybackBar").AddChoice("HDR","HDR range integration"));
        Authored(model_.AddBoolean("enabled","Enabled",true,"Preview"));
        Authored(model_.AddNumericInt("probe_width","Preview width",260,160,600,1,"Probe"));
        Authored(model_.AddNumericDouble("r","Red (raw)",-0.125,-10000,10000,0.125,"Probe"));
        Authored(model_.AddNumericDouble("g","Green (raw)",0.5,-10000,10000,0.125,"Probe"));
        Authored(model_.AddNumericDouble("b","Blue (raw)",4,-10000,10000,0.125,"Probe"));
        Authored(model_.AddNumericDouble("a","Alpha (raw)",0.75,-10000,10000,0.125,"Probe"));
        Authored(model_.AddBoolean("valid","Sample valid",true,"Probe"));
        Authored(model_.AddColor("swatch","Host display swatch",Color(45,130,210),"Probe"));
        Authored(model_.AddText("space_label","Sample space label","Linear","Probe"));
        Authored(model_.AddText("quality","Sample quality label","512 proxy","Probe"));
        Authored(model_.AddChoice("sampling","Sampling mode","Point","Probe").AddChoice("Point","Point").AddChoice("Area","Area"));
        Authored(model_.AddChoice("sample_space","Sampling space","Source","Probe").AddChoice("Source","Source").AddChoice("Display","Display"));
        Authored(model_.AddChoice("number_format","Number format","Float","Probe").AddChoice("Float","Float").AddChoice("Integer","Integer").AddChoice("Hex","Hex"));
        Authored(model_.AddNumericInt("bits","Integer bit depth",8,1,16,1,"Probe"));
        Authored(model_.AddNumericInt("precision","Decimal places",2,0,9,1,"Probe"));
        Authored(model_.AddBoolean("alpha","Show alpha",true,"Probe"));
        Authored(model_.AddBoolean("swatch_shown","Show swatch",true,"Probe"));
        Authored(model_.AddBoolean("copy_colour","Show copy action",true,"Probe"));
        Authored(model_.AddBoolean("accessory","Show borrowed caption",true,"Probe"));
        for(const char* id : {"probe_side","controls_side","range_side"}) {
            String group = String(id) == "probe_side" ? "Probe" : "Playback";
            Authored(model_.AddChoice(id, String(id) == "range_side" ? "Range side" : "Controls side", String(id) == "controls_side" ? "Left" : String(id) == "probe_side" ? "Top" : "Bottom", group)
                .AddChoice("Left","Left").AddChoice("Right","Right").AddChoice("Top","Top").AddChoice("Bottom","Bottom"));
        }
        Authored(model_.AddChoice("direction","Direction","Horizontal","Playback").AddChoice("Horizontal","Horizontal").AddChoice("Vertical","Vertical"));
        // Text editors preserve full int64 precision instead of routing frames through double.
        for(const char* id : {"first","last","position","selection_first","selection_last","marker_frame","coverage_first","coverage_last"}) {
            String initial = String(id) == "first" || String(id) == "coverage_first" ? "1001" : String(id) == "last" ? "1240" : String(id) == "position" ? "1050" : String(id) == "selection_first" ? "1020" : String(id) == "selection_last" ? "1180" : String(id) == "marker_frame" ? "1100" : "1120";
            String label = String(id) == "first" ? "First frame" : String(id) == "last" ? "Last frame" : String(id) == "position" ? "Current frame" : String(id) == "selection_first" ? "Selection first" : String(id) == "selection_last" ? "Selection last" : String(id) == "marker_frame" ? "Marker frame" : String(id) == "coverage_first" ? "Coverage first" : "Coverage last";
            Authored(model_.AddText(id,label,initial,"Playback"));
        }
        Authored(model_.AddBoolean("range","Show selected range",true,"Playback"));
        Authored(model_.AddBoolean("marker","Show review marker",true,"Playback"));
        Authored(model_.AddText("marker_label","Marker label","Review","Playback"));
        Authored(model_.AddBoolean("coverage","Show coverage span",true,"Playback"));
        Authored(model_.AddChoice("playback","Host playback state","Stopped","Playback").AddChoice("Stopped","Stopped").AddChoice("Forward","Forward").AddChoice("Reverse","Reverse"));
        Authored(model_.AddChoice("formatter","Frame labels","Frame","Playback").AddChoice("Frame","Frame").AddChoice("Prefix","Frame prefix"));
        Authored(model_.AddNumericDouble("minimum","Minimum",-12,-10000,10000,0.25,"HDR"));
        Authored(model_.AddNumericDouble("maximum","Maximum",16,-10000,10000,0.25,"HDR"));
        Authored(model_.AddNumericDouble("lower","Lower",-4,-10000,10000,0.25,"HDR"));
        Authored(model_.AddNumericDouble("upper","Upper",4,-10000,10000,0.25,"HDR"));
        Authored(model_.AddNumericDouble("step","Step",0.25,0.000001,1000,0.125,"HDR"));
        Authored(model_.AddBoolean("body","Drag selected interval",true,"HDR"));
        Authored(model_.AddBoolean("cancel","Escape restores interval",true,"HDR"));
        Inherited(override_model_.AddColor("probe_icon_colour","Icon colour",Color(150,210,80),"Probe"));
        Inherited(override_model_.AddNumericInt("probe_row","Row height",16,16,36,1,"Probe"));
        Inherited(override_model_.AddNumericInt("probe_gap","Spacing",4,0,12,1,"Probe"));
        Inherited(override_model_.AddColor("icon_colour","Transport icon colour",Color(150,210,80),"Playback"));
        Inherited(override_model_.AddNumericInt("icon_size","Icon size",12,8,32,1,"Playback"));
        Inherited(override_model_.AddNumericInt("button_extent","Button extent",20,16,48,1,"Playback"));
        Inherited(override_model_.AddNumericInt("time_extent","Time label extent",86,40,200,1,"Playback"));
        Inherited(override_model_.AddNumericInt("range_extent","Range extent",20,16,48,1,"Playback"));
        Inherited(override_model_.AddColor("marker_colour","Marker colour",Color(0,120,212),"Playback"));
        Inherited(override_model_.AddColor("coverage_colour","Coverage colour",Color(0,120,212),"Playback"));
        const char* commands[] = {"First", "Step back", "Reverse", "Pause", "Play", "Step forward", "Last"};
        for(int i=0;i<7;i++) Inherited(AddPropertyIcon(override_model_, "command_icon."+AsString(i), String(commands[i])+" icon", "", "Playback"));
        model_.StructureChanged(); override_model_.StructureChanged();
    }
    bool ReadFrame(const char* id, int64& value) const {
        String text = TrimBoth(AsString(Get(id)));
        if(text.IsEmpty()) return false;
        int start = text[0]=='-' || text[0]=='+' ? 1 : 0;
        if(start == text.GetCount()) return false;
        for(int i=start;i<text.GetCount();i++) if(text[i]<'0'||text[i]>'9') return false;
        const bool negative = text[0] == '-';
        const uint64 limit = negative ? uint64(INT64_MAX) + 1 : uint64(INT64_MAX);
        uint64 magnitude = 0;
        for(int i=start;i<text.GetCount();i++) {
            uint64 digit = text[i] - '0';
            if(magnitude > (limit - digit) / 10) return false;
            magnitude = magnitude * 10 + digit;
        }
        value = negative && magnitude == uint64(INT64_MAX) + 1 ? INT64_MIN
              : negative ? -int64(magnitude) : int64(magnitude);
        return true;
    }
    void UpdateVisibleProperties() {
        String group = Selected();
        for(const auto& item : model_.GetItems()) model_.SetVisible(item.id,item.id!="control"&&(item.group=="Preview"||item.group==group),false);
        for(const auto& item : override_model_.GetItems()) override_model_.SetVisible(item.id,item.group==group,false);
        model_.StructureChanged(); override_model_.StructureChanged();
        select_probe_.SetChecked(group=="Probe");
        select_playback_.SetChecked(group=="Playback");
        select_hdr_.SetChecked(group=="HDR");
        overrides_mode_.Enable(group!="HDR");
        if(group=="HDR" && page_==1) SelectPage(0);
    }
    void QueueControlRefresh() {
        // Model notifications can originate inside an inline editor callback.
        // Its owning PropertyEditor must not rebuild/destroy it on that stack.
        if(control_refresh_posted_) return;
        control_refresh_posted_=true;
        Ptr<MediaDemo> self=this;
        PostCallback([self] {
            if(!self) return;
            self->control_refresh_posted_=false;
            self->UpdateVisibleProperties();
            self->ApplyProjection();
        });
    }
    void ApplyProjection() {
        UiColorSample sample; sample.valid=(bool)Get("valid"); sample.r=(double)Get("r"); sample.g=(double)Get("g"); sample.b=(double)Get("b"); sample.a=(double)Get("a");
        sample.swatch=Color(Get("swatch")); sample.space=AsString(Get("space_label")); sample.quality=AsString(Get("quality"));
        probe_.SetSample(sample).SetPrecision((int)Get("precision")).SetBitDepth((int)Get("bits"));
        probe_.SetMode(AsString(Get("sampling"))=="Area"?UiColorProbe::Mode::Area:UiColorProbe::Mode::Point);
        probe_.SetSpace(AsString(Get("sample_space"))=="Display"?UiColorProbe::Space::Display:UiColorProbe::Space::Source);
        String format=AsString(Get("number_format")); probe_.SetFormat(format=="Hex"?UiColorProbe::Format::Hex:format=="Integer"?UiColorProbe::Format::Integer:UiColorProbe::Format::Float);
        probe_.ShowAlpha((bool)Get("alpha")).ShowSwatch((bool)Get("swatch_shown")).ShowCopy((bool)Get("copy_colour")).SetControlsSide(MediaSide(AsString(Get("probe_side"))));
        probe_.SetIconColor(Active("probe_icon_colour")?Color(Override("probe_icon_colour")):Color(Null));
        probe_.SetRowHeight(DPI(Active("probe_row")?(int)Override("probe_row"):16)).SetGap(DPI(Active("probe_gap")?(int)Override("probe_gap"):4));
        probe_header_.SetText(probe_.IsAlphaShown()?"RGBA":"RGB");
        if((bool)Get("accessory")) probe_.Accessory().SetContent(probe_header_); else probe_.Accessory().ClearContent();
        int64 first,last,position,lower,upper;
        if(ReadFrame("first",first)&&ReadFrame("last",last)&&ReadFrame("position",position)&&ReadFrame("selection_first",lower)&&ReadFrame("selection_last",upper)&&playback_.SetFrames(first,last)) {
            playback_.SetSelection(lower,upper).SetPosition(position);
        }
        else Log("Invalid frame domain: enter integer indices spanning at most INT_MAX steps.");
        playback_.SetDirection(AsString(Get("direction"))=="Vertical"?UiDirection::V:UiDirection::H).SetControlsSide(MediaSide(AsString(Get("controls_side")))).SetRangeSide(MediaSide(AsString(Get("range_side")))).ShowRange((bool)Get("range"));
        String state=AsString(Get("playback")); playback_.SetPlayback(state=="Forward"?UiPlaybackBar::State::Forward:state=="Reverse"?UiPlaybackBar::State::Reverse:UiPlaybackBar::State::Stopped);
        if(AsString(Get("formatter"))=="Prefix") playback_.SetFormatter([](int64 frame){ return "Frame "+AsString(frame); }); else playback_.SetFormatter(Function<String(int64)>());
        UiPlaybackBar::Style style;
        if(Active("icon_colour")) style.icon_color=Color(Override("icon_colour"));
        if(Active("icon_size")) style.icon_size=Size(DPI((int)Override("icon_size")),DPI((int)Override("icon_size")));
        if(Active("button_extent")) style.button_extent=DPI((int)Override("button_extent"));
        if(Active("time_extent")) style.time_extent=DPI((int)Override("time_extent"));
        if(Active("range_extent")) style.range_extent=DPI((int)Override("range_extent"));
        for(int i=0;i<7;i++) { const auto* item=override_model_.Find("command_icon."+AsString(i)); if(item&&item->override_active) style.icons[i]=UiIconFromName(AsString(item->value)); }
        playback_.SetStyle(style);
        Vector<UiPlaybackMarker> markers; int64 frame;
        if((bool)Get("marker") && ReadFrame("marker_frame",frame)) { auto& marker=markers.Add(); marker.frame=frame; marker.label=AsString(Get("marker_label")); marker.color=Active("marker_colour")?Color(Override("marker_colour")):SColorHighlight(); }
        playback_.SetMarkers(markers);
        Vector<UiPlaybackSpan> spans;
        if((bool)Get("coverage")&&ReadFrame("coverage_first",first)&&ReadFrame("coverage_last",last)) { auto& span=spans.Add(); span.first=first; span.last=last; span.color=Active("coverage_colour")?Color(Override("coverage_colour")):SColorHighlight(); }
        playback_.SetCoverage(spans);
        hdr_.SetRange((double)Get("minimum"),(double)Get("maximum")).SetStep((double)Get("step")).SetValues((double)Get("lower"),(double)Get("upper")).EnableRangeDrag((bool)Get("body")).SetCancelReverts((bool)Get("cancel"));
        bool enabled=(bool)Get("enabled"); probe_.Enable(enabled); playback_.Enable(enabled); hdr_.Enable(enabled);
        if(page_==2) code_.SetTextUtf8(Usage()); Layout(); Refresh();
    }
    String Usage() const {
        String out="#include <Ui/Ui.h>\nusing namespace Upp;\n\nclass MediaControlExample : public ParentCtrl {\n";
        if(Selected()=="Probe") {
            if((bool)Get("accessory")) out << "    UiLabel caption;\n";
            out << "    UiColorProbe probe;\n";
        }
        else if(Selected()=="Playback") out << "    UiPlaybackBar bar;\n";
        else out << "    UiRangeSlider range;\n";
        out << "public:\n    MediaControlExample() {\n";
        if(Selected()=="Probe") {
            out << "    UiColorSample sample;\n";
            out << "    sample.valid = " << MediaBool((bool)Get("valid")) << ";\n";
            for(const char* id : {"r","g","b","a"}) out<<"    sample."<<id<<" = "<<FormatDouble((double)Get(id),17)<<";\n";
            out << "    sample.swatch = "<<MediaCppColor(Color(Get("swatch")))<<";\n    sample.space = "<<MediaCppString(AsString(Get("space_label")))<<";\n    sample.quality = "<<MediaCppString(AsString(Get("quality")))<<";\n";
            out << "    probe.SetSample(sample).SetMode(UiColorProbe::Mode::"<<AsString(Get("sampling"))<<").SetSpace(UiColorProbe::Space::"<<AsString(Get("sample_space"))<<");\n";
            out << "    probe.SetFormat(UiColorProbe::Format::"<<AsString(Get("number_format"))<<").SetBitDepth("<<(int)Get("bits")<<").SetPrecision("<<(int)Get("precision")<<");\n";
            out << "    probe.ShowAlpha("<<MediaBool((bool)Get("alpha"))<<").ShowSwatch("<<MediaBool((bool)Get("swatch_shown"))<<").ShowCopy("<<MediaBool((bool)Get("copy_colour"))<<");\n";
            out << "    probe.SetControlsSide("<<MediaSideCode(AsString(Get("probe_side")))<<").SetRowHeight(DPI("<<(Active("probe_row")?(int)Override("probe_row"):16)<<")).SetGap(DPI("<<(Active("probe_gap")?(int)Override("probe_gap"):4)<<"));\n";
            if(Active("probe_icon_colour")) out<<"    probe.SetIconColor("<<MediaCppColor(Color(Override("probe_icon_colour")))<<");\n";
            if((bool)Get("accessory")) out<<"    caption.SetText("<<MediaCppString((bool)Get("alpha")?"RGBA":"RGB")<<");\n    probe.Accessory().SetContent(caption); // caption outlives probe\n";
            out<<"    probe.Enable("<<MediaBool((bool)Get("enabled"))<<");\n    probe.WhenOptions = [&](UiColorProbe::Mode mode, UiColorProbe::Space space) { /* host resamples */ };\n";
        }
        else if(Selected()=="Playback") {
            out<<"    bar.SetFrames("<<MediaFrameCode(playback_.GetFirst())<<", "<<MediaFrameCode(playback_.GetLast())<<");\n    bar.SetSelection("<<MediaFrameCode(playback_.GetSelectionFirst())<<", "<<MediaFrameCode(playback_.GetSelectionLast())<<").SetPosition("<<MediaFrameCode(playback_.GetPosition())<<");\n";
            out<<"    bar.SetDirection(UiDirection::"<<(AsString(Get("direction"))=="Vertical"?"V":"H")<<").SetControlsSide("<<MediaSideCode(AsString(Get("controls_side")))<<").SetRangeSide("<<MediaSideCode(AsString(Get("range_side")))<<").ShowRange("<<MediaBool((bool)Get("range"))<<");\n";
            out<<"    bar.SetPlayback(UiPlaybackBar::State::"<<AsString(Get("playback"))<<");\n";
            bool style_authored=false; for(const auto& item:override_model_.GetItems()) if(item.group=="Playback"&&item.override_active&&item.id!="marker_colour"&&item.id!="coverage_colour") style_authored=true;
            if(style_authored) {
                out<<"    UiPlaybackBar::Style style;\n";
                if(Active("icon_colour")) out<<"    style.icon_color = "<<MediaCppColor(Color(Override("icon_colour")))<<";\n";
                if(Active("icon_size")) out<<"    style.icon_size = Size(DPI("<<(int)Override("icon_size")<<"), DPI("<<(int)Override("icon_size")<<"));\n";
                for(const char* id:{"button_extent","time_extent","range_extent"}) if(Active(id)) out<<"    style."<<id<<" = DPI("<<(int)Override(id)<<");\n";
                for(int i=0;i<7;i++) { const auto* item=override_model_.Find("command_icon."+AsString(i)); if(item&&item->override_active) out<<"    style.icons["<<i<<"] = UiIconFromName("<<MediaCppString(AsString(item->value))<<");\n"; }
                out<<"    bar.SetStyle(style);\n";
            }
            int64 marker_frame, coverage_first, coverage_last;
            if((bool)Get("marker") && ReadFrame("marker_frame",marker_frame)) out<<"    Vector<UiPlaybackMarker> markers;\n    auto& marker = markers.Add(); marker.frame = "<<MediaFrameCode(marker_frame)<<"; marker.label = "<<MediaCppString(AsString(Get("marker_label")))<<";\n    marker.color = "<<(Active("marker_colour")?MediaCppColor(Color(Override("marker_colour"))):String("SColorHighlight()"))<<";\n    bar.SetMarkers(markers);\n";
            if((bool)Get("coverage") && ReadFrame("coverage_first",coverage_first) && ReadFrame("coverage_last",coverage_last)) out<<"    Vector<UiPlaybackSpan> coverage;\n    auto& span = coverage.Add(); span.first = "<<MediaFrameCode(coverage_first)<<"; span.last = "<<MediaFrameCode(coverage_last)<<";\n    span.color = "<<(Active("coverage_colour")?MediaCppColor(Color(Override("coverage_colour"))):String("SColorHighlight()"))<<";\n    bar.SetCoverage(coverage);\n";
            if(AsString(Get("formatter"))=="Prefix") out<<"    bar.SetFormatter([](int64 frame) { return \"Frame \" + AsString(frame); });\n";
            out<<"    bar.Enable("<<MediaBool((bool)Get("enabled"))<<");\n    bar.WhenCommand = [&](UiPlaybackBar::Command command) { /* host acknowledges transport and supplies frames */ };\n    bar.WhenSeek = [&](int64 frame, bool preview) { /* host seeks; preview distinguishes commit */ };\n    bar.WhenRange = [&](int64 first, int64 last, bool preview) { /* host updates range */ };\n    bar.WhenSeekCancel = [&] { /* host cancels seek transaction */ };\n    bar.WhenRangeCancel = [&] { /* host cancels range transaction */ };\n";
        }
        else {
            out<<"    range.SetRange("<<FormatDouble((double)Get("minimum"),17)<<", "<<FormatDouble((double)Get("maximum"),17)<<").SetStep("<<FormatDouble((double)Get("step"),17)<<").SetValues("<<FormatDouble((double)Get("lower"),17)<<", "<<FormatDouble((double)Get("upper"),17)<<");\n    range.EnableRangeDrag("<<MediaBool((bool)Get("body"))<<").SetCancelReverts("<<MediaBool((bool)Get("cancel"))<<");\n    range.Enable("<<MediaBool((bool)Get("enabled"))<<");\n    range.WhenAction = [&] { /* committed HDR interval */ };\n    range.WhenCancelEdit = [&] { /* interval rolled back */ };\n";
        }
        if(Selected()=="Probe") out << "    Add(probe.SizePos());\n";
        else if(Selected()=="Playback") out << "    Add(bar.SizePos());\n";
        else out << "    Add(range.HSizePos().VCenterPos(DPI(40)));\n";
        // Indent the constructor body while retaining concise reusable public API code.
        int body = out.Find("    MediaControlExample() {\n") + String("    MediaControlExample() {\n").GetCount();
        String result = out.Left(body);
        for(const String& line : Split(out.Mid(body), '\n')) result << "    " << line << "\n";
        return result+"    }\n};\n";
    }
    void SelectPage(int page) {
        page_=minmax(page,0,2); inspector_.Show(page_==0); overrides_.Show(page_==1); code_.Show(page_==2); copy_.Show(page_==2);
        inspector_mode_.SetChecked(page_==0); overrides_mode_.SetChecked(page_==1); code_mode_.SetChecked(page_==2);
        if(page_==2) code_.SetTextUtf8(Usage()); Layout();
    }
public:
    MediaDemo() {
        Title("Ui media controls designer").Sizeable().Zoomable(); SetRect(0,0,DPI(1200),DPI(780));
        UiThemeContext context=UiTheme::GetContext(); context.mode=UiThemeMode::Light; context.preset=UiThemePreset::Minimal; UiTheme::Set(context);
        RegisterPropertyEditorEditors(factory_); BuildModels();
        Add(header_); Add(preview_); Add(right_);
        header_.SetTitle("Media controls").SetSubTitle("Raw colour samples and host-driven playback; select a control to inspect and generate its C++").ShowTitleLine(false).SetContentInset(DPI(8)).SetContentCell(header_actions_);
        header_actions_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center); header_actions_.AddSpacer(1).Expand(1);
        theme_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16),DPI(16)).Tip("Theme"); help_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16),DPI(16)).Tip("Help"); exit_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16),DPI(16)).Tip("Close demo");
        for(UiToolButton* button:{&theme_,&help_,&exit_}) header_actions_.Add(*button).Fixed(DPI(34));
        preview_.Add(selector_); preview_.Add(probe_); preview_.Add(playback_); preview_.Add(hdr_); preview_.Add(hint_); preview_.Add(events_);
        selector_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        select_probe_.SetText("Probe").SetCheckable();
        select_playback_.SetText("Playback").SetCheckable();
        select_hdr_.SetText("HDR").SetCheckable().Tip("HDR range integration");
        selector_.Add(select_probe_).Fixed(DPI(72));
        selector_.Add(select_playback_).Fixed(DPI(90));
        selector_.Add(select_hdr_).Fixed(DPI(64));
        select_probe_.WhenAction=[=]{model_.SetValue("control","Probe");};
        select_playback_.WhenAction=[=]{model_.SetValue("control","Playback");};
        select_hdr_.WhenAction=[=]{model_.SetValue("control","HDR");};
        hint_.SetText("HDR samples remain raw. Playback requests go to the host; this control owns no decoder or clock.").SetAlign(UiAlign::CENTER,UiAlign::CENTER);
        right_.Add(tools_); right_.Add(inspector_); right_.Add(overrides_); right_.Add(code_); right_.Add(copy_);
        tools_.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
        inspector_mode_.SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17),DPI(17)).SetCheckable(); inspector_mode_.Tip("Inspector");
        overrides_mode_.SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17),DPI(17)).SetCheckable(); overrides_mode_.Tip("Theme Overrides");
        code_mode_.SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17),DPI(17)).SetCheckable(); code_mode_.Tip("Generated code");
        for(UiToolButton* button:{&inspector_mode_,&overrides_mode_,&code_mode_}) tools_.Add(*button).Fixed(DPI(38)); tools_.AddSpacer(1).Expand(1);
        inspector_.SetFactory(&factory_); inspector_.SetModel(&model_); overrides_.SetFactory(&factory_); overrides_.SetModel(&override_model_);
        code_.SetEditable(false); code_.SetAcceptsTabs(true); copy_.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16),DPI(16)).Tip("Copy generated C++");
        inspector_mode_.WhenAction=[=]{SelectPage(0);}; overrides_mode_.WhenAction=[=]{SelectPage(1);}; code_mode_.WhenAction=[=]{SelectPage(2);}; copy_.WhenAction=[=]{WriteClipboardText(Usage());};
        theme_.WhenAction=[=]{SetDark(!dark_);}; exit_.WhenAction=[=]{Close();}; help_.WhenAction=[=]{PromptOK("Choose Probe, Playback or HDR above the preview. Raw samples, frame indices and commands belong to the host. Theme Overrides remain inherited until enabled. Generated Code contains only the chosen concrete control. HDR range is an integration example; Slider family is its canonical designer.");};
        model_.WhenValueChanged=[=](String id){if(id=="control") { QueueControlRefresh(); return; } ApplyProjection();};
        override_model_.WhenValueChanged=[=](String){ApplyProjection();};
        overrides_.WhenOverride=[=](String id,bool active){if(auto* item=override_model_.Find(id)){item->override_active=active; override_model_.StructureChanged(); ApplyProjection();}};
        inspector_.WhenReset=[=](String id){if(auto* item=model_.Find(id))model_.SetValue(id,item->default_value);};
        overrides_.WhenReset=[=](String id){if(auto* item=override_model_.Find(id)){item->override_active=false; override_model_.SetValue(id,item->default_value); override_model_.StructureChanged(); ApplyProjection();}};
        probe_.WhenOptions=[=](UiColorProbe::Mode mode,UiColorProbe::Space space){model_.SetValue("sampling",mode==UiColorProbe::Mode::Area?"Area":"Point",false);model_.SetValue("sample_space",space==UiColorProbe::Space::Display?"Display":"Source",false);inspector_.RefreshValue("sampling");inspector_.RefreshValue("sample_space");if(page_==2) code_.SetTextUtf8(Usage());Log("Host resamples for selected options");};
        probe_.WhenFormat=[=](UiColorProbe::Format format,int bits){model_.SetValue("number_format",format==UiColorProbe::Format::Hex?"Hex":format==UiColorProbe::Format::Integer?"Integer":"Float",false);model_.SetValue("bits",bits,false);inspector_.RefreshValue("number_format");inspector_.RefreshValue("bits");if(page_==2) code_.SetTextUtf8(Usage());};
        playback_.WhenSeek=[=](int64 frame,bool preview){model_.SetValue("position",AsString(frame),false);inspector_.RefreshValue("position");if(page_==2) code_.SetTextUtf8(Usage());Log(preview?"Seek preview":"Seek committed");};
        playback_.WhenRange=[=](int64 a,int64 b,bool preview){model_.SetValue("selection_first",AsString(a),false);model_.SetValue("selection_last",AsString(b),false);inspector_.RefreshValue("selection_first");inspector_.RefreshValue("selection_last");if(page_==2) code_.SetTextUtf8(Usage());Log(preview?"Range preview":"Range committed");};
        playback_.WhenRangeCancel=[=]{model_.SetValue("selection_first",AsString(playback_.GetSelectionFirst()),false);model_.SetValue("selection_last",AsString(playback_.GetSelectionLast()),false);inspector_.RefreshValue("selection_first");inspector_.RefreshValue("selection_last");if(page_==2) code_.SetTextUtf8(Usage());Log("Range cancelled");};
        playback_.WhenSeekCancel=[=]{model_.SetValue("position",AsString(playback_.GetPosition()),false);inspector_.RefreshValue("position");if(page_==2) code_.SetTextUtf8(Usage());Log("Seek cancelled");};
        playback_.WhenCommand=[=](UiPlaybackBar::Command command){using C=UiPlaybackBar::Command;int64 frame=playback_.GetPosition();if(command==C::First)frame=playback_.GetFirst();if(command==C::Last)frame=playback_.GetLast();if(command==C::StepBack&&frame>playback_.GetFirst())--frame;if(command==C::StepForward&&frame<playback_.GetLast())++frame;model_.SetValue("position",AsString(frame),false);model_.SetValue("playback",command==C::Play?"Forward":command==C::Reverse?"Reverse":"Stopped",false);inspector_.RefreshValue("position");inspector_.RefreshValue("playback");ApplyProjection();Log("Host acknowledged transport request");};
        hdr_.WhenAction=[=]{model_.SetValue("lower",hdr_.GetLowerValue(),false);model_.SetValue("upper",hdr_.GetUpperValue(),false);inspector_.RefreshValue("lower");inspector_.RefreshValue("upper");if(page_==2) code_.SetTextUtf8(Usage());Log("HDR interval committed");}; hdr_.WhenCancelEdit=[=]{Log("HDR interval cancelled");};
        ApplyTheme(); UpdateVisibleProperties(); ApplyProjection(); SelectPage(0);
    }
    void SetDark(bool dark) { if(dark_!=dark){dark_=dark;Ctrl::SwapDarkLight();UiThemeContext context=UiTheme::GetContext();context.mode=dark?UiThemeMode::Dark:UiThemeMode::Light;UiTheme::Set(context);} ApplyTheme(); ApplyProjection(); }
    void Paint(Draw& draw) override { draw.DrawRect(GetSize(),window_face_); }
    void Layout() override {
        Size size=GetSize();int pad=DPI(12),gap=DPI(10),top=DPI(94),rail=min(DPI(450),max(DPI(300),size.cx*38/100));int width=max(0,size.cx-pad*3-rail),height=max(0,size.cy-top-pad);
        header_.SetRect(pad,pad,max(0,size.cx-pad*2),DPI(72));preview_.SetRect(pad,top,width,height);right_.SetRect(pad+width+gap,top,rail,height);
        selector_.SetRect(DPI(12),DPI(10),max(0,width-DPI(24)),DPI(34));
        int available=max(0,width-DPI(48));int probe_width=min(available,DPI((int)Get("probe_width")));Size playback_size=playback_.GetMinSize();
        if(playback_.GetDirection()==UiDirection::H)playback_size.cx=available;else playback_size.cy=min(DPI(340),max(0,height-DPI(120)));
        probe_.SetRect((width-probe_width)/2,max(DPI(16),(height-probe_.GetMinSize().cy)/2-DPI(40)),probe_width,probe_.GetMinSize().cy);
        playback_.SetRect(max(DPI(16),(width-playback_size.cx)/2),max(DPI(16),(height-playback_size.cy)/2-DPI(40)),min(available,playback_size.cx),playback_size.cy);
        hdr_.SetRect(DPI(24),max(DPI(16),height/2-DPI(40)),available,DPI(44));probe_.Show(Selected()=="Probe");playback_.Show(Selected()=="Playback");hdr_.Show(Selected()=="HDR");
        hint_.SetRect(DPI(18),max(0,height-DPI(92)),max(0,width-DPI(36)),DPI(48));events_.SetRect(DPI(18),max(0,height-DPI(42)),max(0,width-DPI(36)),DPI(30));
        tools_.SetRect(DPI(6),DPI(4),max(0,rail-DPI(12)),DPI(40));for(PropertyEditor* editor:{&inspector_,&overrides_})editor->SetRect(DPI(6),DPI(48),max(0,rail-DPI(12)),max(0,height-DPI(54)));
        code_.SetRect(DPI(8),DPI(48),max(0,rail-DPI(16)),max(0,height-DPI(94)));copy_.SetRect(DPI(8),max(DPI(48),height-DPI(40)),DPI(34),DPI(32));
    }
    void RenderStates(const String& directory) {
        RealizeDirectory(directory);auto render=[&](const char* name){Ctrl::ProcessEvents();ImageDraw draw(GetSize());DrawCtrl(draw);PNGEncoder().SaveFile(AppendFileName(directory,name),draw);};
        render("media-light.png");SetDark(true);render("media-dark.png");
        SelectPage(1);render("media-overrides-dark.png");SelectPage(2);render("media-code-dark.png");SelectPage(0);
        model_.SetValue("control","Playback");render("media-playback.png");model_.SetValue("direction","Vertical");render("playback-vertical.png");
        model_.SetValue("enabled",false);render("media-disabled.png");model_.SetValue("enabled",true);SetDark(false);SetRect(0,0,DPI(720),DPI(470));render("media-narrow.png");
    }
    void WriteUsage(const String& path) { SaveFile(path,Usage()); }
    void ExportGenerated(const String& directory) {
        RealizeDirectory(directory);
        for(const char* type:{"Probe","Playback","HDR"}) {
            model_.SetValue("control",type);SaveFile(AppendFileName(directory,String("UiMediaControlsDemo_")+type+"_default.cpp"),Usage());
            if(String(type)=="Probe"){model_.SetValue("quality","Quoted \"take\"\t\r\nC:\\media");auto* item=override_model_.Find("probe_icon_colour");item->override_active=true;override_model_.SetValue(item->id,Color(88,99,111));}
            if(String(type)=="Playback"){model_.SetValue("first","9223372036854774807",false);model_.SetValue("last","9223372036854775807",false);model_.SetValue("position","9223372036854775507",false);model_.SetValue("selection_first","9223372036854774907",false);model_.SetValue("selection_last","9223372036854775707",false);model_.SetValue("marker_label","Quoted \"review\"\n");auto* item=override_model_.Find("button_extent");item->override_active=true;override_model_.SetValue(item->id,28);}
            ApplyProjection();SaveFile(AppendFileName(directory,String("UiMediaControlsDemo_")+type+"_authored.cpp"),Usage());
        }
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
        for(UiButton* button:{&select_probe_,&select_playback_,&select_hdr_}) {
            auto style=UiTheme::ResolveButton(UiRole::Standard);
            style.transparent=true; style.metrics.face_enabled=false; style.metrics.frame_enabled=false;
            style.metrics.focus_enabled=false; style.metrics.shadow.enabled=false;
            style.palette.ink[ST_NORMAL]=dark?Color(180,180,180):Color(110,110,110);
            style.palette.ink[ST_HOT]=dark?White():Color(32,32,32);
            style.palette.ink[ST_PRESSED]=Color(0,120,212);
            button->SetCustomStyle(style);
        }

        for(UiLabel* label : { &hint_, &events_, &probe_header_ })
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
    bool Test(const String& output) {
        probe_.Show(); playback_.Show(); hdr_.Show();
        String failures; int checks = 0;
        auto check = [&](bool ok, const char* name) { ++checks; if(!ok) failures << name << "\n"; };
        UiColorSample sample; sample.valid = true; sample.r = -8; sample.g = 12;
        sample.b = std::numeric_limits<double>::infinity();
        int options = 0; probe_.WhenOptions = [&](UiColorProbe::Mode, UiColorProbe::Space) { ++options; };
        probe_.SetSample(sample).SetMode(UiColorProbe::Mode::Area).SetSpace(UiColorProbe::Space::Display);
        check(probe_.GetSample().r == -8 && probe_.GetSample().g == 12, "HDR raw values");
        check(options == 0, "Silent probe setters");
        check(playback_.SetFrames(INT64_MAX - 1000, INT64_MAX), "Large origin");
        playback_.SetPosition(INT64_MAX - 333);
        check(playback_.GetPosition() == INT64_MAX - 333, "Exact integer frame");
        check(!playback_.SetFrames(INT64_MIN, INT64_MAX), "Overflow domain rejected");
        check(playback_.GetPosition() == INT64_MAX - 333, "Rejected domain preserves state");
        check(!playback_.SetFrames(100, 0), "Reversed domain rejected");
        check(playback_.SetFrames(-10, 10), "Negative frames");
        playback_.SetSelection(99, -99);
        check(playback_.GetSelectionFirst() == -10 && playback_.GetSelectionLast() == 10, "Selection ordered and clamped");
        int commands = 0; playback_.WhenCommand = [&](UiPlaybackBar::Command) { ++commands; };
        playback_.SetPlayback(UiPlaybackBar::State::Stopped); playback_.SetPosition(0);
        playback_.Key(K_SPACE, 1);
        check(commands == 1 && playback_.GetPosition() == 0 && playback_.GetPlayback() == UiPlaybackBar::State::Stopped, "Transport request does not run a clock");
        playback_.Disable(); playback_.Key(K_SPACE, 1);
        check(commands == 1, "Disabled transport"); playback_.Enable();
        int previews = 0, commits = 0, cancels = 0;
        hdr_.SetRange(0, 100).SetStep(1).SetValues(20, 40).EnableRangeDrag().SetCancelReverts();
        hdr_.WhenChanging = [&] { ++previews; }; hdr_.WhenAction = [&] { ++commits; }; hdr_.WhenCancelEdit = [&] { ++cancels; };
        Rect track = hdr_.GetTrackRect();
        Point start(track.left + (track.GetWidth()-1)*30/100, track.CenterPoint().y);
        hdr_.LeftDown(start, 0); hdr_.MouseMove(Point(track.right + 50, start.y), 0);
        check(hdr_.GetUpperValue() == 100 && hdr_.GetLowerValue() == 80, "Range body clamps preserving width");
        hdr_.Key(K_ESCAPE, 1);
        check(hdr_.GetLowerValue() == 20 && hdr_.GetUpperValue() == 40 && cancels == 1 && commits == 0 && !hdr_.HasCapture(), "Escape restores without commit");
        hdr_.LeftDown(start, 0); hdr_.MouseMove(Point(start.x + 30, start.y), 0); hdr_.LeftUp(start, 0);
        check(previews >= 2 && commits == 1 && hdr_.GetUpperValue() - hdr_.GetLowerValue() == 20, "Preview then one commit");
        hdr_.SetValues(20, 40); hdr_.LeftDown(start, 0); hdr_.MouseMove(Point(start.x + 30, start.y), 0);
        hdr_.ReleaseCapture();
        check(cancels == 2 && commits == 1 && hdr_.GetLowerValue() == 20, "Capture loss cancels");
        hdr_.LeftDown(start, 0); hdr_.MouseMove(Point(start.x + 30, start.y), 0); hdr_.Disable();
        check(cancels == 3 && commits == 1 && hdr_.GetLowerValue() == 20 && !hdr_.HasCapture(), "Disable cancels");
        hdr_.Enable();
        // Scalar scrubbing cancellation uses the same preview/commit boundary.
        int seek_cancel = 0, seek_commit = 0;
        playback_.SetFrames(0, 100); playback_.SetPosition(20);
        playback_.WhenSeekCancel = [&] { ++seek_cancel; };
        playback_.WhenSeek = [&](int64, bool preview) { if(!preview) ++seek_commit; };
        UiSlider& seek = playback_.SeekSlider(); Rect st = seek.GetTrackGeometry();
        Point sp(st.left + (st.GetWidth()-1)*20/100, st.CenterPoint().y);
        seek.LeftDown(sp, 0); seek.MouseMove(Point(st.right, sp.y), 0); seek.Key(K_ESCAPE, 1);
        check(playback_.GetPosition() == 20 && seek_cancel == 1 && seek_commit == 0 && !seek.HasCapture(), "Seek Escape rollback");
        Vector<UiPlaybackMarker> marks;
        marks.Add().frame = 90; marks.Add().frame = 40; marks.Add().frame = 101;
        playback_.SetMarkers(marks);
        check(playback_.SeekMarker(true) && playback_.GetPosition() == 40, "Nearest marker from unsorted data");
        check(playback_.SeekMarker(false) == false, "Out of range marker ignored");
        // Vertical and bounded body translation use the same range semantics.
        hdr_.SetDirection(UiDirection::V); hdr_.SetRect(16, 310, 44, 140);
        hdr_.EnableAdjustableBounds().SetBounds(10, 80).SetValues(30, 50);
        Rect vt = hdr_.GetTrackRect();
        Point vp(vt.CenterPoint().x, vt.top + (vt.GetHeight()-1)*40/100);
        hdr_.LeftDown(vp, 0); hdr_.MouseMove(Point(vp.x, vt.bottom + 80), 0);
        check(hdr_.GetLowerValue() == 60 && hdr_.GetUpperValue() == 80, "Vertical translation respects outer bounds");
        hdr_.LeftUp(vp, 0);
        // Callback deletion: no later action or Refresh may touch the object.
        int deleted_action = 0;
        UiRangeSlider* ephemeral = new UiRangeSlider;
        Add(*ephemeral); ephemeral->SetRect(10, 10, 300, 30);
        ephemeral->SetValues(20, 40);
        ephemeral->WhenChanging = [&, ephemeral] { delete ephemeral; };
        ephemeral->WhenAction = [&] { ++deleted_action; };
        ephemeral->Key(K_RIGHT, 1);
        check(deleted_action == 0, "Deleting range in preview suppresses later commit");
        UiSliderEdit scalar_edit;
        Add(scalar_edit); scalar_edit.SetRect(10, 10, 400, 40);
        scalar_edit.SetRange(0, 100).SetValue(25); scalar_edit.Slider().SetCancelReverts();
        int edit_cancel = 0; scalar_edit.WhenCancelEdit = [&] { ++edit_cancel; };
        Rect et = scalar_edit.Slider().GetTrackGeometry(); Point ep = et.CenterPoint();
        scalar_edit.Slider().LeftDown(ep, 0); scalar_edit.Slider().Key(K_ESCAPE, 1);
        check(scalar_edit.Field().GetValue() == 25 && edit_cancel == 1, "Scalar edit field resynchronises after cancel");
        UiRangeSliderEdit range_edit;
        Add(range_edit); range_edit.SetRect(10, 60, 480, 40);
        range_edit.SetRange(0, 100).SetValues(20, 40); range_edit.Slider().SetCancelReverts();
        Rect rt = range_edit.Slider().GetTrackRect(); Point rp(rt.left + (rt.GetWidth()-1)*70/100, rt.CenterPoint().y);
        range_edit.Slider().LeftDown(rp, 0); range_edit.Slider().Key(K_ESCAPE, 1);
        check(range_edit.LowerField().GetValue() == 20 && range_edit.UpperField().GetValue() == 40, "Range edit fields resynchronise after cancel");
        UiColorSample rgb; rgb.valid = true; rgb.r = .5; rgb.g = -1; rgb.b = 2; rgb.swatch = Color(12,34,56);
        probe_.SetSample(rgb).SetFormat(UiColorProbe::Format::Integer).SetBitDepth(8);
        check(probe_.GetChannelText(0) == "128" && probe_.GetChannelText(1) == "0" && probe_.GetChannelText(2) == "255", "8-bit display mapping");
        probe_.SetBitDepth(16); check(probe_.GetChannelText(0) == "32768", "16-bit display mapping");
        probe_.SetBitDepth(5); check(probe_.GetChannelText(0) == "16", "5-bit display mapping");
        probe_.SetFormat(UiColorProbe::Format::Hex);
        probe_.ShowAlpha(false); check(probe_.GetSampleText() == "#8000FF" && probe_.GetDisplayHex() == "#0C2238", "Raw-value hex differs from transformed swatch");
        check(probe_.GetSample().g == -1 && probe_.GetSample().b == 2, "Changing display format preserves HDR source");
        probe_.SetFormat(UiColorProbe::Format::Float).SetPrecision(2);
        check(probe_.GetChannelText(0) == "0.50" && probe_.GetChannelText(1) == "-1.00", "Compact float readout");
        probe_.ShowAlpha(false).ShowCopy(false);
        probe_.SetControlsSide(UiAlign::RIGHT).SetRowHeight(DPI(22));
        check(probe_.GetMinSize() == Size(DPI(196),DPI(22)), "Compact default dimensions with four-pixel gaps");
        probe_.ShowAlpha().ShowCopy(); check(probe_.IsAlphaShown() && probe_.IsCopyShown(), "Optional alpha and copy");
        probe_.SetFormat(UiColorProbe::Format::Hex);
        check(probe_.GetSampleText() == "#8000FFFF", "RGBA hexadecimal includes alpha");
        probe_.SetFormat(UiColorProbe::Format::Float).SetPrecision(2);
        check(probe_.GetChannelText(3) == "1.00", "Alpha float readout");
        check(probe_.GetGap() == DPI(4), "Four-pixel default spacing");
        probe_.SetControlsSide(UiAlign::TOP).SetRowHeight(DPI(16));
        probe_.SetRect(0,0,DPI(180),probe_.GetMinSize().cy);
        check(probe_.Accessory().IsShown() && probe_.Accessory().GetRect().right <= DPI(140), "Top-left borrowed content leaves space for top-right icons");
        probe_.SetControlsSide(UiAlign::RIGHT);
        check(!probe_.Accessory().IsShown(), "Inline mode needs no label/header");
        check(!IsNull(UiIconFromName("ICON_MEDIA_PLAY_48")), "Transport icon in shared catalogue");
        Image custom = ICON_SAMPLE_POINT_48();
        playback_.SetIcon(UiPlaybackBar::Command::Play,custom);
        check(playback_.GetIcon(UiPlaybackBar::Command::Play) == custom, "Per-command icon replacement");
        const UiAlign sides[] = {UiAlign::LEFT,UiAlign::RIGHT,UiAlign::TOP,UiAlign::BOTTOM};
        for(UiDirection dir : {UiDirection::H,UiDirection::V}) for(UiAlign side : sides) {
            playback_.SetDirection(dir).SetControlsSide(side).SetRangeSide(side).ShowRange();
            Size preferred = playback_.GetMinSize(); playback_.SetRect(0,0,preferred.cx,preferred.cy);
            Rect seek_rect = playback_.SeekSlider().GetRect(), range_rect = playback_.RangeSlider().GetRect();
            check(!seek_rect.IsEmpty() && !range_rect.IsEmpty() && !seek_rect.Intersects(range_rect), "Placement has separate nonempty seek/range regions");
        }
        check(playback_.SetFrames(7, 7), "Single frame domain");
        playback_.SetPosition(100); check(playback_.GetPosition() == 7, "Single frame clamp");
        SaveFile(output, Format("%d checks\n", checks) + (failures.IsEmpty() ? "PASS\n" : failures));
        return failures.IsEmpty();
    }
    bool TestSelectors(const String& output) {
        String failures;
        String context;
        int checks=0;
        auto check=[&](bool ok,const char* name) { ++checks; if(!ok) failures << context << ": " << name << " (selected=" << Selected() << ", pending=" << int(control_refresh_posted_) << ")\n"; };
        UiButton* buttons[]={&select_probe_,&select_playback_,&select_hdr_};
        const char* types[]={"Probe","Playback","HDR"};
        const char* concrete[]={"UiColorProbe probe;","UiPlaybackBar bar;","UiRangeSlider range;"};
        for(bool dark:{false,true}) {
            SetDark(dark);
            for(int repeat=0;repeat<12;repeat++) for(int type=0;type<3;type++) {
                context=Format("%s cycle%d %s",dark?"Dark":"Light",repeat,types[type]);
                SelectPage(repeat%3);
                buttons[type]->SetFocus();
                check(buttons[type]->Key(K_SPACE,1),"Native selector keyboard action");
                check(control_refresh_posted_,"Selector defers structural refresh until callback returns");
                for(int pump=0;pump<30 && control_refresh_posted_;pump++) { Sleep(1); ProcessEvents(); }
                check(!control_refresh_posted_ && Selected()==types[type],"Selected model applied after event dispatch");
                check(probe_.IsShown()==(type==0) && playback_.IsShown()==(type==1) && hdr_.IsShown()==(type==2),"Exactly one preview visible");
                check(select_probe_.IsChecked()==(type==0) && select_playback_.IsChecked()==(type==1) && select_hdr_.IsChecked()==(type==2),"Adjacent buttons reflect selected preview");
                check(!model_.Find("control")->visible,"Selector is outside Inspector editors");
                check(model_.Find("r")->visible==(type==0) && model_.Find("position")->visible==(type==1) && model_.Find("lower")->visible==(type==2),"Inspector exposes selected control only");
                check(Usage().Find(concrete[type])>=0,"Generated example owns selected concrete type");
                ImageDraw image(GetSize()); DrawCtrl(image);
            }
        }
        SaveFile(output,Format("%d checks\n",checks)+(failures.IsEmpty()?"PASS\n":failures));
        return failures.IsEmpty();
    }
};

GUI_APP_MAIN
{
    MediaDemo demo;
    const auto& args = CommandLine();
    if(args.GetCount() >= 2 && args[0] == "--export-generated") { demo.ExportGenerated(args[1]); return; }
    if(args.GetCount() >= 2 && args[0] == "--write-usage") { demo.WriteUsage(args[1]); return; }
    if(args.GetCount() >= 2 && args[0] == "--render") {
        demo.Open(); demo.RenderStates(args[1]); demo.Close(); return;
    }
    if(args.GetCount() >= 2 && args[0] == "--self-test") {
        demo.Open();
        bool passed = demo.Test(args[1]); demo.Close();
        SetExitCode(passed ? 0 : 1); return;
    }
    if(args.GetCount() >= 2 && args[0] == "--selector-test") {
        demo.Open(); Ctrl::ProcessEvents();
        bool passed=demo.TestSelectors(args[1]); demo.Close();
        SetExitCode(passed?0:1); return;
    }
    if(args.GetCount() && args[0] == "--smoke") demo.SetTimeCallback(1000, [&] { demo.Close(); });
    demo.Run();
}
