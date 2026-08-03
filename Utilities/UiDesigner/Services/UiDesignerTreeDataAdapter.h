#ifndef _Utilities_UiDesigner_Services_UiDesignerTreeDataAdapter_h_
#define _Utilities_UiDesigner_Services_UiDesignerTreeDataAdapter_h_

#include <Utilities/UiDesigner/Core/UiDesignerCore.h>

namespace Upp {

struct UiDesignerTreeDataRow : Moveable<UiDesignerTreeDataRow> {
    ValueArray path;
    String text;
    bool enabled = true;
    int depth = 0;
};

class UiDesignerTreeDataAdapter {
public:
    static ValueMap Root(const UiDesignerNode& node);
    static ValueArray Children(const ValueMap& item);
    static Value Token(const ValueArray& path);
    static ValueArray Path(const Value& value);
    static ValueMap ItemAt(const ValueMap& root, const ValueArray& path);
    static Vector<UiDesignerTreeDataRow> Rows(const ValueMap& root);

    static bool SetItem(ValueMap& root, const ValueArray& path,
                        const ValueMap& item);
    static bool AppendChild(ValueMap& root, const ValueArray& parent_path,
                            const ValueMap& child);
    static bool RemoveItem(ValueMap& root, const ValueArray& path);
    static bool MoveItem(ValueMap& root, const ValueArray& path, int delta);
};

}

#endif
