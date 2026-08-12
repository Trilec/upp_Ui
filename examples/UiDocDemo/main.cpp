/*
    UiDocDemo
    ---------

    Purpose
    - Polished application-style showcase for UiDoc v2.
    - Demonstrates the real UiDoc/UiDocCore public surface in a familiar
      ribbon-style document workspace.

    Design
    - UiTitleCard provides the application header and quick file actions.
    - UiTab provides text ribbon tabs (Home / Insert / Review / View).
    - UiGroupPanel and UiBoxLayout compose the ribbon groups and workspace.
    - UiDoc remains the real editor; no parallel document model is maintained.
    - The comments pane is optional and uses UiDoc review annotations directly.
*/

#include <Ui/Ui.h>
#include <Ui/UiIcons.h>

using namespace Upp;

namespace {

UiDocRange OrderedSelection(const UiDoc& doc)
{
    UiDocSelection selection = doc.GetSelection();
    return UiDocRange(min(selection.anchor, selection.caret),
                      max(selection.anchor, selection.caret));
}

int CountWords(const WString& text)
{
    int words = 0;
    bool in_word = false;
    for(int i = 0; i < text.GetCount(); i++) {
        wchar ch = text[i];
        bool word = IsAlNum((int)ch) || ch == '_';
        if(word && !in_word)
            words++;
        in_word = word;
    }
    return words;
}

void SetTableCellText(UiDocTable& table, int row, int column,
                      const String& text, bool bold = false)
{
    if(row < 0 || row >= table.rows.GetCount() ||
       column < 0 || column >= table.columns)
        return;

    UiDocTableCell& cell = table.rows[row].cells[column];
    cell.runs.Clear();

    UiDocInlineRun run;
    run.type = "text";
    run.text = ToUnicode(text, CHARSET_UTF8);
    if(bold)
        run.style.flags |= UiDocTextStyle::BOLD;
    cell.runs.Add(pick(run));
}

}

class UiDocDemoWindow : public TopWindow {
public:
    typedef UiDocDemoWindow CLASSNAME;

    UiDocDemoWindow()
    {
        Title("UiDoc - Rich Document Editor");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(1380), DPI(900));

        Add(box_root.SizePos());

        BuildApplicationShell();
        BuildRibbon();
        BuildWorkspace();
        BuildStatusBar();
        WireActions();
        LoadWelcomeDocument();
    }

private:
    UiBoxLayout box_root;
    UiTitleCard tc_app;
    UiBoxLayout box_quick_actions;
    UiButton btn_new;
    UiButton btn_open;
    UiButton btn_save;
    UiButton btn_save_as;
    UiButton btn_undo;
    UiButton btn_redo;

    UiTab tab_ribbon;
    UiBoxLayout box_home;
    UiBoxLayout box_insert;
    UiBoxLayout box_review;
    UiBoxLayout box_view;

    UiGroupPanel gp_clipboard;
    UiBoxLayout box_clipboard;
    UiButton btn_cut;
    UiButton btn_copy;
    UiButton btn_paste;

    UiGroupPanel gp_font;
    UiBoxLayout box_font;
    UiBoxLayout box_font_selectors;
    UiBoxLayout box_font_marks;
    UiDropdown drop_font;
    UiDropdown drop_size;
    UiButton btn_size_up;
    UiButton btn_size_down;
    UiButton btn_bold;
    UiButton btn_italic;
    UiButton btn_underline;
    UiButton btn_strike;
    UiButton btn_upper;
    UiButton btn_lower;

    UiGroupPanel gp_paragraph;
    UiBoxLayout box_paragraph;
    UiBoxLayout box_paragraph_top;
    UiBoxLayout box_paragraph_bottom;
    UiButton btn_bullets;
    UiButton btn_numbering;
    UiButton btn_quote;
    UiButton btn_code;
    UiButton btn_indent;
    UiButton btn_outdent;

    UiGroupPanel gp_styles;
    UiBoxLayout box_styles;
    UiBoxLayout box_style_buttons;
    UiButton btn_normal;
    UiButton btn_heading1;
    UiButton btn_heading2;
    UiDropdown drop_style;

    UiGroupPanel gp_editing;
    UiBoxLayout box_editing;
    UiBoxLayout box_find_row;
    UiBoxLayout box_replace_row;
    UiLineEdit edit_find;
    UiLineEdit edit_replace;
    UiButton btn_find_prev;
    UiButton btn_find_next;
    UiButton btn_replace_current;
    UiButton btn_replace_all;
    UiButton btn_ignore_case;
    UiButton btn_whole_word;

    UiGroupPanel gp_pages;
    UiBoxLayout box_pages;
    UiButton btn_page_break;
    UiButton btn_rule;

    UiGroupPanel gp_table;
    UiBoxLayout box_table;
    UiBoxLayout box_table_top;
    UiBoxLayout box_table_bottom;
    UiButton btn_insert_table;
    UiButton btn_row_add;
    UiButton btn_row_remove;
    UiButton btn_column_add;
    UiButton btn_column_remove;
    UiButton btn_remove_embed;

    UiGroupPanel gp_picture;
    UiBoxLayout box_picture;
    UiBoxLayout box_picture_top;
    UiBoxLayout box_picture_bottom;
    UiButton btn_insert_picture;
    UiButton btn_image_left;
    UiButton btn_image_center;
    UiButton btn_image_right;
    UiButton btn_remove_picture;

    UiGroupPanel gp_screenplay;
    UiBoxLayout box_screenplay;
    UiButton btn_scene;
    UiButton btn_action;
    UiButton btn_character;
    UiButton btn_dialogue;

    UiGroupPanel gp_review_comments;
    UiBoxLayout box_review_comments;
    UiButton btn_new_comment;
    UiButton btn_comments_pane;
    UiButton btn_resolve_comment;
    UiButton btn_delete_comment;

    UiGroupPanel gp_review_info;
    UiBoxLayout box_review_info;
    UiLabel lbl_review_model;
    UiLabel lbl_review_storage;
    UiLabel lbl_review_revision;

    UiGroupPanel gp_view_gutter;
    UiBoxLayout box_view_gutter;
    UiButton btn_line_numbers;
    UiButton btn_metadata_markers;
    UiButton btn_gutter_side;

    UiGroupPanel gp_view_workspace;
    UiBoxLayout box_view_workspace;
    UiButton btn_comments_pane_view;
    UiButton btn_home_tab;
    UiButton btn_select_all;

    UiGroupPanel gp_view_about;
    UiBoxLayout box_view_about;
    UiLabel lbl_view_layout;
    UiLabel lbl_view_scale;

    UiPanel pnl_workspace;
    UiBoxLayout box_workspace;
    UiBoxLayout box_document_frame;
    UiDoc doc_editor;

    UiGroupPanel gp_comments;
    UiBoxLayout box_comments;
    UiButton btn_close_comments;
    UiLabel lbl_comment_count;
    UiLabel lbl_comment_context;
    UiLineEdit edit_comment;
    UiBoxLayout box_comment_actions;
    UiButton btn_comment_new;
    UiButton btn_comment_update;
    UiButton btn_comment_resolve;
    UiButton btn_comment_delete;

    UiPanel pnl_status;
    UiBoxLayout box_status;
    UiLabel lbl_status_left;
    UiLabel lbl_status_center;
    UiLabel lbl_status_right;

    String current_path;
    String active_comment_id;
    String status_message;
    bool dirty = false;
    bool suppress_dirty = false;
    bool comments_visible = false;

private:
    void BuildApplicationShell()
    {
        box_root.SetDirection(UiDirection::V)
                .SetGap(0)
                .SetInset(0);

        box_quick_actions.SetDirection(UiDirection::H)
                         .SetGap(DPI(5))
                         .SetInset(0)
                         .SetAlignItems(UiCrossAlign::Center);

        ConfigureTextButton(btn_new, "New");
        ConfigureTextButton(btn_open, "Open");
        ConfigureTextButton(btn_save, "Save");
        ConfigureTextButton(btn_save_as, "Save As");
        ConfigureTextButton(btn_undo, "Undo");
        ConfigureTextButton(btn_redo, "Redo");

        box_quick_actions.Add(btn_new).Fixed(DPI(58));
        box_quick_actions.Add(btn_open).Fixed(DPI(62));
        box_quick_actions.Add(btn_save).Fixed(DPI(58));
        box_quick_actions.Add(btn_save_as).Fixed(DPI(76));
        box_quick_actions.AddSpacer(1);
        box_quick_actions.Add(btn_undo).Fixed(DPI(60));
        box_quick_actions.Add(btn_redo).Fixed(DPI(60));

        tc_app.SetTitle("UiDoc")
              .SetSubTitle("Rich document editor")
              .SetCopyText("Untitled.uidoc")
              .SetMedia(ICON_DESIGN_DESCRIPTION_48(), Size(DPI(40), DPI(40)))
              .SetMediaReserve(DPI(48))
              .SetMediaMin(DPI(30))
              .SetMediaGap(DPI(10))
              .SetContentInset(Rect(DPI(14), DPI(8), DPI(14), DPI(8)))
              .SetContentCell(box_quick_actions)
              .SetContentCellGap(DPI(18))
              .SetSelectable(false)
              .ShowTitleLine(false)
              .SetCardLine(LARGE, DPI(1), SOLID)
              .SetCardLineSide(UiAlign::BOTTOM);

        box_root.Add(tc_app).Fit();
        box_root.Add(tab_ribbon).Fixed(DPI(176));
        box_root.Add(pnl_workspace).Expand(1);
        box_root.Add(pnl_status).Fixed(DPI(30));
    }

    void BuildRibbon()
    {
        tab_ribbon.SetPlacement(UiAlign::TOP)
                  .SetVisual(UITAB_UNDERLINE)
                  .SetTabFont(SansSerifZ(DPI(10)).Bold())
                  .SetTabIconSize(0)
                  .SetExpandTabs(false)
                  .SetActiveTabUsesBodyFace(true);

        SetupRibbonPage(box_home);
        SetupRibbonPage(box_insert);
        SetupRibbonPage(box_review);
        SetupRibbonPage(box_view);

        BuildHomeRibbon();
        BuildInsertRibbon();
        BuildReviewRibbon();
        BuildViewRibbon();

        tab_ribbon.Add(box_home, "Home");
        tab_ribbon.Add(box_insert, "Insert");
        tab_ribbon.Add(box_review, "Review");
        tab_ribbon.Add(box_view, "View");
        tab_ribbon.SetActiveTab(0);
    }

    void BuildHomeRibbon()
    {
        SetupRibbonGroup(gp_clipboard, box_clipboard, "Clipboard", UiDirection::H);
        ConfigureRibbonButton(btn_cut, "Cut");
        ConfigureRibbonButton(btn_copy, "Copy");
        ConfigureRibbonButton(btn_paste, "Paste");
        box_clipboard.Add(btn_cut).Fixed(DPI(48));
        box_clipboard.Add(btn_copy).Fixed(DPI(54));
        box_clipboard.Add(btn_paste).Fixed(DPI(58));

        SetupRibbonGroup(gp_font, box_font, "Font", UiDirection::V);
        box_font_selectors.SetDirection(UiDirection::H)
                          .SetGap(DPI(4))
                          .SetInset(0)
                          .SetAlignItems(UiCrossAlign::Center);
        box_font_marks.SetDirection(UiDirection::H)
                      .SetGap(DPI(3))
                      .SetInset(0)
                      .SetAlignItems(UiCrossAlign::Center);

        drop_font.UseInternalModel();
        drop_font.SetPlaceholderText("Font")
                 .SetPopupMaxItems(8);
        drop_font.Add("Segoe UI", String("Segoe UI"));
        drop_font.Add("Arial", String("Arial"));
        drop_font.Add("Times New Roman", String("Times New Roman"));
        drop_font.Add("Courier New", String("Courier New"));
        drop_font.Add("Consolas", String("Consolas"));
        drop_font.SelectByData(String("Segoe UI"));

        drop_size.UseInternalModel();
        drop_size.SetPlaceholderText("Size")
                 .SetPopupMaxItems(8);
        const int sizes[] = { 9, 10, 11, 12, 14, 18, 24, 32 };
        for(int size : sizes)
            drop_size.Add(AsString(size), size);
        drop_size.SelectByData(11);

        ConfigureIconButton(btn_size_up, ICON_CONTENT_OUTLINED_ADD_48(), "Increase font size");
        ConfigureIconButton(btn_size_down, ICON_DESIGN_TEXT_DECREASE_48(), "Decrease font size");
        ConfigureIconButton(btn_bold, ICON_EDITOR_FORMAT_BOLD_48(), "Bold");
        ConfigureIconButton(btn_italic, ICON_EDITOR_FORMAT_ITALIC_48(), "Italic");
        ConfigureIconButton(btn_underline, ICON_EDITOR_SERIF_48(), "Underline");
        ConfigureIconButton(btn_strike, ICON_EDITOR_FORMAT_STRIKETHROUGH_48(), "Strikeout");
        ConfigureTextButton(btn_upper, "UP");
        ConfigureTextButton(btn_lower, "low");

        btn_bold.SetCheckable();
        btn_italic.SetCheckable();
        btn_underline.SetCheckable();
        btn_strike.SetCheckable();

        box_font_selectors.Add(drop_font).Fixed(DPI(132));
        box_font_selectors.Add(drop_size).Fixed(DPI(58));
        box_font_selectors.Add(btn_size_up).Fixed(DPI(32));
        box_font_selectors.Add(btn_size_down).Fixed(DPI(32));

        box_font_marks.Add(btn_bold).Fixed(DPI(32));
        box_font_marks.Add(btn_italic).Fixed(DPI(32));
        box_font_marks.Add(btn_underline).Fixed(DPI(32));
        box_font_marks.Add(btn_strike).Fixed(DPI(32));
        box_font_marks.Add(btn_upper).Fixed(DPI(38));
        box_font_marks.Add(btn_lower).Fixed(DPI(38));

        box_font.Add(box_font_selectors).Fit();
        box_font.Add(box_font_marks).Fit();

        SetupRibbonGroup(gp_paragraph, box_paragraph, "Paragraph", UiDirection::V);
        box_paragraph_top.SetDirection(UiDirection::H)
                         .SetGap(DPI(3))
                         .SetInset(0)
                         .SetAlignItems(UiCrossAlign::Center);
        box_paragraph_bottom.SetDirection(UiDirection::H)
                            .SetGap(DPI(3))
                            .SetInset(0)
                            .SetAlignItems(UiCrossAlign::Center);

        ConfigureIconButton(btn_bullets, ICON_EDITOR_FORMAT_LIST_BULLETED_48(), "Bulleted list");
        ConfigureIconButton(btn_numbering, ICON_EDITOR_FORMAT_LIST_NUMBERED_RTL_48(), "Numbered list");
        ConfigureIconButton(btn_quote, ICON_EDITOR_FORMAT_QUOTE_48(), "Quote");
        ConfigureIconButton(btn_code, ICON_EDITOR_CLARIFY_48(), "Code block");
        ConfigureTextButton(btn_indent, "Indent");
        ConfigureTextButton(btn_outdent, "Reset");

        box_paragraph_top.Add(btn_bullets).Fixed(DPI(34));
        box_paragraph_top.Add(btn_numbering).Fixed(DPI(34));
        box_paragraph_top.Add(btn_quote).Fixed(DPI(34));
        box_paragraph_top.Add(btn_code).Fixed(DPI(34));
        box_paragraph_bottom.Add(btn_indent).Fixed(DPI(58));
        box_paragraph_bottom.Add(btn_outdent).Fixed(DPI(52));
        box_paragraph.Add(box_paragraph_top).Fit();
        box_paragraph.Add(box_paragraph_bottom).Fit();

        SetupRibbonGroup(gp_styles, box_styles, "Styles", UiDirection::V);
        box_style_buttons.SetDirection(UiDirection::H)
                         .SetGap(DPI(3))
                         .SetInset(0)
                         .SetAlignItems(UiCrossAlign::Center);

        ConfigureRibbonButton(btn_normal, "Normal");
        ConfigureIconButton(btn_heading1, ICON_EDITOR_FORMAT_H1_48(), "Heading 1");
        ConfigureIconButton(btn_heading2, ICON_EDITOR_FORMAT_H2_48(), "Heading 2");

        box_style_buttons.Add(btn_normal).Fixed(DPI(62));
        box_style_buttons.Add(btn_heading1).Fixed(DPI(36));
        box_style_buttons.Add(btn_heading2).Fixed(DPI(36));

        drop_style.UseInternalModel();
        drop_style.SetPlaceholderText("More styles")
                  .SetPopupMaxItems(12)
                  .SetSizeMin(DPI(156), DPI(28));
        drop_style.Add("Normal", String("paragraph"));
        drop_style.Add("Heading 1", String("heading.1"));
        drop_style.Add("Heading 2", String("heading.2"));
        drop_style.Add("Heading 3", String("heading.3"));
        drop_style.Add("Quote", String("quote"));
        drop_style.Add("Code", String("code"));
        drop_style.Add("Bulleted list", String("list.bullet"));
        drop_style.Add("Numbered list", String("list.numbered"));
        drop_style.AddGroupHeader("Screenplay");
        drop_style.Add("Scene", String("screenplay.scene"));
        drop_style.Add("Action", String("screenplay.action"));
        drop_style.Add("Character", String("screenplay.character"));
        drop_style.Add("Dialogue", String("screenplay.dialogue"));
        drop_style.Add("Transition", String("screenplay.transition"));
        drop_style.SetDataSilently(String("paragraph"));

        box_styles.Add(box_style_buttons).Fit();
        box_styles.Add(drop_style).Fit();

        SetupRibbonGroup(gp_editing, box_editing, "Editing", UiDirection::V);
        box_find_row.SetDirection(UiDirection::H)
                    .SetGap(DPI(3))
                    .SetInset(0)
                    .SetAlignItems(UiCrossAlign::Center);
        box_replace_row.SetDirection(UiDirection::H)
                       .SetGap(DPI(3))
                       .SetInset(0)
                       .SetAlignItems(UiCrossAlign::Center);

        edit_find.SetPlaceholder("Find...");
        edit_replace.SetPlaceholder("Replace...");
        ConfigureIconButton(btn_find_prev, ICON_DESIGN_YOUTUBE_SEARCHED_FOR_48(), "Previous match");
        ConfigureIconButton(btn_find_next, ICON_ACTION_SEARCH_48(), "Next match");
        ConfigureIconButton(btn_replace_current, ICON_DESIGN_FIND_REPLACE_48(), "Replace current match");
        ConfigureTextButton(btn_replace_all, "All");
        ConfigureTextButton(btn_ignore_case, "Ignore case");
        ConfigureTextButton(btn_whole_word, "Whole word");
        btn_ignore_case.SetCheckable();
        btn_whole_word.SetCheckable();
        btn_ignore_case.SetChecked(true);

        box_find_row.Add(edit_find).Fixed(DPI(150));
        box_find_row.Add(btn_find_prev).Fixed(DPI(32));
        box_find_row.Add(btn_find_next).Fixed(DPI(32));
        box_replace_row.Add(edit_replace).Fixed(DPI(126));
        box_replace_row.Add(btn_replace_current).Fixed(DPI(32));
        box_replace_row.Add(btn_replace_all).Fixed(DPI(38));

        box_editing.Add(box_find_row).Fit();
        box_editing.Add(box_replace_row).Fit();

        box_home.Add(gp_clipboard).Fit();
        box_home.Add(gp_font).Fit();
        box_home.Add(gp_paragraph).Fit();
        box_home.Add(gp_styles).Fit();
        box_home.Add(gp_editing).Fit();
    }

    void BuildInsertRibbon()
    {
        SetupRibbonGroup(gp_pages, box_pages, "Pages & breaks", UiDirection::H);
        ConfigureRibbonButton(btn_page_break, "Page Break", ICON_DESIGN_DESCRIPTION_48());
        ConfigureRibbonButton(btn_rule, "Rule", ICON_EDITOR_FORMAT_LINE_SPACING_48());
        box_pages.Add(btn_page_break).Fixed(DPI(96));
        box_pages.Add(btn_rule).Fixed(DPI(66));

        SetupRibbonGroup(gp_table, box_table, "Table", UiDirection::V);
        box_table_top.SetDirection(UiDirection::H).SetGap(DPI(3)).SetInset(0);
        box_table_bottom.SetDirection(UiDirection::H).SetGap(DPI(3)).SetInset(0);

        ConfigureRibbonButton(btn_insert_table, "3 x 3", ICON_EDITOR_TABLE_48());
        ConfigureTextButton(btn_row_add, "Row +");
        ConfigureTextButton(btn_row_remove, "Row -");
        ConfigureTextButton(btn_column_add, "Col +");
        ConfigureTextButton(btn_column_remove, "Col -");
        ConfigureTextButton(btn_remove_embed, "Delete");

        box_table_top.Add(btn_insert_table).Fixed(DPI(76));
        box_table_top.Add(btn_row_add).Fixed(DPI(56));
        box_table_top.Add(btn_row_remove).Fixed(DPI(56));
        box_table_bottom.Add(btn_column_add).Fixed(DPI(56));
        box_table_bottom.Add(btn_column_remove).Fixed(DPI(56));
        box_table_bottom.Add(btn_remove_embed).Fixed(DPI(58));
        box_table.Add(box_table_top).Fit();
        box_table.Add(box_table_bottom).Fit();

        SetupRibbonGroup(gp_picture, box_picture, "Pictures", UiDirection::V);
        box_picture_top.SetDirection(UiDirection::H).SetGap(DPI(3)).SetInset(0);
        box_picture_bottom.SetDirection(UiDirection::H).SetGap(DPI(3)).SetInset(0);

        ConfigureRibbonButton(btn_insert_picture, "Picture", ICON_DESIGN_IMAGE_48());
        ConfigureTextButton(btn_image_left, "Left");
        ConfigureTextButton(btn_image_center, "Center");
        ConfigureTextButton(btn_image_right, "Right");
        ConfigureIconButton(btn_remove_picture, ICON_DESIGN_DELETE_48(), "Remove selected image");

        box_picture_top.Add(btn_insert_picture).Fixed(DPI(84));
        box_picture_top.Add(btn_remove_picture).Fixed(DPI(34));
        box_picture_bottom.Add(btn_image_left).Fixed(DPI(48));
        box_picture_bottom.Add(btn_image_center).Fixed(DPI(58));
        box_picture_bottom.Add(btn_image_right).Fixed(DPI(50));
        box_picture.Add(box_picture_top).Fit();
        box_picture.Add(box_picture_bottom).Fit();

        SetupRibbonGroup(gp_screenplay, box_screenplay, "Screenplay", UiDirection::H);
        ConfigureTextButton(btn_scene, "Scene");
        ConfigureTextButton(btn_action, "Action");
        ConfigureTextButton(btn_character, "Character");
        ConfigureTextButton(btn_dialogue, "Dialogue");
        box_screenplay.Add(btn_scene).Fixed(DPI(58));
        box_screenplay.Add(btn_action).Fixed(DPI(60));
        box_screenplay.Add(btn_character).Fixed(DPI(76));
        box_screenplay.Add(btn_dialogue).Fixed(DPI(72));

        box_insert.Add(gp_pages).Fit();
        box_insert.Add(gp_table).Fit();
        box_insert.Add(gp_picture).Fit();
        box_insert.Add(gp_screenplay).Fit();
    }

    void BuildReviewRibbon()
    {
        SetupRibbonGroup(gp_review_comments, box_review_comments, "Comments", UiDirection::H);
        ConfigureRibbonButton(btn_new_comment, "New Comment", ICON_DESIGN_COMMENT_48());
        ConfigureTextButton(btn_comments_pane, "Comments Pane");
        ConfigureTextButton(btn_resolve_comment, "Resolve");
        ConfigureIconButton(btn_delete_comment, ICON_DESIGN_DELETE_48(), "Delete active comment");
        btn_comments_pane.SetCheckable();

        box_review_comments.Add(btn_new_comment).Fixed(DPI(104));
        box_review_comments.Add(btn_comments_pane).Fixed(DPI(106));
        box_review_comments.Add(btn_resolve_comment).Fixed(DPI(68));
        box_review_comments.Add(btn_delete_comment).Fixed(DPI(34));

        SetupRibbonGroup(gp_review_info, box_review_info, "Document model", UiDirection::V);
        lbl_review_model.SetText("UiDocCore: revisioned model");
        lbl_review_storage.SetText("Storage: native .uidoc JSON");
        lbl_review_revision.SetText("Revision 0");
        box_review_info.Add(lbl_review_model).Fit();
        box_review_info.Add(lbl_review_storage).Fit();
        box_review_info.Add(lbl_review_revision).Fit();

        box_review.Add(gp_review_comments).Fit();
        box_review.Add(gp_review_info).Fit();
    }

    void BuildViewRibbon()
    {
        SetupRibbonGroup(gp_view_gutter, box_view_gutter, "Document markers", UiDirection::H);
        ConfigureTextButton(btn_line_numbers, "Line numbers");
        ConfigureTextButton(btn_metadata_markers, "Markers");
        ConfigureTextButton(btn_gutter_side, "Gutter right");
        btn_line_numbers.SetCheckable();
        btn_metadata_markers.SetCheckable();
        btn_metadata_markers.SetChecked(true);

        box_view_gutter.Add(btn_line_numbers).Fixed(DPI(94));
        box_view_gutter.Add(btn_metadata_markers).Fixed(DPI(72));
        box_view_gutter.Add(btn_gutter_side).Fixed(DPI(88));

        SetupRibbonGroup(gp_view_workspace, box_view_workspace, "Workspace", UiDirection::H);
        ConfigureTextButton(btn_comments_pane_view, "Comments");
        ConfigureTextButton(btn_home_tab, "Home");
        ConfigureTextButton(btn_select_all, "Select all");
        btn_comments_pane_view.SetCheckable();

        box_view_workspace.Add(btn_comments_pane_view).Fixed(DPI(76));
        box_view_workspace.Add(btn_home_tab).Fixed(DPI(56));
        box_view_workspace.Add(btn_select_all).Fixed(DPI(72));

        SetupRibbonGroup(gp_view_about, box_view_about, "Layout", UiDirection::V);
        lbl_view_layout.SetText("Viewport-driven paragraph layout");
        lbl_view_scale.SetText("Sparse styles - no per-character mirror");
        box_view_about.Add(lbl_view_layout).Fit();
        box_view_about.Add(lbl_view_scale).Fit();

        box_view.Add(gp_view_gutter).Fit();
        box_view.Add(gp_view_workspace).Fit();
        box_view.Add(gp_view_about).Fit();
    }

    void BuildWorkspace()
    {
        UiPanel::Style workspace_style = pnl_workspace.GetStyle();
        for(int i = 0; i < 4; i++) {
            workspace_style.palette.face[i] = UiFill::Solid(Color(238, 241, 245));
            workspace_style.palette.frame[i] = Color(220, 224, 230);
        }
        workspace_style.metrics.frame_enabled = false;
        pnl_workspace.SetCustomStyle(workspace_style);

        pnl_workspace.Add(box_workspace.SizePos());
        box_workspace.SetDirection(UiDirection::H)
                     .SetGap(DPI(12))
                     .SetInset(Rect(DPI(12), DPI(10), DPI(12), DPI(10)));

        box_document_frame.SetDirection(UiDirection::H)
                          .SetGap(0)
                          .SetInset(0);
        box_document_frame.AddSpacer(1);
        box_document_frame.Add(doc_editor)
                          .Expand(8)
                          .MinMaxWidth(DPI(680), DPI(920));
        box_document_frame.AddSpacer(1);

        UiDoc::Style doc_style = doc_editor.GetStyle();
        doc_style.font = SansSerifZ(DPI(11));
        doc_style.page_padding = DPI(42);
        doc_style.gutter_width = DPI(24);
        doc_style.annotation_marker_size = DPI(8);
        doc_style.selection_fill = Color(190, 218, 255);
        doc_style.search_fill = Color(255, 236, 158);
        doc_style.annotation_fill = Color(255, 235, 190);
        doc_style.marker_comment = Color(216, 112, 35);
        doc_style.page_face = Color(255, 255, 255);
        doc_style.page_frame = Color(205, 210, 218);
        for(int i = 0; i < 4; i++) {
            doc_style.palette.face[i] = UiFill::Solid(Color(255, 255, 255));
            doc_style.palette.frame[i] = Color(205, 210, 218);
            doc_style.palette.ink[i] = Color(30, 33, 38);
        }
        doc_style.metrics.frame_enabled = true;
        doc_style.metrics.frame_width = DPI(1);
        doc_style.metrics.radius = DPI(1);
        doc_editor.SetCustomStyle(doc_style);

        BuildCommentsPane();

        box_workspace.Add(box_document_frame).Expand(1);
        box_workspace.Add(gp_comments).Fixed(DPI(306));
        SetCommentsVisible(false);
    }

    void BuildCommentsPane()
    {
        ConfigureTextButton(btn_close_comments, "x");
        btn_close_comments.Tip("Close comments pane");

        gp_comments.SetTitle("Comments")
                   .SetSubTitle("Review annotations")
                   .SetHeaderMode(UiGroupPanel::Inside)
                   .SetHeaderContent(btn_close_comments)
                   .SetHeaderContentAlign(UiAlign::RIGHT, UiAlign::CENTER)
                   .SetInset(Rect(DPI(10), DPI(8), DPI(10), DPI(10)))
                   .SetHeaderInset(Rect(DPI(10), DPI(5), DPI(8), DPI(5)))
                   .SetLine(true);

        box_comments.SetDirection(UiDirection::V)
                    .SetGap(DPI(8))
                    .SetInset(0);

        lbl_comment_count.SetText("No comments");
        lbl_comment_context.SetText("Select text and add a review comment.");
        edit_comment.SetPlaceholder("Write a comment...");

        box_comment_actions.SetDirection(UiDirection::H)
                           .SetGap(DPI(4))
                           .SetInset(0);
        ConfigureTextButton(btn_comment_new, "New");
        ConfigureTextButton(btn_comment_update, "Update");
        ConfigureTextButton(btn_comment_resolve, "Resolve");
        ConfigureTextButton(btn_comment_delete, "Delete");

        box_comment_actions.Add(btn_comment_new).Fixed(DPI(54));
        box_comment_actions.Add(btn_comment_update).Fixed(DPI(62));
        box_comment_actions.Add(btn_comment_resolve).Fixed(DPI(62));
        box_comment_actions.Add(btn_comment_delete).Fixed(DPI(58));

        box_comments.Add(lbl_comment_count).Fit();
        box_comments.Add(lbl_comment_context).Fit();
        box_comments.Add(edit_comment).Fit();
        box_comments.Add(box_comment_actions).Fit();
        box_comments.AddSpacer(1);
        gp_comments.SetContent(box_comments);
    }

    void BuildStatusBar()
    {
        UiPanel::Style status_style = pnl_status.GetStyle();
        for(int i = 0; i < 4; i++) {
            status_style.palette.face[i] = UiFill::Solid(Color(248, 249, 251));
            status_style.palette.frame[i] = Color(218, 221, 226);
        }
        status_style.metrics.frame_enabled = true;
        status_style.metrics.frame_width = DPI(1);
        pnl_status.SetCustomStyle(status_style);

        pnl_status.Add(box_status.SizePos());
        box_status.SetDirection(UiDirection::H)
                  .SetGap(DPI(12))
                  .SetInset(Rect(DPI(10), DPI(4), DPI(10), DPI(4)))
                  .SetAlignItems(UiCrossAlign::Center);

        lbl_status_left.SetText("Ready");
        lbl_status_center.SetText("0 words | 0 characters");
        lbl_status_right.SetText("Revision 0");

        box_status.Add(lbl_status_left).Expand(1);
        box_status.Add(lbl_status_center).Fit();
        box_status.Add(lbl_status_right).Fit();
    }

    void WireActions()
    {
        btn_new.WhenAction = [=] { NewDocument(); };
        btn_open.WhenAction = [=] { OpenDocument(); };
        btn_save.WhenAction = [=] { SaveDocument(); };
        btn_save_as.WhenAction = [=] { SaveDocumentAs(); };
        btn_undo.WhenAction = [=] { RunCommand("edit.undo", Value(), "Undo"); };
        btn_redo.WhenAction = [=] { RunCommand("edit.redo", Value(), "Redo"); };

        btn_cut.WhenAction = [=] { doc_editor.Cut(); doc_editor.SetFocus(); };
        btn_copy.WhenAction = [=] { doc_editor.Copy(); doc_editor.SetFocus(); };
        btn_paste.WhenAction = [=] { doc_editor.Paste(); doc_editor.SetFocus(); };

        drop_font.WhenSelect = [=](int) {
            doc_editor.SetSelectionFont(AsString(drop_font.GetSelectedData()));
            doc_editor.SetFocus();
        };
        drop_size.WhenSelect = [=](int) {
            int size = (int)drop_size.GetSelectedData();
            String face = drop_font.HasSelection() ? AsString(drop_font.GetSelectedData()) : String();
            doc_editor.SetSelectionFont(face, DPI(size));
            doc_editor.SetFocus();
        };
        btn_size_up.WhenAction = [=] { RunCommand("format.size_up", Value(), "Increase font size"); };
        btn_size_down.WhenAction = [=] { RunCommand("format.size_down", Value(), "Decrease font size"); };
        btn_bold.WhenAction = [=] { RunCommand("format.bold", Value(), "Bold"); };
        btn_italic.WhenAction = [=] { RunCommand("format.italic", Value(), "Italic"); };
        btn_underline.WhenAction = [=] { RunCommand("format.underline", Value(), "Underline"); };
        btn_strike.WhenAction = [=] { RunCommand("format.strike", Value(), "Strikeout"); };
        btn_upper.WhenAction = [=] { RunCommand("text.upper", Value(), "Uppercase"); };
        btn_lower.WhenAction = [=] { RunCommand("text.lower", Value(), "Lowercase"); };

        btn_bullets.WhenAction = [=] { ApplyBlockRole("list.bullet"); };
        btn_numbering.WhenAction = [=] { ApplyBlockRole("list.numbered"); };
        btn_quote.WhenAction = [=] { ApplyBlockRole("quote"); };
        btn_code.WhenAction = [=] { ApplyBlockRole("code"); };
        btn_indent.WhenAction = [=] { doc_editor.SetBlockIndent(1); doc_editor.SetFocus(); };
        btn_outdent.WhenAction = [=] { doc_editor.SetBlockIndent(0); doc_editor.SetFocus(); };

        btn_normal.WhenAction = [=] { ApplyBlockRole("paragraph"); };
        btn_heading1.WhenAction = [=] { ApplyBlockRole("heading.1"); };
        btn_heading2.WhenAction = [=] { ApplyBlockRole("heading.2"); };
        drop_style.WhenSelect = [=](int) {
            ApplyBlockRole(AsString(drop_style.GetSelectedData()));
        };

        edit_find.WhenChange = [=] {
            doc_editor.SetSearchQuery(edit_find.GetTextUtf8());
            RefreshDocumentUi();
        };
        edit_find.WhenAction = [=] { FindNext(); };
        btn_find_prev.WhenAction = [=] {
            if(!doc_editor.FindPrev())
                SetStatus("No previous match");
            else
                SetStatus("Previous match");
            doc_editor.SetFocus();
        };
        btn_find_next.WhenAction = [=] { FindNext(); };
        btn_replace_current.WhenAction = [=] {
            bool ok = doc_editor.ReplaceCurrentSearch(
                ToUnicode(edit_replace.GetTextUtf8(), CHARSET_UTF8));
            SetStatus(ok ? "Replaced current match" : "No current match to replace");
            doc_editor.SetFocus();
        };
        btn_replace_all.WhenAction = [=] {
            int count = doc_editor.ReplaceAllSearch(
                ToUnicode(edit_replace.GetTextUtf8(), CHARSET_UTF8));
            SetStatus(Format("Replaced %d match%s", count, count == 1 ? "" : "es"));
            doc_editor.SetFocus();
        };
        btn_ignore_case.WhenAction = [=] {
            doc_editor.SetSearchIgnoreCase(btn_ignore_case.IsChecked());
            RefreshDocumentUi();
            doc_editor.SetFocus();
        };
        btn_whole_word.WhenAction = [=] {
            doc_editor.SetSearchWholeWord(btn_whole_word.IsChecked());
            RefreshDocumentUi();
            doc_editor.SetFocus();
        };

        btn_page_break.WhenAction = [=] { RunCommand("insert.page_break", Value(), "Insert page break"); };
        btn_rule.WhenAction = [=] { RunCommand("insert.hr", Value(), "Insert horizontal rule"); };
        btn_insert_table.WhenAction = [=] {
            ValueArray args;
            args.Add(3);
            args.Add(3);
            args.Add(1);
            RunCommand("insert.table", args, "Insert 3 x 3 table");
        };
        btn_row_add.WhenAction = [=] { RunCommand("table.row.add", Value(), "Add table row"); };
        btn_row_remove.WhenAction = [=] { RunCommand("table.row.remove", Value(), "Remove table row"); };
        btn_column_add.WhenAction = [=] { RunCommand("table.column.add", Value(), "Add table column"); };
        btn_column_remove.WhenAction = [=] { RunCommand("table.column.remove", Value(), "Remove table column"); };
        btn_remove_embed.WhenAction = [=] { RunCommand("embed.remove", Value(), "Remove selected table/embed"); };

        btn_insert_picture.WhenAction = [=] { InsertPicture(); };
        btn_image_left.WhenAction = [=] { RunCommand("image.align.left", Value(), "Align image left"); };
        btn_image_center.WhenAction = [=] { RunCommand("image.align.center", Value(), "Align image center"); };
        btn_image_right.WhenAction = [=] { RunCommand("image.align.right", Value(), "Align image right"); };
        btn_remove_picture.WhenAction = [=] { RunCommand("embed.remove", Value(), "Remove selected image"); };

        btn_scene.WhenAction = [=] { ApplyBlockRole("screenplay.scene"); };
        btn_action.WhenAction = [=] { ApplyBlockRole("screenplay.action"); };
        btn_character.WhenAction = [=] { ApplyBlockRole("screenplay.character"); };
        btn_dialogue.WhenAction = [=] { ApplyBlockRole("screenplay.dialogue"); };

        btn_new_comment.WhenAction = [=] { AddCommentFromPane(); };
        btn_comments_pane.WhenAction = [=] { SetCommentsVisible(btn_comments_pane.IsChecked()); };
        btn_resolve_comment.WhenAction = [=] { ResolveActiveComment(); };
        btn_delete_comment.WhenAction = [=] { DeleteActiveComment(); };

        btn_line_numbers.WhenAction = [=] {
            doc_editor.ShowLineNumbers(btn_line_numbers.IsChecked());
            RefreshDocumentUi();
            doc_editor.SetFocus();
        };
        btn_metadata_markers.WhenAction = [=] {
            doc_editor.ShowMetadataMarkers(btn_metadata_markers.IsChecked());
            RefreshDocumentUi();
            doc_editor.SetFocus();
        };
        btn_gutter_side.WhenAction = [=] {
            UiDoc::GutterSide side = doc_editor.GetGutterSide() == UiDoc::GUTTER_LEFT
                                   ? UiDoc::GUTTER_RIGHT : UiDoc::GUTTER_LEFT;
            doc_editor.SetGutterSide(side);
            RefreshDocumentUi();
            doc_editor.SetFocus();
        };
        btn_comments_pane_view.WhenAction = [=] {
            SetCommentsVisible(btn_comments_pane_view.IsChecked());
        };
        btn_home_tab.WhenAction = [=] { tab_ribbon.SetActiveTab(0); doc_editor.SetFocus(); };
        btn_select_all.WhenAction = [=] { doc_editor.SelectAll(); doc_editor.SetFocus(); };

        btn_close_comments.WhenAction = [=] { SetCommentsVisible(false); };
        btn_comment_new.WhenAction = [=] { AddCommentFromPane(); };
        btn_comment_update.WhenAction = [=] { UpdateActiveComment(); };
        btn_comment_resolve.WhenAction = [=] { ResolveActiveComment(); };
        btn_comment_delete.WhenAction = [=] { DeleteActiveComment(); };
        edit_comment.WhenAction = [=] {
            if(active_comment_id.IsEmpty())
                AddCommentFromPane();
            else
                UpdateActiveComment();
        };

        doc_editor.WhenChange = [=] {
            if(!suppress_dirty)
                dirty = true;
            RefreshDocumentUi();
        };
        doc_editor.WhenSelection = [=] { RefreshDocumentUi(); };
        doc_editor.WhenSearch = [=](const String&) { RefreshDocumentUi(); };
        doc_editor.WhenAnnotation = [=](const String& id) {
            active_comment_id = id;
            SetCommentsVisible(true);
            tab_ribbon.SetActiveTab(2);
            RefreshComments();
        };
    }

    void LoadWelcomeDocument()
    {
        suppress_dirty = true;
        doc_editor.NewDocument();

        const String text =
            "Project Aurora\n"
            "UiDoc v2 sample document\n"
            "\n"
            "Executive summary\n"
            "UiDoc combines a non-visual document core with an interactive U++ editor. "
            "This showcase presents the supported features in a familiar ribbon-style workspace.\n"
            "\n"
            "What this demo supports\n"
            "Rich text marks, fonts and local size changes\n"
            "Semantic headings, lists, quotes and screenplay blocks\n"
            "Typed rich tables and resource-backed images\n"
            "Range comments, search, replace and native .uidoc persistence\n"
            "\n"
            "Review note\n"
            "Comments stay attached to logical ranges as text changes around them. "
            "Use the Review tab to open the comments pane and edit the live note.\n"
            "\n"
            "INT. DESIGN REVIEW - DAY\n"
            "The editor sits open on a clean document workspace.\n"
            "EDITOR\n"
            "The document model stays separate from the control.\n"
            "\n"
            "Feature matrix\n";

        doc_editor.SetText(text);
        doc_editor.Core().SetMeta("demo.document_type", "UiDoc v2 showcase");
        doc_editor.Core().SetMeta("demo.workflow", "interactive rich document");

        ApplyRoleToFragment("Project Aurora", "heading.1");
        ApplyRoleToFragment("UiDoc v2 sample document", "heading.3");
        ApplyRoleToFragment("Executive summary", "heading.2");
        ApplyRoleToFragment("What this demo supports", "heading.2");
        ApplyRoleToFragment("Rich text marks, fonts and local size changes", "list.bullet", 1);
        ApplyRoleToFragment("Semantic headings, lists, quotes and screenplay blocks", "list.bullet", 1);
        ApplyRoleToFragment("Typed rich tables and resource-backed images", "list.bullet", 1);
        ApplyRoleToFragment("Range comments, search, replace and native .uidoc persistence", "list.bullet", 1);
        ApplyRoleToFragment("Review note", "heading.2");
        ApplyRoleToFragment("INT. DESIGN REVIEW - DAY", "screenplay.scene");
        ApplyRoleToFragment("The editor sits open on a clean document workspace.", "screenplay.action");
        ApplyRoleToFragment("EDITOR", "screenplay.character");
        ApplyRoleToFragment("The document model stays separate from the control.", "screenplay.dialogue");
        ApplyRoleToFragment("Feature matrix", "heading.2");

        doc_editor.SetSelection(UiDocRange(doc_editor.GetLength(), doc_editor.GetLength()));
        String table_id = doc_editor.InsertTable(3, 4, 1);
        UiDocTable table;
        if(!table_id.IsEmpty() && doc_editor.GetTable(table_id, table)) {
            SetTableCellText(table, 0, 0, "Capability", true);
            SetTableCellText(table, 0, 1, "Model", true);
            SetTableCellText(table, 0, 2, "Workspace", true);
            SetTableCellText(table, 1, 0, "Rich text");
            SetTableCellText(table, 1, 1, "Sparse style runs");
            SetTableCellText(table, 1, 2, "Home ribbon");
            SetTableCellText(table, 2, 0, "Comments");
            SetTableCellText(table, 2, 1, "Range annotations");
            SetTableCellText(table, 2, 2, "Review pane");
            SetTableCellText(table, 3, 0, "Tables & images");
            SetTableCellText(table, 3, 1, "Typed embeds/resources");
            SetTableCellText(table, 3, 2, "Insert ribbon");
            doc_editor.SetTable(table_id, table);
        }

        String current_text = doc_editor.GetText();
        const String comment_phrase = "familiar ribbon-style workspace";
        int comment_at = current_text.Find(comment_phrase);
        if(comment_at >= 0) {
            doc_editor.SetSelection(UiDocRange(comment_at, comment_at + comment_phrase.GetCount()));
            ValueMap meta;
            meta.Add("author", "UiDocDemo");
            active_comment_id = doc_editor.AddComment(
                "This is a live UiDoc range annotation. Edit or resolve it from the Review pane.",
                meta);
        }

        doc_editor.SetSelection(UiDocRange(0, 0));
        doc_editor.SetSearchQuery(String());
        edit_find.Clear();
        edit_replace.Clear();

        current_path.Clear();
        dirty = false;
        suppress_dirty = false;
        status_message = "Welcome document";
        RefreshDocumentUi();
        doc_editor.SetFocus();
    }

    void ApplyRoleToFragment(const String& fragment, const String& role, int indent = 0)
    {
        String text = doc_editor.GetText();
        int at = text.Find(fragment);
        if(at < 0)
            return;
        doc_editor.SetSelection(UiDocRange(at, at + fragment.GetCount()));
        doc_editor.SetBlockRole(role);
        if(indent > 0)
            doc_editor.SetBlockIndent(indent);
    }

    void ApplyBlockRole(const String& role)
    {
        doc_editor.SetBlockRole(role);
        drop_style.SetDataSilently(role);
        SetStatus("Applied " + role);
        doc_editor.SetFocus();
    }

    bool RunCommand(const String& id, const Value& value, const String& label)
    {
        bool ok = doc_editor.ExecuteCommand(id, value);
        SetStatus(ok ? label : label + " - select compatible content first");
        RefreshDocumentUi();
        doc_editor.SetFocus();
        return ok;
    }

    void FindNext()
    {
        if(!doc_editor.FindNext())
            SetStatus("No search match");
        else
            SetStatus(Format("Match %d of %d",
                             doc_editor.GetSearchMatchIndex() + 1,
                             doc_editor.GetSearchMatchCount()));
        doc_editor.SetFocus();
    }

    void NewDocument()
    {
        suppress_dirty = true;
        doc_editor.NewDocument();
        current_path.Clear();
        active_comment_id.Clear();
        dirty = false;
        suppress_dirty = false;
        status_message = "New document";
        RefreshDocumentUi();
        doc_editor.SetFocus();
    }

    void OpenDocument()
    {
        FileSel fs;
        fs.Type("UiDoc documents", "*.uidoc");
        if(!fs.ExecuteOpen())
            return;

        String path = ~fs;
        String error;
        suppress_dirty = true;
        bool ok = doc_editor.Load(path, &error);
        suppress_dirty = false;
        if(!ok) {
            SetStatus(error.IsEmpty() ? String("Unable to open document") : error);
            return;
        }

        current_path = path;
        active_comment_id.Clear();
        dirty = false;
        status_message = "Opened " + GetFileName(path);
        RefreshDocumentUi();
        doc_editor.SetFocus();
    }

    bool SaveDocument()
    {
        if(current_path.IsEmpty())
            return SaveDocumentAs();
        return SaveToPath(current_path);
    }

    bool SaveDocumentAs()
    {
        FileSel fs;
        fs.Type("UiDoc documents", "*.uidoc");
        if(!fs.ExecuteSaveAs())
            return false;

        String path = ~fs;
        if(GetFileExt(path).IsEmpty())
            path << ".uidoc";
        return SaveToPath(path);
    }

    bool SaveToPath(const String& path)
    {
        String error;
        if(!doc_editor.Save(path, &error)) {
            SetStatus(error.IsEmpty() ? String("Unable to save document") : error);
            return false;
        }

        current_path = path;
        dirty = false;
        status_message = "Saved " + GetFileName(path);
        RefreshDocumentUi();
        return true;
    }

    void InsertPicture()
    {
        FileSel fs;
        fs.Type("Image files", "*.png *.jpg *.jpeg");
        if(!fs.ExecuteOpen())
            return;

        String path = ~fs;
        String bytes = LoadFile(path);
        Image image = StreamRaster::LoadFileAny(path);
        if(bytes.IsEmpty() || image.IsEmpty()) {
            SetStatus("Unable to load image");
            return;
        }

        Size native = image.GetSize();
        int width = min(DPI(320), max(DPI(48), native.cx));
        int height = native.cx > 0 ? max(DPI(32), native.cy * width / native.cx)
                                   : DPI(96);

        UiDocResource resource;
        resource.resource_type = "image";
        resource.bytes = bytes;
        resource.original_name = GetFileName(path);
        resource.width = native.cx;
        resource.height = native.cy;
        String ext = ToLower(GetFileExt(path));
        resource.mime = (ext == ".jpg" || ext == ".jpeg") ? "image/jpeg" : "image/png";
        resource.meta.Add("source", "UiDocDemo file picker");

        String key = doc_editor.AddResource(resource, false);
        if(key.IsEmpty() || doc_editor.InsertImage(key, width, height, "center").IsEmpty()) {
            SetStatus("Unable to insert image");
            return;
        }

        SetStatus("Inserted " + GetFileName(path));
        doc_editor.SetFocus();
    }

    void AddCommentFromPane()
    {
        String text = TrimBoth(edit_comment.GetTextUtf8());
        if(text.IsEmpty())
            text = "Review comment";

        String id = doc_editor.AddComment(text);
        if(id.IsEmpty()) {
            SetStatus("Unable to add comment");
            return;
        }

        active_comment_id = id;
        SetCommentsVisible(true);
        SetStatus("Comment added");
        RefreshComments();
        doc_editor.SetFocus();
    }

    void UpdateActiveComment()
    {
        if(active_comment_id.IsEmpty()) {
            SetStatus("No active comment");
            return;
        }
        String text = TrimBoth(edit_comment.GetTextUtf8());
        if(text.IsEmpty()) {
            SetStatus("Comment text is empty");
            return;
        }
        if(!doc_editor.UpdateComment(active_comment_id, text)) {
            SetStatus("Unable to update comment");
            return;
        }
        SetStatus("Comment updated");
        RefreshComments();
        doc_editor.SetFocus();
    }

    void ResolveActiveComment()
    {
        if(active_comment_id.IsEmpty() ||
           !doc_editor.ResolveComment(active_comment_id, true)) {
            SetStatus("No active comment to resolve");
            return;
        }
        SetStatus("Comment resolved");
        RefreshComments();
        doc_editor.SetFocus();
    }

    void DeleteActiveComment()
    {
        if(active_comment_id.IsEmpty() ||
           !doc_editor.RemoveComment(active_comment_id)) {
            SetStatus("No active comment to delete");
            return;
        }
        active_comment_id.Clear();
        SetStatus("Comment deleted");
        RefreshComments();
        doc_editor.SetFocus();
    }

    void SetCommentsVisible(bool visible)
    {
        comments_visible = visible;
        gp_comments.Show(visible);
        btn_comments_pane.SetChecked(visible);
        btn_comments_pane_view.SetChecked(visible);
        box_workspace.RefreshLayout();
        box_root.RefreshLayout();
        RefreshComments();
    }

    void RefreshDocumentUi()
    {
        RefreshTitle();
        RefreshRibbonState();
        RefreshComments();
        RefreshStatus();
    }

    void RefreshTitle()
    {
        String file = current_path.IsEmpty() ? String("Untitled.uidoc")
                                             : GetFileName(current_path);
        String shown = file + (dirty ? " *" : "");
        tc_app.SetCopyText(shown + "   |   Native .uidoc");
        Title(shown + " - UiDoc Demo");
    }

    void RefreshRibbonState()
    {
        UiDocRange selection = OrderedSelection(doc_editor);
        bool has_selection = !selection.IsEmpty();

        btn_cut.Enable(has_selection);
        btn_copy.Enable(has_selection);
        btn_undo.Enable(doc_editor.CanUndo());
        btn_redo.Enable(doc_editor.CanRedo());

        btn_bold.SetChecked(doc_editor.QueryCommandState("format.bold").active);
        btn_italic.SetChecked(doc_editor.QueryCommandState("format.italic").active);
        btn_underline.SetChecked(doc_editor.QueryCommandState("format.underline").active);
        btn_strike.SetChecked(doc_editor.QueryCommandState("format.strike").active);

        btn_find_prev.Enable(doc_editor.GetSearchMatchCount() > 0);
        btn_find_next.Enable(doc_editor.GetSearchMatchCount() > 0);
        btn_replace_current.Enable(doc_editor.GetSearchMatchCount() > 0);
        btn_replace_all.Enable(doc_editor.GetSearchMatchCount() > 0);

        btn_ignore_case.SetChecked(doc_editor.IsSearchIgnoreCase());
        btn_whole_word.SetChecked(doc_editor.IsSearchWholeWord());
        btn_line_numbers.SetChecked(doc_editor.IsLineNumbersShown());
        btn_metadata_markers.SetChecked(doc_editor.IsMetadataMarkersShown());
        btn_gutter_side.SetText(doc_editor.GetGutterSide() == UiDoc::GUTTER_LEFT
                              ? "Gutter left" : "Gutter right");

        String role = doc_editor.GetBlockRole();
        if(role.IsEmpty())
            role = "paragraph";
        drop_style.SetDataSilently(role);

        lbl_review_revision.SetText("Revision " + AsString((int64)doc_editor.Core().GetRevision()));
    }

    void RefreshComments()
    {
        Vector<UiDocAnnotation> comments = doc_editor.GetComments();
        int open_count = 0;
        for(const UiDocAnnotation& comment : comments)
            if(!comment.resolved)
                open_count++;

        lbl_comment_count.SetText(
            Format("%d comment%s - %d open",
                   comments.GetCount(),
                   comments.GetCount() == 1 ? "" : "s",
                   open_count));

        String selection_comment;
        UiDocRange selection = OrderedSelection(doc_editor);
        if(selection.IsEmpty()) {
            int at = min(doc_editor.GetLength(), selection.from);
            UiDocRange probe(at, min(doc_editor.GetLength(), at + 1));
            Vector<UiDocAnnotation> hit = doc_editor.GetComments(&probe);
            if(!hit.IsEmpty())
                selection_comment = hit[0].id;
        }
        else {
            Vector<UiDocAnnotation> hit = doc_editor.GetComments(&selection);
            if(!hit.IsEmpty())
                selection_comment = hit[0].id;
        }

        if(!selection_comment.IsEmpty())
            active_comment_id = selection_comment;

        const UiDocAnnotation* active = nullptr;
        for(const UiDocAnnotation& comment : comments)
            if(comment.id == active_comment_id) {
                active = &comment;
                break;
            }

        if(!active) {
            for(const UiDocAnnotation& comment : comments)
                if(!comment.resolved) {
                    active_comment_id = comment.id;
                    active = &comment;
                    break;
                }
        }

        if(active) {
            String text = active->payload.Find("text") >= 0
                        ? AsString(active->payload["text"]) : String();
            edit_comment.SetTextUtf8(text);
            lbl_comment_context.SetText(
                Format("%s | characters %d-%d",
                       active->resolved ? "Resolved" : "Open",
                       active->range.from, active->range.to));
            btn_comment_update.Enable(true);
            btn_comment_resolve.Enable(!active->resolved);
            btn_comment_delete.Enable(true);
            btn_resolve_comment.Enable(!active->resolved);
            btn_delete_comment.Enable(true);
        }
        else {
            active_comment_id.Clear();
            if(!edit_comment.HasFocus())
                edit_comment.Clear();
            lbl_comment_context.SetText("Select text and add a review comment.");
            btn_comment_update.Enable(false);
            btn_comment_resolve.Enable(false);
            btn_comment_delete.Enable(false);
            btn_resolve_comment.Enable(false);
            btn_delete_comment.Enable(false);
        }
    }

    void RefreshStatus()
    {
        UiDocRange selection = OrderedSelection(doc_editor);
        int words = CountWords(doc_editor.GetTextW());
        int comments = doc_editor.GetComments().GetCount();

        lbl_status_left.SetText(status_message.IsEmpty() ? String("Ready") : status_message);
        lbl_status_center.SetText(
            Format("%d word%s | %d characters | selection %d",
                   words, words == 1 ? "" : "s",
                   doc_editor.GetLength(), selection.GetLength()));
        lbl_status_right.SetText(
            Format("Revision %s | %d comment%s | %d match%s",
                   AsString((int64)doc_editor.Core().GetRevision()),
                   comments, comments == 1 ? "" : "s",
                   doc_editor.GetSearchMatchCount(),
                   doc_editor.GetSearchMatchCount() == 1 ? "" : "es"));
    }

    void SetStatus(const String& message)
    {
        status_message = message;
        RefreshStatus();
    }

    static void SetupRibbonPage(UiBoxLayout& page)
    {
        page.SetDirection(UiDirection::H)
            .SetGap(DPI(2))
            .SetInset(Rect(DPI(8), DPI(5), DPI(8), DPI(6)))
            .SetWrap(UiBoxWrap::Flow)
            .SetWrapAutoResize(true)
            .SetAlignItems(UiCrossAlign::Start);
    }

    static void SetupRibbonGroup(UiGroupPanel& panel, UiBoxLayout& body,
                                 const String& title, UiDirection direction)
    {
        panel.SetTitle(title)
             .SetHeaderMode(UiGroupPanel::Inside)
             .SetInset(Rect(DPI(6), DPI(3), DPI(6), DPI(4)))
             .SetHeaderInset(Rect(DPI(6), DPI(2), DPI(6), DPI(1)))
             .SetTitleFont(SansSerifZ(DPI(9)))
             .SetLine(true);

        body.SetDirection(direction)
            .SetGap(DPI(3))
            .SetInset(0)
            .SetAlignItems(UiCrossAlign::Center);
        panel.SetContent(body);
    }

    static void ConfigureTextButton(UiButton& button, const String& text)
    {
        button.SetText(text)
              .SetContentInset(DPI(2))
              .SetContentGap(DPI(3));
    }

    static void ConfigureIconButton(UiButton& button, const Image& icon,
                                    const String& tip)
    {
        button.SetText("")
              .SetIcon(icon)
              .SetIconSide(UiAlign::CENTER)
              .SetIconSize(DPI(15), DPI(15))
              .SetIconRenderMode(UiIconRenderMode::MonoTint)
              .SetContentInset(DPI(2));
        button.Tip(tip);
    }

    static void ConfigureRibbonButton(UiButton& button, const String& text,
                                      const Image& icon = Image())
    {
        button.SetText(text)
              .SetContentInset(DPI(2))
              .SetContentGap(DPI(4));
        if(!icon.IsEmpty())
            button.SetIcon(icon)
                  .SetIconSide(UiAlign::LEFT)
                  .SetIconSize(DPI(14), DPI(14))
                  .SetIconRenderMode(UiIconRenderMode::MonoTint);
    }
};

GUI_APP_MAIN
{
    UiDocDemoWindow().Run();
}
