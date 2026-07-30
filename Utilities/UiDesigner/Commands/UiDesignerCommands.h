#ifndef _Utilities_UiDesigner_Commands_UiDesignerCommands_h_
#define _Utilities_UiDesigner_Commands_UiDesignerCommands_h_

#include <Utilities/UiDesigner/Core/UiDesignerCore.h>

namespace Upp {

struct UiDesignerHistoryEntry {
    String label;
    String before_json;
    String after_json;
    UiDesignerChangeSet changes;
};

class UiDesignerCommandService {
public:
    typedef UiDesignerCommandService CLASSNAME;

    explicit UiDesignerCommandService(UiDesignerDocument& document);

    bool SetProperty(UiDesignerNodeId node, const String& property,
                     const Value& value, UiDesignerChangeImpact impact,
                     const String& label = String());
    bool SetProperty(const Vector<UiDesignerNodeId>& nodes, const String& property,
                     const Value& value, UiDesignerChangeImpact impact,
                     const String& label = String());
    bool SetThemeOverride(UiDesignerNodeId node, const String& property,
                          const Value& value, UiDesignerChangeImpact impact,
                          const String& label = String());
    bool RemoveThemeOverride(UiDesignerNodeId node, const String& property,
                             UiDesignerChangeImpact impact,
                             const String& label = String());
    bool ClearThemeOverrides(UiDesignerNodeId node,
                             UiDesignerChangeImpact impact,
                             const String& label = String());
    bool SetVirtualSize(Size size, const String& label = "Set canvas size");

    UiDesignerNodeId AddNode(const String& type, const String& name,
                             UiDesignerNodeId parent, dword flags,
                             const ValueMap& defaults = ValueMap(),
                             const String& label = String());
    UiDesignerNodeId AddNodeAt(const String& type, const String& name,
                               UiDesignerNodeId parent, int index, dword flags,
                               const ValueMap& defaults = ValueMap(),
                               const String& label = String());
    bool RemoveNode(UiDesignerNodeId node, const String& label = String());
    bool RemoveNodes(const Vector<UiDesignerNodeId>& nodes,
                     const String& label = "Delete selection");
    bool MoveNode(UiDesignerNodeId node, UiDesignerNodeId parent, int index = -1,
                  const String& label = String());
    bool MoveNodes(const Vector<UiDesignerNodeId>& nodes,
                   UiDesignerNodeId parent, int index = -1,
                   const String& label = "Move selection");
    bool MoveNodesConfigured(
        const Vector<UiDesignerNodeId>& nodes,
        UiDesignerNodeId parent, int index,
        const VectorMap<UiDesignerNodeId, ValueMap>& property_updates,
        const String& label = "Move selection");
    bool RenameNode(UiDesignerNodeId node, const String& name,
                    const String& label = String());
    UiDesignerNodeId AddTabPage(UiDesignerNodeId tab, const String& title);
    bool RemoveTabPage(UiDesignerNodeId page, const String& label = String());
    bool RenameTabPage(UiDesignerNodeId page, const String& title);
    bool MoveTabPage(UiDesignerNodeId page, int index);
    bool SetTabPageEnabled(UiDesignerNodeId page, bool enabled);
    bool SetActiveTabPage(UiDesignerNodeId tab, UiDesignerNodeId page);

    UiDesignerNodeId AddAccordionSection(UiDesignerNodeId accordion,
                                         const String& title);
    bool RemoveAccordionSection(UiDesignerNodeId section);
    bool RenameAccordionSection(UiDesignerNodeId section, const String& title);
    bool SetAccordionSectionProperty(UiDesignerNodeId section,
                                     const String& property, const Value& value);

    bool SetActionBinding(UiDesignerNodeId node,
                          const UiDesignerActionBinding& binding,
                          const String& label = String());
    bool RemoveActionBinding(UiDesignerNodeId node, const String& event_id,
                             const String& label = String());

    bool ReplaceDocument(const UiDesignerDocument& document,
                         const String& label = "Replace document");

    bool Undo();
    bool Redo();
    bool CanUndo() const { return position_ > 0; }
    bool CanRedo() const { return position_ < history_.GetCount(); }
    String GetUndoLabel() const;
    String GetRedoLabel() const;

    int GetHistoryPosition() const { return position_; }
    int GetSavedPosition() const { return saved_position_; }
    void MarkSaved() { saved_position_ = position_; }
    bool IsDirty() const { return position_ != saved_position_; }
    void ClearHistory();

    const String& GetLastError() const { return last_error_; }

    Event<> WhenHistoryChanged;

private:
    bool ApplyAtomic(const String& label,
                     const Function<bool(UiDesignerChangeSet&)>& operation,
                     UiDesignerChangeSet *out_changes = nullptr);
    bool RestoreSnapshot(const String& json, const String& reason);
    void TruncateRedo();

    UiDesignerDocument& document_;
    Array<UiDesignerHistoryEntry> history_;
    int position_ = 0;
    int saved_position_ = 0;
    bool replaying_ = false;
    String last_error_;
};

}

#endif
