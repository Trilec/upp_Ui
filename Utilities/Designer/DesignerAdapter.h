#pragma once

#include "DesignerRegistry.h"

namespace Upp {

struct DesignerOverlayState {
	bool selected = false;
	bool hovered = false;
	bool drop_target = false;
	bool debug = false;
	int radius = 0;
};

enum class DesignerEditorKind {
	ReadOnly,
	Text,
	Int,
	Bool,
	Choice,
	Color,
	Slider
};

struct DesignerApiBinding : Moveable<DesignerApiBinding> {
	String property_id;
	String label;
	DesignerEditorKind editor = DesignerEditorKind::Text;
	String help;
	String api_call;
	String codegen_hint;
	Value default_value;
	Value min_value;
	Value max_value;
	VectorMap<String, Value> choices;
	bool visible = true;
	bool enabled = true;
	String disabled_reason;
};

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
	String type_id_ = "Item";
	DesignerOverlayState overlay_;
};

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
	void Paint(Draw& w) override;

private:
	DesignerNodeId node_id_ = Designer_NULL;
	DesignerOverlayState overlay_;
};

Ctrl* CreateDesignerAdapterCtrl(const DesignerNode& node, DesignerAdapter **adapter = nullptr);
DesignerAdapter* AsDesignerAdapter(Ctrl& ctrl);
const DesignerAdapter* AsDesignerAdapter(const Ctrl& ctrl);

}
