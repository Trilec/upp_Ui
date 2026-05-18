#pragma once

#include "DesignerModel.h"

namespace Upp {

struct DesignerType : Moveable<DesignerType> {
	String id;
	String display_name;
	String toolbox_group;
	Image icon;
	bool is_container = false;
	bool can_have_children = false;
	Size default_size = Size(120, 32);
	Size min_size = Size(24, 20);

	Function<void(DesignerNode&)> init_defaults;
	Function<bool(const DesignerNode& parent, const DesignerNode& child)> can_drop;
};

class DesignerRegistry {
public:
	void Register(const DesignerType& type);
	const DesignerType* Find(const String& type_id) const;
	Vector<String> GetToolboxGroups() const;
	Vector<const DesignerType*> GetToolboxTypes(const String& group) const;
	bool CanDrop(const DesignerNode& parent, const DesignerNode& child) const;

private:
	VectorMap<String, DesignerType> types_;
};

}
