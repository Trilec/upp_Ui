// Shared built-in theme catalogue. Core-only: safe for headless document/codegen tools.
#ifndef _Ui_Themes_UiThemePresets_h_
#define _Ui_Themes_UiThemePresets_h_

#include <Core/Core.h>

namespace Upp {

// Persisted IDs are append-only. Order also defines the public catalogue order.
enum class UiThemePreset : byte {
    Minimal = 0, Pill = 1, Linear = 2, Solid = 3,
    Outline = 4, Compact = 5, Layered = 6, Olive = 7
};

inline constexpr const char* UI_THEME_PRESET_NAMES[] = {
    "Minimal", "Pill", "Linear", "Solid", "Outline", "Compact", "Layered", "Olive"
};

inline int UiThemePresetCount() { return __countof(UI_THEME_PRESET_NAMES); }

inline const char* UiThemePresetName(UiThemePreset preset)
{
    int index = int(preset);
    return index >= 0 && index < UiThemePresetCount()
         ? UI_THEME_PRESET_NAMES[index] : UI_THEME_PRESET_NAMES[0];
}

// Unknown names leave the caller's candidate unchanged.
inline bool UiParseThemePreset(const String& name, UiThemePreset& preset)
{
    for(int i = 0; i < UiThemePresetCount(); ++i)
        if(name == UI_THEME_PRESET_NAMES[i]) {
            preset = UiThemePreset(i);
            return true;
        }
    return false;
}

} // namespace Upp
#endif
