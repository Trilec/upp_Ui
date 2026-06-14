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

void DesignerRegistry::Register(const DesignerType& type)
{
	types_.GetAdd(type.id) = clone(type);
}

const DesignerType* DesignerRegistry::Find(const String& type_id) const
{
	int q = types_.Find(type_id);
	return q >= 0 ? &types_[q] : nullptr;
}

Vector<const DesignerType*> DesignerRegistry::GetTypes() const
{
	Vector<const DesignerType*> out;
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

Vector<const DesignerType*> DesignerRegistry::GetToolboxTypes(const String& group) const
{
	Vector<const DesignerType*> out;
	for(int i = 0; i < types_.GetCount(); i++)
		if(types_[i].toolbox_group == group)
			out.Add(&types_[i]);
	return out;
}

bool DesignerRegistry::CanDrop(const DesignerNode& parent, const DesignerNode& child) const
{
	const DesignerType* parent_type = Find(parent.type_id);
	const DesignerType* child_type = Find(child.type_id);
	if(!parent_type || !child_type)
		return false;
	if(!parent_type->can_have_children)
		return false;
	if(parent_type->can_drop)
		return parent_type->can_drop(parent, child);
	return true;
}

}
