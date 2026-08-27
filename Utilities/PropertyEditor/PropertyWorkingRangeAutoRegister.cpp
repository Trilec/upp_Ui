#include "PropertyWorkingRangeEditors.h"

namespace Upp {

namespace {
struct PropertyWorkingRangeAutoRegister {
    PropertyWorkingRangeAutoRegister()
    {
        RegisterPropertyEditorWorkingRangeEditors(PropertyEditorFactory::Global());
    }
};

PropertyWorkingRangeAutoRegister s_property_working_range_auto_register;
}

}
