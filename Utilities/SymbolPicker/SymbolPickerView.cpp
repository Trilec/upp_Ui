#include "SymbolPickerView.h"

namespace Upp {

static String SymbolPickerExportTypeText(SymbolPickerExportType type)
{
	switch(type) {
	case SymbolPickerExportType::ImageCall: return "Image call";
	case SymbolPickerExportType::IconId: return "Icon id";
	case SymbolPickerExportType::CppSnippet: return "C++ snippet";
	}
	return "Unknown";
}

SymbolPickerView::SymbolPickerView()
{
	Title("Symbol Picker");
	Sizeable().Zoomable();
	SetRect(0, 0, DPI(760), DPI(420));

	Add(root_.SizePos());
	root_.SetCustomStyle(UiTheme::ResolvePanel(UiRole::Standard)).SetInset(12);
	root_.Add(layout_.SizePos());
	layout_.SetGap(10).SetInset(0);

	header_.SetTitle("Symbol Picker")
	       .SetSubTitle("V1 skeleton package: model, commands, and view shell.")
	       .SetMedia(ICON_DESIGN_WIDGETS_48(), Size(DPI(24), DPI(24)))
	       .SetCustomStyle(UiTheme::ResolveTitleCard(UiRole::Accent));
	summary_.SetCustomStyle(UiTheme::ResolveLabel(UiRole::Subtle, UiTextSize::Body));

	layout_.Add(header_).Fixed(DPI(84));
	layout_.Add(summary_).Fit();
}

void SymbolPickerView::SetModel(SymbolPickerModel* model)
{
	model_ = model;
	RefreshFromModel();
}

void SymbolPickerView::SetCommands(SymbolPickerCommandStack* commands)
{
	commands_ = commands;
	RefreshFromModel();
}

void SymbolPickerView::RefreshFromModel()
{
	if(!model_) {
		summary_.SetText("No model attached.");
		return;
	}

	String summary;
	summary << "Style: " << AsString((int)model_->GetCurrentStyle())
	        << " | Category: " << model_->GetCurrentCategory()
	        << " | Filter: " << (model_->GetFilterText().IsEmpty() ? String("(none)") : model_->GetFilterText())
	        << " | Export: " << SymbolPickerExportTypeText(model_->GetExportType())
	        << " @ " << model_->GetExportSize() << "px"
	        << " | Selected: " << model_->GetSelectedIconIds().GetCount();
	summary_.SetText(summary);
}

}
