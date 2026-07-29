#ifndef _UiDesigner_UiDesignerColorPickerContract_h_
#define _UiDesigner_UiDesignerColorPickerContract_h_
#include <Ui/UiColorPicker.h>
namespace Upp {
struct UiDesignerColorPickerEnumChoice { const char *id; const char *label; };
inline const UiDesignerColorPickerEnumChoice *UiDesignerColorPickerPageModes(int& n) { static const UiDesignerColorPickerEnumChoice v[] = {{"color","Color"},{"palettes","Palettes"},{"generator","Generator"}}; n=3; return v; }
inline const UiDesignerColorPickerEnumChoice *UiDesignerColorPickerChannelModes(int& n) { static const UiDesignerColorPickerEnumChoice v[] = {{"rgb_float","RGB float"},{"rgb_integer","RGB integer"},{"hsv","HSV"},{"hsl","HSL"},{"tmi","TMI"},{"cmyk","CMYK"},{"lab","Lab"}}; n=7; return v; }
inline const UiDesignerColorPickerEnumChoice *UiDesignerColorPickerSpectrumModes(int& n) { static const UiDesignerColorPickerEnumChoice v[] = {{"hsv_rectangle","HSV rectangle"},{"hue_strip","Hue strip"},{"rgb_spectrum","RGB spectrum"},{"hsv_wheel","HSV wheel"}}; n=4; return v; }
inline const UiDesignerColorPickerEnumChoice *UiDesignerColorPickerHarmonyModes(int& n) { static const UiDesignerColorPickerEnumChoice v[] = {{"custom","Custom"},{"analogous","Analogous"},{"complementary","Complementary"},{"split_complementary","Split complementary"},{"triad","Triad"},{"square","Square"},{"compound","Compound"},{"shades","Shades"},{"monochromatic","Monochromatic"},{"image_extract","Image extract"}}; n=10; return v; }
template <class T> inline bool UiDesignerColorPickerChoiceId(const UiDesignerColorPickerEnumChoice *v, int n, const String& id, T& out) { for(int i=0;i<n;i++) if(id==v[i].id) { out=(T)i; return true; } return false; }
inline bool UiDesignerColorPickerChoiceId(const String& s, UiColorPicker::PageMode& v) { int n; return UiDesignerColorPickerChoiceId(UiDesignerColorPickerPageModes(n),n,s,v); }
inline bool UiDesignerColorPickerChoiceId(const String& s, UiColorPicker::ChannelMode& v) { int n; return UiDesignerColorPickerChoiceId(UiDesignerColorPickerChannelModes(n),n,s,v); }
inline bool UiDesignerColorPickerChoiceId(const String& s, UiColorPicker::SpectrumMode& v) { int n; return UiDesignerColorPickerChoiceId(UiDesignerColorPickerSpectrumModes(n),n,s,v); }
inline bool UiDesignerColorPickerChoiceId(const String& s, UiColorPicker::HarmonyMode& v) { int n; return UiDesignerColorPickerChoiceId(UiDesignerColorPickerHarmonyModes(n),n,s,v); }
}
#endif
