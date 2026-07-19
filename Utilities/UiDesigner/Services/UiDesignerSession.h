#ifndef _Utilities_UiDesigner_Services_UiDesignerSession_h_
#define _Utilities_UiDesigner_Services_UiDesignerSession_h_

#include <Utilities/UiDesigner/Commands/UiDesignerCommands.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>
#include <Utilities/UiDesigner/CodeGen/UiDesignerCodeGen.h>
#include <Utilities/UiDesigner/ThemeCore/UiDesignerTheme.h>
#include "UiDesignerProjection.h"
#include "UiDesignerDrop.h"

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
    UiDesignerDropService& Drops() {
        drops_.Bind(document_, catalog_, commands_);
        return drops_;
    }

    UiDesignerSessionState& State() { return state_; }
    const UiDesignerSessionState& State() const { return state_; }

    UiDesignerTransientOverlay& PreviewOverlay() { return overlay_; }
    const UiDesignerTransientOverlay& PreviewOverlay() const { return overlay_; }
    UiDesignerThemeDocument& Theme() { return theme_; }
    const UiDesignerThemeDocument& Theme() const { return theme_; }

    PropertyEditorModel& InspectorModel() { return inspector_model_; }
    const PropertyEditorModel& InspectorModel() const { return inspector_model_; }
    PropertyEditorModel& BehaviorModel() { return behavior_model_; }
    const PropertyEditorModel& BehaviorModel() const { return behavior_model_; }
    PropertyEditorModel& ThemeModel() {
        theme_.BuildPropertyModel(theme_model_);
        return theme_model_;
    }
    const PropertyEditorModel& ThemeModel() const { return theme_model_; }

    void AttachProjection(UiDesignerProjectionSink *projection);
    UiDesignerProjectionSink* GetProjection() const { return projection_; }

    void NewDocument(const String& preset = "blank");
    bool Load(const String& path, String& error);
    bool Save(const String& path, String& error);
    bool Export(const String& folder, const String& class_name, String& error);

    UiDesignerDropPlan PlanAddControl(
        const String& type_id, UiDesignerNodeId target = 0,
        Point canvas_position = Point(0, 0),
        bool has_canvas_position = false, int index = -1) const;
    UiDesignerDropPlan PlanMoveSelection(
        UiDesignerNodeId target,
        Point canvas_position = Point(0, 0),
        bool has_canvas_position = false, int index = -1) const;
    bool ExecuteDrop(const UiDesignerDropPlan& plan,
                     UiDesignerNodeId *created, String& error);

    UiDesignerNodeId AddControl(const String& type_id,
                                UiDesignerNodeId parent = 0);
    bool RemoveSelection();
    bool MoveSelection(UiDesignerNodeId parent, int index = -1);
    bool SetVirtualSize(Size size);

    void Select(UiDesignerNodeId node, bool toggle = false);
    void ClearSelection();
    void RebuildInspector();
    void RebuildBehaviorModel();

    bool PreviewProperty(const String& property, const Value& value,
                         String& error);
    bool CommitProperty(const String& property, const Value& value,
                        String& error);
    bool ResetProperty(const String& property, String& error);
    void CancelPreview();

    const String& GetActiveBehaviorEvent() const {
        return active_behavior_event_;
    }
    void SetActiveBehaviorEvent(const String& event_id);
    bool CommitBehaviorField(const String& field, const Value& value,
                             String& error);
    bool RemoveActiveBehavior(String& error);
    const UiDesignerActionBinding* GetActiveBehavior() const;

    bool Undo();
    bool Redo();

    String GenerateCode(const String& class_name = "GeneratedUiWindow") const;
    String GenerateHeader(const String& class_name = "GeneratedUiWindow") const;

    Event<> WhenSelectionChanged;
    Event<> WhenInspectorChanged;
    Event<> WhenBehaviorChanged;
    Event<> WhenCodeChanged;
    Event<String> WhenStatus;

private:
    void WireEvents();
    void ApplyPresetBlank();
    void ApplyPresetThreePane();
    void ApplyPresetDialog();
    UiDesignerNodeId ResolveInsertParent() const;
    Value ResolvePropertyValue(const UiDesignerNode& node,
                               const UiDesignerPropertySpec& property) const;
    bool SelectionSupports(const String& property,
                           const UiDesignerPropertySpec **primary_spec = nullptr) const;
    void SyncInspectorValues(const UiDesignerChangeSet& changes);
    UiDesignerActionBinding MakeEditableBehavior() const;
    String DefaultHandlerName(const UiDesignerNode& node,
                              const String& event_id) const;

    UiDesignerDocument document_;
    UiDesignerCommandService commands_;
    UiDesignerCatalog catalog_;
    UiDesignerDropService drops_;
    UiDesignerSessionState state_;
    UiDesignerTransientOverlay overlay_;
    UiDesignerProjectionSink *projection_ = nullptr;

    PropertyEditorModel inspector_model_;
    PropertyEditorModel behavior_model_;
    UiDesignerThemeDocument theme_;
    PropertyEditorModel theme_model_;

    String current_path_;
    uint64 editor_generation_ = 0;
    String active_behavior_event_;
};

}

#endif
