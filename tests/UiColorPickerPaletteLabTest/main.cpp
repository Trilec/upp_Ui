#include <Core/Core.h>
#include <Draw/Draw.h>
#include <Ui/UiColorPickerPaletteLab.h>

using namespace Upp;
using namespace Upp::UiColorPickerPaletteLab;

namespace {

int failures = 0;
int checks = 0;

void Check(bool condition, const String& message)
{
    checks++;
    if(condition)
        Cout() << "PASS  " << message << '\n';
    else {
        failures++;
        Cout() << "FAIL  " << message << '\n';
    }
}

bool SamePalette(const Vector<GeneratedSwatch>& a, const Vector<GeneratedSwatch>& b)
{
    if(a.GetCount() != b.GetCount())
        return false;
    for(int i = 0; i < a.GetCount(); i++)
        if(a[i].value.color != b[i].value.color || a[i].family_id != b[i].family_id || a[i].hero != b[i].hero)
            return false;
    return true;
}

Image MakeStripeImage()
{
    ImageBuffer buffer(Size(30, 10));
    for(int y = 0; y < 10; y++) {
        RGBA *row = buffer[y];
        for(int x = 0; x < 30; x++) {
            Color color = x < 10 ? White() : x < 20 ? Color(128, 128, 128) : Black();
            row[x] = RGBA(color);
            row[x].a = 255;
        }
    }
    return Image(buffer);
}

Image MakeSolidImage(Size size, Color color)
{
    ImageBuffer buffer(size);
    for(int y = 0; y < size.cy; y++) {
        RGBA *row = buffer[y];
        for(int x = 0; x < size.cx; x++) {
            row[x] = RGBA(color);
            row[x].a = 255;
        }
    }
    return Image(buffer);
}

Image MakeGradientImage(Size size)
{
    ImageBuffer buffer(size);
    for(int y = 0; y < size.cy; y++) {
        RGBA *row = buffer[y];
        for(int x = 0; x < size.cx; x++) {
            row[x] = RGBA(Color(x * 255 / max(1, size.cx - 1),
                                y * 255 / max(1, size.cy - 1),
                                (x + y) * 255 / max(1, size.cx + size.cy - 2)));
            row[x].a = 255;
        }
    }
    return Image(buffer);
}

void TestHarmonyAndAllocation()
{
    Vector<int> triad = HarmonyOffsets(UiColorPicker::HARMONY_TRIAD);
    Check(triad.GetCount() == 3 && triad[0] == 0 && triad[1] == 120 && triad[2] == 240,
          "Triad offsets are canonical and stable");

    GeneratorRecipe recipe;
    recipe.harmony = UiColorPicker::HARMONY_TRIAD;
    recipe.distribution = UiColorPicker::DISTRIBUTION_DOMINANT;
    recipe.requested_count = 11;
    ResetGeneratorFamilies(recipe, false);
    Vector<int> allocation = ApportionFamilies(recipe, recipe.families.GetCount());
    int sum = 0;
    for(int value : allocation)
        sum += value;
    Check(sum == 11, "Largest-remainder allocation reconciles to requested count");
    Check(allocation.GetCount() == 3 && allocation[0] >= 1 && allocation[1] >= 1 && allocation[2] >= 1,
          "Every active family receives a hero when count covers all families");
}

void TestSaturationAndGeneration()
{
    GeneratorRecipe recipe;
    recipe.harmony = UiColorPicker::HARMONY_TRIAD;
    recipe.requested_count = 12;
    ResetGeneratorFamilies(recipe, false);
    recipe.families[0].authored_saturation = 80;
    recipe.families[1].authored_saturation = 60;
    recipe.families[2].authored_saturation = 40;

    recipe.global_saturation = 0;
    Check(fabs(GlobalSaturationScale(recipe)) < 1e-12, "Global Saturation 0 collapses unlocked radii");
    recipe.global_saturation = 100;
    Check(fabs(GlobalSaturationScale(recipe) - 1.0) < 1e-12, "Global Saturation 100 preserves authored radii");
    recipe.global_saturation = 150;
    Check(fabs(GlobalSaturationScale(recipe) - 1.25) < 1e-12, "Global Saturation 150 moves the maximum authored radius to the rim");
    recipe.global_saturation = 100;
    Check(fabs(GlobalSaturationScale(recipe) - 1.0) < 1e-12, "Returning Saturation to 100 restores the authored relationship");

    Vector<GeneratedSwatch> generated = GeneratePalette(recipe);
    Check(generated.GetCount() == 12, "Generator count is the final authored palette count");
    int heroes = 0;
    for(const GeneratedSwatch& swatch : generated)
        heroes += swatch.hero;
    Check(heroes == 3, "Only real harmony families receive hero swatches");

    bool exact_counts = true;
    for(int count = 2; count <= 12; count++) {
        GeneratorRecipe counted = clone(recipe);
        counted.requested_count = count;
        exact_counts = exact_counts && GeneratePalette(counted).GetCount() == count;
    }
    Check(exact_counts, "Every generator count from 2 through 12 is the final palette size");

    GeneratorRecipe unlocked = clone(recipe);
    unlocked.global_gain = 0;
    unlocked.families[0].locked = true;
    Vector<GeneratedSwatch> before_gain = GeneratePalette(unlocked);
    unlocked.global_gain = 30;
    Vector<GeneratedSwatch> after_gain = GeneratePalette(unlocked);
    bool saw_locked = false;
    bool locked_unchanged = true;
    bool unlocked_changed = false;
    for(int i = 0; i < before_gain.GetCount(); i++) {
        if(before_gain[i].family_id == unlocked.families[0].id) {
            saw_locked = true;
            locked_unchanged = locked_unchanged && before_gain[i].value.color == after_gain[i].value.color;
        }
        else
            unlocked_changed = unlocked_changed || before_gain[i].value.color != after_gain[i].value.color;
    }
    Check(saw_locked && locked_unchanged && unlocked_changed,
          "Global Gain skips locked families and changes unlocked families");

    GeneratorRecipe web = clone(recipe);
    GeneratorRecipe paint = clone(recipe);
    web.medium = UiColorPicker::MEDIUM_WEB;
    paint.medium = UiColorPicker::MEDIUM_PAINTING;
    Check(!SamePalette(GeneratePalette(web), GeneratePalette(paint)), "Medium changes the generated colour result");
    Check(SamePalette(GeneratePalette(recipe), GeneratePalette(recipe)), "Generator is deterministic");
}

void TestStaticLibrary()
{
    const Vector<StaticPaletteDefinition>& library = StaticPaletteLibrary();
    Check(library.GetCount() >= 16, "Agreed creative and reference palette catalogue is present");
    Index<String> ids;
    Index<String> names;
    bool unique = true;
    bool valid = true;
    for(const StaticPaletteDefinition& palette : library) {
        if(ids.Find(palette.id) >= 0 || names.Find(palette.name) >= 0)
            unique = false;
        else {
            ids.Add(palette.id);
            names.Add(palette.name);
        }
        if(palette.id.IsEmpty() || palette.name.IsEmpty() || palette.swatches.IsEmpty())
            valid = false;
        for(const UiColorPicker::SlotValue& swatch : palette.swatches)
            if(IsNull(swatch.color) || swatch.alpha < 0 || swatch.alpha > 255)
                valid = false;
    }
    Check(unique, "Static palette IDs and display names are unique");
    Check(valid, "Static palette entries contain valid non-empty colour data");
}

void TestProxyAndImageAnalysis()
{
    Image large = MakeGradientImage(Size(1024, 256));
    Image proxy = MakeAnalysisProxy(large, 512);
    Check(proxy.GetSize() == Size(512, 128), "Analysis proxy is bounded to 512 while preserving aspect ratio");

    ImageAnalysisSettings settings;
    settings.requested_count = 6;
    settings.analysis = UiColorPicker::IMAGE_REPRESENTATIVE;
    settings.coverage = UiColorPicker::COVERAGE_BALANCED;
    ImageAnalysisResult first = AnalyzeImage(large, settings);
    ImageAnalysisResult second = AnalyzeImage(large, settings);
    Check(first.IsValid() && second.IsValid(), "Deterministic image analysis returns a valid palette");
    Check(SamePalette(first.swatches, second.swatches), "Image analysis is deterministic across repeated runs");
    Check(first.proxy_size.cx <= 512 && first.proxy_size.cy <= 512, "Image analysis never exceeds the proxy bound");
}

void TestIndependentExclusions()
{
    Image stripes = MakeStripeImage();
    ImageAnalysisSettings settings;
    settings.requested_count = 2;
    settings.tolerance = 5;
    settings.exclusion[0].enabled = true;
    settings.exclusion[0].placed = true;
    settings.exclusion[0].position = Pointf(0.1, 0.5);
    settings.exclusion[0].color = White();
    settings.exclusion[1].enabled = true;
    settings.exclusion[1].placed = true;
    settings.exclusion[1].position = Pointf(0.9, 0.5);
    settings.exclusion[1].color = Black();
    ImageAnalysisResult result = AnalyzeImage(stripes, settings);
    Check(result.IsValid(), "Independent A/B exclusion leaves analysable middle pixels");
    Check(result.ignored_fraction > 0.5 && result.ignored_fraction < 0.8,
          "Independent fills exclude the two seeded regions without bridging the grey region");

    ImageAnalysisSettings excessive;
    excessive.requested_count = 4;
    excessive.exclusion[0].enabled = true;
    excessive.exclusion[0].placed = true;
    excessive.exclusion[0].position = Pointf(0.5, 0.5);
    excessive.exclusion[0].color = White();
    ImageAnalysisResult warning = AnalyzeImage(MakeSolidImage(Size(40, 40), White()), excessive);
    Check(!warning.IsValid() && warning.ignored_fraction >= 0.98,
          "Nearly complete exclusion fails closed instead of replacing a valid palette");
}

void TestTransactionalStash()
{
    Vector<UiColorPicker::SlotValue> destination;
    for(int i = 0; i < 27; i++) {
        UiColorPicker::SlotValue& value = destination.Add();
        value.color = Color(i, i, i);
        value.alpha = 255;
    }
    Vector<UiColorPicker::SlotValue> source;
    for(int i = 0; i < 2; i++) {
        UiColorPicker::SlotValue& value = source.Add();
        value.color = Color(200 + i, 10, 20);
        value.alpha = 255;
    }
    Vector<UiColorPicker::SlotValue> before = clone(destination);
    Check(!AddUniqueTransactional(destination, source, 28, false, nullptr),
          "Grouped transfer refuses when the complete selection cannot fit");
    Check(destination == before, "Failed grouped transfer leaves the stash unchanged");

    source.SetCount(1);
    Check(AddUniqueTransactional(destination, source, 28, false, nullptr) && destination.GetCount() == 28,
          "A fitting unique transfer commits atomically");
    Check(AddUniqueTransactional(destination, source, 28, false, nullptr) && destination.GetCount() == 28,
          "Duplicate transfer is ignored without consuming capacity");
}

} // namespace

CONSOLE_APP_MAIN
{
    TestHarmonyAndAllocation();
    TestSaturationAndGeneration();
    TestStaticLibrary();
    TestProxyAndImageAnalysis();
    TestIndependentExclusions();
    TestTransactionalStash();
    Cout() << "\nPalette Lab checks: " << checks << ", failures: " << failures << '\n';
    SetExitCode(failures ? 1 : 0);
}
