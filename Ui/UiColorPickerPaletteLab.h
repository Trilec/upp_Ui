#ifndef _Ui_UiColorPickerPaletteLab_h_
#define _Ui_UiColorPickerPaletteLab_h_

#include <Ui/UiColorPicker.h>

namespace Upp {
namespace UiColorPickerPaletteLab {

struct PaletteFamily : Moveable<PaletteFamily> {
    int    id = 0;
    int    canonical_offset = 0;
    int    custom_offset = 0;
    int    authored_saturation = 82;
    int    gain = 0;
    int    priority = 0;
    String role;
    bool   locked = false;
};

struct GeneratedSwatch : Moveable<GeneratedSwatch> {
    UiColorPicker::SlotValue value;
    int family_id = -1;
    int source_index = -1;
    bool hero = false;
    bool gamut_mapped = false;
};

struct GeneratorRecipe : Moveable<GeneratorRecipe> {
    UiColorPicker::HarmonyMode harmony = UiColorPicker::HARMONY_TRIAD;
    UiColorPicker::DistributionMode distribution = UiColorPicker::DISTRIBUTION_BALANCED;
    UiColorPicker::MediumMode medium = UiColorPicker::MEDIUM_UI;
    int base_hue = 200;
    int requested_count = 6;
    int global_gain = 0;
    int global_saturation = 100;
    bool free_angles = false;
    Vector<PaletteFamily> families;

    GeneratorRecipe() = default;
    GeneratorRecipe(const GeneratorRecipe& source) { operator=(source); }
    GeneratorRecipe& operator=(const GeneratorRecipe& source)
    {
        if(this == &source)
            return *this;
        harmony = source.harmony;
        distribution = source.distribution;
        medium = source.medium;
        base_hue = source.base_hue;
        requested_count = source.requested_count;
        global_gain = source.global_gain;
        global_saturation = source.global_saturation;
        free_angles = source.free_angles;
        families = clone(source.families);
        return *this;
    }
};

struct StaticPaletteDefinition : Moveable<StaticPaletteDefinition> {
    String id;
    String name;
    String category;
    String source_reference;
    bool authoritative = false;
    int preferred_columns = 6;
    Vector<UiColorPicker::SlotValue> swatches;
};

struct ImageExclusionSeed : Moveable<ImageExclusionSeed> {
    bool enabled = false;
    bool placed = false;
    Pointf position = Pointf(0.5, 0.5);
    Color color = Null;
};

struct ImageAnalysisSettings : Moveable<ImageAnalysisSettings> {
    UiColorPicker::ImageAnalysisMode analysis = UiColorPicker::IMAGE_REPRESENTATIVE;
    UiColorPicker::ImageCoverageMode coverage = UiColorPicker::COVERAGE_BALANCED;
    UiColorPicker::MediumMode medium = UiColorPicker::MEDIUM_IMAGE_VFX;
    int requested_count = 6;
    int tolerance = 18;
    ImageExclusionSeed exclusion[2];
};

struct ImageAnalysisResult : Moveable<ImageAnalysisResult> {
    Image proxy;
    Size original_size;
    Size proxy_size;
    Vector<GeneratedSwatch> swatches;
    Vector<Pointf> representative_positions;
    Vector<byte> exclusion_mask;
    double ignored_fraction = 0.0;
    String diagnostic;

    ImageAnalysisResult() = default;
    ImageAnalysisResult(const ImageAnalysisResult& source) { operator=(source); }
    ImageAnalysisResult& operator=(const ImageAnalysisResult& source)
    {
        if(this == &source)
            return *this;
        proxy = source.proxy;
        original_size = source.original_size;
        proxy_size = source.proxy_size;
        swatches = clone(source.swatches);
        representative_positions = clone(source.representative_positions);
        exclusion_mask = clone(source.exclusion_mask);
        ignored_fraction = source.ignored_fraction;
        diagnostic = source.diagnostic;
        return *this;
    }

    bool IsValid() const { return !proxy.IsEmpty() && !swatches.IsEmpty(); }
};

int NormalizeHue(int hue);
void ColorToHsv(Color color, int& hue, int& saturation, int& value);
Color HsvToColor(double hue, double saturation, double value);
void ColorToHsl(Color color, int& hue, int& saturation, int& lightness);
Color HslToColor(double hue, double saturation, double lightness);
void ColorToCmyk(Color color, int& cyan, int& magenta, int& yellow, int& black);
Color CmykToColor(double cyan, double magenta, double yellow, double black);
void ColorToLab(Color color, double& l, double& a, double& b);
Color LabToColor(double l, double a, double b);
void ColorToTmi(Color color, double& temperature, double& magenta, double& intensity);
Color TmiToColor(double temperature, double magenta, double intensity);

String FormatHex(Color color);
String FormatHex8(Color color, int alpha);
bool ParseColorText(const String& text, Color& color, int& alpha, bool& has_alpha);

Vector<int> HarmonyOffsets(UiColorPicker::HarmonyMode harmony);
void ResetGeneratorFamilies(GeneratorRecipe& recipe, bool preserve_family_state = true);
double GlobalSaturationScale(const GeneratorRecipe& recipe);
Vector<int> ApportionFamilies(const GeneratorRecipe& recipe, int active_family_count);
Vector<GeneratedSwatch> GeneratePalette(const GeneratorRecipe& recipe);

const Vector<StaticPaletteDefinition>& StaticPaletteLibrary();
int FindStaticPalette(const String& id);

Image MakeAnalysisProxy(const Image& image, int maximum_dimension = 512);
ImageAnalysisResult AnalyzeImage(const Image& original, const ImageAnalysisSettings& settings);
Vector<GeneratedSwatch> ApplyImagePostProcessing(const Vector<GeneratedSwatch>& source,
                                                  UiColorPicker::MediumMode medium,
                                                  int global_gain,
                                                  int global_saturation,
                                                  int hero_index,
                                                  int hero_gain);

bool AddUniqueTransactional(Vector<UiColorPicker::SlotValue>& destination,
                            const Vector<UiColorPicker::SlotValue>& source,
                            int capacity,
                            bool allow_duplicates,
                            int *rejected = nullptr);

} // namespace UiColorPickerPaletteLab
} // namespace Upp

#endif
