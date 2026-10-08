#include <Ui/UiFileBrowser/UiFileBrowser.h>
#include <plugin/png/png.h>

using namespace Upp;
namespace Upp
{
bool RunBrowserUiTests(const String &root, const String &report);
}

static String BuildFixture()
{
    String root = AppendFileName(GetCurrentDirectory(), "build/browser-fixture");
    RealizeDirectory(root);
    // Remove only these two generated preview fixtures so repeated runs retain
    // the original seven-item catalogue contract before preview tests add them.
    FileDelete(AppendFileName(root, "preview.json"));
    FileDelete(AppendFileName(root, "preview.jpg"));
    FileDelete(AppendFileName(root, "preview.url"));
    FileDelete(AppendFileName(root, "preview.inc"));
    DirectoryDelete(AppendFileName(root, "new folder test"));
    for(const char *name : {"starter-note.txt", "starter-code.h", "starter-code.cpp", "starter-collision.cpp",
                            "starter-solid.png", "starter-gradient.jpg"})
        FileDelete(AppendFileName(root, name));
    RealizeDirectory(AppendFileName(root, "precomp"));
    RealizeDirectory(AppendFileName(root, "archive"));

    auto touch = [&](const String &name) { SaveFile(AppendFileName(root, name), "x"); };

    for(int frame = 1001; frame <= 1118; ++frame)
        touch(Format("010_020_comp_v012.%04d.exr", frame));

    for(int frame = 1001; frame <= 1118; ++frame)
        if(frame != 1048 && frame != 1058)
            touch(Format("010_020_plateMain.%04d.exr", frame));

    for(int frame = 1; frame <= 240; frame += 2)
        touch(Format("010_020_holdout.%05d.png", frame));

    touch("010_020_review_v012.mov");
    touch("slate.png");
    return root;
}

static bool RunModelSelfTest(const String &report)
{
    String root = BuildFixture();
    String error;
    Vector<String> failures;
    int checks = 0;
    auto check = [&](bool condition, const String &label)
    {
        ++checks;
        if(!condition)
            failures.Add(label);
    };

    UiFileBrowserModel grouped;
    check(grouped.Scan(root, true, error), "grouped scan: " + error);

    const UiFileBrowserEntry *comp = nullptr;
    const UiFileBrowserEntry *plate = nullptr;
    const UiFileBrowserEntry *holdout = nullptr;
    for(const UiFileBrowserEntry &entry : grouped.Entries())
    {
        if(entry.friendly_name == "010_020_comp_v012")
            comp = &entry;
        if(entry.friendly_name == "010_020_plateMain")
            plate = &entry;
        if(entry.friendly_name == "010_020_holdout")
            holdout = &entry;
    }

    check(comp && comp->IsSequence(), "complete EXR sequence grouped");
    if(comp)
    {
        check(comp->first == 1001 && comp->last == 1118 && comp->increment == 1, "complete sequence range");
        check(comp->complete && comp->GetPresentFrameCount() == 118, "complete sequence health");
        check(comp->pattern == "010_020_comp_v012.%04d.exr", "literal sequence pattern");
    }

    check(plate && plate->IsSequence(), "plate sequence grouped");
    if(plate)
    {
        check(!plate->complete, "missing frames detected");
        check(plate->GetExpectedFrameCount() == 118 && plate->GetPresentFrameCount() == 116,
              "missing frame counts");
        UiFileBrowserFilterRule missing;
        missing.field = UiFileBrowserFilterField::SequenceStatus;
        missing.value = "missing";
        check(UiFileBrowserModel::MatchesFilter(*plate, missing), "missing sequence filter");
    }

    check(holdout && holdout->IsSequence(), "incremented PNG sequence grouped");
    if(holdout)
        check(holdout->first == 1 && holdout->last == 239 && holdout->increment == 2,
              "incremented sequence range");

    UiFileBrowserModel raw;
    error.Clear();
    check(raw.Scan(root, false, error), "raw scan: " + error);
    int raw_images = 0;
    for(const UiFileBrowserEntry &entry : raw.Entries())
        if(entry.kind == UiFileBrowserEntryKind::Image)
            ++raw_images;
    check(raw_images == 355, Format("raw image count (%d)", raw_images));
    check(UiFileBrowserModel::CompareNames("shot2", "shot10") < 0, "natural number sorting");
    check(UiFileBrowserModel::CompareNames("Shot002", "shot2") > 0, "natural padding tie-break");
    UiFileBrowserEntry exe;
    exe.extension = ".EXE";
    check(UiFileBrowserModel::GetFileType(exe) == UiFileBrowserFileType::Executable,
          "executable type family");
    UiFileBrowserEntry text_file;
    text_file.extension = ".txt";
    check(UiFileBrowserModel::GetFileType(text_file) == UiFileBrowserFileType::Text, "text type family");
    String dot_root = AppendFileName(GetCurrentDirectory(), "build/browser-dotfiles");
    RealizeDirectory(dot_root);
    for(const char *name : {".hidden.txt", ".gitignore", ".shot.1001.exr", ".shot.1002.exr"})
        SaveFile(AppendFileName(dot_root, name), "metadata fixture");
    UiFileBrowserModel dotfiles;
    check(dotfiles.Scan(dot_root, true, error), "dot-prefixed filename scan");
    bool hidden_text = false, dot_config = false, dot_sequence = false;
    for(const auto &entry : dotfiles.Entries())
    {
        hidden_text |= entry.name == ".hidden.txt" && entry.extension == ".txt" &&
                       UiFileBrowserModel::GetFileType(entry) == UiFileBrowserFileType::Text;
        dot_config |= entry.name == ".gitignore" && entry.extension.IsEmpty() &&
                      UiFileBrowserModel::GetFileType(entry) == UiFileBrowserFileType::Config;
        dot_sequence |= entry.IsSequence() && entry.extension == ".exr" && entry.GetPresentFrameCount() == 2;
    }
    check(hidden_text, "dot-prefixed text keeps its real extension and type");
    check(dot_config, "extensionless dotfile keeps name-based classification");
    check(dot_sequence, "dot-prefixed numbered EXRs still group as a sequence");
    UiFileBrowserModel cancelled;
    check(!cancelled.Scan(root, true, error, [] { return true; }), "scan cancellation stops publication");
    UiFileBrowserScanner scanner;
    scanner.Request(root, false);
    uint64 generation = scanner.Request(AppendFileName(root, "archive"), true);
    UiFileBrowserScanner::Result result;
    int deadline = msecs() + 10000;
    while(!scanner.Poll(result) && msecs() < deadline)
        Sleep(2);
    check(result.model && result.generation == generation && result.error.IsEmpty() &&
              result.model->GetCount() == 0,
          "obsolete scan discarded; latest empty directory published");
    scanner.Request(AppendFileName(root, "does-not-exist"), true);
    result = UiFileBrowserScanner::Result();
    deadline = msecs() + 10000;
    while(!scanner.Poll(result) && msecs() < deadline)
        Sleep(2);
    check(result.model && !result.error.IsEmpty(), "async missing-directory error");

    // Force a long missing-frame expansion with only three tiny fixture files.
    // Cancellation after enumeration must stop grouping before it grows a huge
    // catalogue, rather than waiting for the entire sparse sequence to finish.
    String sparse_root = AppendFileName(root, "cancel-sparse");
    RealizeDirectory(sparse_root);
    for(int frame : {1, 2, 999999})
        SaveFile(AppendFileName(sparse_root, Format("cancel.%06d.exr", frame)), "x");
    bool enumerated = false;
    int after_enumeration = 0;
    UiFileBrowserModel sparse;
    bool sparse_done = sparse.Scan(
        sparse_root, true, error, [&] { return enumerated && ++after_enumeration >= 5; },
        [&](int) { enumerated = true; });
    check(!sparse_done && sparse.GetCount() == 1 && sparse.Entries()[0].frames.GetCount() < 1024,
          "sparse expansion cancels after enumeration before allocating a million frame records");
    for(int frame : {1, 2, 999999})
        FileDelete(AppendFileName(sparse_root, Format("cancel.%06d.exr", frame)));
    DirectoryDelete(sparse_root);
    String text = Format("%d checks\n", checks) + (failures.IsEmpty() ? "PASS\n" : "FAIL\n");
    text << Format("grouped entries: %d\nraw images: %d\n", grouped.GetCount(), raw_images);
    for(const String &failure : failures)
        text << "FAIL: " << failure << '\n';
    SaveFile(report, text);
    return failures.IsEmpty();
}

GUI_APP_MAIN
{
    // Standalone application policy; embedded controls simply inherit this font.
    SetStdFont(UiFileBrowserAppearance::ModernFont());
    const Vector<String> &args = CommandLine();

    if(args.GetCount() >= 2 && args[0] == "--ui-test")
    {
        SetExitCode(RunBrowserUiTests(BuildFixture(), args[1]) ? 0 : 1);
        return;
    }

    if(args.GetCount() >= 1 && args[0] == "--self-test")
    {
        String report = args.GetCount() >= 2
                            ? args[1]
                            : AppendFileName(GetCurrentDirectory(), "build/file-browser-self-test.txt");
        bool ok = RunModelSelfTest(report);
        SetExitCode(ok ? 0 : 1);
        return;
    }

    bool render_dark = args.GetCount() >= 2 && args[0] == "--render-dark";
    bool render_light = args.GetCount() >= 2 && args[0] == "--render-light";
    bool render = render_dark || render_light;
    bool dark = !render_light;

    if(dark)
        Ctrl::SwapDarkLight();
    UiThemeContext context = UiTheme::GetContext();
    context.mode = dark ? UiThemeMode::Dark : UiThemeMode::Light;
    UiTheme::Set(context);

    TopWindow window;
    UiFileBrowser browser;
    browser.ShowThemeButton();
    browser.WhenThemeRequested = [](UiThemeMode mode)
    {
        auto context = UiTheme::GetContext();
        if(context.mode != mode)
            Ctrl::SwapDarkLight();
        context.mode = mode;
        UiTheme::Set(context);
    };

    window.Title("UiFileBrowser · file and sequence browser").Sizeable().Zoomable();
    window.SetRect(0, 0, DPI(1240), DPI(760));
    window.Add(browser.SizePos());

    if(args.GetCount() && args[0] == "--fixture")
        browser.SetFolder(BuildFixture());
    else if(render)
    {
        browser.SetFolder(args.GetCount() >= 3 ? args[2] : BuildFixture());
        UiFileBrowserFilterRule temp;
        temp.include = false;
        temp.field = UiFileBrowserFilterField::Name;
        temp.value = "temp";
        browser.AddFilter(temp);
        UiFileBrowserFilterRule backup;
        backup.include = false;
        backup.field = UiFileBrowserFilterField::Name;
        backup.value = "backup";
        browser.AddFilter(backup);
    }
    else if(args.GetCount() && args[0] != "--smoke" && DirectoryExists(args[0]))
        browser.SetFolder(args[0]);

    browser.WhenAccept = [&](const UiFileBrowserSelection &selection)
    {
        LOG("UiFileBrowser accept: " << selection.path);
        window.Title("UiFileBrowser · " + selection.path);
    };
    browser.WhenError = [](const String &error) { LOG(error); };
    browser.WhenCancel = [&] { window.Close(); };

    if(render)
    {
        String output = args[1];
        window.SetTimeCallback(700,
                               [&]
                               {
                                   Ctrl::ProcessEvents();
                                   ImageDraw image(window.GetSize());
                                   window.DrawCtrl(image);
                                   if(!PNGEncoder().SaveFile(output, image))
                                       SetExitCode(1);
                                   window.Close();
                               });
    }
    else if(args.GetCount() && args[0] == "--smoke")
        window.SetTimeCallback(700, [&] { window.Close(); });

    window.Run();
}
