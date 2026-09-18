#ifndef _UiGraphWorkspace_h_
#define _UiGraphWorkspace_h_

#include <Ui/Ui.h>

// Authoring-only package. JSON never enters UiNodeGraph's paint/layout path.
namespace Upp {
namespace GraphWorkspace {

constexpr int SHAPE_COUNT = 8;
extern const UiGraphNodeShape shapes[SHAPE_COUNT];
extern const char* const shape_names[SHAPE_COUNT];
extern const char* const region_names[8];
extern const char* const kind_names[8];

// Null/-1 means theme inherited. Layout and appearance detach independently.
struct Appearance {
    Color face[4] = { Null, Null, Null, Null };
    Color frame[4] = { Null, Null, Null, Null };
    Color ink[4] = { Null, Null, Null, Null };
    Color header[4] = { Null, Null, Null, Null };
    int role = 0;
    int radius = -1, frame_width = -1, header_band = -1;
    int shadow = -1, shadow_y = -1, shadow_alpha = -1;
    Color shadow_color = Null;
    void Apply(UiGraphNodeStyle& style) const;
    void Apply(UiGraphNode& node) const;
};

struct Family {
    String name = "Untitled";
    UiGraphNodeTemplate base_layout;
    Appearance base_style;
    UiGraphNodeTemplate shape_layout[SHAPE_COUNT];
    Appearance shape_style[SHAPE_COUNT];
    bool layout_override[SHAPE_COUNT] = {};
    bool style_override[SHAPE_COUNT] = {};

    const UiGraphNodeTemplate& Layout(int shape) const;
    const Appearance& Style(int shape) const;
    void DetachLayout(int shape);
    void DetachStyle(int shape);
};

struct Document {
    Family family;
    // Preview-only data and camera do not become production template structure.
    String title = "Harbour / convert EXR";
    String subtitle = "Image processing", description = "Selected take";
    ValueMap data;
    int shape = 0;
    bool edit_base = true;
    int inputs = 1, outputs = 1, connector = 1;
    Size size = Size(320, 210); // logical 96-DPI units
    double zoom = 1;
    Pointf pan = Pointf(0, 0);
    String file;
    bool dirty = false;
    int revision = 0; // session-only stale-drag protection
};

Document MakeDocument(UiGraphNodeTemplateKind kind = UiGraphNodeTemplateKind::Media);
bool Validate(const Document& document, String& error);
Value Encode(const Document& document);
bool Decode(const Value& value, Document& output, String& error);
bool Load(const String& text, Document& output, String& error);
bool SaveAtomic(const String& path, const String& text, String& error);
String GenerateCpp(const Document& document, String& error);

// One transaction shared by palette, region diagrams and structure table.
// shape == -1 edits Base; inherited shapes are explicitly read-only.
bool PlaceComponent(Document& document, int shape, int expected_revision,
                    const UiGraphNodeSlotRule& component, bool move,
                    UiGraphNodeSlotRegion region, const String& before,
                    String& error);
bool RemoveComponent(Document& document, int shape, const String& id, String& error);
UiGraphNodeSlotRule NewComponent(UiGraphNodeComponentKind kind, const UiGraphNodeTemplate& layout);
String QuoteCpp(const String& text);

} // namespace GraphWorkspace
} // namespace Upp
#endif
