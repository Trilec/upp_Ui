#pragma once

#include "DesignerModel.h"
#include "DesignerRegistry.h"

namespace Upp {

String StoreDesignerModelJson(const DesignerModel& model);
bool LoadDesignerModelJson(DesignerModel& model, const DesignerRegistry& registry,
                           const String& json, String& error, Vector<String>* notes = nullptr);

}
