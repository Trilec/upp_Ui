#ifndef _Utilities_UiDesigner_Services_UiDesignerSession_h_
#define _Utilities_UiDesigner_Services_UiDesignerSession_h_

#include <Utilities/UiDesigner/Commands/UiDesignerCommands.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.h>
#include <Utilities/UiDesigner/ThemeCore/UiDesignerTheme.h>
#include "UiDesignerProjection.h"

namespace Upp {

struct UiDesignerEditIntent {
    enum Phase { Begin, Preview, Commit, Cancel };

    Phase phase = Preview;
    Vector<UiDesignerNodeId> targets;
    String property;
    Value candidate;
    uint64 selection_revision = 0;
    uint64 document_revision = 0;
    uint64 editor_generation = 0;
};

class UiDesignerSession {
public:
    typedef UiDesignerSession CLASSNAME;

    UiDesignerSession();

    UiDesignerDocument& Document() { return document_; }
    const UiDesignerDocument& Document() const { return document_; }

    UiDesignerCommandService& Commands() { return commands_; }
    UiDesignerCatalog& Catalog() { return catalog_; }
    const UiDesignerCatalog& Catalog() const { return catalog_; }

    UiDesignerSessionState& State() { return state_; }
    const UiDesignerSessionState& State() const { return state_; }

    UiDesignerTransientOverlay& PreviewOverlay() { return overlay_; }
    const UiDesignerTransientOverlay& PreviewOverlay() const { return overlay_; }
    UiDesignerThemeDocument& Theme() { return theme_; }
    const UiDesignerThemeDocument& Theme() const { return theme_; }

    PropertyEditorModel& InspectorModel() { return inspector_model_; }
    const PropertyEditorModel& InspectorModel() const { return inspector_model_; }
    PropertyEditorModel& ThemeModel() { return theme_model_; }
    const PropertyEditorModel& ThemeModel() const { return theme_model_; }

    void AttachProjection(UiDesignerProjectionSink *projection);
    UiDesignerProjectionSink* GetProjection() const { return projection_; }

    void NewDocument(const String& preset = "blank");
    bool Load(const String& path, String& error);
    bool Save(const String& path, String& error);
    bool Export(const String& folder, const String& class_name, String& error);

    UiDesignerNodeId AddControl(const String& type_id,
                                UiDesignerNodeId parent = 0);
    bool RemoveSelection();
    bool MoveSelection(UiDesignerNodeId parent, int index = -1);
    bool SetVirtualSize(Size size);

    void Select(UiDesignerNodeId node, bool toggle = false);
    void ClearSelection();
    void RebuildInspector();

    bool PreviewProperty(const String& property, const Value& value,
                         String& error);
    bool CommitProperty(const String& property, const Value& value,
                        String& error);
    bool ResetProperty(const String& property, String& error);
    void CancelPreview();

    bool Undo();
    bool Redo();

    String GenerateCode(const String& class_name = "GeneratedUiWindow") const;
    String GenerateHeader(const String& class_name = "GeneratedUiWindow") const;

    Event<> WhenSelectionChanged;
    Event<> WhenInspectorChanged;
    Event<> WhenCodeChanged;
    Event<String> WhenStatus;

private:
    void WireEvents();
    void ApplyPresetBlank();
    void ApplyPresetThreePane();
    void ApplyPresetSettings();
    UiDesignerNodeId ResolveInsertParent() const;
    Value ResolvePropertyValue(const UiDesignerNode& node,
                               const UiDesignerPropertySpec& property) const;
    bool SelectionSupports(const String& property,
                           const UiDesignerPropertySpec **primary_spec = nullptr) const;

    UiDesignerDocument document_;
    UiDesignerCommandService commands_;
    UiDesignerCatalog catalog_;
    UiDesignerSessionState state_;
    UiDesignerTransientOverlay overlay_;
    UiDesignerProjectionSink *projection_ = nullptr;

    PropertyEditorModel inspector_model_;
    UiDesignerThemeDocument theme_;
    PropertyEditorModel theme_model_;

    String current_path_;
    uint64 editor_generation_ = 0;
};

}

#endif
