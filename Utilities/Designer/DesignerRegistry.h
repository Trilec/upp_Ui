#ifndef _Utilities_DesignerRegistry_h_
#define _Utilities_DesignerRegistry_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    DesignerRegistry
    ================

    Purpose
    - Public header for the DesignerRegistry component.
     The registry is the small catalog that tells the app which node types exist,
     how they appear in the toolbox, whether they can contain children, and how a
     new node should be initialized. New controls should start here plus an adapter.

    Intent
    - Define the runtime API, style contract, and integration points used by the rest of the Ui package.

    Thread context
    - GUI thread only.

    Usage
    - Include this header where the component is used or extended. Keep implementation details in the matching .cpp when present.

    Changelog
    - 2026-06: normalized the top-level header documentation.
*/

#include "DesignerDefaults.h"
#include "DesignerModel.h"

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
	Size default_size = DesignerDefaultSize();
	Size min_size = DesignerMinSize();

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
	Vector<const DesignerType*> GetTypes() const;
	Vector<String> GetToolboxGroups() const;
	Vector<const DesignerType*> GetToolboxTypes(const String& group) const;
	bool CanDrop(const DesignerNode& parent, const DesignerNode& child) const;

private:
	VectorMap<String, DesignerType> types_;
};

}
#endif
