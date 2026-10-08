#include <Ui/UiFileBrowser/UiFileBrowser.h>
#include <atomic>
#include <chrono>
#include <plugin/png/png.h>
#include <thread>

namespace Upp
{

struct UiFileBrowserTest
{
    static bool Run(const String &root, const String &report)
    {
        Vector<String> failures;
        int checks = 0;
        // Retain the last completed check if a native assertion/crash interrupts the run.
        SaveFile(report, "RUNNING: constructing browser\n");
        auto check = [&](bool ok, const String &label)
        {
            ++checks;
            if(!ok)
                failures.Add(label);
            SaveFile(report, Format("%d checks\nRUNNING: %s\n", checks, label));
        };
        TopWindow window;
        UiFileBrowser browser;
        window.SetRect(0, 0, DPI(1240), DPI(760));
        window.Add(browser.SizePos());
        window.Open();
        auto wait = [&]
        {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while(browser.IsScanning() && std::chrono::steady_clock::now() < deadline)
            {
                Ctrl::ProcessEvents();
                Sleep(2);
            }
            Ctrl::ProcessEvents();
            check(!browser.IsScanning() && browser.scan_error_.IsEmpty(), "scan completes without error");
        };
        browser.SetFolder(root);
        wait();
        check(browser.GetEntryCount() == 7, "browser consumes grouped scan result");
        for(const auto &entry : browser.model_->Entries())
            if(entry.friendly_name == "010_020_comp_v012")
            {
                check(entry.pattern == "010_020_comp_v012.%04d.exr",
                      "sequence pattern is literal printf syntax");
                browser.pattern_mode_.SelectByData(1);
                check(browser.EntryDisplayName(entry) == "010_020_comp_v012.####.exr",
                      "hash pattern presentation");
                browser.pattern_mode_.SelectByData(0);
            }
        check(browser.FormatFrameNumber(7, 5) == "00007", "frame padding is stable across build modes");
        check(browser.navigation_panel_.GetStyle().metrics.radius == 0 &&
                  browser.shell_.GetItemCount() == 6 &&
                  browser.search_.GetParent() == &browser.filter_layout_,
              "one navigation row and one compact command row; search belongs to its popup");
        browser.ShowOptions(true);
        check(browser.options_popup_.IsOpen() &&
                  browser.filter_group_.GetParent() == &browser.options_layout_ &&
                  !browser.sequence_group_.GetParent(),
              "Filter popup contains only search/filter controls");
        check(browser.filter_title_.GetStyle().font == browser.places_title_.GetStyle().font &&
                  browser.filter_title_.GetStyle().palette.ink[ST_NORMAL] ==
                      browser.places_title_.GetStyle().palette.ink[ST_NORMAL] &&
                  browser.filter_group_.GetStyle().metrics.radius > 0,
              "option cards use rounded surfaces and shared small muted headings");
        bool spaced_commands = true;
        for(UiToolButton *button : {&browser.new_button_, &browser.filter_button_, &browser.sequence_button_,
                                    &browser.view_button_, &browser.sort_button_})
            spaced_commands &= button->GetContentGap() >= DPI(8) &&
                               button->GetStyle().font == browser.places_title_.GetStyle().font;
        check(spaced_commands,
              "Commands use sidebar-heading fonts and at least eight pixels between icon and text");
        UiFileBrowserEntry folder_colour;
        folder_colour.kind = UiFileBrowserEntryKind::Folder;
        Color folder_ink = UiFileBrowserAppearance::TypeInk(folder_colour);
        check(browser.breadcrumbs_.GetStyle().text_ink ==
                      UiTheme::ResolveToolButton().palette.ink[ST_NORMAL] &&
                  browser.breadcrumbs_.GetStyle().current_ink == folder_ink &&
                  browser.breadcrumbs_.GetStyle().current_font.IsBold(),
              "Only the bold current breadcrumb uses folder blue");
        check(!browser.mode_button_.GetStyle().metrics.face_enabled &&
                  !browser.mode_button_.GetStyle().metrics.frame_enabled,
              "Mode selection uses coloured ink without a pill background");
        browser.ShowOptions(false);
        browser.sequence_button_.WhenAction();
        Ctrl::ProcessEvents();
        check(browser.options_popup_.IsOpen() &&
                  browser.sequence_group_.GetParent() == &browser.options_layout_ &&
                  !browser.filter_group_.GetParent(),
              "Sequence command opens only sequence controls");
        browser.sequence_mode_.OpenPopup();
        for(int i = 0; i < 15; ++i)
        {
            Ctrl::ProcessEvents();
            Sleep(2);
        }
        check(browser.options_popup_.IsOpen() && browser.sequence_mode_.IsPopupOpen(),
              "Owned dropdown keeps its options popup open");
        browser.sequence_mode_.Key(K_ESCAPE, 1);
        for(int i = 0; i < 15; ++i)
        {
            Ctrl::ProcessEvents();
            Sleep(2);
        }
        check(browser.options_popup_.IsOpen() && !browser.sequence_mode_.IsPopupOpen(),
              "Escape dismisses nested dropdown first");
        browser.options_popup_.Key(K_ESCAPE, 1);
        check(!browser.options_popup_.IsOpen() && !browser.options_open_,
              "Escape dismisses the options popup without cancelling the dialog");
        browser.Key(K_CTRL_F, 1);
        browser.Key(K_CTRL_F, 1);
        check(browser.options_open_ && browser.search_.HasFocus(),
              "Repeated Ctrl+F retains the search popup and focus");
        browser.filter_button_.LeftDown(browser.filter_button_.GetSize() / 2, 0);
        browser.options_popup_.WhenDismiss();
        check(browser.options_open_ && browser.filter_button_.HasCapture(),
              "Held anchor click defers dismissal until mouse-up");
        browser.filter_button_.LeftUp(browser.filter_button_.GetSize() / 2, 0);
        check(!browser.options_open_, "Anchor mouse-up toggles its popup closed");
        browser.CloseOptions();
        browser.sort_button_.WhenAction();
        Ctrl::ProcessEvents();
        check(browser.sort_group_.GetParent() == &browser.options_layout_, "Sort has its own popup");
        browser.table_.SetFocus();
        for(int i = 0; i < 15; ++i)
        {
            Ctrl::ProcessEvents();
            Sleep(2);
        }
        check(!browser.options_open_, "Outside focus dismisses options");
        browser.view_button_.WhenAction();
        Ctrl::ProcessEvents();
        check(browser.view_group_.GetParent() == &browser.options_layout_ &&
                  browser.details_button_.GetParent() == &browser.view_display_row_,
              "View popup contains display modes and inspector switches");
        ImageDraw popup_draw(browser.options_popup_.GetSize());
        browser.options_popup_.DrawCtrl(popup_draw);
        PNGEncoder().SaveFile(report + "-view-popup.png", popup_draw);
        browser.CloseOptions();
        bool hint = false, small_headings = true;
        for(int i = 0; i < browser.places_.Model().GetCount(); ++i)
        {
            const auto &item = browser.places_.Model().Get(i);
            hint |= TrimBoth(item.text) == "Drop here" && item.text.StartsWith("  ") && !item.enabled &&
                    IsNull(item.data);
            if(item.group_header)
                small_headings &=
                    item.use_custom_font && item.custom_font.GetHeight() < browser.GetFont().GetHeight();
        }
        check(hint && small_headings, "small sidebar headings and non-navigable drop hint");
        check(browser.breadcrumbs_.GetCount() > 2, "Windows path has separate clickable crumbs");
        int last = browser.breadcrumbs_.GetCount() - 1;
        check(NormalizePath(AsString(browser.breadcrumbs_.GetItemData(last))) == NormalizePath(root),
              "crumb stores absolute folder");
        browser.folder_ = "\\\\renderhost\\shots\\sequence";
        browser.RefreshBreadcrumbs();
        check(browser.breadcrumbs_.GetCount() == 2 &&
                  AsString(browser.breadcrumbs_.GetItemData(0)) == "\\\\renderhost\\shots",
              "UNC breadcrumbs stop at the share without network access");
        browser.folder_ = root;
        browser.RefreshBreadcrumbs();
        String parent = GetFileFolder(root);
        browser.breadcrumbs_.WhenAction(last - 1);
        wait();
        check(NormalizePath(browser.GetFolder()) == NormalizePath(parent), "parent crumb navigates");
        browser.GoBack();
        wait();
        check(NormalizePath(browser.GetFolder()) == NormalizePath(root), "back restores previous folder");
        browser.GoForward();
        wait();
        check(NormalizePath(browser.GetFolder()) == NormalizePath(parent), "forward restores folder");
        browser.SetFolder(root);
        wait();
        browser.search_.SetData("no-matching-fixture-entry");
        browser.RefreshProjection(true);
        check(!browser.HasSelection() && browser.rows_.IsEmpty(), "filtered-out selection cannot be opened");
        browser.search_.SetData("");
        browser.RefreshProjection(true);
        auto medium_font = browser.GetFont();
        int medium_rows = browser.places_.GetStyle().row_height;
        String selected_path = browser.GetSelection().path;
        browser.SetViewSize(UiFileBrowser::ViewSize::Small);
        check(browser.GetFont().GetHeight() < medium_font.GetHeight() &&
                  browser.places_.GetStyle().row_height < medium_rows,
              "Small reduces fonts and sidebar spacing together");
        check(browser.GetSelection().path == selected_path && !browser.IsScanning(),
              "view size preserves selection without rescanning");
        browser.SetViewSize(UiFileBrowser::ViewSize::Large);
        Ctrl::ProcessEvents();
        check(browser.GetFont().GetHeight() > medium_font.GetHeight() &&
                  browser.places_.GetStyle().row_height > medium_rows,
              "Large increases fonts and sidebar spacing together");
        check(browser.name_label_.GetSize().cx >= browser.name_label_.GetMinSize().cx &&
                  browser.meta_labels_[1].GetSize().cx >= browser.meta_labels_[1].GetMinSize().cx,
              "Large remeasures footer and metadata labels without clipping");
        browser.SetViewSize(UiFileBrowser::ViewSize::Medium);
        check(browser.GetFont() == medium_font && browser.places_.GetStyle().row_height == medium_rows,
              "returning to Medium restores dimensions without accumulated scaling");
        int sequence_row = -1;
        for(int i = 0; i < browser.rows_.GetCount(); ++i)
            if((*browser.model_)[browser.rows_[i].entry].friendly_name == "010_020_comp_v012")
                sequence_row = i;
        if(sequence_row >= 0)
        {
            int entry = browser.rows_[sequence_row].entry;
            int before = browser.rows_.GetCount();
            browser.table_.SetActiveCell(sequence_row, 0);
            browser.table_.Key(K_ENTER, 1);
            check(browser.IsSequenceExpanded(entry) && browser.rows_.GetCount() > before,
                  "compact disclosure column expands sequence members");
            browser.table_.SetActiveCell(sequence_row, 0);
            browser.table_.Key(K_ENTER, 1);
            check(!browser.IsSequenceExpanded(entry) && browser.rows_.GetCount() == before,
                  "sequence disclosure collapses without navigating or opening media");
        }
        int folder_row = -1;
        for(int i = 0; i < browser.rows_.GetCount(); ++i)
            if((*browser.model_)[browser.rows_[i].entry].name == "archive")
                folder_row = i;
        check(folder_row >= 0, "fixture folder row exists");
        if(folder_row >= 0)
        {
            const auto &style = browser.table_.GetStyle();
            Point p(style.metrics.content_margin.left + browser.table_.GetColumnWidth(0) + DPI(40),
                    style.metrics.content_margin.top + style.header_height + folder_row * style.row_height +
                        style.row_height / 2);
            browser.table_.LeftDouble(p, 0);
            wait();
            check(GetFileName(browser.GetFolder()) == "archive",
                  "read-only table double-click activates directory");
            browser.GoUp();
            wait();
            browser.table_.SetActiveCell(folder_row, 1);
            browser.table_.Key(K_ENTER, 1);
            wait();
            check(GetFileName(browser.GetFolder()) == "archive", "table Enter activates directory");
        }
        browser.SetFolder(root);
        wait();
        browser.AddPlace(root).AddPlace(root);
        int pins = 0;
        for(int i = 0; i < browser.places_.Model().GetCount(); ++i)
        {
            const auto &item = browser.places_.Model().Get(i);
            if(!IsNull(item.data) && NormalizePath(AsString(item.data)) == NormalizePath(root) &&
               !item.group_header)
                ++pins;
        }
        check(pins <= 2 && pins > 0, "pinned folder deduplicated (recent may also show it)");
        browser.AddProductionRoot("Test production", root);
        bool roots = false, recent = false, drives = false, documents = false, generic_roots = true;
        for(int i = 0; i < browser.places_.Model().GetCount(); ++i)
        {
            const auto &item = browser.places_.Model().Get(i);
            roots |= item.text == "Test production";
            generic_roots &= item.text != "PRODUCTION ROOTS";
            recent |= item.text == "RECENT";
            drives |= item.text.Find(":/") >= 0;
            documents |= item.text == "Documents";
        }
        check(roots && recent && drives && documents && generic_roots,
              "generic locations, recent, known folders and drives populated");
        auto original_theme = UiTheme::GetContext();
        auto follow_host = [&]
        {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while(
                (browser.theme_revision_ != UiTheme::GetRevision() ||
                 (!browser.font_override_ && browser.base_font_ != UiFileBrowser::ResolveInheritedFont())) &&
                std::chrono::steady_clock::now() < deadline)
            {
                Ctrl::ProcessEvents();
                Sleep(2);
            }
            Ctrl::ProcessEvents();
        };
        check(!browser.theme_button_.IsShown(), "embedded browser leaves theme switch to host by default");
        UiThemeMode requested = original_theme.mode;
        browser.WhenThemeRequested = [&](UiThemeMode mode) { requested = mode; };
        browser.ShowThemeButton().theme_button_.WhenAction();
        check(requested != original_theme.mode && UiTheme::GetContext().mode == original_theme.mode,
              "theme action requests host policy without changing global theme");
        Color old_header = browser.table_.GetStyle().header_bg;
        auto opposite_theme = original_theme;
        opposite_theme.mode = requested;
        UiTheme::Set(opposite_theme);
        follow_host();
        check(browser.table_.GetStyle().header_bg != old_header, "header responds to theme change");
        UiTheme::Set(original_theme);
        follow_host();
        check(browser.table_.GetStyle().header_bg == old_header, "header restores original theme");
        for(auto preset :
            {UiThemePreset::Minimal, UiThemePreset::Pill, UiThemePreset::Linear, UiThemePreset::Solid,
             UiThemePreset::Outline, UiThemePreset::Compact, UiThemePreset::Layered})
        {
            auto host = original_theme;
            host.preset = preset;
            UiTheme::Set(host);
            follow_host();
            UiTable host_table;
            check(browser.cancel_button_.GetStyle().metrics.radius ==
                          UiTheme::ResolveButton().metrics.radius &&
                      browser.view_size_buttons_[1].GetStyle().metrics.radius ==
                          UiTheme::ResolveToolButton().metrics.radius &&
                      browser.table_.GetStyle().table_bg == host_table.GetStyle().table_bg &&
                      browser.browser_panel_.GetStyle().metrics.radius == 0,
                  "host preset controls button geometry and background; browser retains square surfaces");
        }
        UiTheme::Set(original_theme);
        follow_host();
        Font original_font = GetStdFont();
        Font host_font = Font(original_font).Height(original_font.GetHeight() + DPI(7));
        SetStdFont(host_font);
        follow_host();
        check(browser.GetFont() == host_font, "browser follows a host font change without rebuilding");
        check(browser.name_label_.GetSize().cy >= browser.name_label_.GetMinSize().cy &&
                  browser.table_.GetStyle().row_height > host_font.GetHeight(),
              "host font change remeasures layout and row heights");
        browser.SetFont(original_font);
        UiTheme::Set(opposite_theme);
        follow_host();
        check(browser.GetFont() == original_font, "explicit browser font survives a host theme change");
        browser.UseThemeFont();
        check(browser.GetFont() == host_font, "UseThemeFont restores application font inheritance");
        SetStdFont(original_font);
        UiTheme::Set(original_theme);
        follow_host();
        // A shared family change is independent of the application font height.
        // The Body/Code roles must invalidate inherited snapshots while caller
        // overrides remain explicit, and an unchanged poll must not rebuild UI.
        UiTypography original_typography = UiFonts::GetTypography();
        UiTypography typography = original_typography;
        typography.body = "system:Consolas";
        typography.code = "system:Courier New";
        UiFonts::SetTypography(typography);
        follow_host();
        Font resolved_body = UiFileBrowser::ResolveInheritedFont();
        Font resolved_code =
            UiFonts::Inherit(Monospace(max(1, browser.font_.GetHeight() - DPI(2))), UiTypographyRole::Code);
        check(browser.base_font_ == resolved_body &&
                  browser.base_font_.GetHeight() == original_font.GetHeight() &&
                  browser.base_font_.GetFaceName() == "Consolas",
              "Shared Body family change preserves host height and updates the inherited browser font");
        check(browser.preview_text_.GetStyle().font == resolved_code &&
                  resolved_code.GetFaceName() == "Courier New",
              "Text preview follows the shared Code family while retaining its compact height");
        int poll_notifications = 0;
        auto original_change = browser.table_.Model().WhenChange;
        browser.table_.Model().WhenChange = [&](const UiModelChange &change)
        {
            ++poll_notifications;
            original_change(change);
        };
        browser.PollTheme();
        browser.table_.Model().WhenChange = original_change;
        check(poll_notifications == 0 && browser.base_font_ == resolved_body,
              "Unchanged typography poll avoids repeated table projection and restyling");
        browser.SetFont(original_font);
        typography.body = "system:Courier New";
        UiFonts::SetTypography(typography);
        follow_host();
        check(browser.GetFont() == original_font, "Explicit SetFont survives a shared Body-family change");
        browser.UseThemeFont();
        check(browser.base_font_ == UiFileBrowser::ResolveInheritedFont() && !browser.font_override_,
              "UseThemeFont restores the current shared Body-family selection");
        UiFonts::SetTypography(original_typography);
        follow_host();
        browser.ShowOptions(true).SetMode(UiFileBrowser::Mode::Simple);
        check(!browser.commands_panel_.GetParent() && browser.shell_.GetItemCount() == 5 &&
                  browser.new_button_.GetParent() == &browser.navigation_layout_,
              "Simple hides advanced commands and keeps New folder in navigation");
        browser.Key(K_CTRL_F, 1);
        check(browser.options_open_ && browser.options_popup_.IsOpen() && browser.search_.HasFocus() &&
                  !browser.filter_enable_.IsEnabled(),
              "Ctrl+F opens search in Simple mode; advanced filter rules remain inactive");
        browser.mode_button_.WhenAction();
        check(browser.GetMode() == UiFileBrowser::Mode::Advanced && browser.mode_button_.IsChecked(),
              "single mode icon switches Simple to Advanced");
        bool old_inspector = browser.show_inspector_.IsOn();
        browser.show_inspector_.Key(K_SPACE, 1);
        check(browser.show_inspector_.IsOn() != old_inspector && !browser.inspector_panel_.GetParent(),
              "inspector switch responds to keyboard and removes its workspace panel");
        browser.show_inspector_.Key(K_SPACE, 1);
        browser.SetMode(UiFileBrowser::Mode::Advanced);
        browser.Key(K_CTRL_L, 1);
        check(browser.address_stack_.GetActiveKey() == "edit", "Ctrl+L edits address");
        browser.Key(K_ESCAPE, 1);
        check(browser.address_stack_.GetActiveKey() == "crumbs", "Escape restores crumbs before cancelling");
        int cancels = 0;
        browser.WhenCancel = [&] { ++cancels; };
        browser.cancel_button_.WhenAction();
        browser.Key(K_ESCAPE, 1);
        check(cancels == 2, "Cancel and Escape notify host");
        // Bulk notifications are checked at the shared model seam.
        UiTableModel model;
        int notifications = 0;
        model.WhenChange = [&](const UiModelChange &) { ++notifications; };
        {
            UiModelUpdate update(model);
            model.SetSize(1000, 2);
            {
                UiModelUpdate nested(model);
                for(int i = 0; i < 1000; ++i)
                    model.SetCellValue(i, 0, i);
            }
        }
        check(notifications == 1 && (int)model.GetCellValue(999, 0) == 999,
              "nested bulk update emits one final reset");
        // Content previews exercise real codecs and selection supersession.
        ImageBuffer image(320, 160);
        Fill(image.Begin(), RGBAZero(), image.GetLength());
        for(int i = 0; i < image.GetLength(); ++i)
        {
            image.Begin()[i].r = 160;
            image.Begin()[i].g = 80;
            image.Begin()[i].b = 40;
            image.Begin()[i].a = 255;
        }
        PNGEncoder().SaveFile(AppendFileName(root, "slate.png"), Image(image));
        String text_path = AppendFileName(root, "preview.json");
        String preview_source;
        for(int i = 1; i <= 20; ++i)
            preview_source << Format("line %d: ", i) << String('x', 80) << "\n";
        SaveFile(text_path, preview_source);
        SaveFile(AppendFileName(root, "preview.url"),
                 "[InternetShortcut]\r\nURL=https://example.invalid/review?frame=1\r\nIconIndex=0\r\n");
        SaveFile(AppendFileName(root, "preview.inc"), "// include fragment\nint sample = 1;\n");
        String jpeg_source = AppendFileName(GetCurrentDirectory(), "build/fixtures/still.jpg");
        String jpeg_path = AppendFileName(root, "preview.jpg");
        check(FileExists(jpeg_source) && FileCopy(jpeg_source, jpeg_path),
              "JPEG preview fixture from existing generated media");
        browser.RefreshFolder();
        wait();
        auto select_name = [&](const String &name)
        {
            for(int i = 0; i < browser.model_->GetCount(); ++i)
                if((*browser.model_)[i].name == name)
                {
                    browser.SelectEntry(i);
                    return true;
                }
            return false;
        };
        auto await_preview = [&](const String &page)
        {
            auto until = std::chrono::steady_clock::now() + std::chrono::seconds(8);
            do
            {
                Ctrl::ProcessEvents();
                if(browser.preview_content_.GetActiveKey() == page)
                    return true;
                Sleep(2);
            } while(std::chrono::steady_clock::now() < until);
            return false;
        };
        check(select_name("slate.png") && await_preview("image") &&
                  browser.preview_image_.GetIcon().GetSize() == Size(320, 160),
              "PNG content preview retains aspect ratio");
        check(browser.preview_content_.GetSize().cx > DPI(200) &&
                  browser.preview_content_.GetSize().cy > DPI(100),
              "Preview content uses the inspector's available width and height");
        ImageDraw png_draw(window.GetSize());
        window.DrawCtrl(png_draw);
        PNGEncoder().SaveFile(report + "-image.png", png_draw);
        check(select_name("preview.jpg") && await_preview("image") &&
                  browser.meta_values_[5].GetText() == "384 × 216" &&
                  browser.preview_subtitle_.GetText().IsEmpty(),
              "JPEG preview uses native codec without an Imaging dependency");
        check(select_name("preview.json") && await_preview("text") &&
                  browser.preview_text_.GetText().Find("line 10:") >= 0 &&
                  browser.preview_text_.GetText().Find("line 11:") < 0 &&
                  browser.preview_text_.GetMinSize().cy >=
                      10 * browser.preview_text_.GetStyle().font.GetHeight(),
              "Text preview shows ten actual lines without wrapping");
        check(browser.preview_text_scroll_.GetContentSize().cx > browser.preview_text_scroll_.GetSize().cx,
              "Long text lines can be scrolled horizontally");
        browser.preview_text_scroll_.SetScrollPos(Point(0, 10000));
        check(browser.preview_text_scroll_.GetScrollPos().y > 0 ||
                  browser.preview_text_scroll_.GetContentSize().cy <=
                      browser.preview_text_scroll_.GetSize().cy,
              "All ten text lines fit or remain reachable by vertical scrolling");
        check(browser.preview_subtitle_.GetText().IsEmpty() && browser.inspector_name_.IsSelectable(),
              "Preview has no redundant caption; details filename is selectable");
        for(auto size :
            {UiFileBrowser::ViewSize::Small, UiFileBrowser::ViewSize::Medium, UiFileBrowser::ViewSize::Large})
        {
            browser.SetViewSize(size);
            check(browser.preview_text_.GetStyle().font.GetHeight() == browser.GetFont().GetHeight() - DPI(2),
                  "Each density keeps preview text two points smaller");
        }
        browser.SetViewSize(UiFileBrowser::ViewSize::Medium);
        browser.preview_text_scroll_.SetScrollPos(Point(0, 0));
        ImageDraw text_draw(window.GetSize());
        window.DrawCtrl(text_draw);
        PNGEncoder().SaveFile(report + "-text.png", text_draw);
        UiThemeContext preview_theme = UiTheme::GetContext();
        UiThemeContext dark_preview = preview_theme;
        dark_preview.mode = UiThemeMode::Dark;
        UiTheme::Set(dark_preview);
        browser.ApplyTheme();
        Ctrl::ProcessEvents();
        ImageDraw dark_draw(window.GetSize());
        window.DrawCtrl(dark_draw);
        PNGEncoder().SaveFile(report + "-text-dark.png", dark_draw);
        UiTheme::Set(preview_theme);
        browser.ApplyTheme();
        Ctrl::ProcessEvents();
        check(select_name("preview.url") && await_preview("text") &&
                  browser.preview_text_.GetText() == "https://example.invalid/review?frame=1",
              "URL preview shows stored target without navigation or network access");
        UiFileBrowserPreviewRequest safe_preview;
        safe_preview.path = AppendFileName(root, "preview.json");
        check(LoadFileBrowserPreview(safe_preview, {}).type == UiFileBrowserPreviewData::Type::Text,
              "Public preview accepts an omitted cancellation callback");
        safe_preview.max_size = Size(INT_MAX, INT_MAX);
        check(LoadFileBrowserPreview(safe_preview, {}).info == "Invalid preview limits",
              "Invalid thumbnail dimensions are rejected before allocation or decoding");
        check(select_name("preview.inc") && await_preview("text") &&
                  browser.preview_text_.GetText().StartsWith("// include fragment"),
              "Include fragments have plain text previews");
        browser.new_button_.WhenAction();
        Ctrl::ProcessEvents();
        // Commit through the native dropdown popup, not a manually fired callback.
        auto choose_new_kind = [&](dword letter, int kind)
        {
            browser.new_item_kind_.OpenPopup();
            Ctrl::ProcessEvents();
            Ctrl *popup = nullptr;
            for(Ctrl *top : Ctrl::GetTopCtrls())
                if(top->GetOwnerCtrl() == &browser.new_item_kind_)
                {
                    popup = top;
                    break;
                }
            if(!popup)
                return false;
            popup->Key(letter, 1);
            popup->Key(K_ENTER, 1);
            Ctrl::ProcessEvents();
            return browser.options_open_ && !browser.new_item_kind_.IsPopupOpen() &&
                   (int)browser.new_item_kind_.GetData() == kind;
        };
        int folder_height = browser.options_popup_.GetSize().cy;
        check(choose_new_kind('J', 4) &&
                  browser.new_image_size_row_.GetParent() == &browser.new_item_layout_ &&
                  browser.new_image_fill_row_.GetParent() == &browser.new_item_layout_ &&
                  browser.options_popup_.GetSize().cy > folder_height &&
                  !browser.new_image_swatch_.GetScreenRect().IsEmpty(),
              "Folder to JPEG immediately exposes dimensions and colour without reopening New");
        ImageDraw jpeg_fields(browser.options_popup_.GetSize());
        browser.options_popup_.DrawCtrl(jpeg_fields);
        PNGEncoder().SaveFile(report + "-new-jpeg-fields.png", jpeg_fields);
        browser.new_item_name_.SetData("keep-name");
        browser.new_item_error_.SetText("old image error");
        browser.new_image_swatch_.WhenAction();
        Ctrl::ProcessEvents();
        check(choose_new_kind('F', 0) && !browser.new_image_size_row_.GetParent() &&
                  !browser.new_image_fill_row_.GetParent() && !browser.colour_popup_.IsOpen() &&
                  browser.new_item_error_.GetText().IsEmpty() &&
                  AsString(browser.new_item_name_.GetData()) == "keep-name" &&
                  browser.options_popup_.GetSize().cy == folder_height,
              "JPEG to Folder removes image fields, dismisses colour picker and preserves the typed name");
        browser.new_item_name_.SetData("");
        ImageDraw folder_fields(browser.options_popup_.GetSize());
        browser.options_popup_.DrawCtrl(folder_fields);
        PNGEncoder().SaveFile(report + "-new-folder-fields.png", folder_fields);
        check(choose_new_kind('P', 3) &&
                  browser.new_image_size_row_.GetParent() == &browser.new_item_layout_ &&
                  browser.new_image_fill_row_.GetParent() == &browser.new_item_layout_,
              "Folder to PNG immediately restores image fields in the same New popup");
        ImageDraw png_fields(browser.options_popup_.GetSize());
        browser.options_popup_.DrawCtrl(png_fields);
        PNGEncoder().SaveFile(report + "-new-png-fields.png", png_fields);
        check(choose_new_kind('F', 0) && !browser.new_image_size_row_.GetParent() &&
                  !browser.new_image_fill_row_.GetParent(),
              "PNG to Folder detaches both image rows without reopening");
        browser.new_item_name_.SetData("../outside");
        browser.create_item_button_.WhenAction();
        check(browser.options_open_ && !browser.new_item_error_.GetText().IsEmpty(),
              "New folder rejects traversal without closing");
        browser.new_item_name_.SetData("archive");
        browser.create_item_button_.WhenAction();
        check(browser.options_open_ && browser.new_item_error_.GetText().Find("exists") >= 0,
              "New folder refuses an existing name");
        browser.new_item_name_.SetData("new folder test");
        browser.new_item_name_.WhenAction();
        wait();
        check(!browser.options_open_ && DirectoryExists(AppendFileName(root, "new folder test")) &&
                  GetFileName(browser.GetSelection().path) == "new folder test",
              "Create folder refreshes and selects the new directory");
        check(DirectoryDelete(AppendFileName(root, "new folder test")), "Remove the owned empty test folder");
        browser.RefreshFolder();
        wait();
        check(browser.inspector_name_.GetStyle().palette.ink[ST_NORMAL] ==
                      UiFileBrowserAppearance::TypeInk(folder_colour) &&
                  browser.open_button_.GetStyle().palette.ink[ST_NORMAL] ==
                      UiFileBrowserAppearance::TypeInk(folder_colour) &&
                  browser.cancel_button_.GetStyle().metrics.frame_enabled &&
                  browser.cancel_button_.GetStyle().palette.frame[ST_NORMAL].GetR() >
                      browser.cancel_button_.GetStyle().palette.frame[ST_NORMAL].GetB(),
              "Filename and Open share folder blue; Cancel has a quiet red outline");
        browser.new_button_.WhenAction();
        browser.new_item_kind_.SelectByData(1);
        browser.new_item_name_.SetData("starter-note");
        browser.create_item_button_.WhenAction();
        wait();
        String note = AppendFileName(root, "starter-note.txt");
        check(FileExists(note) && LoadFile(note).IsEmpty() &&
                  GetFileName(browser.GetSelection().path) == "starter-note.txt",
              "New text file appends .txt, starts empty and is selected");
        SaveFile(note, "keep this content");
        browser.new_button_.WhenAction();
        browser.new_item_name_.SetData("starter-note.txt");
        browser.create_item_button_.WhenAction();
        check(browser.options_open_ && LoadFile(note) == "keep this content",
              "Creating an existing text file never overwrites it");
        browser.CloseOptions();
        UiFileBrowserCreateRequest create;
        String created, error;
        create.kind = UiFileBrowserCreateKind::CppPair;
        create.name = "starter-code.cpp";
        check(CreateFileBrowserItem(root, create, created, error) &&
                  GetFileName(created) == "starter-code.h" &&
                  LoadFile(AppendFileName(root, "starter-code.cpp")).Find("#include \"starter-code.h\"") >= 0,
              "C++ pair creates a header and matching include in the source");
        String collision = AppendFileName(root, "starter-collision.cpp");
        SaveFile(collision, "existing source");
        create.name = "starter-collision";
        check(!CreateFileBrowserItem(root, create, created, error) &&
                  !FileExists(AppendFileName(root, "starter-collision.h")) &&
                  LoadFile(collision) == "existing source",
              "Source collision rolls back only the newly created header");
        create.kind = UiFileBrowserCreateKind::Text;
        create.name = "NUL.txt";
        check(!CreateFileBrowserItem(root, create, created, error), "Reserved device names are rejected");
        create.name = "../escape";
        check(!CreateFileBrowserItem(root, create, created, error), "File creation rejects traversal");
        Color fill;
        check(UiColorPickerMicro::ParseHex("#d0d0d0", fill) && fill == Color(208, 208, 208) &&
                  !UiColorPickerMicro::ParseHex("#12345Z", fill),
              "Fill accepts six-digit hex and rejects invalid colour input");
        browser.new_button_.WhenAction();
        browser.new_item_kind_.SelectByData(3);
        browser.new_image_width_.SetData("128");
        browser.new_image_height_.SetData("64");
        browser.new_image_colour_.SetData("#204060");
        browser.new_image_colour_.WhenChange();
        browser.new_item_name_.SetData("starter-solid");
        check(browser.new_image_size_row_.GetParent() == &browser.new_item_layout_ &&
                  browser.new_image_swatch_.GetParent() == &browser.new_image_fill_row_,
              "Image creation shows size and live fill swatch");
        browser.new_image_swatch_.WhenAction();
        Ctrl::ProcessEvents();
        check(browser.colour_popup_.IsOpen() && browser.options_open_ &&
                  browser.colour_popup_.GetOwner() == &browser.options_popup_ &&
                  browser.colour_micro_.GetPaletteCount() == 40 &&
                  browser.colour_popup_.GetSize().cx < DPI(280) &&
                  browser.colour_popup_.GetSize().cy < DPI(240),
              "Swatch opens a small owned 8 by 5 colour popup and keeps New alive");
        Ctrl *ramps = browser.colour_micro_.GetFirstChild()->GetFirstChild()->GetNext();
        Ctrl *editor_row = ramps->GetNext();
        auto *mode_button = dynamic_cast<UiToolButton *>(editor_row->GetLastChild()->GetPrev());
        if(mode_button)
        {
            Point centre = mode_button->GetSize() / 2;
            mode_button->LeftDown(centre, 0);
            mode_button->LeftUp(centre, 0);
        }
        Ctrl::ProcessEvents();
        check(mode_button && browser.colour_micro_.IsRGBMode() && browser.colour_popup_.IsOpen() &&
                  browser.colour_popup_.GetSize() == browser.colour_micro_.GetMinSize() &&
                  browser.colour_popup_.GetSize().cx < DPI(280),
              "Sliders icon expands RGB below the footer and remeasures without committing");
        Ctrl *rgb_row = editor_row->GetNext();
        auto *red_slider = dynamic_cast<UiSlider *>(rgb_row->GetFirstChild()->GetLastChild());
        auto *green_slider = dynamic_cast<UiSlider *>(rgb_row->GetFirstChild()->GetNext()->GetLastChild());
        auto *blue_slider = dynamic_cast<UiSlider *>(rgb_row->GetLastChild()->GetLastChild());
        if(red_slider)
            red_slider->Key(K_RIGHT, 1);
        if(green_slider)
            green_slider->Key(K_RIGHT, 1);
        check(red_slider && green_slider && blue_slider &&
                  browser.colour_micro_.GetColor() == Color(33, 65, 96) && browser.colour_popup_.IsOpen() &&
                  AsString(browser.new_image_colour_.GetData()) == "#204060" &&
                  red_slider->GetTrackGeometry().GetWidth() > 0 &&
                  blue_slider->GetScreenRect().right <= browser.colour_popup_.GetScreenRect().right,
              "Three compact sliders preview exact RGB values without accepting or overflowing");
        Ctrl *red_label = rgb_row->GetFirstChild()->GetFirstChild()->GetFirstChild();
        check(red_slider &&
                  red_label->GetScreenRect().left ==
                      red_slider->GetScreenRect().left + red_slider->GetTrackGeometry().left &&
                  red_slider->GetTrackGeometry().GetWidth() ==
                      red_slider->GetSize().cx - 2 * max(DPI(5), (DPI(10) + 1) / 2),
              "RGB headings align to full available tracks with only the safe thumb inset");
        auto *palette_button = dynamic_cast<UiToolButton *>(editor_row->GetLastChild());
        auto cycle_palette = [&]
        {
            if(palette_button)
            {
                Point centre = palette_button->GetSize() / 2;
                palette_button->LeftDown(centre, 0);
                palette_button->LeftUp(centre, 0);
            }
        };
        Ctrl *first_cell = browser.colour_micro_.GetFirstChild()->GetFirstChild()->GetFirstChild();
        cycle_palette();
        check(browser.colour_micro_.GetPaletteMode() == UiColorPickerMicro::PaletteMode::Greyscale &&
                  first_cell == browser.colour_micro_.GetFirstChild()->GetFirstChild()->GetFirstChild() &&
                  browser.colour_popup_.IsOpen(),
              "Greyscale view reuses palette cells without closing or committing");
        ImageDraw grey_view(browser.colour_popup_.GetSize());
        browser.colour_popup_.DrawCtrl(grey_view);
        PNGEncoder().SaveFile(report + "-micro-greyscale.png", grey_view);
        cycle_palette();
        check(browser.colour_micro_.GetPaletteMode() == UiColorPickerMicro::PaletteMode::Spectrum &&
                  browser.colour_micro_.GetColor() == Color(33, 65, 96),
              "Spectrum view preserves the dialled colour");
        ImageDraw spectrum_view(browser.colour_popup_.GetSize());
        browser.colour_popup_.DrawCtrl(spectrum_view);
        PNGEncoder().SaveFile(report + "-micro-spectrum.png", spectrum_view);
        cycle_palette();
        check(browser.colour_micro_.GetPaletteMode() == UiColorPickerMicro::PaletteMode::Standard &&
                  AsString(browser.new_image_colour_.GetData()) == "#204060",
              "Third palette click restores Standard without accepting the fill");
        auto *grey = dynamic_cast<UiSlider *>(ramps->GetFirstChild());
        auto *hue = dynamic_cast<UiSlider *>(ramps->GetLastChild());
        if(grey)
        {
            grey->Key(K_HOME, 1);
        }
        bool black = browser.colour_micro_.GetColor() == Black();
        if(grey)
        {
            grey->Key(K_END, 1);
        }
        check(grey && black && browser.colour_micro_.GetColor() == White() &&
                  browser.colour_popup_.IsOpen() &&
                  AsString(browser.new_image_colour_.GetData()) == "#204060",
              "Grey ramp reaches black and white without committing the host fill");
        if(hue)
        {
            Rect track = hue->GetTrackGeometry();
            Point middle = track.CenterPoint();
            hue->LeftDown(middle, 0);
            hue->LeftUp(middle, 0);
        }
        Color ramp_colour = browser.colour_micro_.GetColor();
        check(hue && ramp_colour.GetR() <= 8 && ramp_colour.GetG() >= 247 && ramp_colour.GetB() >= 247 &&
                  browser.colour_popup_.IsOpen() &&
                  hue->GetScreenRect().bottom <= browser.colour_popup_.GetScreenRect().bottom,
              "Rainbow ramp mouse selection reaches cyan and remains inside the compact popup");
        ImageDraw rgb_light(browser.colour_popup_.GetSize());
        browser.colour_popup_.DrawCtrl(rgb_light);
        PNGEncoder().SaveFile(report + "-micro-rgb.png", rgb_light);
        UiTheme::Set(opposite_theme);
        follow_host();
        browser.colour_micro_.Layout();
        ImageDraw rgb_dark(browser.colour_popup_.GetSize());
        browser.colour_popup_.DrawCtrl(rgb_dark);
        PNGEncoder().SaveFile(report + "-micro-rgb-dark.png", rgb_dark);
        UiTheme::Set(original_theme);
        follow_host();
        browser.colour_popup_.Key(K_ESCAPE, 1);
        check(browser.options_open_ && AsString(browser.new_image_colour_.GetData()) == "#204060",
              "Cancelling RGB adjustment keeps the original fill");
        browser.new_image_swatch_.WhenAction();
        Ctrl::ProcessEvents();
        browser.colour_micro_.SetColor(Color(12, 34, 56));
        auto *apply_colour = dynamic_cast<UiToolButton *>(editor_row->GetFirstChild());
        if(apply_colour)
        {
            Point centre = apply_colour->GetSize() / 2;
            apply_colour->LeftDown(centre, 0);
            apply_colour->LeftUp(centre, 0);
        }
        check(apply_colour && apply_colour->GetText() == "Apply" && apply_colour->GetSize().cx >= DPI(80) &&
                  !browser.colour_popup_.IsOpen() && browser.options_open_ &&
                  AsString(browser.new_image_colour_.GetData()) == "#0C2238",
              "Current swatch applies RGB adjustment and closes only the palette");
        browser.new_image_colour_.SetData("#204060");
        browser.new_image_colour_.WhenChange();
        browser.new_image_swatch_.WhenAction();
        Ctrl::ProcessEvents();
        if(mode_button)
        {
            Point centre = mode_button->GetSize() / 2;
            mode_button->LeftDown(centre, 0);
            mode_button->LeftUp(centre, 0);
        }
        check(!browser.colour_micro_.IsRGBMode() && browser.colour_micro_.GetHex() == "#204060" &&
                  browser.colour_popup_.IsOpen(),
              "Mode icon restores hex with the same colour and leaves the popup open");
        browser.colour_micro_.SetColor(Color(100, 150, 200));
        auto *micro_hex = dynamic_cast<UiLineEdit *>(editor_row->GetFirstChild()->GetNext());
        check(micro_hex && micro_hex->GetSize().cx >= micro_hex->GetMinSize().cx,
              "Hex entry reserves the complete value plus theme padding at the popup minimum size");
        check(micro_hex && !micro_hex->Key(K_ESCAPE, 1),
              "Hex entry lets Escape reach the popup's Cancel policy");
        browser.colour_popup_.Key(K_ESCAPE, 1);
        check(browser.options_open_ && !browser.colour_popup_.IsOpen() &&
                  AsString(browser.new_image_colour_.GetData()) == "#204060",
              "Micro picker Escape cancels without altering the fill or closing New");
        browser.new_image_swatch_.WhenAction();
        Ctrl::ProcessEvents();
        ImageDraw micro_light(browser.colour_popup_.GetSize());
        browser.colour_popup_.DrawCtrl(micro_light);
        PNGEncoder().SaveFile(report + "-micro-picker.png", micro_light);
        check(!browser.colour_micro_.CommitHex("#12345Z") && browser.colour_popup_.IsOpen(),
              "Invalid hex keeps the micro picker open");
        check(browser.colour_micro_.CommitHex("#6496C8") && !browser.colour_popup_.IsOpen() &&
                  AsString(browser.new_image_colour_.GetData()) == "#6496C8",
              "Micro picker hex commit applies the fill and closes only the palette");
        UiTheme::Set(opposite_theme);
        follow_host();
        browser.new_image_swatch_.WhenAction();
        Ctrl::ProcessEvents();
        ImageDraw micro_dark(browser.colour_popup_.GetSize());
        browser.colour_popup_.DrawCtrl(micro_dark);
        PNGEncoder().SaveFile(report + "-micro-picker-dark.png", micro_dark);
        browser.colour_micro_.Key(K_RIGHT, 1);
        browser.colour_micro_.Key(K_ENTER, 1);
        check(!browser.colour_popup_.IsOpen() &&
                  AsString(browser.new_image_colour_.GetData()) == browser.colour_micro_.GetHex(),
              "Micro picker keyboard choice commits in Dark theme");
        browser.new_image_swatch_.WhenAction();
        browser.CloseOptions();
        check(!browser.colour_popup_.IsOpen() && !browser.options_popup_.IsOpen(),
              "Closing New closes its owned micro picker");
        UiTheme::Set(original_theme);
        follow_host();
        browser.new_button_.WhenAction();
        browser.new_image_colour_.SetData("#204060");
        browser.new_image_colour_.WhenChange();
        browser.new_item_name_.SetData("starter-solid");
        // The same control also runs directly in a normal window without popup policy.
        browser.CloseOptions();
        TopWindow embedded_window;
        UiColorPickerMicro embedded;
        embedded_window.Add(embedded.SizePos());
        embedded_window.SetRect(0, 0, DPI(250), DPI(180));
        embedded_window.Open(&window);
        int micro_commits = 0;
        embedded.WhenAction = [&] { ++micro_commits; };
        embedded.SetData(Color(100, 120, 140));
        check((Color)embedded.GetData() == Color(100, 120, 140) && micro_commits == 0,
              "Embedded micro picker binding changes value without committing");
        check(!embedded.HasCustomStyle() && embedded.GetStyle().metrics.content_margin == Rect(0, 0, 0, 0),
              "Micro defaults retain real theme inheritance with layout-owned padding");
        embedded.SetFaceColor(Color(12, 34, 56));
        UiTheme::Set(opposite_theme);
        check(embedded.HasCustomStyle() &&
                  embedded.GetStyle().palette.face[ST_NORMAL].color == Color(12, 34, 56),
              "Native panel style setters preserve explicit overrides across host theme changes");
        embedded.ClearCustomStyle();
        check(!embedded.HasCustomStyle() && embedded.GetStyle().metrics.content_margin == Rect(0, 0, 0, 0),
              "Clearing a micro override restores inherited compact panel metrics");
        UiTheme::Set(original_theme);
        follow_host();
        Vector<Color> palette;
        palette.Add(Red());
        palette.Add(Green());
        palette.Add(Blue());
        palette.Add(White());
        check(embedded.SetPalette(palette) && embedded.GetPaletteCount() == 4,
              "Embedded micro picker accepts a custom palette");
        embedded.SetPaletteMode(UiColorPickerMicro::PaletteMode::Greyscale);
        embedded.SetPaletteMode(UiColorPickerMicro::PaletteMode::Spectrum);
        embedded.SetPaletteMode(UiColorPickerMicro::PaletteMode::Standard);
        embedded.SetColor(Red());
        embedded.Key(K_RIGHT, 1);
        check(embedded.GetColor() == Green() && micro_commits == 0,
              "Palette cycling restores a caller's original palette exactly");
        embedded.SetColor(Color(100, 120, 140));
        embedded.Disable();
        check(!embedded.Key(K_ENTER, 1) && !embedded.Key(K_RIGHT, 1) && micro_commits == 0 &&
                  embedded.GetColor() == Color(100, 120, 140),
              "Disabled micro picker cannot browse or commit through keyboard input");
        embedded.Enable();
        Vector<Color> invalid_palette;
        check(!embedded.SetPalette(invalid_palette) && embedded.GetPaletteCount() == 4 &&
                  embedded.GetColor() == Color(100, 120, 140),
              "Invalid palette imports preserve value and configuration");
        embedded.SetColumns(2).ShowHex(false);
        embedded_window.SetRect(0, 0, DPI(80), DPI(80));
        Ctrl::ProcessEvents();
        check(!embedded.GetSwatchRect(0).IsEmpty() &&
                  embedded.GetSwatchRect(3).bottom <= embedded.GetSize().cy,
              "Palette-only embedding fits a compact two-column layout");
        embedded.SetColor(Red());
        embedded.Key(K_DOWN, 1);
        check(embedded.GetColor() == Blue() && micro_commits == 0,
              "Arrow navigation previews a palette value without commit");
        embedded.Key(K_ENTER, 1);
        check(micro_commits == 1, "Enter commits the embedded palette choice once");
        auto *cell = dynamic_cast<UiToolButton *>(embedded.GetFirstChild()->GetFirstChild()->GetFirstChild());
        if(cell)
        {
            Point centre = cell->GetSize() / 2;
            cell->LeftDown(centre, 0);
            cell->LeftUp(centre, 0);
        }
        check(cell && embedded.GetColor() == Red() && micro_commits == 2,
              "Native palette button click changes the bound colour and commits once");
        embedded_window.Close();
        browser.new_button_.WhenAction();
        browser.new_item_name_.SetData("starter-solid");
        browser.new_item_kind_.OpenPopup();
        Ctrl::ProcessEvents();
        browser.new_item_kind_.Key(K_ESCAPE, 1);
        check(browser.options_open_ && !browser.new_item_kind_.IsPopupOpen(),
              "Creation dropdown Escape retains the New popup");
        ImageDraw creation_draw(browser.options_popup_.GetSize());
        browser.options_popup_.DrawCtrl(creation_draw);
        PNGEncoder().SaveFile(report + "-new-image.png", creation_draw);
        browser.create_item_button_.WhenAction();
        wait();
        UiFileBrowserPreviewRequest preview;
        preview.path = AppendFileName(root, "starter-solid.png");
        auto png = LoadFileBrowserPreview(preview, [] { return false; });
        check(png.type == UiFileBrowserPreviewData::Type::Image && png.image_size == Size(128, 64) &&
                  png.image[0][0].r == 32 && png.image[0][0].g == 64 && png.image[0][0].b == 96 &&
                  png.image[0][0].a == 255,
              "Created PNG decodes with exact dimensions and opaque fill");
        create.kind = UiFileBrowserCreateKind::Jpeg;
        create.name = "starter-gradient";
        create.size = Size(128, 64);
        create.colour = Color(32, 64, 96);
        create.gradient = true;
        check(CreateFileBrowserItem(root, create, created, error),
              "Create a JPEG gradient through the native encoder");
        preview.path = created;
        auto jpg = LoadFileBrowserPreview(preview, [] { return false; });
        check(jpg.type == UiFileBrowserPreviewData::Type::Image && jpg.image_size == Size(128, 64) &&
                  abs((int)jpg.image[0][0].r - 32) <= 5 && jpg.image[63][0].r >= 250,
              "Created JPEG decodes at the requested size with both gradient endpoints");
        create.kind = UiFileBrowserCreateKind::Png;
        create.name = "starter-invalid";
        create.size = Size(8192, 8192);
        check(!CreateFileBrowserItem(root, create, created, error) &&
                  !FileExists(AppendFileName(root, "starter-invalid.png")),
              "Oversized starter image is rejected before a file is created");
        for(const char *name : {"starter-note.txt", "starter-code.h", "starter-code.cpp",
                                "starter-collision.cpp", "starter-solid.png", "starter-gradient.jpg"})
            check(FileDelete(AppendFileName(root, name)), "Remove owned creation fixture");
        browser.RefreshFolder();
        wait();
        auto old_split = browser.browser_inspector_.GetSplitPercent();
        browser.browser_inspector_.SetSplitPercent(55);
        check(browser.browser_inspector_.GetCount() == 2 &&
                  std::abs(browser.browser_inspector_.GetSplitPercent() - 55) < .1,
              "Inspector splitter permits compacting the preview/metadata pane");
        browser.browser_inspector_.SetSplitPercent(old_split);
        auto started = std::make_shared<std::atomic<bool>>(false);
        browser.SetPreviewProvider(
            [started](const UiFileBrowserPreviewRequest &request, UiFileBrowserPreviewData &data,
                      const UiFileBrowserPreviewCancel &cancelled)
            {
                if(ToLower(GetFileExt(request.path)) != ".exr")
                    return false;
                *started = true;
                while(!cancelled())
                    Sleep(2);
                data.type = UiFileBrowserPreviewData::Type::Text;
                data.text = "stale result";
                return true;
            });
        select_name("010_020_comp_v012.1001.exr");
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while(!*started && std::chrono::steady_clock::now() < deadline)
        {
            Ctrl::ProcessEvents();
            Sleep(2);
        }
        check(*started, "Host preview extension executes asynchronously");
        select_name("preview.json");
        check(await_preview("text") && browser.preview_text_.GetText().Find("line 1:") == 0,
              "New selection cancels provider work and excludes stale preview pixels/text");
        browser.SetPreviewProvider({});
        browser.RefreshFolder();
        check(!browser.ExistsTimeCallback(4) && !browser.ExistsTimeCallback(5) &&
                  browser.preview_image_.GetIcon().IsEmpty(),
              "Folder refresh clears stale preview and cancels its debounce/poll timers");
        wait();
        window.Close();
        UiFileBrowserDialog dialog;
        dialog.SetTimeCallback(100, [&] { dialog.browser_.cancel_button_.WhenAction(); });
        UiFileBrowserSelection selection;
        check(!dialog.Execute(root, true, selection), "modal Cancel closes and returns rejection");
        UiFileBrowserDialog open_dialog;
        open_dialog.SetTimeCallback(-30,
                                    [&]
                                    {
                                        if(open_dialog.browser_.IsScanning())
                                            return;
                                        for(int i = 0; i < open_dialog.browser_.model_->GetCount(); ++i)
                                        {
                                            const auto &entry = (*open_dialog.browser_.model_)[i];
                                            if(entry.friendly_name != "010_020_comp_v012")
                                                continue;
                                            open_dialog.KillTimeCallback();
                                            open_dialog.browser_.SelectEntry(i);
                                            open_dialog.browser_.AcceptSelection();
                                            return;
                                        }
                                    });
        open_dialog.SetTimeCallback(10000, [&] { open_dialog.RejectBreak(IDCANCEL); }, 10);
        check(open_dialog.Execute(root, true, selection) && selection.sequence &&
                  GetFileName(selection.path) == "010_020_comp_v012.1001.exr",
              "modal Open returns first sequence filename and discovery flag to caller");
        String text = Format("%d checks\n", checks) + (failures.IsEmpty() ? "PASS\n" : "FAIL\n");
        for(const String &failure : failures)
            text << "FAIL: " << failure << '\n';
        SaveFile(report, text);
        return failures.IsEmpty();
    }
};

bool RunBrowserUiTests(const String &root, const String &report)
{
    return UiFileBrowserTest::Run(root, report);
}
} // namespace Upp
