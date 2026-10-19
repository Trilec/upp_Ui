// Native caption raster diagnostic; source-only preparation until root allocates a run.
#include <Ui/UiLabel.h>
#include <Ui/UiStyle.h>
#include <Shared/UiTheme/CineSpaceTheme.h>
#include <plugin/png/png.h>

using namespace Upp;
namespace {
int checks=0,failures=0;
String output;
ValueArray cases;

void Check(bool pass,const String& message)
{
    ++checks;
    if(!pass){++failures;Cout()<<"FAIL "<<message<<"\n";}
}
Value RectValue(Rect r){ValueArray a;a.Add(r.left);a.Add(r.top);a.Add(r.right);a.Add(r.bottom);return a;}
Value SizeValue(Size s){ValueArray a;a.Add(s.cx);a.Add(s.cy);return a;}
Value PointValue(Point p){ValueArray a;a.Add(p.x);a.Add(p.y);return a;}
Value ColorValue(Color c){ValueArray a;a.Add(c.GetR());a.Add(c.GetG());a.Add(c.GetB());return a;}
Value FontValue(Font f)
{
    ValueMap m;m.Add("face",f.GetFace());m.Add("face_name",f.GetFaceName());
    m.Add("height",f.GetHeight());m.Add("bold",f.IsBold());m.Add("italic",f.IsItalic());return m;
}
bool EqualPixel(const RGBA& a,const RGBA& b)
{
    return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;
}
void IncludePixel(Rect& bounds,bool& any,int x,int y)
{
    Rect p=RectC(x,y,1,1);if(any)bounds|=p;else{bounds=p;any=true;}
}
struct Projection {
    Rect outer,content,main;
    Point origin;
};
// UiLabel's cached main rectangle is private. This is explicitly a PUBLIC
// helper projection for an icon-free, single-line caption, not a private read.
// Raster agreement in the middle of the text independently checks alignment;
// the complete raster comparison below exempts no end or overhang pixels.
Projection ProjectPlainCaption(UiLabel& label,Size advance)
{
    const auto& s=label.GetStyle();Projection p;p.outer=Rect(label.GetSize());
    p.content=UiStyledInnerRect(p.outer,s.metrics,s.skin);
    p.main=UiComputeBlocksLayout(p.content,Size(0,0),advance,s.align_h,s.align_v,s.icon_side,DPI(16),0).main;
    p.origin=p.main.TopLeft();
    if(s.align_h==UiAlign::RIGHT)p.origin.x+=p.main.GetWidth()-advance.cx;
    else if(s.align_h==UiAlign::CENTER)p.origin.x+=(p.main.GetWidth()-advance.cx)/2;
    if(s.align_v==UiAlign::BOTTOM)p.origin.y+=p.main.GetHeight()-advance.cy;
    else if(s.align_v==UiAlign::CENTER)p.origin.y+=(p.main.GetHeight()-advance.cy)/2;
    return p;
}
void RunCaption(CineSpace::CineSpaceTheme& theme,const String& name,const String& text,bool natural_height)
{
    // Exact current UIMediaSlots recipe, including its returned skin.
    UiLabel label;auto recipe=theme.Label(UiRole::Subtle);
    recipe.font=theme.CssFont(10);
    label.SetCustomStyle(recipe);label.SetAlign(UiAlign::RIGHT,UiAlign::CENTER);label.SetText(text);
    const auto& style=label.GetStyle();
    Font effective=style.metrics.use_text_font?style.metrics.text_font:style.font;
    if(IsNull(effective))effective=StdFont();
    Size advance=GetTextSize(label.GetText(),effective),natural=label.GetContentSize();
    Size allocated(natural.cx,natural_height?natural.cy:max(DPI(22),natural.cy));
    Check(natural==label.GetMinSize(),name+" natural/minimum public sizes agree");
    Check(!IsNull(effective)&&advance.cx>0&&advance.cy>0,name+" native effective font and advance available");
    Check(label.GetIcon().IsEmpty()&&!style.metrics.face_enabled&&!style.metrics.frame_enabled
          &&!style.metrics.focus_enabled&&!style.metrics.shadow.enabled&&!style.underline
          &&style.transparent&&!label.IsRichEnabled(),
          name+" exact plain transparent caption contract");
    label.SetRect(0,0,allocated.cx,allocated.cy);label.Layout();
    Projection geometry=ProjectPlainCaption(label,advance);

    // Both native rasters use the same opaque parent colour. No style/font/inset
    // is changed to create a fit. Guard space belongs only to the oracle canvas.
    Color background=theme.Tokens().panel,ink=style.palette.ink[ST_NORMAL];
    ImageDraw native(allocated);native.DrawRect(Rect(allocated),background);label.DrawCtrl(native);
    Image actual=native;
    const int guard=max(DPI(24),effective.GetHeight()*2);
    Size canvas(allocated.cx+2*guard,allocated.cy+2*guard);
    Point oracle_origin=geometry.origin+Point(guard,guard);
    Rect control_clip=RectC(guard,guard,allocated.cx,allocated.cy);
    Rect advance_rect=RectC(oracle_origin.x,oracle_origin.y,advance.cx,advance.cy);
    // Widening a RIGHT-aligned UiLabel would retain the same right-edge clip.
    // Direct native DrawText is the independent larger-canvas ink oracle.
    ImageDraw reference_draw(canvas),actual_draw(canvas);
    reference_draw.DrawRect(Rect(canvas),background);actual_draw.DrawRect(Rect(canvas),background);
    reference_draw.DrawText(oracle_origin.x,oracle_origin.y,label.GetText(),effective,ink);
    actual_draw.DrawImage(guard,guard,actual);
    Image reference=reference_draw,padded=actual_draw;
    ImageBuffer difference(canvas);RGBA bg=RGBA(background);
    Rect reference_ink,actual_ink,lost_bounds;bool have_reference=false,have_actual=false,have_lost=false;
    int reference_count=0,actual_count=0,different=0,lost=0,extra=0,changed=0;
    int lost_inside=0,lost_outside=0,ink_beyond_advance=0,core_ink=0,core_difference=0;
    // Alignment check uses the central half only; full difference/loss below
    // still compares EVERY canvas pixel and all glyph ends.
    Rect core=RectC(advance_rect.left+advance.cx/4,advance_rect.top,
                    max(1,advance.cx/2),advance.cy);
    for(int y=0;y<canvas.cy;++y)for(int x=0;x<canvas.cx;++x){
        const RGBA& r=reference[y][x];const RGBA& a=padded[y][x];
        bool ri=!EqualPixel(r,bg),ai=!EqualPixel(a,bg),same=EqualPixel(r,a);
        RGBA mark=RGBA(White());
        if(ri){++reference_count;IncludePixel(reference_ink,have_reference,x,y);
            if(!advance_rect.Contains(Point(x,y)))++ink_beyond_advance;}
        if(ai){++actual_count;IncludePixel(actual_ink,have_actual,x,y);}
        if(!same){++different;
            if(ri&&!ai){++lost;IncludePixel(lost_bounds,have_lost,x,y);mark=RGBA(Color(220,40,40));
                if(control_clip.Contains(Point(x,y)))++lost_inside;else ++lost_outside;}
            else if(ai&&!ri){++extra;mark=RGBA(Color(40,90,220));}
            else{++changed;mark=RGBA(Color(220,140,30));}
        }
        if(core.Contains(Point(x,y))){if(ri)++core_ink;if(!same)++core_difference;}
        mark.a=255;difference[y][x]=mark;
    }
    Check(reference_count>0,name+" independent native ink reference is nonempty");
    Check(have_reference&&reference_ink.left>0&&reference_ink.top>0
          &&reference_ink.right<canvas.cx&&reference_ink.bottom<canvas.cy,
          name+" large oracle canvas does not clip its own ink");
    Check(core_ink>0&&core_difference==0,name+" central native raster verifies projected text origin");
    Check(different==0,name+" measured allocation retains complete native caption raster");
    Check(PNGEncoder().SaveFile(AppendFileName(output,name+"-actual.png"),actual),name+" original native raster saved");
    Check(PNGEncoder().SaveFile(AppendFileName(output,name+"-reference.png"),reference),name+" large reference raster saved");
    Check(PNGEncoder().SaveFile(AppendFileName(output,name+"-difference.png"),Image(difference)),name+" difference raster saved");

    ValueMap row;row.Add("case",name);row.Add("text",label.GetText());row.Add("requested_css_px",10);
    row.Add("effective_font",FontValue(effective));row.Add("style_font",FontValue(style.font));
    row.Add("metrics_text_font",FontValue(style.metrics.text_font));row.Add("use_text_font",style.metrics.use_text_font);
    row.Add("align_h",int(style.align_h));row.Add("align_v",int(style.align_v));
    row.Add("advance",SizeValue(advance));row.Add("natural_outer",SizeValue(natural));row.Add("allocated",SizeValue(allocated));
    row.Add("local_outer_clip",RectValue(geometry.outer));row.Add("public_content_projection",RectValue(geometry.content));
    row.Add("public_main_projection",RectValue(geometry.main));row.Add("projected_origin",PointValue(geometry.origin));
    row.Add("content_margin",RectValue(style.metrics.content_margin));row.Add("skin_content_inset",RectValue(style.skin.content_inset));
    row.Add("frame_width",style.metrics.frame_width);row.Add("frame_enabled",style.metrics.frame_enabled);
    row.Add("background",ColorValue(background));row.Add("ink",ColorValue(ink));
    row.Add("oracle_origin",PointValue(oracle_origin));row.Add("original_clip_in_oracle",RectValue(control_clip));
    row.Add("advance_rect_in_oracle",RectValue(advance_rect));row.Add("reference_ink_bounds",RectValue(reference_ink));
    row.Add("actual_ink_bounds",RectValue(actual_ink));row.Add("lost_ink_bounds",RectValue(lost_bounds));
    row.Add("reference_ink_pixels",reference_count);row.Add("actual_ink_pixels",actual_count);
    row.Add("different_pixels",different);row.Add("lost_pixels",lost);row.Add("extra_pixels",extra);row.Add("changed_pixels",changed);
    row.Add("lost_inside_original_clip",lost_inside);row.Add("lost_outside_original_clip",lost_outside);
    row.Add("reference_ink_outside_advance",ink_beyond_advance);row.Add("alignment_core",RectValue(core));
    row.Add("alignment_core_ink",core_ink);row.Add("alignment_core_differences",core_difference);
    row.Add("origin_evidence","public helper projection, independently checked by central raster; no private cached geometry read");
    cases.Add(row);
}
}
GUI_APP_MAIN
{
    try{
        const auto& args=CommandLine();
        if(args.GetCount()!=1)throw Exc("Usage: UiLabelTextFitTest <owned-evidence-directory>");
        output=args[0];RealizeDirectory(output);
        CineSpace::CineSpaceTheme theme;theme.SetVisualProfile(CineSpace::ThemeVisualProfile::CineCastR9);
        for(int m=0;m<2;++m){
            theme.SetMode(m?UiThemeMode::Light:UiThemeMode::Dark);theme.ApplyHostMode();
            String mode=m?"light":"dark";
            RunCaption(theme,mode+"-revision-0-row","Library revision 0",false);
            RunCaption(theme,mode+"-revision-0-natural","Library revision 0",true);
            RunCaption(theme,mode+"-revision-12-row","Library revision 12",false);
            RunCaption(theme,mode+"-source-count-row","0 / 16 supplied sources",false);
            RunCaption(theme,mode+"-reference-ink-row","Mgyp 0",false);
        }
        ValueMap report;report.Add("kind","UiLabelTextFitTest");report.Add("checks",checks);report.Add("failures",failures);
        report.Add("cases",cases);report.Add("result_scope","native ImageDraw/UiLabel/DrawText comparison at this process DPI; no normal window or HTML equivalence");
        if(!SaveFile(AppendFileName(output,"caption-raster.json"),AsJSON(report,true)))throw Exc("Cannot save caption raster evidence");
        Cout()<<"UiLabelTextFitTest: "<<checks<<" checks, "<<failures<<" failures\n";
        if(checks==0||failures)SetExitCode(1);
    }catch(const Exc& error){Cerr()<<"UiLabelTextFitTest ERROR: "<<error<<"\n";SetExitCode(2);}
}
