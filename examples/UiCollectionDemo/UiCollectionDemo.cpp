#include "UiCollectionDemo.h"

namespace Upp {
namespace {

PropertyEditorItem& Authored(PropertyEditorItem& item)
{
    item.SetDefault(item.value);
    return item;
}

PropertyEditorItem& LocalOverride(PropertyEditorItem& item)
{
    Authored(item);
    item.overrideable = true;
    item.override_active = false;
    return item;
}

Image SampleImage(int seed)
{
    ImageDraw draw(48, 48);
    Color face(48 + seed * 37 % 176, 48 + seed * 59 % 176, 48 + seed * 83 % 176);
    Color ink(48 + (seed * 37 + 31) % 176, 48 + (seed * 59 + 47) % 176,
              48 + (seed * 83 + 19) % 176);
    draw.DrawRect(0, 0, 48, 48, face);
    if(seed % 3 == 0) draw.DrawEllipse(8, 8, 32, 32, ink);
    else if(seed % 3 == 1) draw.DrawRect(8, 8, 32, 32, ink);
    else draw.DrawEllipse(6, 12, 36, 24, ink);
    return draw;
}

} // namespace

UiCollectionDemo::UiCollectionDemo()
{
    Title("List + Gallery Designer");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1340), DPI(820));
    RegisterPropertyEditorEditors(factory);
    BuildShell();
    BuildProperties();
    BuildData((int)Config("count"));
    list.SetModel(model);
    gallery.SetModel(model);
    ConnectEvents();
    ApplyTheme();
    ApplyConfiguration();
    SelectPage(0);
    InspectItem(0);
}

UiCollectionDemo::~UiCollectionDemo()
{
    pe_inspector.SetModel(nullptr);
    pe_overrides.SetModel(nullptr);
    pe_data.SetModel(nullptr);
}

void UiCollectionDemo::BuildShell()
{
    Add(tc_header);
    tc_header.SetTitle("List + Gallery")
             .SetSubTitle("Design two views of one collection · live properties and reusable C++")
             .SetMedia(ICON_DESIGN_WIDGETS_48()).SetMediaSide(UiAlign::LEFT)
             .SetMediaAutoFit(true).ShowTitleLine(false).SetContentCell(box_header);
    box_header.SetGap(DPI(4)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    box_header.AddSpacer(1).Expand(1);
    btn_theme.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Switch Light / Dark");
    btn_help.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("Using the collection designer");
    btn_exit.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close designer");
    box_header.Add(btn_theme).Fixed(DPI(34));
    box_header.Add(btn_help).Fixed(DPI(34));
    box_header.Add(btn_exit).Fixed(DPI(34));

    Add(pnl_preview);
    Add(pnl_rail);
    pnl_preview.Add(box_view);
    pnl_preview.Add(lbl_list);
    pnl_preview.Add(lbl_gallery);
    pnl_preview.Add(list);
    pnl_preview.Add(gallery);
    pnl_preview.Add(lbl_status);
    box_view.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    btn_list.SetText("List").SetCheckable();
    btn_gallery.SetText("Gallery").SetCheckable();
    btn_compare.SetText("Compare").SetCheckable();
    btn_first.SetText("First");
    btn_last.SetText("Last");
    box_view.Add(btn_list).Fixed(DPI(68));
    box_view.Add(btn_gallery).Fixed(DPI(82));
    box_view.Add(btn_compare).Fixed(DPI(90));
    box_view.AddSpacer(1).Expand(1);
    box_view.Add(btn_first).Fixed(DPI(56));
    box_view.Add(btn_last).Fixed(DPI(56));
    lbl_list.SetText("LIST");
    lbl_gallery.SetText("GALLERY");

    pnl_rail.Add(box_pages);
    pnl_rail.Add(stk_pages);
    box_pages.SetGap(DPI(3)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    btn_inspector.SetCheckable().SetIcon(ICON_DESIGN_TUNE_48()).SetIconSize(DPI(17), DPI(17)).Tip("Inspector");
    btn_overrides.SetCheckable().SetIcon(ICON_DESIGN_FORMAT_PAINT_48()).SetIconSize(DPI(17), DPI(17)).Tip("Theme overrides");
    btn_data.SetCheckable().SetIcon(ICON_DESIGN_WIDGETS_48()).SetIconSize(DPI(17), DPI(17)).Tip("Data");
    btn_code.SetCheckable().SetIcon(ICON_DESIGN_CODE_BLOCKS_48()).SetIconSize(DPI(17), DPI(17)).Tip("Generated C++");
    box_pages.Add(btn_inspector).Fixed(DPI(38));
    box_pages.Add(btn_overrides).Fixed(DPI(38));
    box_pages.Add(btn_data).Fixed(DPI(38));
    box_pages.Add(btn_code).Fixed(DPI(38));
    box_pages.AddSpacer(1).Expand(1);
    stk_pages.Add(pnl_inspector, "inspector");
    stk_pages.Add(pnl_overrides, "style");
    stk_pages.Add(pnl_data, "data");
    stk_pages.Add(pnl_code, "code");
    pnl_inspector.Add(pe_inspector.SizePos());
    pnl_overrides.Add(pe_overrides.SizePos());
    pnl_data.Add(box_data_tools.HSizePos(6, 6).TopPos(4, DPI(30)));
    pnl_data.Add(pe_data.HSizePos(0, 0).VSizePos(DPI(40), 0));
    btn_add.SetText("Add item");
    btn_remove.SetText("Remove item");
    box_data_tools.SetGap(DPI(5)).SetInset(0);
    box_data_tools.Add(btn_add).Expand(1);
    box_data_tools.Add(btn_remove).Expand(1);
    pnl_code.Add(btn_copy.RightPos(DPI(8), DPI(36)).TopPos(DPI(5), DPI(30)));
    pnl_code.Add(edit_code.HSizePos(4, 4).VSizePos(DPI(42), 4));
    btn_copy.SetIcon(ICON_CONTENT_CONTENT_COPY_48()).SetIconSize(DPI(16), DPI(16)).Tip("Copy configuration C++");
    edit_code.SetReadOnly();
    for(PropertyEditor* editor : { &pe_inspector, &pe_overrides, &pe_data }) {
        editor->SetFactory(&factory);
        editor->SetLabelRatio(48);
        PropertyEditorStyle style = PropertyEditorStyle::System();
        style.show_group_summaries = true;
        editor->SetStyle(style);
    }
    pe_inspector.SetModel(&inspector_model);
    pe_overrides.SetModel(&override_model);
    pe_data.SetModel(&data_model);
}

void UiCollectionDemo::BuildProperties()
{
    Authored(inspector_model.AddChoice("view", "Preview", "List", "Collection")
        .AddChoice("List", "List").AddChoice("Gallery", "Gallery").AddChoice("Compare", "Compare"));
    inspector_model.Find("view")->visible=false;
    Authored(inspector_model.AddChoice("count", "Sample size", 10000, "Collection")
        .AddChoice(24, "24 items").AddChoice(10000, "10,000 items").AddChoice(100000, "100,000 items"));
    Authored(inspector_model.AddChoice("renderer", "Presentation", "Image", "Collection")
        .AddChoice("Basic", "Basic").AddChoice("Image", "Image"));
    Authored(inspector_model.AddBoolean("enabled", "Enabled", true, "Behaviour"));
    Authored(inspector_model.AddBoolean("multi", "Multiple selection", true, "Behaviour"));
    Authored(inspector_model.AddBoolean("list.rename", "Rename on double-click", true, "List"));
    Authored(inspector_model.AddBoolean("list.reorder", "Drag reorder", false, "List"));
    Authored(inspector_model.AddBoolean("list.drag_handle", "Drag handles", true, "List"));
    Authored(inspector_model.AddChoice("list.drag_side", "Drag handle side", "Right", "List")
        .AddChoice("Left", "Left").AddChoice("Right", "Right"));
    Authored(inspector_model.AddSliderInt("list.height", "Row height", 30, 18, 100, 1, "List"));
    Authored(inspector_model.AddSliderInt("list.spacing", "Row spacing", 0, 0, 24, 1, "List"));
    Authored(inspector_model.AddBoolean("list.checks", "Checks", true, "List"));
    Authored(inspector_model.AddBoolean("list.icons", "Icons", true, "List"));
    Authored(inspector_model.AddBoolean("list.metadata", "Metadata markers", true, "List"));
    Authored(inspector_model.AddBoolean("list.separators", "Row separators", false, "List"));
    Authored(inspector_model.AddBoolean("list.striped", "Alternating rows", false, "List"));
    Authored(inspector_model.AddBoolean("list.badges", "Right text as badge", false, "List"));
    Authored(inspector_model.AddSliderInt("gallery.width", "Tile width", 104, 32, 300, 1, "Gallery"));
    Authored(inspector_model.AddSliderInt("gallery.height", "Tile height", 108, 32, 300, 1, "Gallery"));
    Authored(inspector_model.AddSliderInt("gallery.gap", "Tile gap", 7, 0, 32, 1, "Gallery"));
    Authored(inspector_model.AddSliderInt("gallery.inset", "Inset", 8, 0, 40, 1, "Gallery"));
    Authored(inspector_model.AddSlider("gallery.zoom", "Zoom", 1.0, 0.5, 2.5, 0.05, "Gallery"));
    Authored(inspector_model.AddSliderInt("gallery.overscan", "Overscan rows", 2, 0, 6, 1, "Gallery"));
    inspector_model.SetGroupSubtitle("Collection", "one shared model, independent views");
    inspector_model.SetGroupSubtitle("List", "row content and interaction");
    inspector_model.SetGroupSubtitle("Gallery", "uniform tile geometry and zoom");

    UiList::Style ls = UiTheme::ResolveList();
    UiItemRenderImage sample;
    UiItemRenderStyle rs = sample.GetStyle();
    static const char* states[] = { "normal", "hot", "selected", "disabled" };
    for(int state = 0; state < 4; state++) {
        String id = states[state];
        Color face = ls.palette.face[state].IsSolid() ? ls.palette.face[state].color : SColorPaper();
        if(state == ST_NORMAL || state == ST_DISABLED) {
            LocalOverride(override_model.AddColor("surface.face." + id, states[state], face, "Surface / Face"));
            LocalOverride(override_model.AddColor("surface.frame." + id, states[state], ls.palette.frame[state], "Surface / Frame"));
        }
        LocalOverride(override_model.AddColor("renderer.ink." + id, states[state], rs.palette.ink[state], "Gallery / Ink"));
        LocalOverride(override_model.AddColor("renderer.icon." + id, states[state], rs.palette.icon[state], "Gallery / Icon"));
    }
    auto number = [&](const char* id, const char* label, int value, int minimum, int maximum, const char* group) {
        LocalOverride(override_model.AddSliderInt(id, label, value, minimum, maximum, 1, group));
    };
    auto boolean = [&](const char* id, const char* label, bool value, const char* group) {
        LocalOverride(override_model.AddBoolean(id, label, value, group));
    };
    auto color = [&](const char* id, const char* label, Color value, const char* group) {
        LocalOverride(override_model.AddColor(id, label, value, group));
    };
    boolean("surface.face", "Face enabled", true, "Surface / Geometry");
    boolean("surface.frame", "Frame enabled", true, "Surface / Geometry");
    number("surface.frame_width", "Frame width", 1, 0, 8, "Surface / Geometry");
    number("surface.radius", "Radius", 0, 0, 32, "Surface / Geometry");
    number("surface.margin_x", "Content margin X", 0, 0, 32, "Surface / Geometry");
    number("surface.margin_y", "Content margin Y", 0, 0, 32, "Surface / Geometry");
    boolean("surface.focus", "Focus ring", true, "Surface / Effects");
    boolean("surface.shadow", "Shadow", false, "Surface / Effects");
    boolean("surface.highlight", "Highlight", false, "Surface / Effects");
    LocalOverride(AddPropertyFont(override_model, "renderer.font", "Font face", rs.title_font.GetFaceName(), "Gallery / Typography"));
    number("renderer.font_height", "Font height", rs.title_font.GetHeight(), 8, 32, "Gallery / Typography");
    boolean("renderer.font_bold", "Bold", false, "Gallery / Typography");
    number("renderer.icon_size", "Icon size", 20, 8, 64, "Gallery / Media");
    boolean("renderer.description", "Description", true, "Gallery / Media");
    boolean("renderer.right_text", "Right text", true, "Gallery / Media");
    LocalOverride(AddPropertyFont(override_model, "list.font", "Font face", ls.font.GetFaceName(), "List / Typography"));
    number("list.font_height", "Font height", ls.font.GetHeight(), 8, 32, "List / Typography");
    number("list.icon_size", "Icon size", ls.icon_size, 8, 64, "List / Typography");
    color("list.ink", "Normal ink", ls.ink, "List / State");
    color("list.icon", "Normal icon ink", ls.palette.icon[ST_NORMAL], "List / State");
    number("list.radius", "Row radius", ls.row_radius, 0, 32, "List / Layout");
    number("list.pad_x", "Horizontal padding", ls.h_padding, 0, 32, "List / Layout");
    number("list.pad_y", "Vertical padding", ls.v_padding, 0, 32, "List / Layout");
    number("list.drag_size", "Drag handle size", ls.drag_size, 8, 32, "List / Layout");
    number("list.drag_gap", "Drag handle gap", ls.drag_gap, 0, 24, "List / Layout");
    color("list.hot_face", "Hover face", ls.hot_face, "List / State");
    color("list.selected_face", "Selected face", ls.selected_face, "List / State");
    color("list.even", "Even row face", SColorPaper(), "List / State");
    color("list.odd", "Odd row face", Blend(SColorPaper(), SColorText(), 12), "List / State");
    color("list.badge_face", "Badge face", ls.badge_face, "List / Badge");
    color("list.badge_ink", "Badge ink", ls.badge_ink, "List / Badge");
    color("gallery.selection", "Selection frame", ls.selected_frame, "Gallery / Selection");
    number("gallery.selection_width", "Selection frame width", DPI(2), 1, 8, "Gallery / Selection");
    color("gallery.marquee", "Marquee frame", ls.drag_marker, "Gallery / Selection");
    color("gallery.marquee_fill", "Marquee fill", Blend(SColorPaper(), ls.selected_face, 96), "Gallery / Selection");

    data_model.AddInteger("index", "Item index", 0, "Selected item").SetRange(0, 9999, 1);
    data_model.AddReadOnly("key", "Stable data key", 0, "Selected item");
    data_model.AddText("text", "Title", String(), "Selected item");
    data_model.AddText("description", "Description", String(), "Selected item");
    data_model.AddText("right_text", "Right text", String(), "Selected item");
    data_model.AddBoolean("enabled", "Enabled", true, "State");
    data_model.AddBoolean("editable", "Rename allowed", true, "State");
    data_model.AddBoolean("group_header", "Group header", false, "State");
    data_model.AddBoolean("has_check", "Show check", false, "State");
    data_model.AddBoolean("checked", "Checked", false, "State");
    data_model.AddBoolean("metadata", "Metadata marker", false, "State");
    data_model.AddColor("metadata_color", "Marker colour", Color(37, 99, 235), "State");
    data_model.AddSliderInt("image", "Sample image", 0, 0, 63, 1, "Media");
    data_model.SetGroupSubtitle("Selected item", "edits the live shared UiListModel");
    inspector_model.StructureChanged();
    override_model.StructureChanged();
    data_model.StructureChanged();
}

Value UiCollectionDemo::Config(const String& id) const
{
    const PropertyEditorItem* item = inspector_model.Find(id);
    return item ? item->value : Value();
}
Value UiCollectionDemo::Override(const String& id) const
{
    const PropertyEditorItem* item = override_model.Find(id);
    return item ? item->value : Value();
}
bool UiCollectionDemo::Active(const String& id) const
{
    const PropertyEditorItem* item = override_model.Find(id);
    return item && item->override_active;
}
bool UiCollectionDemo::AnyOverride(const String& prefix) const
{
    for(const auto& item : override_model.GetItems())
        if(item.override_active && item.id.StartsWith(prefix))
            return true;
    return false;
}

void UiCollectionDemo::BuildData(int count)
{
    if(sample_images.IsEmpty())
        for(int i = 0; i < 64; i++) sample_images.Add(SampleImage(i));
    Vector<UiModelItem> records;
    records.Reserve(count);
    for(int i = 0; i < count; i++) {
        UiModelItem item(Format("Item %d", i + 1), i);
        item.description = Format("Seed %02d", i % 64);
        item.right_text = Format("#%d", i + 1);
        item.image = item.icon = sample_images[i % 64];
        item.icon_render_mode = UiIconRenderMode::PreserveColor;
        item.has_metadata = i % 97 == 0;
        item.metadata_color = Color(37, 99, 235);
        item.editable = true;
        records.Add(pick(item));
    }
    model.Clear();
    model.AddRange(records);
}

void UiCollectionDemo::ConnectEvents()
{
    btn_list.WhenAction = [=] { SelectView("List"); };
    btn_gallery.WhenAction = [=] { SelectView("Gallery"); };
    btn_compare.WhenAction = [=] { SelectView("Compare"); };
    btn_first.WhenAction = [=] { list.SetCursor(0); gallery.SetCursor(0); InspectItem(0); UpdateStatus(); };
    btn_last.WhenAction = [=] { int i = model.GetCount() - 1; list.SetCursor(i); gallery.SetCursor(i); InspectItem(i); UpdateStatus(); };
    btn_inspector.WhenAction = [=] { SelectPage(0); };
    btn_overrides.WhenAction = [=] { SelectPage(1); };
    btn_data.WhenAction = [=] { SelectPage(2); };
    btn_code.WhenAction = [=] { UpdateCode(); SelectPage(3); };
    btn_copy.WhenAction = [=] { WriteClipboardText(generated_code); };
    btn_exit.WhenAction = [=] { Close(); };
    btn_help.WhenAction = [=] { PromptOK("Choose List, Gallery or Compare. Inspector changes layout and behaviour. Style fields inherit until you activate an override; reset restores inheritance. Data edits the selected item in both views. Code exports configuration for your own model. List: double-click rename and optional drag reorder. Gallery: Ctrl+wheel zoom, Ctrl/Shift marquee and Escape cancellation."); };
    btn_theme.WhenAction = [=] {
        UiTheme::Set(UiTheme::GetContext().mode == UiThemeMode::Dark ? UiThemeMode::Light : UiThemeMode::Dark);
        Ctrl::SwapDarkLight();
        ApplyTheme(); ApplyConfiguration();
    };
    auto changed = [=](String id, Value) { if(!projecting) ApplyConfiguration(id); };
    pe_inspector.WhenPreview = pe_inspector.WhenCommit = changed;
    pe_overrides.WhenPreview = pe_overrides.WhenCommit = changed;
    pe_inspector.WhenReset = [=](String id) { Reset(inspector_model, id); };
    pe_overrides.WhenReset = [=](String id) { Reset(override_model, id); };
    pe_overrides.WhenOverride = [=](String id, bool active) {
        override_model.Find(id)->override_active = active;
        override_model.ValueChanged(id);
        ApplyConfiguration(id);
    };
    pe_data.WhenCommit = [=](String id, Value) { ApplyData(id); };
    list.WhenSelection = [=] { if(!projecting) InspectItem(list.GetCursor()); UpdateStatus(); };
    gallery.WhenSelection = [=] { if(!projecting) InspectItem(gallery.GetCursor()); UpdateStatus(); };
    gallery.WhenVisibleRange = [=](int, int) { UpdateStatus(); };
    gallery.WhenZoom = [=](double zoom) {
        if(!projecting) { inspector_model.SetValue("gallery.zoom", zoom); UpdateCode(); }
        UpdateStatus();
    };
    model.WhenChange << [=](const UiModelChange& change) {
        if(projecting_data || projecting) return;
        data_index = UiRemapSequentialIndex(data_index, change);
        InspectItem(data_index);
        UpdateStatus();
    };
    btn_add.WhenAction = [=] {
        int64 key = 0;
        for(int i = 0; i < model.GetCount(); i++)
            if(model.Get(i).data.Is<int>() || model.Get(i).data.Is<int64>())
                key = max(key, (int64)model.Get(i).data + 1);
        UiModelItem item("New item", key);
        item.editable = true;
        item.image = item.icon = sample_images[0];
        int index = model.Add(item);
        list.SetCursor(index); gallery.SetCursor(index); InspectItem(index);
    };
    btn_remove.WhenAction = [=] {
        if(data_index >= 0 && data_index < model.GetCount()) {
            int next = data_index;
            model.Remove(data_index);
            InspectItem(min(next, model.GetCount() - 1));
        }
    };
}

void UiCollectionDemo::Reset(PropertyEditorModel& properties, const String& id)
{
    PropertyEditorItem* item = properties.Find(id);
    if(item->overrideable) item->override_active = false;
    properties.Reset(id);
    properties.ValueChanged(id);
    ApplyConfiguration(id);
}

void UiCollectionDemo::SelectView(const String& view)
{
    inspector_model.SetValue("view", view);
    ApplyConfiguration("view");
}

void UiCollectionDemo::SelectPage(int page)
{
    stk_pages.SetActivePage(page);
    btn_inspector.SetChecked(page == 0); btn_overrides.SetChecked(page == 1);
    btn_data.SetChecked(page == 2); btn_code.SetChecked(page == 3);
}

void UiCollectionDemo::ApplyConfiguration(const String& changed)
{
    projecting = true;
    if(changed == "count") { BuildData((int)Config("count")); data_index = 0; }
    bool common = changed.IsEmpty() || changed.StartsWith("surface.") || changed.StartsWith("renderer.") || changed == "renderer";
    if(changed.IsEmpty() || changed.StartsWith("surface.") || changed == "renderer" || changed.StartsWith("list.")) {
        list.SetCustomStyle(ListStyle());
        if(common) {
            if(AsString(Config("renderer")) == "Image") {
                UiItemRenderImage render;
                list.SetItemRender(render);
            }
            else {
                UiItemRenderBasic render;
                list.SetItemRender(render);
            }
        }
        list.EnableRenameOnDblClick((bool)Config("list.rename"))
            .EnableDragReorder((bool)Config("list.reorder"));
    }
    if(common || changed.StartsWith("gallery.")) {
        if(common || changed == "gallery.selection" || changed == "gallery.selection_width" ||
           changed == "gallery.marquee" || changed == "gallery.marquee_fill") {
            gallery.ClearCustomStyle();
            if(AnyOverride("surface.") || AnyOverride("gallery.")) gallery.SetCustomStyle(GalleryStyle());
        }
        if(common) {
            if(AsString(Config("renderer")) == "Image") {
                UiItemRenderImage render;
                if(AnyOverride("renderer.")) render.SetCustomStyle(RenderStyle());
                gallery.SetItemRender(render);
            }
            else {
                UiItemRenderBasic render;
                if(AnyOverride("renderer.")) render.SetCustomStyle(RenderStyle());
                gallery.SetItemRender(render);
            }
        }
        if(changed.IsEmpty() || changed == "gallery.width" || changed == "gallery.height")
            gallery.SetItemSize(Size(DPI((int)Config("gallery.width")), DPI((int)Config("gallery.height"))));
        gallery.SetGap(DPI((int)Config("gallery.gap"))).SetInset(DPI((int)Config("gallery.inset")))
               .SetOverscanRows((int)Config("gallery.overscan"))
               .SetZoomRange(0.5, 2.5).SetZoom((double)Config("gallery.zoom"));
    }
    list.Enable((bool)Config("enabled")); gallery.Enable((bool)Config("enabled"));
    list.SetSelectionMode((bool)Config("multi") ? UILISTSEL_MULTI : UILISTSEL_SINGLE);
    gallery.SetSelectionMode((bool)Config("multi") ? UIGALLERYSEL_MULTI : UIGALLERYSEL_SINGLE);
    String view = AsString(Config("view"));
    btn_list.SetChecked(view == "List"); btn_gallery.SetChecked(view == "Gallery"); btn_compare.SetChecked(view == "Compare");
    for(const auto& item : inspector_model.GetItems()) {
        bool visible = !item.id.StartsWith("list.") && !item.id.StartsWith("gallery.") && !item.id.StartsWith("renderer.");
        if(item.id.StartsWith("list.")) visible = view != "Gallery";
        if(item.id.StartsWith("gallery.")) visible = view != "List";
        if(item.id.StartsWith("renderer.")) visible = view != "List";
        if(item.visible != visible) inspector_model.SetVisible(item.id, visible);
    }
    for(const auto& item : override_model.GetItems()) {
        bool visible = !item.id.StartsWith("list.") && !item.id.StartsWith("gallery.") && !item.id.StartsWith("renderer.");
        if(item.id.StartsWith("list.")) visible = view != "Gallery";
        if(item.id.StartsWith("gallery.")) visible = view != "List";
        if(item.id.StartsWith("renderer.")) visible = view != "List";
        if(item.visible != visible) override_model.SetVisible(item.id, visible);
    }
    projecting = false;
    Layout();
    if(changed == "count") InspectItem(data_index);
    UpdateCode(); UpdateStatus();
}

void UiCollectionDemo::InspectItem(int index)
{
    projecting_data = true;
    data_index = index >= 0 && index < model.GetCount() ? index : -1;
    bool valid = data_index >= 0;
    for(const auto& row : data_model.GetItems())
        data_model.SetEnabled(row.id, valid || row.id == "index");
    btn_remove.Enable(valid);
    if(PropertyEditorItem* row = data_model.Find("index")) row->SetRange(0, max(0, model.GetCount() - 1), 1);
    if(valid) {
        const UiModelItem& item = model.Get(data_index);
        data_model.SetValue("index", data_index);
        data_model.SetValue("key", item.data);
        data_model.SetValue("text", item.text);
        data_model.SetValue("description", item.description);
        data_model.SetValue("right_text", item.right_text);
        data_model.SetValue("enabled", item.enabled);
        data_model.SetValue("editable", item.editable);
        data_model.SetValue("group_header", item.group_header);
        data_model.SetValue("has_check", item.has_check);
        data_model.SetValue("checked", item.checked);
        data_model.SetValue("metadata", item.has_metadata);
        data_model.SetValue("metadata_color", item.metadata_color);
        int image_index = FindIndex(sample_images, item.image);
        data_model.SetValue("image", max(0, image_index));
    }
    projecting_data = false;
}

void UiCollectionDemo::ApplyData(const String& id)
{
    if(projecting_data) return;
    if(id == "index") { InspectItem((int)data_model.Find(id)->value); return; }
    if(data_index < 0 || data_index >= model.GetCount()) return;
    UiModelItem item = model.Get(data_index);
    Value value = data_model.Find(id)->value;
    if(id == "text") item.text = AsString(value);
    else if(id == "description") item.description = AsString(value);
    else if(id == "right_text") item.right_text = AsString(value);
    else if(id == "enabled") item.enabled = value;
    else if(id == "editable") item.editable = value;
    else if(id == "group_header") item.group_header = value;
    else if(id == "has_check") item.has_check = value;
    else if(id == "checked") item.checked = value;
    else if(id == "metadata") item.has_metadata = value;
    else if(id == "metadata_color") item.metadata_color = Color(value);
    else if(id == "image") item.image = item.icon = sample_images[minmax((int)value, 0, 63)];
    model.Set(data_index, item);
}

void UiCollectionDemo::ApplyTheme()
{
    const bool dark = UiTheme::GetContext().mode == UiThemeMode::Dark;
    btn_theme.SetIcon(UiTheme::GetContext().mode == UiThemeMode::Dark ? ICON_ACTION_LIGHT_MODE_48() : ICON_ACTION_DARK_MODE_48());
    UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Accent);
    header_style.media_tint_mono = true;
    tc_header.SetCustomStyle(header_style);
    window_face = UiTheme::ResolvePanel(UiPanelRole::Surface).palette.face[ST_NORMAL].color;
    UiPanel::Style surface = UiTheme::ResolvePanel(UiPanelRole::Surface);
    const Color panel_face = dark ? Color(18, 18, 18) : Color(245, 245, 245);
    surface.transparent = false;
    surface.metrics.face_enabled = surface.metrics.frame_enabled = true;
    surface.metrics.frame_width = DPI(1);
    surface.metrics.radius = DPI(8);
    surface.metrics.shadow.enabled = surface.metrics.focus_enabled = false;
    for(int state = 0; state < 4; state++) {
        surface.palette.face[state] = UiFill::Solid(panel_face);
        surface.palette.frame[state] = dark ? Color(48, 48, 48) : Color(220, 220, 220);
    }
    pnl_preview.SetCustomStyle(surface);
    pnl_rail.SetCustomStyle(surface);
    UiPanel::Style page_style = surface;
    page_style.transparent = true;
    page_style.metrics.face_enabled = page_style.metrics.frame_enabled = false;
    for(UiPanel* panel : { &pnl_inspector, &pnl_overrides, &pnl_data, &pnl_code })
        panel->SetCustomStyle(page_style);
    PropertyEditorPaletteMode mode = UiTheme::GetContext().mode == UiThemeMode::Dark ? PropertyEditorPaletteMode::Dark : PropertyEditorPaletteMode::Light;
    for(PropertyEditor* editor : { &pe_inspector, &pe_overrides, &pe_data }) {
        editor->SetPaletteMode(mode);
        PropertyEditorStyle editor_style = editor->GetStyle();
        editor_style.show_frame = false;
        editor_style.background = panel_face;
        editor_style.show_group_summaries = true;
        editor->SetStyle(editor_style);
    }
    for(UiToolButton* button : { &btn_theme, &btn_help, &btn_exit, &btn_inspector, &btn_overrides, &btn_data, &btn_code, &btn_copy }) {
        UiToolButton::Style style = UiTheme::ResolveToolButton(UiRole::Standard);
        style.transparent = true;
        style.metrics.face_enabled = style.metrics.frame_enabled = false;
        style.metrics.shadow.enabled = style.metrics.focus_enabled = false;
        style.underline = false;
        for(int state = 0; state < 4; state++) {
            style.palette.face[state] = UiFill::None();
            style.palette.frame[state] = Null;
        }
        const Color neutral = dark ? Color(180, 180, 180) : Color(110, 110, 110);
        style.palette.icon[ST_NORMAL] = neutral;
        style.palette.icon[ST_HOT] = dark ? White() : Color(32, 32, 32);
        style.palette.icon[ST_PRESSED] = Color(0, 120, 212);
        style.palette.icon[ST_DISABLED] = Blend(neutral, panel_face, 150);
        button->SetCustomStyle(style);
    }
    UiToolButton::Style exit_style = btn_exit.GetStyle();
    exit_style.palette.icon[ST_NORMAL] = Color(200, 60, 60);
    exit_style.palette.icon[ST_HOT] = Color(240, 85, 85);
    exit_style.palette.icon[ST_PRESSED] = Color(180, 45, 45);
    btn_exit.SetCustomStyle(exit_style);
    lbl_list.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    lbl_gallery.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    lbl_status.SetCustomStyle(UiTheme::ResolveLabel(UiLabelRole::Caption));
    Refresh();
}

void UiCollectionDemo::Paint(Draw& w)
{
    w.DrawRect(GetSize(), window_face);
}

void UiCollectionDemo::Layout()
{
    Size size = GetSize();
    int pad = DPI(12), gap = DPI(10), top = DPI(94);
    int width = max(0, size.cx - 2 * pad);
    int rail = min(DPI(430), max(DPI(300), width * 34 / 100));
    rail = min(rail, width);
    int preview_width = max(0, width - rail - gap);
    int height = max(0, size.cy - top - pad);
    tc_header.SetRect(pad, pad, width, DPI(72));
    pnl_preview.SetRect(pad, top, preview_width, height);
    pnl_rail.SetRect(pad + preview_width + gap, top, rail, height);
    box_view.SetRect(DPI(8), DPI(6), max(0, preview_width - DPI(16)), DPI(32));
    box_pages.SetRect(DPI(6), DPI(6), max(0, rail - DPI(12)), DPI(34));
    stk_pages.SetRect(DPI(4), DPI(48), max(0, rail - DPI(8)), max(0, height - DPI(52)));
    int body_top = DPI(66), body_height = max(0, height - body_top - DPI(40));
    int body_width = max(0, preview_width - DPI(16));
    String view = AsString(Config("view"));
    bool compare = view == "Compare";
    int list_width = compare ? max(0, (body_width - gap) * 38 / 100) : body_width;
    int gallery_left = compare ? DPI(8) + list_width + gap : DPI(8);
    int gallery_width = compare ? max(0, body_width - list_width - gap) : body_width;
    list.Show(view != "Gallery"); gallery.Show(view != "List");
    lbl_list.Show(view != "Gallery"); lbl_gallery.Show(view != "List");
    lbl_list.SetRect(DPI(8), DPI(42), list_width, DPI(20));
    lbl_gallery.SetRect(gallery_left, DPI(42), gallery_width, DPI(20));
    if(list.IsShown()) list.SetRect(DPI(8), body_top, list_width, body_height);
    if(gallery.IsShown()) gallery.SetRect(gallery_left, body_top, gallery_width, body_height);
    lbl_status.SetRect(DPI(8), max(0, height - DPI(32)), max(0, preview_width - DPI(16)), DPI(24));
    UpdateStatus();
}

void UiCollectionDemo::UpdateStatus()
{
    lbl_status.SetText(Format("%d items · selected L %d / G %d · zoom %.0f%%",
        model.GetCount(), list.GetSelectionCount(), gallery.GetSelectionCount(), gallery.GetZoom() * 100));
}

void UiCollectionDemo::UpdateCode()
{
    generated_code = GenerateCode();
    edit_code.SetText(generated_code.ToWString());
}

} // namespace Upp
