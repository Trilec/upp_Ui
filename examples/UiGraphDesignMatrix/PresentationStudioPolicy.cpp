#include "PresentationStudioPolicy.h"

namespace Upp {

const UiGraphNodeShape kStudioShapes[STUDIO_SHAPE_COUNT] = {
    UiGraphNodeShape::Rectangle, UiGraphNodeShape::Ellipse,
    UiGraphNodeShape::Diamond, UiGraphNodeShape::Triangle,
    UiGraphNodeShape::Hexagon, UiGraphNodeShape::Cloud,
    UiGraphNodeShape::Document, UiGraphNodeShape::Database
};

const char *kStudioShapeIds[STUDIO_SHAPE_COUNT] = {
    "rectangle", "ellipse", "diamond", "triangle",
    "hexagon", "cloud", "document", "database"
};

const char *kStudioShapeNames[STUDIO_SHAPE_COUNT] = {
    "Rectangle", "Ellipse", "Diamond", "Triangle",
    "Hexagon", "Cloud", "Document", "Database"
};

const char *kStudioRowNames[STUDIO_LOD_COUNT] = {
    "Normal", "LOD 1", "LOD 2", "LOD 3"
};

const StudioFeatureInfo kStudioFeatures[STUDIO_FEATURE_COUNT] = {
    { "TLE",  "Title",                    "title" },
    { "SUB",  "Subtitle",                 "subtitle" },
    { "ICO",  "Icon",                     "icon" },
    { "BGE",  "Badge",                    "badge" },
    { "STA",  "Status",                   "status" },
    { "PRG",  "Progress",                 "progress" },
    { "DES",  "Description",              "description" },
    { "MED",  "Media",                    "media" },
    { "FLD",  "Fields / parameter rows",  "fields" },
    { "CONT", "Interactive controls",      "controls" },
    { "ACT",  "Actions / buttons",         "actions" },
    { "PLAB", "Individual port labels",    "port_labels" },
    { "PSUM", "Port summary",              "port_summary" },
    { "FOOT", "Footer",                    "footer" },
};

const StudioTemplateSpec kStudioTemplates[STUDIO_TEMPLATE_COUNT] = {
    { "minimal",   "Minimal",   "Minimal — pure recognition with almost no chrome",
      UiGraphPresentationProfile::Centred, Color(40, 123, 180), false },
    { "identity",  "Identity",  "Identity — recognition and classification",
      UiGraphPresentationProfile::Standard, Color(18, 111, 192), false },
    { "summary",   "Summary",   "Summary — recognition plus concise understanding",
      UiGraphPresentationProfile::Standard, Color(11, 127, 202), true },
    { "status",    "Status",    "Status — state, progress and activity first",
      UiGraphPresentationProfile::Standard, Color(23, 126, 104), true },
    { "media",     "Media",     "Media — thumbnail/content-led presentation",
      UiGraphPresentationProfile::MediaCard, Color(52, 112, 184), false },
    { "parameter", "Parameter", "Parameter — fields, values and direct manipulation",
      UiGraphPresentationProfile::Standard, Color(86, 104, 200), true },
    { "operator",  "Operator",  "Operator — maximum disciplined graph-node complexity",
      UiGraphPresentationProfile::Standard, Color(49, 82, 143), true },
};

String StudioPresentationLevelName(UiGraphPresentationLevel level)
{
    switch(level) {
    case UiGraphPresentationLevel::Normal: return "Normal";
    case UiGraphPresentationLevel::Lod1:   return "LOD 1";
    case UiGraphPresentationLevel::Lod2:   return "LOD 2";
    case UiGraphPresentationLevel::Lod3:   return "LOD 3";
    default:                               return "?";
    }
}

void StudioThresholdSet::Normalize()
{
    normal = minmax(normal,
                    STUDIO_RESOLUTION_MIN + 3.0 * STUDIO_RESOLUTION_MIN_SPAN,
                    STUDIO_RESOLUTION_MAX - STUDIO_RESOLUTION_MIN_SPAN);
    lod1 = minmax(lod1,
                  STUDIO_RESOLUTION_MIN + 2.0 * STUDIO_RESOLUTION_MIN_SPAN,
                  normal - STUDIO_RESOLUTION_MIN_SPAN);
    lod2 = minmax(lod2,
                  STUDIO_RESOLUTION_MIN + STUDIO_RESOLUTION_MIN_SPAN,
                  lod1 - STUDIO_RESOLUTION_MIN_SPAN);
}

void StudioThresholdSet::Jsonize(JsonIO& io)
{
    io("normal_px", normal)
      ("lod1_px", lod1)
      ("lod2_px", lod2);
    if(io.IsLoading())
        Normalize();
}

void StudioFeatureSet::Jsonize(JsonIO& io)
{
    bool values[STUDIO_FEATURE_COUNT];
    for(int i = 0; i < STUDIO_FEATURE_COUNT; i++)
        values[i] = Get(i);
    for(int i = 0; i < STUDIO_FEATURE_COUNT; i++)
        io(kStudioFeatures[i].json_key, values[i]);
    if(io.IsLoading()) {
        bits = 0;
        for(int i = 0; i < STUDIO_FEATURE_COUNT; i++)
            Set(i, values[i]);
    }
}

void StudioShapePolicy::Jsonize(JsonIO& io)
{
    io("shape", shape)
      ("threshold_override", threshold_override)
      ("thresholds", thresholds)
      ("lod", lod);
}

void StudioTemplatePolicy::Jsonize(JsonIO& io)
{
    io("id", id)
      ("name", name)
      ("global_thresholds", global_thresholds)
      ("shapes", shapes);
}

void StudioDocument::Jsonize(JsonIO& io)
{
    io("schema_version", schema_version)
      ("active_template", active_template)
      ("authored_size", authored_size)
      ("port_preset", port_preset)
      ("templates", templates);
}

int StudioFindTemplateIndex(const String& id)
{
    for(int i = 0; i < STUDIO_TEMPLATE_COUNT; i++)
        if(id == kStudioTemplates[i].id)
            return i;
    return 0;
}

static StudioFeatureSet FeatureMask(dword bits)
{
    StudioFeatureSet out;
    out.bits = bits;
    return out;
}

StudioFeatureSet StudioDefaultFeatures(int template_index, int lod_index)
{
    auto B = [](StudioFeature f) { return StudioFeatureBit(f); };
    lod_index = minmax(lod_index, 0, STUDIO_LOD_COUNT - 1);
    switch(template_index) {
    case 0: // Minimal
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::PortLabels));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::PortLabels));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Icon));
        return FeatureMask(0);
    case 1: // Identity
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::PortLabels));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::PortLabels));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon));
        return FeatureMask(B(StudioFeature::Icon));
    case 2: // Summary
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::Description) | B(StudioFeature::Footer) | B(StudioFeature::PortLabels));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::Description) | B(StudioFeature::PortLabels));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon));
        return FeatureMask(0);
    case 3: // Status
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Status) | B(StudioFeature::Progress) | B(StudioFeature::Description) | B(StudioFeature::Footer) | B(StudioFeature::PortLabels));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Status) | B(StudioFeature::Progress) | B(StudioFeature::PortLabels));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Status));
        return FeatureMask(B(StudioFeature::Status));
    case 4: // Media
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Description) | B(StudioFeature::Media) | B(StudioFeature::Footer) | B(StudioFeature::PortLabels));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Media) | B(StudioFeature::PortLabels));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Icon) | B(StudioFeature::Media) | B(StudioFeature::PortSummary));
        return FeatureMask(0);
    case 5: // Parameter
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::Fields) | B(StudioFeature::Controls) | B(StudioFeature::PortLabels) | B(StudioFeature::PortSummary));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::Fields) | B(StudioFeature::PortLabels) | B(StudioFeature::PortSummary));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::PortSummary));
        return FeatureMask(0);
    case 6: // Operator
        if(lod_index == 0) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Subtitle) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Status) | B(StudioFeature::Fields) | B(StudioFeature::Controls) | B(StudioFeature::Actions) | B(StudioFeature::PortLabels) | B(StudioFeature::PortSummary) | B(StudioFeature::Footer));
        if(lod_index == 1) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::Badge) | B(StudioFeature::Status) | B(StudioFeature::Fields) | B(StudioFeature::PortLabels) | B(StudioFeature::PortSummary));
        if(lod_index == 2) return FeatureMask(B(StudioFeature::Title) | B(StudioFeature::Icon) | B(StudioFeature::Status) | B(StudioFeature::PortSummary));
        return FeatureMask(B(StudioFeature::Status));
    default:
        return FeatureMask(0);
    }
}

StudioShapePolicy StudioMakeShapePolicy(int template_index, int shape_index)
{
    StudioShapePolicy out;
    shape_index = minmax(shape_index, 0, STUDIO_SHAPE_COUNT - 1);
    out.shape = kStudioShapeIds[shape_index];
    out.lod.SetCount(STUDIO_LOD_COUNT);
    for(int row = 0; row < STUDIO_LOD_COUNT; row++)
        out.lod[row] = StudioDefaultFeatures(template_index, row);
    return out;
}

StudioDocument StudioMakeDefaultDocument()
{
    StudioDocument doc;
    doc.templates.SetCount(STUDIO_TEMPLATE_COUNT);
    for(int t = 0; t < STUDIO_TEMPLATE_COUNT; t++) {
        StudioTemplatePolicy& policy = doc.templates[t];
        policy.id = kStudioTemplates[t].id;
        policy.name = kStudioTemplates[t].name;
        policy.shapes.SetCount(STUDIO_SHAPE_COUNT);
        for(int s = 0; s < STUDIO_SHAPE_COUNT; s++)
            policy.shapes[s] = StudioMakeShapePolicy(t, s);
    }
    return doc;
}

bool StudioValidateDocument(const StudioDocument& doc, String& error)
{
    if(doc.schema_version != 2) {
        error = "Unsupported schema_version. Presentation Studio expects version 2.";
        return false;
    }
    if(doc.templates.GetCount() != STUDIO_TEMPLATE_COUNT) {
        error = "Presentation Studio policy must contain the seven built-in templates.";
        return false;
    }
    for(int t = 0; t < STUDIO_TEMPLATE_COUNT; t++) {
        const StudioTemplatePolicy& policy = doc.templates[t];
        if(policy.id != kStudioTemplates[t].id || policy.shapes.GetCount() != STUDIO_SHAPE_COUNT) {
            error = "Template or shape catalogue does not match this Presentation Studio version.";
            return false;
        }
        for(int s = 0; s < STUDIO_SHAPE_COUNT; s++) {
            const StudioShapePolicy& shape = policy.shapes[s];
            if(shape.shape != kStudioShapeIds[s] || shape.lod.GetCount() != STUDIO_LOD_COUNT) {
                error = "Shape/LOD feature policy is incomplete.";
                return false;
            }
        }
    }
    return true;
}

} // namespace Upp
