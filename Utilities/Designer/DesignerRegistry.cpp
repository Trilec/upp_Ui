#include "DesignerRegistry.h"

// DesignerRegistry.cpp - runtime catalog of designer-visible node types.
// The registry keeps toolbox metadata and default initializers centralized so
// adding controls does not require special cases in preview or inspector code.

namespace Upp {

static int FindString(const Vector<String>& v, const String& s)
{
	for(int i = 0; i < v.GetCount(); i++)
		if(v[i] == s)
			return i;
	return -1;
}

void DesignerRegistry::Register(const DesignerControlSpec& type)
{
	DesignerControlSpec spec = clone(type);
	if(spec.default_base_name.IsEmpty())
		spec.default_base_name = spec.display_name;
	if(spec.runtime_cpp_type.IsEmpty())
		spec.runtime_cpp_type = spec.id;
	spec.capabilities.is_container = spec.capabilities.is_container || spec.is_container;
	spec.capabilities.can_have_children = spec.capabilities.can_have_children || spec.can_have_children;
	spec.capabilities.supports_children = spec.capabilities.supports_children || spec.capabilities.can_have_children;
	spec.is_container = spec.capabilities.is_container;
	spec.can_have_children = spec.capabilities.can_have_children;
	types_.GetAdd(type.id) = pick(spec);
}

const DesignerControlSpec* DesignerRegistry::FindSpec(const String& type_id) const
{
	int q = types_.Find(type_id);
	return q >= 0 ? &types_[q] : nullptr;
}

Vector<const DesignerControlSpec*> DesignerRegistry::GetSpecs() const
{
	Vector<const DesignerControlSpec*> out;
	for(int i = 0; i < types_.GetCount(); i++)
		out.Add(&types_[i]);
	return out;
}

Vector<String> DesignerRegistry::GetToolboxGroups() const
{
	Vector<String> groups;
	for(int i = 0; i < types_.GetCount(); i++) {
		const String& group = types_[i].toolbox_group;
		if(!group.IsEmpty() && FindString(groups, group) < 0)
			groups.Add(group);
	}
	return groups;
}

Vector<const DesignerControlSpec*> DesignerRegistry::GetToolboxSpecs(const String& group) const
{
	Vector<const DesignerControlSpec*> out;
	for(int i = 0; i < types_.GetCount(); i++)
		if(types_[i].toolbox_group == group)
			out.Add(&types_[i]);
	return out;
}

bool DesignerRegistry::CanDrop(const DesignerNode& parent, const DesignerNode& child) const
{
	const DesignerControlSpec* parent_type = FindSpec(parent.type_id);
	const DesignerControlSpec* child_type = FindSpec(child.type_id);
	if(!parent_type || !child_type)
		return false;
	if(!parent_type->CanHaveChildren())
		return false;
	if(parent_type->can_drop)
		return parent_type->can_drop(parent, child);
	return true;
}

}
