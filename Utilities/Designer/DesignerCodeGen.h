#pragma once

#include "DesignerModel.h"
#include "DesignerRegistry.h"

namespace Upp {

String GenerateDesignerCode(const DesignerModel& model, const DesignerRegistry& registry,
                              const String& class_name = "GeneratedDesignerWindow",
                              bool emit_designer_appearance = false);

}
