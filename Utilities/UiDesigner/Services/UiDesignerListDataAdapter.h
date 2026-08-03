#ifndef _Utilities_UiDesigner_Services_UiDesignerListDataAdapter_h_
#define _Utilities_UiDesigner_Services_UiDesignerListDataAdapter_h_

#include <Utilities/UiDesigner/Core/UiDesignerCore.h>

namespace Upp {

struct UiDesignerListDataRow : Moveable<UiDesignerListDataRow> {
    int index = -1;
    String text;
    bool enabled = true;
};

class UiDesignerListDataAdapter {
public:
    static ValueMap Root(const UiDesignerNode& node);
    static ValueArray Items(const ValueMap& root);
    static Value Token(int index);
    static int Index(const Value& token);
    static ValueMap Item(const ValueMap& root, int index);
    static Vector<UiDesignerListDataRow> Rows(const ValueMap& root);
    static bool SetItem(ValueMap& root, int index, const ValueMap& item);
    static bool AppendItem(ValueMap& root, const ValueMap& item);
    static bool RemoveItem(ValueMap& root, int index);
    static bool MoveItem(ValueMap& root, int index, int delta);
};

}

#endif
