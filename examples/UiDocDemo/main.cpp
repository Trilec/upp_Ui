/*
    UiDocDemo
    ------------

    Purpose
    - Active Ui control demo used as a build smoke test and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Prefer BuilderDemoSupport/shared shell and UiComposite inspector rows where practical.
    - Prefer UiTheme defaults; add local styling only when the demo intentionally showcases that variation.

    Changelog
    - 2026-05: active demo sweep verified; header added during demo cleanup pass.
*/
#include <Ui/Ui.h>
#include <Ui/UiIcons.h>

using namespace Upp;

// UiDocDemo intentionally routes mutating actions through command execution
// to mirror production transaction semantics and undo/redo behavior.

static String NormalizeFindInput(String s)
{
    s = TrimBoth(s);
    if(s.IsEmpty())
        return s;

    int first_ascii = -1;
    for(int i = 0; i < s.GetCount(); i++) {
        if((byte)s[i] < 0x80) {
            first_ascii = i;
            break;
        }
    }

    if(first_ascii > 0) {
        String tail = TrimBoth(s.Mid(first_ascii));
        if(!tail.IsEmpty())
            return tail;
    }

    return s;
}

class UiDocDemoWindow : public TopWindow {
public:
    typedef UiDocDemoWindow CLASSNAME;

    UiDocDemoWindow()
    {
        Title("UiDoc Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1260), DPI(860));

        Add(toolbar_);

        Add(inspector_title_); Add(inspector_state_); Add(inspector_modes_); Add(inspector_block_); Add(inspector_find_); Add(inspector_margin_); Add(inspector_meta_);
        Add(insp_margin_plus_); Add(insp_margin_minus_); Add(insp_margin_reset_);
        Add(insp_lead_plus_); Add(insp_lead_minus_);
        Add(insp_track_plus_); Add(insp_track_minus_);
        Add(insp_tab_plus_); Add(insp_tab_minus_); Add(insp_tab_mode_);
        Add(insp_tx_check_); Add(insp_map_check_); Add(insp_accept_check_); Add(inspector_checks_);
        Add(comment_title_); Add(comment_pick_); Add(comment_edit_); Add(comment_save_); Add(comment_delete_); Add(comment_close_);
        Add(inspector_keys_);
        Add(doc_);

        UiGridLayout::Style tool_style = toolbar_.GetStyle();
        tool_style.group_header = true;
        tool_style.group_divider = true;
        tool_style.cluster_box_pad = DPI(4);
        toolbar_.SetMode(UiGridLayout::Flow)
                .SetDirection(UiDirection::H)
                .SetWrap(true)
                .SetGap(DPI(6))
                .SetInset(DPI(8))
                .SetAlignItems(UiCrossAlign::Start)
                .SetCustomStyle(tool_style)
                .SetGroupHeaders(true);

        const int cl_format = toolbar_.NewCluster();
        const int cl_structure = toolbar_.NewCluster();
        const int cl_table = toolbar_.NewCluster();
        const int cl_embed = toolbar_.NewCluster();
        const int cl_find = toolbar_.NewCluster();
        const int cl_view = toolbar_.NewCluster();
        toolbar_.SetClusterDecor(cl_format, true, true)
                .SetClusterDecor(cl_structure, true, true)
                .SetClusterDecor(cl_table, true, true)
                .SetClusterDecor(cl_embed, true, true)
                .SetClusterDecor(cl_find, true, true)
                .SetClusterDecor(cl_view, true, true);
        toolbar_.WhenClusterText([=](int id) -> String {
            if(id == cl_format) return "Formatting";
            if(id == cl_structure) return "Structure";
            if(id == cl_table) return "Table";
            if(id == cl_embed) return "Embed";
            if(id == cl_find) return "Find";
            if(id == cl_view) return "View";
            return String();
        });

        inspector_title_.SetText("Inspector");
        find_status_ = "Find: idle";
        inspector_state_.SetReadOnly();
        inspector_modes_.SetReadOnly();
        inspector_block_.SetReadOnly();
        inspector_find_.SetReadOnly();
        inspector_margin_.SetReadOnly();
        inspector_meta_.SetReadOnly();
        inspector_keys_.SetReadOnly();
        inspector_find_.SetText((find_status_ + " | mode: ignore-case:on, whole-word:off").ToWString());
        inspector_meta_.SetText("Meta: none");
        inspector_keys_.SetText("Mods: Ctrl off Alt off Shift off");
        comment_title_.SetText("Comment note");
        comment_edit_.SetPlaceholder("Type annotation comment...");
        comment_save_.SetText("Save");
        comment_delete_.SetText("Delete");
        comment_close_.SetText("Close");
        comment_delete_.Disable();
        comment_close_.Disable();

        bold_.SetText("").SetIcon(ICON_EDITOR_FORMAT_BOLD_48());
        italic_.SetText("").SetIcon(ICON_EDITOR_FORMAT_ITALIC_48());
        underline_.SetText("").SetIcon(ICON_EDITOR_SERIF_48());
        strike_.SetText("").SetIcon(ICON_EDITOR_FORMAT_STRIKETHROUGH_48());
        upper_.SetText("").SetIcon(ICON_DESIGN_UPPERCASE_48());
        lower_.SetText("").SetIcon(ICON_DESIGN_LOWERCASE_48());
        titlecase_.SetText("").SetIcon(ICON_EDITOR_TITLECASE_48());
        size_up_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_48());
        size_down_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48());
        comment_.SetText("").SetIcon(ICON_COMMUNICATION_COMMENT_48());

        bullet_.SetText("").SetIcon(ICON_EDITOR_FORMAT_LIST_BULLETED_48());
        numbered_.SetText("").SetIcon(ICON_EDITOR_FORMAT_LIST_NUMBERED_RTL_48());
        bullet_circle_.SetText("").SetIcon(ICON_NAVIGATION_OUTLINED_MORE_HORIZ_48());
        bullet_dash_.SetText("").SetIcon(ICON_NAVIGATION_OUTLINED_MENU_48());

        h1_.SetText("").SetIcon(ICON_EDITOR_FORMAT_H1_48());
        h2_.SetText("").SetIcon(ICON_EDITOR_FORMAT_H2_48());
        h3_.SetText("").SetIcon(ICON_EDITOR_FORMAT_H3_48());
        indent_.SetText("").SetIcon(ICON_EDITOR_FORMAT_INDENT_INCREASE_48());
        outdent_.SetText("").SetIcon(ICON_EDITOR_FORMAT_INDENT_DECREASE_48());
        quote_.SetText("").SetIcon(ICON_EDITOR_FORMAT_QUOTE_48());
        code_.SetText("").SetIcon(ICON_EDITOR_CLARIFY_48());

        table_.SetText("").SetIcon(ICON_EDITOR_TABLE_48());
        table_row_add_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_CIRCLE_OUTLINE_48());
        table_row_del_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_CIRCLE_OUTLINE_48());
        table_col_add_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_48());
        table_col_del_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48());
        table_decor_.SetText("Frame");
        table_del_.SetText("").SetIcon(ICON_DESIGN_DELETE_48());
        hr_.SetText("").SetIcon(ICON_EDITOR_FORMAT_LINE_SPACING_48());
        image_file_ins_.SetText("").SetIcon(ICON_DESIGN_FOLDER_48());
        image_flow_demo_.SetText("T+L+T").SetIcon(ICON_DESIGN_IMAGE_48());
        image_align_left_.SetText("").SetIcon(ICON_EDITOR_BORDER_LEFT_48());
        image_align_center_.SetText("C").SetIcon(ICON_NAVIGATION_OUTLINED_DRAG_INDICATOR_48());
        image_align_right_.SetText("").SetIcon(ICON_EDITOR_BORDER_RIGHT_48());
        svg_ins_.SetText("M+").SetIcon(ICON_NAVIGATION_OUTLINED_APPS_48());
        meta_del_.SetText("M-").SetIcon(ICON_DESIGN_DELETE_48());
        embed_del_.SetText("").SetIcon(ICON_DESIGN_DELETE_48());

        note_.SetText("").SetIcon(ICON_EDITOR_NOTES_48());
        find_prev_.SetText("").SetIcon(ICON_NAVIGATION_OUTLINED_ARROW_LEFT_48());
        find_.SetText("").SetIcon(ICON_ACTION_SEARCH_48());
        find_ignore_case_.SetText("Aa");
        find_whole_word_.SetText("W");
        view_gutter_side_.SetText("").SetIcon(ICON_EDITOR_BORDER_LEFT_48());
        view_line_numbers_.SetText("Ln").SetIcon(ICON_NAVIGATION_OUTLINED_DRAG_INDICATOR_48());
        view_meta_markers_.SetText("").SetIcon(ICON_ACTION_OUTLINED_VISIBILITY_48());
        search_.SetPlaceholder("Search text...");
        insp_margin_plus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_48());
        insp_margin_minus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48());
        insp_margin_reset_.SetText("").SetIcon(ICON_EDITOR_MARGIN_48());
        insp_lead_plus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_48());
        insp_lead_minus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48());
        insp_track_plus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_48());
        insp_track_minus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48());
        insp_tab_plus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_ADD_48());
        insp_tab_minus_.SetText("").SetIcon(ICON_CONTENT_OUTLINED_REMOVE_48());
        insp_tab_mode_.SetText("Tabs=spaces").SetIcon(ICON_EDITOR_MODE_OFF_ON_48());
        insp_tab_mode_.SetCheckable();
        insp_tx_check_.SetText("").SetIcon(ICON_EDITOR_CLARIFY_48());
        insp_map_check_.SetText("").SetIcon(ICON_ACTION_CHECK_CIRCLE_48());
        insp_accept_check_.SetText("ACPT");
        inspector_checks_.SetReadOnly();
        inspector_checks_.SetText("Checks: idle");

        auto setup_icon_button = [&](UiButton& b) {
            b.SetIconSide(UiAlign::CENTER);
            b.SetIconSize(DPI(18), DPI(18));
            b.SetIconRenderMode(UiIconRenderMode::MonoTint);
            b.SetIconColor(Color(28, 31, 38), 10, -8);
            b.SetMargin(DPI(3));
            b.SetContentGap(DPI(3));
        };

        setup_icon_button(bold_);
        setup_icon_button(italic_);
        setup_icon_button(underline_);
        setup_icon_button(strike_);
        setup_icon_button(upper_);
        setup_icon_button(lower_);
        setup_icon_button(titlecase_);
        setup_icon_button(size_up_);
        setup_icon_button(size_down_);
        setup_icon_button(comment_);
        setup_icon_button(bullet_);
        setup_icon_button(numbered_);
        setup_icon_button(bullet_circle_);
        setup_icon_button(bullet_dash_);
        setup_icon_button(h1_);
        setup_icon_button(h2_);
        setup_icon_button(h3_);
        setup_icon_button(indent_);
        setup_icon_button(outdent_);
        setup_icon_button(quote_);
        setup_icon_button(code_);
        setup_icon_button(table_);
        setup_icon_button(table_row_add_);
        setup_icon_button(table_row_del_);
        setup_icon_button(table_col_add_);
        setup_icon_button(table_col_del_);
        setup_icon_button(table_del_);
        setup_icon_button(hr_);
        setup_icon_button(image_file_ins_);
        setup_icon_button(image_flow_demo_);
        setup_icon_button(image_align_left_);
        setup_icon_button(image_align_center_);
        setup_icon_button(image_align_right_);
        setup_icon_button(svg_ins_);
        setup_icon_button(meta_del_);
        setup_icon_button(embed_del_);
        setup_icon_button(note_);
        setup_icon_button(find_prev_);
        setup_icon_button(find_);
        setup_icon_button(find_ignore_case_);
        setup_icon_button(find_whole_word_);
        setup_icon_button(view_gutter_side_);
        setup_icon_button(view_line_numbers_);
        setup_icon_button(view_meta_markers_);
        setup_icon_button(insp_margin_plus_);
        setup_icon_button(insp_margin_minus_);
        setup_icon_button(insp_margin_reset_);
        setup_icon_button(insp_lead_plus_);
        setup_icon_button(insp_lead_minus_);
        setup_icon_button(insp_track_plus_);
        setup_icon_button(insp_track_minus_);
        setup_icon_button(insp_tab_plus_);
        setup_icon_button(insp_tab_minus_);
        setup_icon_button(insp_tx_check_);
        setup_icon_button(insp_map_check_);
        setup_icon_button(insp_accept_check_);

        bold_.Tip("Toggle bold. With no selection, affects typing mode.");
        italic_.Tip("Toggle italic. With no selection, affects typing mode.");
        underline_.Tip("Toggle underline. With no selection, affects typing mode.");
        strike_.Tip("Toggle strikeout.");
        upper_.Tip("Convert selection to UPPERCASE.");
        lower_.Tip("Convert selection to lowercase.");
        titlecase_.Tip("Convert selection to Title Case.");
        size_up_.Tip("Increase selection/typing font size.");
        size_down_.Tip("Decrease selection/typing font size.");
        comment_.Tip("Toggle // line comment on selected lines.");
        bullet_.Tip("Toggle bullet list mode. Enter continues list.");
        numbered_.Tip("Toggle numbered list mode. Enter continues list.");
        bullet_circle_.Tip("Set bullet style to circle marker.");
        bullet_dash_.Tip("Set bullet style to dash marker.");
        h1_.Tip("Apply H1 block style.");
        h2_.Tip("Apply H2 block style.");
        h3_.Tip("Apply H3 block style.");
        indent_.Tip("Increase paragraph margin for selection.");
        outdent_.Tip("Decrease paragraph margin for selection.");
        quote_.Tip("Wrap selection in double quotes.");
        code_.Tip("Apply code block style.");
        table_.Tip("Insert Table (embedded).");
        table_row_add_.Tip("Add table row below current row (inside table).");
        table_row_del_.Tip("Remove current table row (inside table).");
        table_col_add_.Tip("Add table column to the right (inside table).");
        table_col_del_.Tip("Remove current table column (inside table).");
        table_decor_.Tip("Toggle frame/grid decorations for active table.");
        table_del_.Tip("Delete current table at caret.");
        hr_.Tip("Insert horizontal rule embed.");
        image_flow_demo_.Tip("Insert text + logo + text (table cell if active, otherwise paragraph). ");
        image_file_ins_.Tip("Insert PNG/JPEG from file (table cell if active, otherwise paragraph). ");
        image_align_left_.Tip("Align selected block image left.");
        image_align_center_.Tip("Align selected block image center.");
        image_align_right_.Tip("Align selected block image right.");
        svg_ins_.Tip("Attach demo metadata at caret (blue square marker).");
        meta_del_.Tip("Delete demo metadata attached at caret.");
        embed_del_.Tip("Delete embed at caret (image/svg/hr/table embed ref).");
        note_.Tip("Attach metadata comment to selected text.");
        search_.Tip("Type search query (supports * and ? wildcards).");
        find_prev_.Tip("Find previous match.");
        find_.Tip("Find next match. Highlights all matches.");
        find_ignore_case_.Tip("Toggle case-insensitive matching.");
        find_whole_word_.Tip("Toggle whole-word matching.");
        view_gutter_side_.Tip("Toggle gutter side (left/right).");
        view_line_numbers_.Tip("Toggle line numbers in gutter.");
        view_meta_markers_.Tip("Toggle metadata markers in gutter.");
        insp_margin_plus_.Tip("Increase paragraph margin steps.");
        insp_margin_minus_.Tip("Decrease paragraph margin steps.");
        insp_margin_reset_.Tip("Reset paragraph margin steps to zero.");
        insp_lead_plus_.Tip("Increase line spacing for selected text.");
        insp_lead_minus_.Tip("Decrease line spacing for selected text.");
        insp_track_plus_.Tip("Increase letter spacing for selected text.");
        insp_track_minus_.Tip("Decrease letter spacing for selected text.");
        insp_tab_plus_.Tip("Increase tab size.");
        insp_tab_minus_.Tip("Decrease tab size.");
        insp_tab_mode_.Tip("Toggle typing Tab key as spaces.");
        insp_tx_check_.Tip("Run dispatch transaction fan-out check.");
        insp_map_check_.Tip("Run annotation position mapping check.");
        insp_accept_check_.Tip("Run acceptance checks for comment undo/redo/remap/reveal.");
        comment_save_.Tip("Save comment metadata for selected range.");
        comment_delete_.Tip("Delete current selected comment metadata.");
        comment_close_.Tip("Collapse currently active comment.");

        toolbar_.Add(bold_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(italic_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(underline_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(strike_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(upper_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(lower_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(titlecase_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(size_up_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(size_down_, cl_format, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(comment_, cl_format, false, Size(DPI(34), DPI(30)));

        toolbar_.Add(bullet_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(numbered_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(bullet_circle_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(bullet_dash_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(h1_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(h2_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(h3_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(indent_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(outdent_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(quote_, cl_structure, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(code_, cl_structure, false, Size(DPI(42), DPI(30)));

        toolbar_.Add(table_, cl_table, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(table_row_add_, cl_table, false, Size(DPI(40), DPI(30)));
        toolbar_.Add(table_row_del_, cl_table, false, Size(DPI(40), DPI(30)));
        toolbar_.Add(table_col_add_, cl_table, false, Size(DPI(40), DPI(30)));
        toolbar_.Add(table_col_del_, cl_table, false, Size(DPI(40), DPI(30)));
        toolbar_.Add(table_decor_, cl_table, false, Size(DPI(54), DPI(30)));
        toolbar_.Add(table_del_, cl_table, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(note_, cl_table, false, Size(DPI(34), DPI(30)));

        toolbar_.Add(hr_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(image_flow_demo_, cl_embed, false, Size(DPI(52), DPI(30)));
        toolbar_.Add(image_file_ins_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(image_align_left_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(image_align_center_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(image_align_right_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(svg_ins_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(meta_del_, cl_embed, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(embed_del_, cl_embed, false, Size(DPI(34), DPI(30)));

        toolbar_.Add(search_, cl_find, false, Size(DPI(240), DPI(30)));
        toolbar_.Add(find_prev_, cl_find, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(find_, cl_find, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(find_ignore_case_, cl_find, false, Size(DPI(40), DPI(30)));
        toolbar_.Add(find_whole_word_, cl_find, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(view_gutter_side_, cl_view, false, Size(DPI(34), DPI(30)));
        toolbar_.Add(view_line_numbers_, cl_view, false, Size(DPI(40), DPI(30)));
        toolbar_.Add(view_meta_markers_, cl_view, false, Size(DPI(34), DPI(30)));

        bullet_.SetCheckable();
        numbered_.SetCheckable();
        bullet_circle_.SetCheckable();
        bullet_dash_.SetCheckable();
        view_line_numbers_.SetCheckable();
        view_meta_markers_.SetCheckable();
        find_ignore_case_.SetCheckable();
        find_whole_word_.SetCheckable();
        bullet_circle_.SetChecked(true);
        view_meta_markers_.SetChecked(true);
        find_ignore_case_.SetChecked(true);

        bold_.WhenAction = [=] { doc_.ExecuteCommand("mark.bold"); doc_.SetFocus(); };
        italic_.WhenAction = [=] { doc_.ExecuteCommand("mark.italic"); doc_.SetFocus(); };
        underline_.WhenAction = [=] { doc_.ExecuteCommand("mark.underline"); doc_.SetFocus(); };
        strike_.WhenAction = [=] { doc_.ExecuteCommand("mark.strike"); doc_.SetFocus(); };
        upper_.WhenAction = [=] { doc_.ExecuteCommand("text.upper"); doc_.SetFocus(); };
        lower_.WhenAction = [=] { doc_.ExecuteCommand("text.lower"); doc_.SetFocus(); };
        titlecase_.WhenAction = [=] { doc_.ExecuteCommand("text.title"); doc_.SetFocus(); };
        size_up_.WhenAction = [=] { doc_.ExecuteCommand("text.size.inc"); doc_.SetFocus(); };
        size_down_.WhenAction = [=] { doc_.ExecuteCommand("text.size.dec"); doc_.SetFocus(); };
        comment_.WhenAction = [=] { doc_.ExecuteCommand("comment.line"); doc_.SetFocus(); };

        bullet_.WhenAction = [=] {
            doc_.ExecuteCommand("list.bullet");
            RefreshListButtons();
            doc_.SetFocus();
        };
        numbered_.WhenAction = [=] {
            doc_.ExecuteCommand("list.numbered");
            RefreshListButtons();
            doc_.SetFocus();
        };
        bullet_circle_.WhenAction = [=] {
            doc_.ExecuteCommand("list.style.circle");
            bullet_circle_.SetChecked(true);
            bullet_dash_.SetChecked(false);
            doc_.SetFocus();
        };
        bullet_dash_.WhenAction = [=] {
            doc_.ExecuteCommand("list.style.dash");
            bullet_circle_.SetChecked(false);
            bullet_dash_.SetChecked(true);
            doc_.SetFocus();
        };

        h1_.WhenAction = [=] { doc_.ExecuteCommand("block.h1"); doc_.SetFocus(); };
        h2_.WhenAction = [=] { doc_.ExecuteCommand("block.h2"); doc_.SetFocus(); };
        h3_.WhenAction = [=] { doc_.ExecuteCommand("block.h3"); doc_.SetFocus(); };
        indent_.WhenAction = [=] { doc_.ExecuteCommand("block.indent"); doc_.SetFocus(); };
        outdent_.WhenAction = [=] { doc_.ExecuteCommand("block.outdent"); doc_.SetFocus(); };
        quote_.WhenAction = [=] { doc_.ExecuteCommand("text.quote.wrap"); doc_.SetFocus(); };
        code_.WhenAction = [=] { doc_.ExecuteCommand("block.code"); doc_.SetFocus(); };

        table_.WhenAction = [=] {
            ValueArray args;
            args.Add(3);
            args.Add(3);
            doc_.ExecuteCommand("insert.table", args);
            doc_.SetFocus();
        };
        table_row_add_.WhenAction = [=] { doc_.ExecuteCommand("table.row.add"); doc_.SetFocus(); };
        table_row_del_.WhenAction = [=] { doc_.ExecuteCommand("table.row.remove"); doc_.SetFocus(); };
        table_col_add_.WhenAction = [=] { doc_.ExecuteCommand("table.col.add"); doc_.SetFocus(); };
        table_col_del_.WhenAction = [=] { doc_.ExecuteCommand("table.col.remove"); doc_.SetFocus(); };
        table_decor_.WhenAction = [=] { doc_.ExecuteCommand("table.decor.toggle"); doc_.SetFocus(); };
        table_del_.WhenAction = [=] { doc_.ExecuteCommand("table.delete"); doc_.SetFocus(); };
        hr_.WhenAction = [=] { doc_.ExecuteCommand("embed.hr.insert"); doc_.SetFocus(); };
        image_file_ins_.WhenAction = [=] {
            FileSel fs;
            fs.Type("Image files", "*.png *.jpg *.jpeg");
            if(!fs.ExecuteOpen()) {
                doc_.SetFocus();
                return;
            }
            String path = ~fs;
            String bytes = LoadFile(path);
            if(bytes.IsEmpty()) {
                doc_.SetFocus();
                return;
            }
            String ext = ToLower(GetFileExt(path));
            String mime = (ext == ".jpg" || ext == ".jpeg") ? "image/jpeg" : "image/png";
            Image img = StreamRaster::LoadFileAny(path);
            int w = 48;
            int h = 48;
            if(!img.IsEmpty()) {
                Size isz = img.GetSize();
                w = max(12, isz.cx);
                h = max(12, isz.cy);
            }
            String key = doc_.AddResource("image", bytes, mime, GetFileName(path), w, h, true);
            if(!key.IsEmpty()) {
                ValueMap add;
                add.Add("resource_key", key);
                add.Add("width", min(96, w));
                add.Add("height", min(64, h));
                if(!doc_.ExecuteCommand("table.cell.image.insert", add)) {
                    add.Add("pos", doc_.GetSelection().caret);
                    doc_.ExecuteCommand("embed.image.inline.insert", add);
                }
            }
            doc_.SetFocus();
        };
        image_flow_demo_.WhenAction = [=] {
            Image logo = Unmultiply(ICON_BRAND_NEWLOGO_V5_48());
            String png = PNGEncoder().SaveString(logo);
            Size sz = logo.GetSize();
            String key = doc_.AddResource("image", png, "image/png", "DATA_BRAND_NEWLOGO_V5_48", sz.cx, sz.cy, true);
            if(!key.IsEmpty()) {
                bool in_table = false;
                ValueMap add;
                add.Add("resource_key", key);
                add.Add("width", min(28, max(12, sz.cx)));
                add.Add("height", min(28, max(12, sz.cy)));
                if(doc_.ExecuteCommand("table.cell.image.insert", add)) {
                    in_table = true;
                    const char* a = "Before ";
                    for(int i = 0; a[i]; i++)
                        doc_.Key(a[i], 1);
                    doc_.ExecuteCommand("table.cell.image.insert", add);
                    const char* b = " After";
                    for(int i = 0; b[i]; i++)
                        doc_.Key(b[i], 1);
                }
                if(!in_table) {
                    int pos = doc_.GetSelection().caret;
                    ValueMap rep;
                    rep.Add("from", pos);
                    rep.Add("to", pos);
                    rep.Add("text", "Before After");
                    doc_.ExecuteCommand("doc.replace", rep);
                    add.Add("pos", pos + 7);
                    doc_.ExecuteCommand("embed.image.inline.insert", add);
                    doc_.SetSelection(UiDocRange(pos + 12, pos + 12));
                }
            }
            doc_.SetFocus();
        };
        image_align_left_.WhenAction = [=] { doc_.ExecuteCommand("embed.image.align.set", "left"); doc_.SetFocus(); };
        image_align_center_.WhenAction = [=] { doc_.ExecuteCommand("embed.image.align.set", "center"); doc_.SetFocus(); };
        image_align_right_.WhenAction = [=] { doc_.ExecuteCommand("embed.image.align.set", "right"); doc_.SetFocus(); };
        svg_ins_.WhenAction = [=] {
            UiDocSelection s = doc_.GetSelection();
            int from = min(s.anchor, s.caret);
            int to = max(s.anchor, s.caret);
            if(from == to)
                to = min(doc_.GetLength(), from + 1);
            ValueMap add;
            add.Add("from", from);
            add.Add("to", to);
            add.Add("type", "meta.demo");
            add.Add("text", "metadata attached");
            doc_.ExecuteCommand("annot.add", add);
            RefreshInspector();
            doc_.SetFocus();
        };
        meta_del_.WhenAction = [=] {
            int pos = doc_.GetSelection().caret;
            UiDocRange q(pos, pos + 1);
            Vector<UiDocAnnotation> aa = doc_.QueryAnnotations(&q);
            for(const UiDocAnnotation& a : aa) {
                if(a.type == "meta.demo") {
                    doc_.ExecuteCommand("annot.remove", a.id);
                    break;
                }
            }
            RefreshInspector();
            doc_.SetFocus();
        };
        embed_del_.WhenAction = [=] { doc_.ExecuteCommand("embed.delete.at_caret"); doc_.SetFocus(); };
        insp_margin_plus_.WhenAction = [=] { doc_.ExecuteCommand("block.indent.by", 1); doc_.SetFocus(); RefreshInspector(); };
        insp_margin_minus_.WhenAction = [=] { doc_.ExecuteCommand("block.outdent.by", 1); doc_.SetFocus(); RefreshInspector(); };
        insp_margin_reset_.WhenAction = [=] { doc_.ExecuteCommand("block.margin.reset"); doc_.SetFocus(); RefreshInspector(); };
        insp_lead_plus_.WhenAction = [=] { doc_.ExecuteCommand("text.leading.inc"); RefreshInspector(); doc_.SetFocus(); };
        insp_lead_minus_.WhenAction = [=] { doc_.ExecuteCommand("text.leading.dec"); RefreshInspector(); doc_.SetFocus(); };
        insp_track_plus_.WhenAction = [=] { doc_.ExecuteCommand("text.tracking.inc"); RefreshInspector(); doc_.SetFocus(); };
        insp_track_minus_.WhenAction = [=] { doc_.ExecuteCommand("text.tracking.dec"); RefreshInspector(); doc_.SetFocus(); };
        insp_tab_plus_.WhenAction = [=] { doc_.ExecuteCommand("doc.tab.inc"); RefreshInspector(); doc_.SetFocus(); };
        insp_tab_minus_.WhenAction = [=] { doc_.ExecuteCommand("doc.tab.dec"); RefreshInspector(); doc_.SetFocus(); };
        insp_tab_mode_.WhenAction = [=] { doc_.ExecuteCommand("doc.tab.mode", insp_tab_mode_.IsChecked()); RefreshInspector(); doc_.SetFocus(); };
        insp_tx_check_.WhenAction = [=] {
            int base_sel = selection_evt_count_;
            int base_chg = change_evt_count_;
            int base_map = mapped_evt_count_;

            UiDocSelection ss = doc_.GetSelection();
            UiDocRange r(ss.anchor, ss.caret);
            r.Normalize();
            if(r.IsEmpty()) {
                if(doc_.GetLength() > 0)
                    r = UiDocRange(0, min(1, doc_.GetLength()));
                else {
                    doc_.ExecuteCommand("doc.set_text", "batch-check");
                    r = UiDocRange(0, 1);
                }
            }

            UiDocTransaction tx;
            tx.add_to_history = true;
            for(int i = 0; i < 20; i++) {
                UiDocChange ch;
                ch.type = (i & 1) ? UiDocChange::SET_BOLD : UiDocChange::SET_ITALIC;
                ch.range = r;
                ch.enabled = ((i % 4) < 2);
                tx.changes.Add(pick(ch));
            }
            doc_.Dispatch(tx);

            int dsel = selection_evt_count_ - base_sel;
            int dchg = change_evt_count_ - base_chg;
            int dmap = mapped_evt_count_ - base_map;
            bool ok = (dchg == 1 && dsel <= 1 && dmap <= 1);
            strict_checks_status_ = String().Cat() << "TxCheck " << (ok ? "PASS" : "FAIL")
                                                   << " (chg=" << dchg << " sel=" << dsel << " map=" << dmap << ")";
            RefreshInspector();
            doc_.SetFocus();
        };
        insp_map_check_.WhenAction = [=] {
            if(doc_.GetLength() < 6)
                doc_.ExecuteCommand("doc.set_text", "abcdef");

            ValueMap add;
            add.Add("from", 0);
            add.Add("to", 3);
            add.Add("type", "check.map");
            ValueMap payload;
            add.Add("payload", payload);
            doc_.ExecuteCommand("annot.add", add);

            String aid;
            Vector<UiDocAnnotation> before = doc_.QueryAnnotations(nullptr, "check.map");
            if(!before.IsEmpty())
                aid = before.Top().id;

            UiDocTransaction tx;
            tx.add_to_history = true;
            UiDocChange ins;
            ins.type = UiDocChange::REPLACE_TEXT;
            ins.range = UiDocRange(0, 0);
            ins.text = "XY";
            tx.changes.Add(pick(ins));
            doc_.Dispatch(tx);

            bool ok = false;
            Vector<UiDocAnnotation> aa = doc_.QueryAnnotations(nullptr, "check.map");
            for(const UiDocAnnotation& a : aa) {
                if(a.id == aid) {
                    ok = (a.range.from == 0 && a.range.to == 5);
                    break;
                }
            }
            if(!aid.IsEmpty())
                doc_.ExecuteCommand("annot.remove", aid);
            strict_checks_status_ = String().Cat() << "MapCheck " << (ok ? "PASS" : "FAIL") << " (expect 0..5)";
            RefreshInspector();
            doc_.SetFocus();
        };
        insp_accept_check_.WhenAction = [=] {
            int pass = 0;
            int fail = 0;
            auto Check = [&](bool ok, const String& label) {
                if(ok) pass++;
                else {
                    fail++;
                    RLOG("UiDocDemo acceptance fail: " << label);
                }
            };

            doc_.ExecuteCommand("doc.set_text", "zero alpha beta gamma");
            active_comment_id_.Clear();
            active_comment_range_ = UiDocRange();

            ValueMap p;
            p.Add("text", "first");
            ValueMap addc;
            addc.Add("from", 5);
            addc.Add("to", 10);
            addc.Add("type", "note");
            addc.Add("payload", p);
            Check(doc_.ExecuteCommand("annot.add", addc), "add annotation command returns true");
            String ann_id;
            {
                Vector<UiDocAnnotation> tmp = doc_.QueryAnnotations(nullptr, "note");
                if(!tmp.IsEmpty())
                    ann_id = tmp[0].id;
            }
            Check(!ann_id.IsEmpty(), "add annotation yields id");

            Check(doc_.QueryAnnotations(nullptr, "note").GetCount() == 1, "add comment visible");
            Check(doc_.Undo(), "undo add comment returns true");
            Check(doc_.QueryAnnotations(nullptr, "note").IsEmpty(), "undo add removes comment");
            Check(doc_.Redo(), "redo add comment returns true");
            Check(doc_.QueryAnnotations(nullptr, "note").GetCount() == 1, "redo add restores comment");

            ValueMap upd;
            upd.Add("text", "edited");
            ValueMap upcmd;
            upcmd.Add("id", ann_id);
            upcmd.Add("payload", upd);
            Check(doc_.ExecuteCommand("annot.update", upcmd), "update annotation command returns true");
            String txt_now;
            Vector<UiDocAnnotation> aa = doc_.QueryAnnotations(nullptr, "note");
            for(const UiDocAnnotation& a : aa)
                if(a.id == ann_id && a.payload.Find("text") >= 0)
                    txt_now = AsString(a.payload["text"]);
            Check(txt_now == "edited", "update annotation text applied");

            Check(doc_.Undo(), "undo update returns true");
            txt_now.Clear();
            aa = doc_.QueryAnnotations(nullptr, "note");
            for(const UiDocAnnotation& a : aa)
                if(a.id == ann_id && a.payload.Find("text") >= 0)
                    txt_now = AsString(a.payload["text"]);
            Check(txt_now == "first", "undo update restores old text");

            Check(doc_.Redo(), "redo update returns true");
            txt_now.Clear();
            aa = doc_.QueryAnnotations(nullptr, "note");
            for(const UiDocAnnotation& a : aa)
                if(a.id == ann_id && a.payload.Find("text") >= 0)
                    txt_now = AsString(a.payload["text"]);
            Check(txt_now == "edited", "redo update restores edited text");

            ValueMap rep;
            rep.Add("from", 0);
            rep.Add("to", 0);
            rep.Add("text", "XX ");
            rep.Add("caret", 3);
            doc_.ExecuteCommand("doc.replace", rep);
            int rf = -1, rt = -1;
            aa = doc_.QueryAnnotations(nullptr, "note");
            for(const UiDocAnnotation& a : aa)
                if(a.id == ann_id) {
                    rf = a.range.from;
                    rt = a.range.to;
                }
            Check(rf == 8 && rt == 13, "insert-before remaps annotation to 8..13");

            ValueMap selm;
            selm.Add("anchor", 9);
            selm.Add("caret", 9);
            doc_.ExecuteCommand("doc.select", selm);
            SyncCommentEditorFromSelection();
            Check(active_comment_id_ == ann_id, "selection reveal focuses annotation");

            strict_checks_status_ = String().Cat() << "ACPT " << (fail == 0 ? "PASS" : "FAIL")
                                                    << " (pass=" << pass << " fail=" << fail << ")";
            RefreshInspector();
            doc_.SetFocus();
        };

        note_.WhenAction = [=] {
            SyncCommentEditorFromSelection();
            doc_.SetFocus();
            RefreshInspector();
        };

        comment_save_.WhenAction = [=] {
            UiDocSelection s = doc_.GetSelection();
            UiDocRange sel(s.anchor, s.caret);
            sel.Normalize();
            if(!sel.IsEmpty())
                active_comment_range_ = sel;

            if(active_comment_range_.IsEmpty())
                return;

            if(active_comment_id_.IsEmpty()) {
                Vector<UiDocAnnotation> aa = doc_.QueryAnnotations(&active_comment_range_, "note");
                if(!aa.IsEmpty())
                    active_comment_id_ = aa[0].id;
            }

            ValueMap meta;
            meta.Add("title", "Production note");
            meta.Add("created_by", "UiDocDemo");
            meta.Add("text", comment_edit_.GetText().ToString());
            if(!active_comment_id_.IsEmpty()) {
                ValueMap upd;
                upd.Add("id", active_comment_id_);
                upd.Add("payload", meta);
                doc_.ExecuteCommand("annot.update", upd);
                ValueMap ex;
                ex.Add("id", active_comment_id_);
                ex.Add("expanded", true);
                doc_.ExecuteCommand("annot.expanded", ex);
            }
            else {
                ValueMap add;
                add.Add("from", active_comment_range_.from);
                add.Add("to", active_comment_range_.to);
                add.Add("type", "note");
                add.Add("payload", meta);
                doc_.ExecuteCommand("annot.add", add);
            }
            SyncCommentEditorFromSelection();
            doc_.SetFocus();
            RefreshInspector();
        };

        comment_delete_.WhenAction = [=] {
            if(active_comment_id_.IsEmpty()) {
                SyncCommentEditorFromSelection();
            }
            if(!active_comment_id_.IsEmpty())
                doc_.ExecuteCommand("annot.remove", active_comment_id_);
            active_comment_id_.Clear();
            comment_edit_.SetText(String().ToWString());
            SyncCommentEditorFromSelection();
            doc_.SetFocus();
            RefreshInspector();
        };

        comment_close_.WhenAction = [=] {
            if(active_comment_id_.IsEmpty())
                return;
            bool expanded = true;
            Vector<UiDocAnnotation> all = doc_.QueryAnnotations(nullptr, "note");
            for(const UiDocAnnotation& a : all)
                if(a.id == active_comment_id_)
                    expanded = a.expanded;
            ValueMap ex;
            ex.Add("id", active_comment_id_);
            ex.Add("expanded", !expanded);
            doc_.ExecuteCommand("annot.expanded", ex);
            SyncCommentEditorFromSelection();
            doc_.SetFocus();
            RefreshInspector();
        };

        comment_pick_.WhenAction = [=] {
            if(active_comment_ids_.IsEmpty())
                return;
            String id = AsString(comment_pick_.GetData());
            if(id.IsEmpty())
                return;
            active_comment_id_ = id;
            UiDocRange rr;
            bool found = false;
            Vector<UiDocAnnotation> all = doc_.QueryAnnotations(nullptr, "note");
            for(const UiDocAnnotation& a : all) {
                if(a.id != id)
                    continue;
                rr = a.range;
                found = true;
                break;
            }
            if(found)
                active_comment_range_ = rr;
            SyncCommentEditorFromSelection();
            RefreshInspector();
        };

        find_.WhenAction = [=] {
            String raw = TrimBoth(search_.GetText().ToString());
            String q = NormalizeFindInput(raw);
            if(q != doc_.GetSearchQuery())
                doc_.SetSearchQuery(q);
            bool ok = doc_.FindNext();
            if(q.IsEmpty())
                find_status_ = "Find: empty query";
            else if(!ok)
                find_status_ = "Find '" + q + "': 0 matches";
            else
                find_status_ = "Find '" + q + "': " + AsString(doc_.GetSearchMatchIndex() + 1) + "/" + AsString(doc_.GetSearchMatchCount());
            if(raw != q)
                find_status_ << " (normalized)";
            RefreshInspector();
            doc_.SetFocus();
        };
        find_prev_.WhenAction = [=] {
            String raw = TrimBoth(search_.GetText().ToString());
            String q = NormalizeFindInput(raw);
            if(q != doc_.GetSearchQuery())
                doc_.SetSearchQuery(q);
            bool ok = doc_.FindPrev();
            if(q.IsEmpty())
                find_status_ = "Find: empty query";
            else if(!ok)
                find_status_ = "Find '" + q + "': 0 matches";
            else
                find_status_ = "Find '" + q + "': " + AsString(doc_.GetSearchMatchIndex() + 1) + "/" + AsString(doc_.GetSearchMatchCount());
            if(raw != q)
                find_status_ << " (normalized)";
            RefreshInspector();
            doc_.SetFocus();
        };
        search_.WhenAction = find_.WhenAction;
        find_ignore_case_.WhenAction = [=] {
            doc_.SetSearchIgnoreCase(find_ignore_case_.IsChecked());
            find_.WhenAction();
            doc_.SetFocus();
        };
        find_whole_word_.WhenAction = [=] {
            doc_.SetSearchWholeWord(find_whole_word_.IsChecked());
            find_.WhenAction();
            doc_.SetFocus();
        };

        view_gutter_side_.WhenAction = [=] {
            UiDoc::GutterSide side = doc_.GetGutterSide();
            bool to_right = (side == UiDoc::GUTTER_LEFT);
            doc_.SetGutterSide(to_right ? UiDoc::GUTTER_RIGHT : UiDoc::GUTTER_LEFT);
            view_gutter_side_.SetIcon(to_right ? ICON_EDITOR_BORDER_RIGHT_48() : ICON_EDITOR_BORDER_LEFT_48());
            doc_.SetFocus();
        };
        view_line_numbers_.WhenAction = [=] {
            doc_.ShowLineNumbers(view_line_numbers_.IsChecked());
            doc_.SetFocus();
        };
        view_meta_markers_.WhenAction = [=] {
            doc_.ShowMetadataMarkers(view_meta_markers_.IsChecked());
            doc_.SetFocus();
        };

        doc_.WhenSelection = [=] {
            selection_evt_count_++;
            RefreshListButtons();
            SyncCommentEditorFromSelection();
            RefreshInspector();
        };

        doc_.WhenChange = [=] {
            change_evt_count_++;
            RefreshInspector();
        };
        doc_.WhenMapped = [=](const UiDocPositionMap& m) {
            mapped_evt_count_++;
            mapped_edit_total_ += m.edits.GetCount();
            RefreshInspector();
        };
        doc_.WhenSearch = [=](const String& q) {
            find_status_ = "Find '" + q + "': " + AsString(doc_.GetSearchMatchCount()) + " matches";
            RefreshInspector();
        };

        doc_.SetGutterSide(UiDoc::GUTTER_LEFT);
        doc_.ShowLineNumbers(false);
        doc_.ShowMetadataMarkers(true);

        doc_.SetText(
            "UiDoc demo controls:\n"
            "- rich formatting (bold/italic/underline/strike, upper/lower/title, A+/A-)\n"
            "- comments, headings H1/H2/H3, quote/code\n"
            "- list modes (bullet/numbered) with Enter continuation and empty-item exit\n"
            "- bullet style: circle or dash\n"
            "- embedded table create + row/col edits + Tab/Shift+Tab cell navigation\n"
            "- embed insert: HR, image, SVG + delete at caret\n"
            "- search with glob support (*, ?)\n\n"
            "Try selecting several lines and press Tab / Shift+Tab.\n");

        RefreshListButtons();
        TickModifierState();
        RefreshInspector();
    }

    void Layout() override
    {
        Rect r = GetSize();
        int pad = DPI(8);
        int row_h = DPI(30);

        const int toolbar_w = max(1, r.GetWidth() - 2 * pad);
        int toolbar_h = max(toolbar_.MeasureHeightForWidth(toolbar_w) + DPI(6), row_h + DPI(20));
        toolbar_.SetRect(pad, pad, toolbar_w, toolbar_h);

        Rect doc_r = r;
        doc_r.top = toolbar_.GetRect().bottom + DPI(8);
        doc_r.Deflate(pad, pad);

        int insp_w = DPI(250);
        Rect insp = doc_r;
        insp.left = max(insp.left, insp.right - insp_w);
        doc_r.right = insp.left - DPI(8);
        doc_.SetRect(doc_r);

        int iy = insp.top;
        int ih = DPI(28);
        inspector_title_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih + DPI(2);
        inspector_state_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih;
        inspector_modes_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih;
        inspector_block_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih;
        inspector_find_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih;
        inspector_margin_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih + DPI(6);
        inspector_meta_.SetRect(insp.left, iy, insp.GetWidth(), ih); iy += ih + DPI(6);
        int bw = (insp.GetWidth() - DPI(8)) / 3;
        insp_margin_plus_.SetRect(insp.left, iy, bw, ih);
        insp_margin_minus_.SetRect(insp.left + bw + DPI(4), iy, bw, ih);
        insp_margin_reset_.SetRect(insp.left + 2 * (bw + DPI(4)), iy, bw, ih);
        iy += ih + DPI(4);
        int bw2 = (insp.GetWidth() - DPI(4)) / 2;
        insp_lead_plus_.SetRect(insp.left, iy, bw2, ih);
        insp_lead_minus_.SetRect(insp.left + bw2 + DPI(4), iy, bw2, ih);
        iy += ih + DPI(4);
        insp_track_plus_.SetRect(insp.left, iy, bw2, ih);
        insp_track_minus_.SetRect(insp.left + bw2 + DPI(4), iy, bw2, ih);
        iy += ih + DPI(4);
        insp_tab_plus_.SetRect(insp.left, iy, bw2, ih);
        insp_tab_minus_.SetRect(insp.left + bw2 + DPI(4), iy, bw2, ih);
        iy += ih + DPI(4);
        insp_tab_mode_.SetRect(insp.left, iy, insp.GetWidth(), ih);
        iy += ih + DPI(4);
        int bw3 = (insp.GetWidth() - DPI(8)) / 3;
        insp_tx_check_.SetRect(insp.left, iy, bw3, ih);
        insp_map_check_.SetRect(insp.left + bw3 + DPI(4), iy, bw3, ih);
        insp_accept_check_.SetRect(insp.left + 2 * (bw3 + DPI(4)), iy, bw3, ih);
        iy += ih + DPI(4);
        inspector_checks_.SetRect(insp.left, iy, insp.GetWidth(), ih);
        iy += ih + DPI(6);
        comment_title_.SetRect(insp.left, iy, insp.GetWidth(), ih);
        iy += ih + DPI(2);
        comment_pick_.SetRect(insp.left, iy, insp.GetWidth(), ih);
        iy += ih + DPI(2);
        comment_edit_.SetRect(insp.left, iy, insp.GetWidth(), DPI(76));
        iy += DPI(76) + DPI(4);
        int cbw = (insp.GetWidth() - DPI(8)) / 3;
        comment_save_.SetRect(insp.left, iy, cbw, ih);
        comment_delete_.SetRect(insp.left + cbw + DPI(4), iy, cbw, ih);
        comment_close_.SetRect(insp.left + 2 * (cbw + DPI(4)), iy, cbw, ih);
        iy += ih + DPI(4);
        inspector_keys_.SetRect(insp.left, iy, insp.GetWidth(), ih);
    }

    void RefreshInspector()
    {
        RefreshListButtons();
        inspector_state_.SetText(Format("Line:%d Sel:%d Chg:%d Map:%d", doc_.GetCurrentLine() + 1, selection_evt_count_, change_evt_count_, mapped_evt_count_).ToWString());
        inspector_modes_.SetText(Format("Bullet:%s Number:%s", doc_.IsBulletMode() ? "on" : "off", doc_.IsNumberedMode() ? "on" : "off").ToWString());
        int line = doc_.GetCurrentLine();
        Vector<UiDocBlockRecord> blocks = doc_.GetBlocks();
        String b = "Block:n/a";
        if(line >= 0 && line < blocks.GetCount()) {
            const UiDocBlockRecord& r = blocks[line];
            b = Format("Block:t%d l%d c%d tid:%d tc:%d me:%d", r.block_type, (int)r.list_kind, (int)r.commented, r.table_id, r.table_cols, mapped_edit_total_);
        }
        inspector_block_.SetText(b.ToWString());
        String fmode = String().Cat() << "mode: ignore-case:" << (doc_.IsSearchIgnoreCase() ? "on" : "off")
                                      << ", whole-word:" << (doc_.IsSearchWholeWord() ? "on" : "off");
        inspector_find_.SetText((find_status_ + " | " + fmode).ToWString());
        inspector_margin_.SetText(Format("Margin:%d Lead:%d Track:%d", doc_.GetCurrentParagraphMarginSteps(), doc_.GetCurrentLeadingDelta(), doc_.GetCurrentTrackingDelta()).ToWString());
        UiDocSelection sel = doc_.GetSelection();
        UiDocRange rr(sel.anchor, sel.caret);
        rr.Normalize();
        if(rr.IsEmpty())
            rr = UiDocRange(doc_.GetSelection().caret, min(doc_.GetSelection().caret + 1, doc_.GetLength()));
        Vector<UiDocAnnotation> anns = doc_.QueryAnnotations(&rr);
        if(anns.IsEmpty())
            inspector_meta_.SetText("Meta: none");
        else {
            const UiDocAnnotation& a = anns[0];
            inspector_meta_.SetText(Format("Meta:%d first:%s", anns.GetCount(), a.type).ToWString());
        }
        insp_tab_mode_.SetChecked(doc_.IsInsertTabAsSpaces());
        inspector_checks_.SetText(strict_checks_status_.ToWString());
        UpdateModifierState();
    }

    void RefreshListButtons()
    {
        Vector<UiDocBlockRecord> blocks = doc_.GetBlocks();
        UiDocSelection s = doc_.GetSelection();
        UiDocRange rr(s.anchor, s.caret);
        rr.Normalize();

        int first_line = doc_.GetCurrentLine();
        int last_line = first_line;

        if(!rr.IsEmpty() && !blocks.IsEmpty()) {
            first_line = 0;
            last_line = (int)blocks.GetCount() - 1;
            for(int i = 0; i < blocks.GetCount(); i++) {
                if(blocks[i].pos_to > rr.from) {
                    first_line = i;
                    break;
                }
            }
            for(int i = first_line; i < blocks.GetCount(); i++) {
                if(blocks[i].pos_from >= rr.to) {
                    last_line = max(first_line, i - 1);
                    break;
                }
            }
        }

        first_line = max(0, first_line);
        last_line = max(first_line, last_line);
        if(!blocks.IsEmpty()) {
            first_line = min(first_line, blocks.GetCount() - 1);
            last_line = min(last_line, blocks.GetCount() - 1);
        }

        bool all_bullet = !blocks.IsEmpty();
        bool all_number = !blocks.IsEmpty();
        for(int line = first_line; line <= last_line && line < blocks.GetCount(); line++) {
            all_bullet = all_bullet && blocks[line].list_kind == 1;
            all_number = all_number && blocks[line].list_kind == 2;
        }

        bullet_.SetChecked(all_bullet);
        numbered_.SetChecked(all_number);

        Color idle = Color(28, 31, 38);
        Color on = Color(32, 107, 196);
        bullet_.SetIconColor(all_bullet ? on : idle, 10, -8);
        numbered_.SetIconColor(all_number ? on : idle, 10, -8);
    }

    void SyncCommentEditorFromSelection()
    {
        UiDocSelection s = doc_.GetSelection();
        UiDocRange rr(s.anchor, s.caret);
        rr.Normalize();
        if(rr.IsEmpty())
            rr = UiDocRange(s.caret, min(s.caret + 1, doc_.GetLength()));

        Vector<UiDocAnnotation> anns = doc_.QueryAnnotations(&rr, "note");
        if(!anns.IsEmpty()) {
            int pick = 0;
            for(int i = 0; i < anns.GetCount(); i++) {
                if(anns[i].range.from <= s.caret && s.caret < anns[i].range.to) {
                    pick = i;
                    break;
                }
            }

            active_comment_id_ = anns[pick].id;
            active_comment_range_ = anns[pick].range;
            active_comment_ids_.Clear();
            comment_pick_.Clear();
            for(int i = 0; i < anns.GetCount(); i++) {
                const UiDocAnnotation& x = anns[i];
                String label = Format("%s [%d..%d]", x.id, x.range.from, x.range.to);
                if(x.resolved)
                    label << " (resolved)";
                comment_pick_.Add(label, x.id);
                active_comment_ids_.Add(x.id);
            }
            comment_pick_.SetData(active_comment_id_);
            String text;
            if(anns[pick].payload.Find("text") >= 0)
                text = AsString(anns[pick].payload["text"]);
            comment_edit_.SetText(text.ToWString());
            comment_title_.SetText(Format("Comment note [%s %d]", anns[pick].expanded ? "ACTIVE" : "CLOSED", anns.GetCount()));
            comment_edit_.Enable(anns[pick].expanded);
            comment_save_.Enable(anns[pick].expanded);
            comment_close_.SetText(anns[pick].expanded ? "Close" : "Open");
            comment_delete_.Enable();
            comment_close_.Enable();
            return;
        }

        active_comment_id_.Clear();
        active_comment_ids_.Clear();
        comment_pick_.Clear();
        UiDocRange sel(s.anchor, s.caret);
        sel.Normalize();
        active_comment_range_ = sel;
        if(!active_comment_range_.IsEmpty()) {
            comment_title_.SetText("Comment note [NEW]");
            comment_edit_.SetText(String().ToWString());
        }
        else {
            comment_title_.SetText("Comment note");
            comment_edit_.SetText(String().ToWString());
        }
        comment_edit_.Enable();
        comment_save_.Enable();
        comment_close_.SetText("Close");
        comment_delete_.Disable();
        comment_close_.Disable();
    }

    void UpdateModifierState()
    {
        String t;
        t << "Mods: Ctrl " << (GetCtrl() ? "on" : "off")
          << " Alt " << (GetAlt() ? "on" : "off")
          << " Shift " << (GetShift() ? "on" : "off");
        inspector_keys_.SetText(t.ToWString());
    }

    void TickModifierState()
    {
        UpdateModifierState();
        SetTimeCallback(70, [=] { TickModifierState(); }, 9101);
    }

private:
    UiGridLayout toolbar_;

    UiButton   bold_;
    UiButton   italic_;
    UiButton   underline_;
    UiButton   strike_;
    UiButton   upper_;
    UiButton   lower_;
    UiButton   titlecase_;
    UiButton   size_up_;
    UiButton   size_down_;
    UiButton   comment_;

    UiButton   bullet_;
    UiButton   numbered_;
    UiButton   bullet_circle_;
    UiButton   bullet_dash_;

    UiButton   h1_;
    UiButton   h2_;
    UiButton   h3_;
    UiButton   indent_;
    UiButton   outdent_;
    UiButton   quote_;
    UiButton   code_;

    UiButton   table_;
    UiButton   table_row_add_;
    UiButton   table_row_del_;
    UiButton   table_col_add_;
    UiButton   table_col_del_;
    UiButton   table_decor_;
    UiButton   table_del_;
    UiButton   hr_;
    UiButton   image_file_ins_;
    UiButton   image_flow_demo_;
    UiButton   image_align_left_;
    UiButton   image_align_center_;
    UiButton   image_align_right_;
    UiButton   svg_ins_;
    UiButton   meta_del_;
    UiButton   embed_del_;

    UiButton   note_;
    UiLineEdit search_;
    UiButton   find_prev_;
    UiButton   find_;
    UiButton   find_ignore_case_;
    UiButton   find_whole_word_;
    UiButton   view_gutter_side_;
    UiButton   view_line_numbers_;
    UiButton   view_meta_markers_;
    UiLabel    inspector_title_;
    UiLineEdit inspector_state_;
    UiLineEdit inspector_modes_;
    UiLineEdit inspector_block_;
    UiLineEdit inspector_find_;
    UiLineEdit inspector_margin_;
    UiLineEdit inspector_meta_;
    UiButton   insp_margin_plus_;
    UiButton   insp_margin_minus_;
    UiButton   insp_margin_reset_;
    UiButton   insp_lead_plus_;
    UiButton   insp_lead_minus_;
    UiButton   insp_track_plus_;
    UiButton   insp_track_minus_;
    UiButton   insp_tab_plus_;
    UiButton   insp_tab_minus_;
    UiButton   insp_tab_mode_;
    UiButton   insp_tx_check_;
    UiButton   insp_map_check_;
    UiButton   insp_accept_check_;
    UiLineEdit inspector_checks_;
    UiLineEdit inspector_keys_;
    UiLabel    comment_title_;
    UiDropdown comment_pick_;
    UiMultiEdit comment_edit_;
    UiButton   comment_save_;
    UiButton   comment_delete_;
    UiButton   comment_close_;
    String     find_status_;
    String     strict_checks_status_ = "Checks: idle";
    UiDocRange active_comment_range_;
    String     active_comment_id_;
    Vector<String> active_comment_ids_;
    int        selection_evt_count_ = 0;
    int        change_evt_count_ = 0;
    int        mapped_evt_count_ = 0;
    int        mapped_edit_total_ = 0;
    UiDoc      doc_;
};

GUI_APP_MAIN
{
    StdLogSetup(LOG_COUT|LOG_FILE);
    UiDocDemoWindow().Run();
}
