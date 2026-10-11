#ifndef _UiScreenplayDemo_Demo_h_
#define _UiScreenplayDemo_Demo_h_
#include <Ui/Ui.h>
#include <Utilities/PropertyEditor/PropertyEditor.h>
#include <Utilities/PropertyEditor/PropertyValueEditors.h>
#include <PdfDraw/PdfDraw.h>

using namespace Upp;

class ScreenplayDemo : public TopWindow {
    UiTitleCard header_;
    UiBoxLayout header_actions_, writer_actions_, rail_actions_;
    UiToolButton theme_, help_, exit_, inspector_mode_, style_mode_, data_mode_, code_mode_;
    UiToolButton open_, save_, import_, export_, pdf_, art_, copy_;
    UiPanel preview_, rail_, code_page_;
    UiStack pages_;
    UiScreenplay script_;
    UiLabel status_;
    UiButton scene_, action_, character_, dialogue_, parenthetical_, transition_, comment_, guidance_;
    PropertyEditorFactory factory_;
    PropertyEditorModel inspector_model_, style_model_, data_model_;
    PropertyEditor inspector_, style_, data_;
    UiMultiEdit code_;
    String selected_annotation_, generated_;
    Color face_ = SColorFace();
    bool syncing_ = false;
    Value ValueOf(const PropertyEditorModel& model, const char* id) const;
    void BuildModels();
    void ApplyInspector(String id);
    void ApplyStyle();
    void ApplyData(String id);
    void SyncSelection();
    void SelectAnnotation(const String& id);
    void Theme();
    void Page(int page);
    void UpdateCode();
    void FileAction(int action);
    void AddReference(bool comment);
    void AddArtwork();
public:
    ScreenplayDemo();
    ~ScreenplayDemo();
    Size GetMinSize() const override { return Size(DPI(1320), DPI(720)); }
    void Layout() override;
    void Paint(Draw& w) override { w.DrawRect(GetSize(), face_); }
    String Generate() const;
    void ExportGenerated(const String& directory);
    int SelfTest();
};
#endif
