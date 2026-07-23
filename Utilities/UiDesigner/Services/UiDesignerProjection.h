#ifndef _Utilities_UiDesigner_Services_UiDesignerProjection_h_
#define _Utilities_UiDesigner_Services_UiDesignerProjection_h_

#include <Utilities/UiDesigner/Core/UiDesignerCore.h>
#include <Utilities/UiDesigner/Catalog/UiDesignerCatalog.h>

namespace Upp {

class UiDesignerProjectionSink {
public:
    virtual ~UiDesignerProjectionSink() {}

    virtual void Bind(const UiDesignerDocument *document,
                      const UiDesignerCatalog *catalog,
                      const UiDesignerTransientOverlay *overlay,
                      const UiDesignerSelection *selection) = 0;
    virtual void SetSelection(const UiDesignerSelection *selection) = 0;
    virtual void RebuildDocument() = 0;
    virtual void ApplyChangeSet(const UiDesignerChangeSet& changes) = 0;
    virtual void ApplyTransient(UiDesignerNodeId node,
                                UiDesignerTransientValueKind kind,
                                const String& property,
                                const Value& value) = 0;
};

}

#endif
