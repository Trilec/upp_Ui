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
    - Central catalog of Designer control specifications.

    Intent
    - Keep one registered specification per Designer type so toolbox metadata,
      defaults, declared capabilities, adapter creation hooks, and codegen/theme
      integration can converge on a single record instead of being scattered
      across loosely related modules.

    Thread context
    - GUI thread only.

    Usage
    - Register built-in control specs during startup. Query the registry from
      toolbox, preview, inspector, serialization, and codegen paths. The model
      remains the source of truth; the registry describes how each type plugs
      into those projections.

    Changelog
    - 2026-06: normalized the top-level header documentation.
    - 2026-07: introduced DesignerControlSpec as the architectural target for
      non-fragmented Designer control integration, with compatibility aliases
      kept in place while callers migrate off DesignerType wording.
*/

#include "DesignerDefaults.h"
#include "DesignerModel.h"

namespace Upp {

class Ctrl;
class DesignerAdapter;

struct DesignerControlCapabilities : Moveable<DesignerControlCapabilities> {
	bool is_layout = false;
	bool is_visible_control = true;
	bool is_container = false;
	bool is_page_container = false;
	bool is_slot_node = false;
	bool is_pane_container = false;
	bool can_have_children = false;
	bool supports_children = false;
	bool is_headless_node = false;
	bool supports_inspector = true;
	bool supports_preview = true;
	bool supports_codegen = true;
	bool supports_appearance_overrides = false;
	bool supports_theme_export = false;
	bool requires_default_child_slots = false;
};

enum class DesignerThemeCapability {
	None,
	RoleOnly,
	CommonSurface,
	PartAware
};

enum class DesignerPropertyDomain {
	DesignerOnly,
	Content,
	Layout,
	Behaviour,
	ThemeStyle
};

enum class DesignerDefaultChildSlotSet {
	None,
	SplitterTwoPanes,
	QuadSplitterFourPanes,
	PageContainerThreePages,
	AccordionThreeSections
};

enum class DesignerLayoutChildEmissionStrategy {
	None,
	DirectChild,
	BoxLayoutItem,
	GridItem,
	SplitterPane,
	PageContainerPage,
	AccordionSection,
	ScrollContent,
	GroupPanelContent,
	PanelContent,
	SlotPassthrough
};

struct DesignerCodeGenHooks : Moveable<DesignerCodeGenHooks> {
	Function<void(String& out, const DesignerNode& node)> emit_declaration;
	Function<void(String& out, const DesignerNode& node)> emit_setup;
	Function<void(String& out, const DesignerNode& node)> emit_layout;
	Function<void(String& out, const DesignerNode& node)> emit_post_build;
};

struct DesignerThemeSchema : DeepCopyOption<DesignerThemeSchema> {
	Vector<String> common_fields;
	Vector<String> part_fields;
	VectorMap<String, String> unsupported_fields;

	DesignerThemeSchema() = default;
	DesignerThemeSchema(const DesignerThemeSchema& s, int)
	{
		common_fields <<= s.common_fields;
		part_fields <<= s.part_fields;
		unsupported_fields.Clear();
		for(int i = 0; i < s.unsupported_fields.GetCount(); i++)
			unsupported_fields.Add(s.unsupported_fields.GetKey(i), s.unsupported_fields[i]);
	}
	DesignerThemeSchema(const DesignerThemeSchema& s)
	{
		common_fields <<= s.common_fields;
		part_fields <<= s.part_fields;
		unsupported_fields.Clear();
		for(int i = 0; i < s.unsupported_fields.GetCount(); i++)
			unsupported_fields.Add(s.unsupported_fields.GetKey(i), s.unsupported_fields[i]);
	}
	void operator=(const DesignerThemeSchema& s)
	{
		common_fields <<= s.common_fields;
		part_fields <<= s.part_fields;
		unsupported_fields.Clear();
		for(int i = 0; i < s.unsupported_fields.GetCount(); i++)
			unsupported_fields.Add(s.unsupported_fields.GetKey(i), s.unsupported_fields[i]);
	}
};

struct DesignerControlSpec : DeepCopyOption<DesignerControlSpec> {
	String id;
	String display_name;
	String default_base_name;
	String toolbox_group;
	String runtime_cpp_type;
	Image icon;

	// Transitional mirrors kept for existing Designer callers while the
	// capabilities struct becomes the single long-term contract.
	bool is_container = false;
	bool can_have_children = false;

	DesignerControlCapabilities capabilities;
	DesignerThemeCapability theme_capability = DesignerThemeCapability::None;
	String theme_default_source;
	DesignerDefaultChildSlotSet default_child_slots = DesignerDefaultChildSlotSet::None;
	DesignerLayoutChildEmissionStrategy child_emission = DesignerLayoutChildEmissionStrategy::DirectChild;

	Size default_size = DesignerDefaultSize();
	Size min_size = DesignerMinSize();

	Function<void(DesignerNode&)> init_defaults;
	Function<bool(const DesignerNode& parent, const DesignerNode& child)> can_drop;
	Function<Ctrl*(const DesignerNode&, DesignerAdapter**)> create_adapter;

	DesignerCodeGenHooks codegen;
	DesignerThemeSchema theme_schema;

	DesignerControlSpec() = default;
	DesignerControlSpec(const DesignerControlSpec& s, int)
	    : id(s.id)
	    , display_name(s.display_name)
	    , default_base_name(s.default_base_name)
	    , toolbox_group(s.toolbox_group)
	    , runtime_cpp_type(s.runtime_cpp_type)
	    , icon(s.icon)
	    , is_container(s.is_container)
	    , can_have_children(s.can_have_children)
	    , capabilities(s.capabilities)
	    , theme_capability(s.theme_capability)
	    , theme_default_source(s.theme_default_source)
	    , default_child_slots(s.default_child_slots)
	    , child_emission(s.child_emission)
	    , default_size(s.default_size)
	    , min_size(s.min_size)
	    , init_defaults(s.init_defaults)
	    , can_drop(s.can_drop)
	    , create_adapter(s.create_adapter)
	    , codegen(s.codegen)
	    , theme_schema(s.theme_schema)
	{
	}
	DesignerControlSpec(const DesignerControlSpec& s)
	    : id(s.id)
	    , display_name(s.display_name)
	    , default_base_name(s.default_base_name)
	    , toolbox_group(s.toolbox_group)
	    , runtime_cpp_type(s.runtime_cpp_type)
	    , icon(s.icon)
	    , is_container(s.is_container)
	    , can_have_children(s.can_have_children)
	    , capabilities(s.capabilities)
	    , theme_capability(s.theme_capability)
	    , theme_default_source(s.theme_default_source)
	    , default_child_slots(s.default_child_slots)
	    , child_emission(s.child_emission)
	    , default_size(s.default_size)
	    , min_size(s.min_size)
	    , init_defaults(s.init_defaults)
	    , can_drop(s.can_drop)
	    , create_adapter(s.create_adapter)
	    , codegen(s.codegen)
	    , theme_schema(s.theme_schema)
	{
	}
	void operator=(const DesignerControlSpec& s)
	{
		id = s.id;
		display_name = s.display_name;
		default_base_name = s.default_base_name;
		toolbox_group = s.toolbox_group;
		runtime_cpp_type = s.runtime_cpp_type;
		icon = s.icon;
		is_container = s.is_container;
		can_have_children = s.can_have_children;
		capabilities = s.capabilities;
		theme_capability = s.theme_capability;
		theme_default_source = s.theme_default_source;
		default_child_slots = s.default_child_slots;
		child_emission = s.child_emission;
		default_size = s.default_size;
		min_size = s.min_size;
		init_defaults = s.init_defaults;
		can_drop = s.can_drop;
		create_adapter = s.create_adapter;
		codegen = s.codegen;
		theme_schema = s.theme_schema;
	}

	bool IsContainer() const      { return capabilities.is_container || is_container; }
	bool CanHaveChildren() const  { return capabilities.can_have_children || can_have_children; }
	bool SupportsChildren() const { return capabilities.supports_children || CanHaveChildren(); }
	bool IsLayout() const         { return capabilities.is_layout; }
	bool IsVisibleControl() const { return capabilities.is_visible_control; }
	bool IsPageContainer() const  { return capabilities.is_page_container; }
	bool IsSlotNode() const       { return capabilities.is_slot_node; }
	bool IsPaneContainer() const  { return capabilities.is_pane_container; }
	bool IsHeadlessNode() const   { return capabilities.is_headless_node; }
	bool SupportsAppearanceOverrides() const { return capabilities.supports_appearance_overrides; }
	bool SupportsThemeExport() const { return capabilities.supports_theme_export; }
	bool RequiresDefaultChildSlots() const { return capabilities.requires_default_child_slots; }
};

using DesignerType = DesignerControlSpec;

class DesignerRegistry {
public:
	void Register(const DesignerControlSpec& type);
	const DesignerControlSpec* FindSpec(const String& type_id) const;
	Vector<const DesignerControlSpec*> GetSpecs() const;
	Vector<String> GetToolboxGroups() const;
	Vector<const DesignerControlSpec*> GetToolboxSpecs(const String& group) const;
	bool CanDrop(const DesignerNode& parent, const DesignerNode& child) const;

	// Transitional compatibility API while the rest of Designer migrates to the
	// control-spec naming.
	const DesignerType* Find(const String& type_id) const { return FindSpec(type_id); }
	Vector<const DesignerType*> GetTypes() const { return GetSpecs(); }
	Vector<const DesignerType*> GetToolboxTypes(const String& group) const { return GetToolboxSpecs(group); }

private:
	ArrayMap<String, DesignerControlSpec> types_;
};

}
#endif
