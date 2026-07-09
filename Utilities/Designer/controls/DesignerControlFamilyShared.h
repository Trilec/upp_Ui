#ifndef _Utilities_Designer_controls_DesignerControlFamilyShared_h_
#define _Utilities_Designer_controls_DesignerControlFamilyShared_h_

#include "../DesignerAdapter.h"

namespace Upp {

inline Value DesignerNodeProperty(const DesignerNode& n, const String& key, const Value& def)
{
	int q = n.properties.Find(key);
	return q >= 0 ? n.properties.GetValue(q) : def;
}

inline bool DesignerNodeHasProperty(const DesignerNode& n, const String& key)
{
	return n.properties.Find(key) >= 0;
}

Color DesignerLayoutFace();
Color DesignerLayoutFrame();
Color DesignerDebugRed();
Color DesignerPanelFace();
Color DesignerPanelFrame();
Color DesignerControlFace();
Color DesignerControlFrame();
void EmitDesignerLayoutChild(DesignerCodeGenContext& ctx, const DesignerNode& parent, const DesignerNode& child, int child_index);

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
                                   std::initializer_list<DesignerThemeSchema::DesignerThemeFieldSpec> fields,
                                   std::initializer_list<const char*> common_fields = {},
                                   std::initializer_list<const char*> part_fields = {},
                                   std::initializer_list<std::pair<const char*, const char*>> unsupported_fields = {})
{
	t.theme_schema.fields.Clear();
	t.theme_schema.common_fields.Clear();
	t.theme_schema.part_fields.Clear();
	t.theme_schema.unsupported_fields.Clear();
	for(const auto& field : fields)
		t.theme_schema.fields.Add(field);
	for(const char *field : common_fields)
		t.theme_schema.common_fields.Add(field);
	for(const char *field : part_fields)
		t.theme_schema.part_fields.Add(field);
	for(const auto& field : unsupported_fields)
		t.theme_schema.unsupported_fields.Add(field.first, field.second);
}

inline DesignerThemeSchema::DesignerThemeFieldSpec ThemeField(const char *property_id, const char *style_target,
                                                              bool preview_supported = true, bool exact_codegen_supported = true,
                                                              bool theme_export_supported = true, const char *unsupported_reason = "")
{
	DesignerThemeSchema::DesignerThemeFieldSpec f;
	f.property_id = property_id;
	f.style_target = style_target;
	f.preview_supported = preview_supported;
	f.exact_codegen_supported = exact_codegen_supported;
	f.theme_export_supported = theme_export_supported;
	f.unsupported_reason = unsupported_reason;
	f.domain = DesignerPropertyDomain::ThemeStyle;
	return f;
}

inline void SetDesignerThemeSchema(DesignerType& t,
                                   std::initializer_list<const char*> common_fields,
                                   std::initializer_list<const char*> part_fields = {},
                                   std::initializer_list<std::pair<const char*, const char*>> unsupported_fields = {})
{
	t.theme_schema.fields.Clear();
	t.theme_schema.common_fields.Clear();
	t.theme_schema.part_fields.Clear();
	t.theme_schema.unsupported_fields.Clear();
	for(const char *field : common_fields) {
		t.theme_schema.common_fields.Add(field);
		t.theme_schema.fields.Add(ThemeField(field, field));
	}
	for(const char *field : part_fields) {
		t.theme_schema.part_fields.Add(field);
		t.theme_schema.fields.Add(ThemeField(field, field));
	}
	for(const auto& field : unsupported_fields) {
		t.theme_schema.unsupported_fields.Add(field.first, field.second);
		t.theme_schema.fields.Add(ThemeField(field.first, field.first, false, false, false, field.second));
	}
}

}

#endif
