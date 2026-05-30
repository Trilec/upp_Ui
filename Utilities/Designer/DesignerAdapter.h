#pragma once

#include "DesignerRegistry.h"

// Ui Designer real-control adapter layer.
// Copyright (c) 2026 C Edwards (dodobar). MIT licensed, matching the Ui package.
//
// Adapters keep runtime controls as real Ui controls while adding only designer
// behavior: property descriptors, selection/drop/debug overlays, and sync from
// DesignerNode values. This keeps design-time behavior out of production controls.

namespace Upp {

// Visual state painted on top of real controls while they are inside the designer.
// It is transient view state only; do not serialize it into DesignerNode.
struct DesignerOverlayState {
	bool selected = false;
	bool hovered = false;
	bool drop_target = false;
	bool debug = false;
	Color debug_color = Color(220, 38, 38);
	int radius = 0;
};

// Inspector editor kind requested by an adapter property descriptor.
// The inspector maps these values to Ui composite rows so each adapter can expose
// properties without constructing inspector widgets itself.
enum class DesignerEditorKind {
	ReadOnly,
	Text,
	Int,
	Bool,
	Choice,
	Color,
	Slider
};

// Description of one exposed property/API connection.
// The property_id is the model key; api_call documents the runtime Ui method that
// the property represents and is also useful for codegen and future help text.
struct DesignerApiBinding : Moveable<DesignerApiBinding> {
	String property_id;
	String label;
	DesignerEditorKind editor = DesignerEditorKind::Text;
	String help;
	String api_call;
	String codegen_hint;
	String group;
	Value default_value;
	Value min_value;
	Value max_value;
	VectorMap<String, Value> choices;
	bool visible = true;
	bool enabled = true;
	String disabled_reason;
};

// Small helper used inside adapters to build consistent property descriptors.
// It also lets adapters hide or disable common properties when the real control
// cannot support them in the current state.
class DesignerApiBuilder {
public:
	DesignerApiBuilder(Vector<DesignerApiBinding>& out) : out(out) {}

	DesignerApiBinding& Add(const String& id, const String& label, DesignerEditorKind editor,
	                          const String& api_call, const String& help);
	DesignerApiBinding& AddChoice(const String& id, const String& label, const String& api_call,
	                                const String& help, std::initializer_list<std::pair<const char *, const char *>> choices);
	DesignerApiBinding& AddInt(const String& id, const String& label, DesignerEditorKind editor,
	                             const String& api_call, const String& help, int min_value, int max_value);
	void Disable(const String& id, const String& reason);
	void Hide(const String& id);

private:
	DesignerApiBinding* Find(const String& id);
	Vector<DesignerApiBinding>& out;
};

// Common interface implemented by every designer-wrapped real control.
// SyncFromNode pushes model state into the real control; DescribeApi reports the
// editable surface for the inspector and generated-code documentation.
class DesignerAdapter {
public:
	virtual ~DesignerAdapter() {}

	virtual Ctrl& GetCtrl() = 0;
	virtual const Ctrl& GetCtrl() const = 0;
	virtual DesignerNodeId GetNodeId() const = 0;
	virtual String GetTypeId() const = 0;
	virtual void SyncFromNode(const DesignerNode& node) = 0;
	virtual void SetOverlayState(const DesignerOverlayState& state) = 0;
	virtual const DesignerOverlayState& GetOverlayState() const = 0;
	virtual void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const = 0;
};

// Panel-like designer surface backed by UiPanel.
// Used for real panel containers plus internal pane/page slots where the model
// needs a selectable drop target but not a custom runtime control.
class DesignerPanelAdapter : public UiPanel, public DesignerAdapter {
public:
	typedef DesignerPanelAdapter CLASSNAME;

	DesignerPanelAdapter();

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return type_id_; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	String type_id_ = "UiPanel";
	DesignerOverlayState overlay_;
};

class DesignerGroupPanelAdapter : public UiGroupPanel, public DesignerAdapter {
public:
	typedef DesignerGroupPanelAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiGroupPanel"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiLabel.
// Exposes label text, alignment, face/frame toggles, radius, and sizing while
// keeping the rendered result close to the actual runtime label.
class DesignerLabelAdapter : public UiLabel, public DesignerAdapter {
public:
	typedef DesignerLabelAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiLabel"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiTitleCard.
// Used to test richer control-specific properties such as title, subtitle, line
// visibility, and typography without turning the designer into a style hack.
class DesignerTitleCardAdapter : public UiTitleCard, public DesignerAdapter {
public:
	typedef DesignerTitleCardAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiTitleCard"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiSlider.
// Keeps slider behavior real while adding optional face/frame/radius preview so
// the designer can test sizing and appearance inside layouts.
class DesignerSliderAdapter : public UiSlider, public DesignerAdapter {
public:
	typedef DesignerSliderAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiSlider"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	Color face_;
	Color frame_;
	bool face_enabled_ = true;
	bool frame_enabled_ = true;
	int radius_ = 0;
	DesignerOverlayState overlay_;
};


// Designer wrapper for stable Ui composite controls.
// The wrapper owns the real composite as a child so design-time overlay behavior
// stays outside the reusable composite implementation.
class DesignerCompositeAdapter : public ParentCtrl, public DesignerAdapter {
public:
	typedef DesignerCompositeAdapter CLASSNAME;

	DesignerCompositeAdapter();

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return type_id_; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Layout() override;
	Size GetMinSize() const override;
	void Paint(Draw& w) override;
	void PaintTopOverlay(Draw& w) const;

private:
	class OverlayCtrl : public Ctrl {
	public:
		DesignerCompositeAdapter *owner = nullptr;
		void Paint(Draw& w) override { if(owner) owner->PaintTopOverlay(w); }
	};

	DesignerNodeId node_id_ = Designer_NULL;
	String type_id_;
	One<Ctrl> inner_;
	OverlayCtrl overlay_ctrl_;
	DesignerOverlayState overlay_;
};
// Designer wrapper for UiButton.
// Provides a simple command-control example for layout testing and generated code.
class DesignerButtonAdapter : public UiButton, public DesignerAdapter {
public:
	typedef DesignerButtonAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiButton"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};


// Designer wrapper for UiToolButton.
// Tool buttons are button-family controls for compact icon command surfaces.
class DesignerToolButtonAdapter : public UiToolButton, public DesignerAdapter {
public:
	typedef DesignerToolButtonAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiToolButton"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiLineEdit.
// Used to validate field sizing, text editing appearance, and generated edit code.
class DesignerLineEditAdapter : public UiLineEdit, public DesignerAdapter {
public:
	typedef DesignerLineEditAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiLineEdit"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiIntEdit.
// Exposes numeric field behavior while sharing the same edit appearance path.
class DesignerIntEditAdapter : public UiIntEdit, public DesignerAdapter {
public:
	typedef DesignerIntEditAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiIntEdit"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiFloatEdit.
// Provides floating-point field coverage for forms and grid/box sizing tests.
class DesignerFloatEditAdapter : public UiFloatEdit, public DesignerAdapter {
public:
	typedef DesignerFloatEditAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiFloatEdit"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiToggle.
// Keeps toggle state and theme rendering visible in generated layouts.
class DesignerToggleAdapter : public UiToggle, public DesignerAdapter {
public:
	typedef DesignerToggleAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiToggle"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiDropdown.
// Exercises internal-model dropdown behavior and selection-property syncing.
class DesignerDropdownAdapter : public UiDropdown, public DesignerAdapter {
public:
	typedef DesignerDropdownAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiDropdown"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiCheckBox.
// Covers boolean and tri-state form rows while preserving real checkbox input
// and theme drawing.
class DesignerCheckBoxAdapter : public UiCheckBox, public DesignerAdapter {
public:
	typedef DesignerCheckBoxAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiCheckBox"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiBreadcrumbs.
// Provides a compact navigation/path control for layout and theme testing.
class DesignerBreadcrumbsAdapter : public UiBreadcrumbs, public DesignerAdapter {
public:
	typedef DesignerBreadcrumbsAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiBreadcrumbs"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiTab.
// Uses small placeholder pages so tab placement, visual style, icons, close
// buttons, and drag handles can be assessed in real layouts.
class DesignerTabAdapter : public UiTab, public DesignerAdapter {
public:
	typedef DesignerTabAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiTab"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiStack.
// Page slots are explicit child nodes, so the hierarchy and drag/drop model can
// target each stack page while the real stack shows only the selected page.
class DesignerStackAdapter : public UiStack, public DesignerAdapter {
public:
	typedef DesignerStackAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiStack"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiTable.
// Seeds a small internal model so row/column/header behavior is visible without
// external data wiring.
class DesignerTableAdapter : public UiTable, public DesignerAdapter {
public:
	typedef DesignerTableAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiTable"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiTree.
// Seeds a compact hierarchy to test tree sizing, indentation, connector lines,
// row metadata, and themed selection.
class DesignerTreeAdapter : public UiTree, public DesignerAdapter {
public:
	typedef DesignerTreeAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiTree"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	UiTreeModel preview_model_;
	DesignerOverlayState overlay_;
};


// Designer wrapper for UiAccordion.
// Section children stay explicit model nodes so hierarchy, drag/drop, and codegen
// all see the same structure.
class DesignerAccordionAdapter : public UiAccordion, public DesignerAdapter {
public:
	typedef DesignerAccordionAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiAccordion"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiScrollPanel.
// Used as a container node where content size reporting and scrolling behavior
// need to remain visible to the designer app.
class DesignerScrollPanelAdapter : public UiScrollPanel, public DesignerAdapter {
public:
	typedef DesignerScrollPanelAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiScrollPanel"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiBoxLayout.
// It is both a real layout engine and a design-time drop target; debug rendering
// should come from UiBoxLayout itself, with the adapter only drawing overlays.
class DesignerBoxLayoutAdapter : public UiBoxLayout, public DesignerAdapter {
public:
	typedef DesignerBoxLayoutAdapter CLASSNAME;

	DesignerBoxLayoutAdapter();

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "BoxLayout"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiGridLayout.
// Maintains stable grid cells, per-axis child expansion, and real grid debug
// output while exposing row/column/min-cell settings to the inspector.
class DesignerGridLayoutAdapter : public UiGridLayout, public DesignerAdapter {
public:
	typedef DesignerGridLayoutAdapter CLASSNAME;

	DesignerGridLayoutAdapter();

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "GridLayout"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Layout() override;
	void Paint(Draw& w) override;

private:
	class DebugOverlay : public Ctrl {
	public:
		DesignerGridLayoutAdapter *owner = nullptr;
		void Paint(Draw& w) override;
	};

	void PaintTopOverlay(Draw& w) const;

	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
	DebugOverlay debug_overlay_;
};

// Designer wrapper for UiSplitter.
// Represents a two-pane container; panes are explicit child nodes so users can
// drop layouts/controls into each reserved region.
class DesignerSplitterAdapter : public UiSplitter, public DesignerAdapter {
public:
	typedef DesignerSplitterAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiSplitter"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Designer wrapper for UiQuadSplitter.
// Represents a four-pane split container for applications that need quadrant
// editing without nesting several splitters by hand.
class DesignerQuadSplitterAdapter : public UiQuadSplitter, public DesignerAdapter {
public:
	typedef DesignerQuadSplitterAdapter CLASSNAME;

	Ctrl& GetCtrl() override { return *this; }
	const Ctrl& GetCtrl() const override { return *this; }
	DesignerNodeId GetNodeId() const override { return node_id_; }
	String GetTypeId() const override { return "UiQuadSplitter"; }
	void SyncFromNode(const DesignerNode& node) override;
	void SetOverlayState(const DesignerOverlayState& state) override;
	const DesignerOverlayState& GetOverlayState() const override { return overlay_; }
	void DescribeApi(Vector<DesignerApiBinding>& out, const DesignerNode& node) const override;
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

// Factory used by preview, inspector probing, and tests.
// Returns a real Ui control with its adapter interface attached when supported.
Ctrl* CreateDesignerAdapterCtrl(const DesignerNode& node, DesignerAdapter **adapter = nullptr);
DesignerAdapter* AsDesignerAdapter(Ctrl& ctrl);
const DesignerAdapter* AsDesignerAdapter(const Ctrl& ctrl);
String DesignerAdapterHelp(const String& type_id);

}
