// Focused native Basic renderer regression. No windows/input or private renderer access.
#include <Ui/UiItemRender.h>
#include <Ui/UiDropdown.h>
#include <Ui/UiFonts.h>
#include <Ui/UiTheme.h>
#include <plugin/png/png.h>
#include <memory>

using namespace Upp;
namespace {
int checks=0, failures=0;
ValueArray records;
String output;

void Check(bool pass,const String& message)
{
    ++checks;
    if(!pass){++failures;Cout()<<"FAIL "<<message<<"\n";}
}
Value RectValue(const Rect& r)
{
    ValueArray a;a.Add(r.left);a.Add(r.top);a.Add(r.right);a.Add(r.bottom);return a;
}
Value SizeValue(Size s){ValueArray a;a.Add(s.cx);a.Add(s.cy);return a;}

// Observe production HitTest after production PrepareLayout; never reconstruct
// the renderer's private part rectangles from requested-font-height formulas.
Rect PartBounds(const UiItemRender& render,UiItemRenderPart part)
{
    Rect b=render.GetBounds(),found;bool any=false;
    for(int y=b.top;y<b.bottom;++y)
        for(int x=b.left;x<b.right;++x)
            if(render.HitTest(Point(x,y)).part==part){
                Rect p=RectC(x,y,1,1);if(any)found|=p;else{found=p;any=true;}
            }
    return any?found:Rect(0,0,0,0);
}
Font TitleFont(const UiItemRender& render)
{
    const auto& data=render.GetData();Font f=data.use_custom_font?data.custom_font:render.GetStyle().title_font;
    if(data.emphasized&&!data.use_custom_font)f.Bold();
    return UiFonts::Normalize(f);
}
Font Segoe(int px){int face=Font::FindFaceNameIndex("Segoe UI");return Font(face,DPI(px));}
UiItemRenderStyle FlatStyle(Font font)
{
    UiItemRenderBasic basic;auto s=basic.GetStyle();
    s.title_font=s.subtitle_font=s.description_font=s.right_font=font;
    s.metrics.face_enabled=s.metrics.frame_enabled=s.metrics.focus_enabled=false;
    s.metrics.shadow.enabled=false;s.metrics.content_margin=Rect(DPI(3),DPI(2),DPI(3),DPI(2));
    s.show_face=false;s.show_icon=s.show_metadata=false;s.muted_ink=Black();
    for(int i=0;i<4;++i)s.palette.ink[i]=Black();
    return s;
}
bool Fits(Rect r,Size glyph){return !r.IsEmpty()&&r.GetWidth()>=glyph.cx&&r.GetHeight()>=glyph.cy;}
void ObserveParts(UiItemRender& render,const String& name,bool require_width)
{
    const auto& data=render.GetData();const auto& style=render.GetStyle();
    const String strings[]={data.title,data.subtitle,data.description,data.right_text};
    const Font fonts[]={TitleFont(render),style.subtitle_font,style.description_font,style.right_font};
    const bool visible[]={!data.title.IsEmpty(),style.show_subtitle&&!data.subtitle.IsEmpty(),
                          style.show_description&&!data.description.IsEmpty(),style.show_right_text&&!data.right_text.IsEmpty()};
    const UiItemRenderPart parts[]={UIITEMPART_TITLE,UIITEMPART_SUBTITLE,UIITEMPART_DESCRIPTION,UIITEMPART_RIGHT_TEXT};
    const char* labels[]={"title","subtitle","description","right"};
    ValueArray measured;
    for(int i=0;i<4;++i){
        Rect part=PartBounds(render,parts[i]);Size glyph=GetTextSize(strings[i],fonts[i]);
        ValueMap row;row.Add("part",labels[i]);row.Add("visible",visible[i]);row.Add("bounds",RectValue(part));
        row.Add("glyph",SizeValue(glyph));row.Add("font_face",fonts[i].GetFace());row.Add("font_em",fonts[i].GetHeight());
        measured.Add(row);
        if(visible[i]){
            Check(!part.IsEmpty(),name+" "+labels[i]+" exists");
            Check(part.GetHeight()>=glyph.cy,name+" "+labels[i]+" native glyph height fits");
            if(require_width)Check(part.GetWidth()>=glyph.cx,name+" "+labels[i]+" natural glyph width fits");
        }else Check(part.IsEmpty(),name+" hidden "+labels[i]+" has no hit part");
    }
    ValueMap row;row.Add("case",name);row.Add("bounds",RectValue(render.GetBounds()));
    row.Add("direction",render.GetDirection()==UiDirection::H?"H":"V");
    row.Add("content",SizeValue(render.GetContentSize()));row.Add("minimum",SizeValue(render.GetMinSize()));
    row.Add("parts",measured);records.Add(row);
}
void SavePaint(UiItemRender& render,const String& name)
{
    Size size(render.GetBounds().right+3,render.GetBounds().bottom+3);
    ImageDraw draw(size);draw.DrawRect(size,White());
    int serial=render.GetLayoutSerial();UiItemRenderState state;render.Paint(draw,state);
    Check(render.GetLayoutSerial()==serial&&!render.IsLayoutDirty(),name+" Paint consumes prepared geometry");
    if(!output.IsEmpty()&&!PNGEncoder().SaveFile(AppendFileName(output,name+".png"),draw))
        Check(false,name+" raster saved");
}
void BasicCase(const String& name,Font font,UiDirection direction,int kind,bool inherited=false,UiRole role=UiRole::Standard)
{
    UiItemRenderBasic render;UiItemRenderData data;data.role=role;data.title=kind==1?"Ag":"All genders";
    if(kind==1){data.subtitle="gyp";data.description="lower";data.right_text="R";}
    if(kind==2)data.emphasized=true;
    if(kind==3){data.use_custom_font=true;data.custom_font=Segoe(18);}
    if(!inherited){
        auto style=FlatStyle(font);
        if(kind==1){style.subtitle_font=Segoe(11);style.description_font=Segoe(12);style.right_font=Segoe(16);}
        render.SetCustomStyle(style);
    }
    render.SetData(data);render.PrepareLayout(RectC(3,5,1,1),direction);
    Size natural=render.GetContentSize(),minimum=render.GetMinSize();
    Check(natural.cx>0&&natural.cy>0&&minimum.cx>0&&minimum.cy>0,name+" positive native measurements");
    // Full natural size must fit all displayed content. The minimum may ellipsize
    // horizontally; check its vertical contract at a natural-content width.
    render.PrepareLayout(RectC(3,5,natural.cx,natural.cy),direction);
    ObserveParts(render,name+"-natural",true);
    int serial=render.GetLayoutSerial();
    Check(!render.PrepareLayout(render.GetBounds(),direction)&&render.GetLayoutSerial()==serial,name+" unchanged layout reused");
    SavePaint(render,name);
    render.PrepareLayout(RectC(3,5,max(natural.cx,minimum.cx),minimum.cy),direction);
    ObserveParts(render,name+"-minimum-height",false);
}
struct Trace {
    bool seen=false;Rect bounds,title;Font font;String text;
};
class TrackedBasic : public UiItemRenderBasic {
    std::shared_ptr<Trace> trace_;
public:
    TrackedBasic(std::shared_ptr<Trace> trace):trace_(trace){}
    One<UiItemRender> Clone() const override
    {
        One<UiItemRender> out=new TrackedBasic(trace_);CopyConfigurationTo(*out);return out;
    }
protected:
    void Layout() override
    {
        UiItemRenderBasic::Layout();trace_->seen=true;trace_->bounds=GetBounds();
        trace_->title=PartBounds(*this,UIITEMPART_TITLE);trace_->font=TitleFont(*this);trace_->text=GetData().title;
    }
};
void DropdownCase(const String& mode)
{
    UiDropdown dropdown;auto style=dropdown.GetStyle();
    style.font=Segoe(13);style.metrics.use_text_font=true;style.metrics.text_font=style.font;
    style.popup_item_style.font=style.font;style.metrics.content_margin=Rect(DPI(9),DPI(6),DPI(9),DPI(6));
    style.metrics.frame_enabled=true;style.metrics.frame_width=DPI(1);style.indicator_size=DPI(12);style.content_gap=DPI(6);
    dropdown.SetCustomStyle(style);dropdown.Add("All genders","all");dropdown.SelectByData("all");
    auto trace=std::make_shared<Trace>();TrackedBasic prototype(trace);
    // Copy the public default recipe, then let Dropdown's real collapsed Clone
    // bind selected data and prepare its own bounds. The prototype is NOT live.
    prototype.SetCustomStyle(dropdown.GetItemRender().GetStyle());dropdown.SetItemRender(prototype);
    Size minimum=dropdown.GetMinSize();dropdown.SetRect(0,0,minimum.cx,minimum.cy+DPI(1));dropdown.Layout();
    Check(trace->seen,mode+" actual collapsed native clone was prepared");
    Size glyph=GetTextSize("All genders",trace->font);
    Check(trace->text=="All genders"&&dropdown.GetSelectedData()==Value("all"),mode+" actual clone retains selected data");
    Check(Fits(trace->title,glyph),mode+" dropdown actual prepared title fits native glyph");
    Check(dropdown.GetLiveItemRenderCount()==1,mode+" one collapsed renderer, no popup");
    ValueMap row;row.Add("case",mode+"-actual-dropdown-clone");row.Add("minimum",SizeValue(minimum));
    row.Add("bounds",RectValue(trace->bounds));row.Add("title",RectValue(trace->title));row.Add("glyph",SizeValue(glyph));
    row.Add("font_face",trace->font.GetFace());row.Add("font_em",trace->font.GetHeight());row.Add("selected",dropdown.GetSelectedData());
    row.Add("evidence","instrumented native collapsed clone; prototype not used as live geometry");records.Add(row);
    ImageDraw raster(dropdown.GetSize());dropdown.DrawCtrl(raster);
    Check(output.IsEmpty()||PNGEncoder().SaveFile(AppendFileName(output,mode+"-dropdown.png"),raster),mode+" dropdown raster saved");
}
void RasterCase(const String& mode)
{
    UiItemRenderBasic render;render.SetCustomStyle(FlatStyle(Segoe(13)));
    UiItemRenderData data;data.title="All genders";render.SetData(data);
    Size natural=render.GetContentSize();Rect bounds=RectC(3,5,natural.cx,natural.cy);
    render.PrepareLayout(bounds,UiDirection::H);Rect title=PartBounds(render,UIITEMPART_TITLE);
    Font font=TitleFont(render);Size glyph=GetTextSize(data.title,font),size(bounds.right+3,bounds.bottom+3);
    ImageDraw actual(size),expected(size);actual.DrawRect(size,White());expected.DrawRect(size,White());
    UiItemRenderState state;render.Paint(actual,state);
    // Independent unclipped native glyph at the observed title origin; no
    // requested-font-height surrogate or private rectangle is used.
    expected.DrawText(title.left,title.top+max(0,(title.GetHeight()-glyph.cy)/2),data.title,font,Black());
    Image a=actual,b=expected;int different=0,ink=0;
    for(int y=0;y<size.cy;++y)for(int x=0;x<size.cx;++x){
        if(a[y][x].r!=b[y][x].r||a[y][x].g!=b[y][x].g||a[y][x].b!=b[y][x].b||a[y][x].a!=b[y][x].a)++different;
        if(b[y][x].r<250||b[y][x].g<250||b[y][x].b<250)++ink;
    }
    Check(ink>0,mode+" independent glyph raster nonempty");
    Check(different==0,mode+" renderer Paint retains complete native glyph raster");
    ValueMap row;row.Add("case",mode+"-native-raster");row.Add("different_pixels",different);row.Add("reference_ink_pixels",ink);records.Add(row);
    if(!output.IsEmpty()){
        Check(PNGEncoder().SaveFile(AppendFileName(output,mode+"-basic.png"),a),mode+" Basic raster saved");
        Check(PNGEncoder().SaveFile(AppendFileName(output,mode+"-glyph.png"),b),mode+" glyph reference raster saved");
    }
}
void CacheAndTiny(const String& mode)
{
    UiItemRenderBasic render;auto style=FlatStyle(Segoe(13));render.SetCustomStyle(style);
    UiItemRenderData data;data.title="Ag";render.SetData(data);render.PrepareLayout(RectC(0,0,90,30),UiDirection::H);
    int serial=render.GetLayoutSerial();data.title="New";render.SetData(data);
    Check(render.IsLayoutDirty()&&render.PrepareLayout(render.GetBounds(),UiDirection::H)&&render.GetLayoutSerial()>serial,mode+" data invalidates layout");
    style.title_font=Segoe(18);render.SetCustomStyle(style);
    Check(render.IsLayoutDirty()&&render.PrepareLayout(render.GetBounds(),UiDirection::H),mode+" font style invalidates layout");
    render.ClearCustomStyle();
    Check(!render.HasCustomStyle()&&render.IsLayoutDirty(),mode+" clear restores inheritance");
    for(auto dir:{UiDirection::H,UiDirection::V})for(int side:{0,1,3}){
        render.PrepareLayout(RectC(3,5,side,side),dir);
        ImageDraw draw(Size(12,12));draw.DrawRect(0,0,12,12,White());UiItemRenderState state;render.Paint(draw,state);
        Check(render.HitTest(Point(2,4)).part==UIITEMPART_NONE,mode+" tiny exterior has no hit");
    }
}
}
GUI_APP_MAIN
{
    try{
        const auto& args=CommandLine();if(args.GetCount()!=1)throw Exc("Usage: UiItemRenderTextFitTest <owned-evidence-directory>");
        output=args[0];RealizeDirectory(output);
        Check(Font::FindFaceNameIndex("Segoe UI")>0,"Segoe UI reproduction font available");
        for(int m=0;m<2;++m){
            UiThemeContext context;context.preset=UiThemePreset::Minimal;context.mode=m?UiThemeMode::Light:UiThemeMode::Dark;UiTheme::Set(context);
            String mode=m?"light":"dark";DropdownCase(mode);RasterCase(mode);
            for(auto dir:{UiDirection::H,UiDirection::V}){
                String prefix=mode+(dir==UiDirection::H?"-H":"-V");
                BasicCase(prefix+"-segoe13",Segoe(13),dir,0);
                BasicCase(prefix+"-standard",StdFont(),dir,0);
                BasicCase(prefix+"-emphasis",Segoe(13),dir,2);
                BasicCase(prefix+"-custom18",Segoe(13),dir,3);
                BasicCase(prefix+"-four-parts",Segoe(13),dir,1);
                BasicCase(prefix+"-small8",Segoe(8),dir,1);
                for(auto role:{UiRole::Standard,UiRole::Subtle,UiRole::Accent,UiRole::Alert})
                    BasicCase(prefix+"-inherited-"+AsString(int(role)),StdFont(),dir,0,true,role);
            }
            CacheAndTiny(mode);
        }
        ValueMap report;report.Add("kind","UiItemRenderTextFitTest");report.Add("checks",checks);report.Add("failures",failures);
        report.Add("records",records);report.Add("scope","native public Basic layout/hit/paint and actual instrumented Dropdown collapsed clone; current process DPI only");
        if(!SaveFile(AppendFileName(output,"geometry.json"),AsJSON(report,true)))throw Exc("Cannot save geometry evidence");
        Cout()<<"UiItemRenderTextFitTest: "<<checks<<" checks, "<<failures<<" failures\n";
        if(checks==0||failures)SetExitCode(1);
    }catch(const Exc& error){Cerr()<<"UiItemRenderTextFitTest ERROR: "<<error<<"\n";SetExitCode(2);}
}
