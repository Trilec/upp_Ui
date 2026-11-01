#include "PropertyWorkingRangeEditors.h"

namespace Upp {

namespace {
struct PropertyWorkingRangeAutoRegister {
    PropertyWorkingRangeAutoRegister()
    {
        RegisterUiPropertyInspectorWorkingRangeEditors(UiPropertyInspectorFactory::Global());
    }
};

PropertyWorkingRangeAutoRegister s_property_working_range_auto_register;
}

}
