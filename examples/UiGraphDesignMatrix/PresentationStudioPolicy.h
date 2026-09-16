#ifndef _UiGraphDesignMatrix_PresentationStudioPolicy_h_
#define _UiGraphDesignMatrix_PresentationStudioPolicy_h_

#include <Ui/Ui.h>

namespace Upp {

static constexpr int STUDIO_SHAPE_COUNT = 8;
static constexpr int STUDIO_LOD_COUNT = 4;
static constexpr int STUDIO_TEMPLATE_COUNT = 7;
static constexpr double STUDIO_RESOLUTION_MIN = 32.0;
static constexpr double STUDIO_RESOLUTION_MAX = 300.0;
static constexpr double STUDIO_RESOLUTION_MIN_SPAN = 8.0;

enum class StudioFeature : byte {
    Title = 0,
    Subtitle,
    Icon,
    Badge,
    Status,
    Progress,
    Description,
    Media,
    Fields,
    Controls,
    Actions,
    PortLabels,
    PortSummary,
    Footer,
    Count,
};

static constexpr int STUDIO_FEATURE_COUNT = (int)StudioFeature::Count;

inline dword StudioFeatureBit(StudioFeature feature)
{
    return (dword)1 << (int)feature;
}

struct StudioFeatureInfo {
    const char *tag;
    const char *name;
    const char *json_key;
};

extern const UiGraphNodeShape kStudioShapes[STUDIO_SHAPE_COUNT];
extern const char *kStudioShapeIds[STUDIO_SHAPE_COUNT];
extern const char *kStudioShapeNames[STUDIO_SHAPE_COUNT];
extern const char *kStudioRowNames[STUDIO_LOD_COUNT];
extern const StudioFeatureInfo kStudioFeatures[STUDIO_FEATURE_COUNT];

String StudioPresentationLevelName(UiGraphPresentationLevel level);

struct StudioThresholdSet : Moveable<StudioThresholdSet> {
    double normal = 160.0;
    double lod1 = 80.0;
    double lod2 = 48.0;

    void Normalize();
    void Jsonize(JsonIO& io);
};

struct StudioFeatureSet : Moveable<StudioFeatureSet> {
    dword bits = 0;

    bool Get(StudioFeature feature) const { return (bits & StudioFeatureBit(feature)) != 0; }
    bool Get(int index) const { return index >= 0 && index < STUDIO_FEATURE_COUNT && Get((StudioFeature)index); }
    void Set(StudioFeature feature, bool value = true)
    {
        if(value)
            bits |= StudioFeatureBit(feature);
        else
            bits &= ~StudioFeatureBit(feature);
    }
    void Set(int index, bool value = true)
    {
        if(index >= 0 && index < STUDIO_FEATURE_COUNT)
            Set((StudioFeature)index, value);
    }
    void Jsonize(JsonIO& io);
};

struct StudioShapePolicy : Moveable<StudioShapePolicy> {
    String shape;
    bool threshold_override = false;
    StudioThresholdSet thresholds;
    Vector<StudioFeatureSet> lod;

    void Jsonize(JsonIO& io);
};

struct StudioTemplatePolicy : Moveable<StudioTemplatePolicy> {
    String id;
    String name;
    StudioThresholdSet global_thresholds;
    Vector<StudioShapePolicy> shapes;

    void Jsonize(JsonIO& io);
};

struct StudioDocument : Moveable<StudioDocument> {
    int schema_version = 2;
    String active_template = "media";
    String authored_size = "reference";
    String port_preset = "1x1";
    Vector<StudioTemplatePolicy> templates;

    void Jsonize(JsonIO& io);
};

struct StudioTemplateSpec {
    const char *id;
    const char *name;
    const char *intent;
    UiGraphPresentationProfile profile;
    Color accent;
    bool header_band;
};

extern const StudioTemplateSpec kStudioTemplates[STUDIO_TEMPLATE_COUNT];

int StudioFindTemplateIndex(const String& id);
StudioFeatureSet StudioDefaultFeatures(int template_index, int lod_index);
StudioShapePolicy StudioMakeShapePolicy(int template_index, int shape_index);
StudioDocument StudioMakeDefaultDocument();
bool StudioValidateDocument(const StudioDocument& doc, String& error);

} // namespace Upp

#endif
