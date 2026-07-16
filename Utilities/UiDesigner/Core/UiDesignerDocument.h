#ifndef _Utilities_UiDesigner_Core_UiDesignerDocument_h_
#define _Utilities_UiDesigner_Core_UiDesignerDocument_h_

#include "UiDesignerTypes.h"

namespace Upp {

struct UiDesignerNode {
    UiDesignerNodeId id = 0;
    UiDesignerNodeId parent = 0;
    String type;
    String name;
    dword flags = UiDesignerNodeNone;
    Vector<UiDesignerNodeId> children;
    ValueMap properties;
    Vector<UiDesignerActionBinding> actions;

    UiDesignerNode() {}
    UiDesignerNode(const UiDesignerNode& other)
        : id(other.id), parent(other.parent), type(other.type), name(other.name),
          flags(other.flags), properties(other.properties)
    {
        children.Append(clone(other.children));
        actions.Append(clone(other.actions));
    }

    Value GetProperty(const String& id, const Value& fallback = Value()) const;
    void SetProperty(const String& id, const Value& value);

    int FindAction(const String& event_id) const;
    const UiDesignerActionBinding* GetAction(const String& event_id) const;
    UiDesignerActionBinding* GetAction(const String& event_id);
    void SetAction(UiDesignerActionBinding binding);
    bool RemoveAction(const String& event_id);
};

class UiDesignerDocument {
public:
    typedef UiDesignerDocument CLASSNAME;

    UiDesignerDocument();

    void Clear();
    void NewDocument(Size virtual_size = Size(1020, 668),
                     UiDesignerNodeId root_id = 1);

    int GetCount() const { return nodes_.GetCount(); }
    const Array<UiDesignerNode>& GetNodes() const { return nodes_; }

    UiDesignerNode* Find(UiDesignerNodeId id);
    const UiDesignerNode* Find(UiDesignerNodeId id) const;
    UiDesignerNodeId GetRootId() const { return root_id_; }

    Size GetVirtualSize() const { return virtual_size_; }
    void SetVirtualSize(Size size);

    uint64 GetRevision() const { return revision_; }
    String GetDocumentId() const { return document_id_; }
    void SetDocumentId(const String& id) { document_id_ = id; }

    UiDesignerNodeId AddNode(const String& type, const String& name,
                             UiDesignerNodeId parent, dword flags,
                             int index = -1);
    UiDesignerNodeId AddNodeWithId(UiDesignerNodeId id,
                                   const String& type, const String& name,
                                   UiDesignerNodeId parent, dword flags,
                                   int index = -1);
    bool RemoveNode(UiDesignerNodeId id);
    bool MoveNode(UiDesignerNodeId id, UiDesignerNodeId new_parent, int new_index = -1);
    bool RenameNode(UiDesignerNodeId id, const String& name);
    bool SetProperty(UiDesignerNodeId id, const String& property, const Value& value,
                     UiDesignerChangeImpact impact);
    Value GetProperty(UiDesignerNodeId id, const String& property,
                      const Value& fallback = Value()) const;

    bool SetActionBinding(UiDesignerNodeId id, UiDesignerActionBinding binding);
    bool RemoveActionBinding(UiDesignerNodeId id, const String& event_id);
    const UiDesignerActionBinding* GetActionBinding(UiDesignerNodeId id,
                                                    const String& event_id) const;

    void BeginBatch(const String& reason);
    void CommitBatch();
    void CancelBatch();
    bool IsBatching() const { return batch_depth_ > 0; }

    void ReplaceFrom(const UiDesignerDocument& other, const String& reason,
                     bool notify = true);

    Event<const UiDesignerChangeSet&> WhenChanged;

private:
    int FindIndexById(UiDesignerNodeId id) const;
    void RemoveNodeRecursive(UiDesignerNodeId id, UiDesignerChangeSet& changes);
    void RemoveBindingsTargeting(const Index<UiDesignerNodeId>& removed,
                                 UiDesignerChangeSet& changes);
    void QueueChange(const UiDesignerChangeSet& changes);
    void EmitChange(UiDesignerChangeSet changes);

    Array<UiDesignerNode> nodes_;
    UiDesignerNodeId root_id_ = 0;
    UiDesignerNodeId next_id_ = 1;
    Size virtual_size_ = Size(1020, 668);
    uint64 revision_ = 0;
    uint64 transaction_sequence_ = 0;
    String document_id_;

    int batch_depth_ = 0;
    UiDesignerChangeSet pending_;
};

}

#endif
