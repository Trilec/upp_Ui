#include "UiThemeDemo.h"

CollectionsSection::CollectionsSection()
    : ThemeSection("Collections and documents", "Small real models, meaningful selection, and editable native views")
{
    for(int i = 0; i < 6; ++i) items_.Add(Format("Shot %03d  /  %s",(i+1)*10,i%2 ? "Ready" : "In review"),i);
    list_.SetModel(items_).SetCursor(1);
    gallery_.SetModel(items_).SetItemSize(Size(DPI(150),DPI(74))).Select(1);
    auto project=tree_.Model().AddChild(tree_.Model().Root(),UiModelItem("Project Aurora","project"));
    auto sequence=tree_.Model().AddChild(project,UiModelItem("Sequence 020","sq020"));
    tree_.Model().AddChild(sequence,UiModelItem("Shot 010 / review","sh010"));
    tree_.Model().AddChild(sequence,UiModelItem("Shot 020 / ready","sh020"));
    tree_.Expand(project).Expand(sequence).SelectNode(sequence);
    AddSpecimenPair("UiList",list_,"UiTree",tree_,150,true);
    AddSpecimen("UiGallery",gallery_,120);
    table_.Model().SetSize(3,3);
    for(int r=0;r<3;++r) {
        table_.Model().SetCellValue(r,0,Format("SH%03d",(r+1)*10));
        table_.Model().SetCellValue(r,1,r%2 ? "Ready" : "In review");
        table_.Model().SetCellValue(r,2,Format("%d frames",96+r*24));
    }
    table_.SetRowHeight(DPI(30)).SetHeaderHeight(DPI(32)).SetActiveCell(1,1); AddSpecimen("UiTable",table_,175);
    document_.SetText("Project Aurora\nA native document sample\n\nWrite a note here, then change the theme or semantic role. The document model and your edits are retained.");
    AddSpecimen("UiDoc",document_,230);

}

MediaSection::MediaSection()
    : ThemeSection("Media and file selection", "Media stays host-owned; the optional browser is the same public Ui package")
{
    media_.SetTitle("SH030 take 03").SetSubTitle("1536 x 864 / review").SetImage(ThemeSampleImage()).SetMediaAspect(Size(16,9)).SetMediaFit(UiMediaFit::Contain);
    AddSpecimen("UiMediaCard",media_,270);
    UiColorSample sample; sample.r=.24;sample.g=.48;sample.b=.62;sample.a=1;
    sample.swatch=Color(61,122,158);sample.valid=true;sample.space="Display";
    probe_.SetSample(sample); AddSpecimen("UiColorProbe",probe_,40);
    playback_.SetFrames(1001,1120); playback_.SetPosition(1048); playback_.SetSelection(1020,1080);
    AddSpecimen("UiPlaybackBar",playback_,80);
    open_os_.SetText("Open OS-native file dialog");
    open_os_.WhenAction=[] { UiOsFileDialog dialog; dialog.SetTitle("Choose a file"); dialog.SetMode(UiOsFileDialog::Mode::OpenFile); dialog.Execute(); };
    AddSpecimen("UiOsFileDialog",open_os_);
    open_browser_.SetText("Open Ui file and sequence browser");
    open_browser_.WhenAction=[this] {
        // This composite owns application-themed popup chrome. Give its modal
        // lifetime the selected specimen context, then restore the workbench.
        UiThemeContext saved=UiTheme::GetContext();
        UiTheme::Set(context_);
        { UiFileBrowserDialog dialog; UiFileBrowserSelection selection;
          dialog.Execute(GetHomeDirectory(),true,selection); }
        UiTheme::Set(saved);
    };
    AddSpecimen("UiFileBrowser",open_browser_);
}
void MediaSection::Apply(const UiThemeContext& context, UiRole role)
{
    context_=context; media_.SetRole(role);
    ThemeSection::Apply(context,role);
}
