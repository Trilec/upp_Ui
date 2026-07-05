#ifndef _Utilities_Designer_controls_DesignerControlFamilyShared_h_
#define _Utilities_Designer_controls_DesignerControlFamilyShared_h_

#include "../DesignerAdapter.h"

namespace Upp {

Color DesignerLayoutFace();
Color DesignerLayoutFrame();
Color DesignerDebugRed();
Color DesignerPanelFace();
Color DesignerPanelFrame();
Color DesignerControlFace();
Color DesignerControlFrame();

Image MakeDesignerTypeIcon(const String& id);

template <class T>
inline Ctrl* MakeDesignerAdapterCtrl(DesignerAdapter **adapter)
{
	T *p = new T;
	if(adapter)
		*adapter = p;
	return p;
}

template <class T>
inline void SetDesignerAdapterFactory(DesignerType& t)
{
	t.create_adapter = [](const DesignerNode&, DesignerAdapter **adapter) -> Ctrl* {
		return MakeDesignerAdapterCtrl<T>(adapter);
	};
}

DesignerType MakeControlType(const String& id, const String& name, Size size);
DesignerType MakeCompositeType(const String& id, const String& name, Size size);
DesignerType MakePageContainerType(const String& id, const String& name, Size size);
DesignerType MakePanelControlType(const String& id, const String& name, Size size);
DesignerType MakeGroupPanelType();
DesignerType MakeAccordionType();
DesignerType MakeGenericType();

inline void SetDesignerThemeSchema(DesignerType& t,
                                   std::initializer_list<const char*> common_fields,
                                   std::initializer_list<const char*> part_fields = {},
                                   std::initializer_list<std::pair<const char*, const char*>> unsupported_fields = {})
{
	t.theme_schema.common_fields.Clear();
	t.theme_schema.part_fields.Clear();
	t.theme_schema.unsupported_fields.Clear();
	for(const char *field : common_fields)
		t.theme_schema.common_fields.Add(field);
	for(const char *field : part_fields)
		t.theme_schema.part_fields.Add(field);
	for(const auto& field : unsupported_fields)
		t.theme_schema.unsupported_fields.Add(field.first, field.second);
}

}

#endif
