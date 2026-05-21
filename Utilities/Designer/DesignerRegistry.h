#pragma once

#include "DesignerModel.h"

// Ui Designer type registry.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// The registry is the small catalog that tells the app which node types exist,
// how they appear in the toolbox, whether they can contain children, and how a
// new node should be initialized. New controls should start here plus an adapter.

namespace Upp {

// Metadata for one designer-visible layout/control type.
// This is intentionally lightweight: behavior stays in adapters and commands,
// while this struct handles toolbox grouping, iconography, and defaults.
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

// Runtime catalog of designer-visible types.
// Register built-ins once at startup; use Find() when creating toolbox rows,
// preview adapters, inspector descriptors, or generated code.
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
