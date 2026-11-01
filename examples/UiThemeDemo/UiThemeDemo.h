#ifndef _UiThemeDemo_h_
#define _UiThemeDemo_h_

// A native theme workbench. Sections own their specimens and models; the two
// panes resolve the same public recipes in independent Light/Dark snapshots.
#include <Ui/Ui.h>
#include <Ui/UiFileBrowser/UiFileBrowser.h>
#include <Ui/UiOsFileDialog/UiOsFileDialog.h>
#include <plugin/png/png.h>

using namespace Upp;

const char* ThemeRoleName(UiRole role);
Image ThemeSampleImage();
void FreezeThemeTree(Ctrl& root, UiRole role);

class ThemeSection : public UiPanel {
public:
    ThemeSection(const String& title, const String& detail);
    void AddSpecimen(const char* type, Ctrl& control, int height = 36);
    void AddSpecimenPair(const char* first_name, Ctrl& first, const char* second_name, Ctrl& second,
                         int height = 36, bool labels_above = false);
    int Arrange(int width);
    virtual void Apply(const UiThemeContext& context, UiRole role);
private:
    struct Row : Moveable<Row> { Ctrl* control; Ctrl* second = nullptr; int height; int label; bool labels_above = false; };
    UiLabel heading_, detail_;
    Array<UiLabel> names_;
    Vector<Row> rows_;
};

class ThemeTags : public Ctrl {
public:
    void Apply();
    void Layout() override;
    void Paint(Draw& draw) override;
private:
    UiTagStyle styles_[4];
    UiTagPresentation prepared_[4];
};

class ActionsSection : public ThemeSection {
public:
    ActionsSection();
    void Apply(const UiThemeContext& context, UiRole role) override;
private:
    UiBoxLayout commands_{UiDirection::H}, icons_{UiDirection::H}, choices_{UiDirection::H};
    UiButton buttons_[4], menu_button_, disabled_;
    UiToolButton tools_[4];
    UiSplitButton split_;
    UiCheckBox check_;
    UiRadioButton radio_[2];
    UiToggle toggle_;
    UiLabel label_;
    UiBreadcrumbs path_;
    UiDropdown dropdown_;
    UiMenu menu_;
    ThemeTags tags_;
};

class FieldsSection : public ThemeSection {
public:
    FieldsSection();
private:
    UiLineEdit text_;
    UiPasswordEdit password_;
    UiMaskEdit mask_;
    UiIntEdit integer_;
    UiFloatEdit decimal_;
    UiMultiEdit notes_;
    UiDateTime date_;
};

class ValuesSection : public ThemeSection {
public:
    ValuesSection();
    void Apply(const UiThemeContext& context, UiRole role) override;
private:
    UiSliderEdit slider_edit_;
    UiRangeSliderEdit interval_edit_;
    UiRangeSegments segments_;
    UiProgressBar progress_;
    UiBoxLayout rings_{UiDirection::H};
    UiProgressRing ring_;
    UiChartRing chart_;
    UiBezierCurveField curve_field_;
};

class ColourSection : public ThemeSection {
public:
    ColourSection();
private:
    UiMatrixSelector anchor_;
    UiColorMatrix swatches_;
    UiColorPickerMicro micro_;
    UiColorPicker picker_;
};

class SurfacesSection : public ThemeSection {
public:
    SurfacesSection();
    void Apply(const UiThemeContext& context, UiRole role) override;
private:
    UiTitleCard title_;
    UiPanel panel_;
    UiLabel panel_text_, group_text_, tab_text_[2], accordion_text_[2];
    UiGroupPanel group_;
    UiTab tabs_;
    UiPanel pages_[2];
    UiAccordion accordion_;
    UiSplitter splitter_;
    UiPanel split_panels_[2], quad_panels_[4];
    UiQuadSplitter quad_;
};

class CollectionsSection : public ThemeSection {
public:
    CollectionsSection();
private:
    UiListModel items_; // Outlives both bound views.
    UiList list_;
    UiGallery gallery_;
    UiTree tree_;
    UiTable table_;
    UiDoc document_;
};

class MediaSection : public ThemeSection {
public:
    MediaSection();
    void Apply(const UiThemeContext& context, UiRole role) override;
private:
    UiMediaCard media_;
    UiColorProbe probe_;
    UiPlaybackBar playback_;
    UiButton open_os_;
    UiButton open_browser_;
    UiThemeContext context_;
};

class ThemePane : public UiPanel {
public:
    ThemePane(UiThemeMode mode);
    void Apply(UiThemePreset preset, UiRole role);
    void SetCategory(int category);
    void EnableSamples(bool enabled);
    void Layout() override;
    UiScrollPanel& Scroller() { return scroll_; }
private:
    UiThemeMode mode_;
    UiThemePreset preset_ = UiThemePreset::Minimal;
    UiRole role_ = UiRole::Standard;
    UiLabel caption_;
    UiScrollPanel scroll_;
    Array<ThemeSection> sections_;
    int category_ = 0;
};

class UiThemeDemo : public TopWindow {
public:
    UiThemeDemo();
    void Layout() override;
    void Paint(Draw& draw) override;
    void Apply();
    bool CheckPresets(const String& folder);
    void Capture(const String& path);
private:
    void ApplyShell();
    void SelectRole(UiRole role);
    void SelectCategory(int category);
    UiThemePreset preset_ = UiThemePreset::Olive;
    UiRole role_ = UiRole::Standard;
    UiThemeMode shell_mode_ = UiThemeMode::Dark;
    UiTitleCard heading_;
    UiBoxLayout actions_{UiDirection::H};
    UiToolButton theme_, help_, exit_;
    UiLabel theme_label_, role_label_, footer_;
    UiDropdown presets_;
    UiButton categories_[8];
    int category_ = 0;
    UiButton roles_[4];
    UiCheckBox enabled_;
    ThemePane dark_{UiThemeMode::Dark}, light_{UiThemeMode::Light};
    bool scrolling_ = false;
};
#endif
