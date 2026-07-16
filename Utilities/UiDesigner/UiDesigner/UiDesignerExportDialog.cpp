#include "UiDesignerExportDialog.h"

namespace Upp {

static int ProfileIndex(UiDesignerExportProfile profile)
{
    return (int)profile;
}

UiDesignerExportDialog::UiDesignerExportDialog(
    UiDesignerSession& session, UiDesignerExportProfile profile)
    : session_(session), initial_profile_(profile)
{
    Title("Export UiDesigner project").Sizeable();
    SetRect(0, 0, DPI(760), DPI(620));
    BuildControls();
    ProfileChanged();
    RefreshPreview();
}

void UiDesignerExportDialog::BuildControls()
{
    profile_label_.SetText("Export profile");
    destination_label_.SetText("Destination");
    package_label_.SetText("Package name");
    class_label_.SetText("Class name");
    namespace_label_.SetText("Namespace");
    appearance_label_.SetText("Appearance mode");
    overwrite_label_.SetText("Overwrite policy");
    inventory_label_.SetText("Files to create or preserve");

    profile_.UseInternalModel().Clear()
        .Add("Complete C++ package", (int)UiDesignerExportProfile::CompleteCppPackage)
        .Add("C++ component / class", (int)UiDesignerExportProfile::ComponentOnly)
        .Add("UiDesigner project JSON", (int)UiDesignerExportProfile::ProjectJson)
        .Add("Document JSON", (int)UiDesignerExportProfile::DocumentJson)
        .Add("Theme JSON", (int)UiDesignerExportProfile::ThemeJson);
    profile_.Select(ProfileIndex(initial_profile_));
    profile_.WhenAction = [=] { ProfileChanged(); };

    destination_.SetPlaceholder("Choose a folder or JSON file...");
    destination_.WhenChanging = [=] { RefreshPreview(); };
    browse_.SetText("Browse...");
    browse_.WhenAction = [=] { Browse(); };

    package_.SetData("GeneratedUi");
    class_.SetData("GeneratedUiWindow");
    namespace_.SetData("Upp");
    package_.WhenChanging = class_.WhenChanging = namespace_.WhenChanging =
        [=] { RefreshPreview(); };

    appearance_.UseInternalModel().Clear()
        .Add("Theme first", "ThemeFirst")
        .Add("Explicit overrides", "ExplicitOverrides");
    appearance_.Select(0);
    appearance_.WhenAction = [=] { RefreshPreview(); };

    overwrite_.UseInternalModel().Clear()
        .Add("Refuse existing files", "refuse")
        .Add("Replace generated files", "generated")
        .Add("Replace every file", "all");
    overwrite_.Select(1);
    overwrite_.WhenAction = [=] { RefreshPreview(); };

    include_design_.SetText("Include source design JSON").SetData(true);
    include_theme_.SetText("Include Theme JSON").SetData(true);
    preserve_user_.SetText("Preserve user-owned .h/.cpp files").SetData(true);
    include_design_.WhenAction = include_theme_.WhenAction =
        preserve_user_.WhenAction = [=] { RefreshPreview(); };

    inventory_.SetReadOnly();
    status_.SetText("Ready").SetAlign(UiAlign::LEFT, UiAlign::CENTER);
    refresh_.SetText("Refresh preview");
    refresh_.WhenAction = [=] { RefreshPreview(); };
    export_.SetText("Export");
    export_.WhenAction = [=] { ExportNow(); };
    cancel_.SetText("Cancel");
    cancel_.WhenAction = [=] { Break(IDCANCEL); };

    Add(profile_label_); Add(profile_);
    Add(destination_label_); Add(destination_); Add(browse_);
    Add(package_label_); Add(package_);
    Add(class_label_); Add(class_);
    Add(namespace_label_); Add(namespace_);
    Add(appearance_label_); Add(appearance_);
    Add(overwrite_label_); Add(overwrite_);
    Add(include_design_); Add(include_theme_); Add(preserve_user_);
    Add(inventory_label_); Add(inventory_); Add(status_);
    Add(refresh_); Add(export_); Add(cancel_);
}

UiDesignerExportProfile UiDesignerExportDialog::SelectedProfile() const
{
    return (UiDesignerExportProfile)(int)profile_.GetData();
}

void UiDesignerExportDialog::ProfileChanged()
{
    const UiDesignerExportProfile profile = SelectedProfile();
    const bool cpp = profile == UiDesignerExportProfile::CompleteCppPackage ||
                     profile == UiDesignerExportProfile::ComponentOnly;
    package_.Enable(cpp);
    class_.Enable(cpp);
    namespace_.Enable(cpp);
    appearance_.Enable(cpp);
    include_design_.Enable(cpp);
    include_theme_.Enable(cpp || profile == UiDesignerExportProfile::ProjectJson);
    preserve_user_.Enable(cpp);
    RefreshPreview();
}

void UiDesignerExportDialog::Browse()
{
    const UiDesignerExportProfile profile = SelectedProfile();
    const bool cpp = profile == UiDesignerExportProfile::CompleteCppPackage ||
                     profile == UiDesignerExportProfile::ComponentOnly;
    FileSel selector;
    if(cpp) {
        if(!selector.ExecuteSelectDir("Select export folder"))
            return;
    }
    else {
        selector.Type("JSON", "*.json");
        if(!selector.ExecuteSaveAs("Select export file"))
            return;
    }
    destination_.SetData(~selector);
    RefreshPreview();
}

UiDesignerExportRequest UiDesignerExportDialog::BuildRequest(String& error) const
{
    UiDesignerExportRequest request;
    request.profile = SelectedProfile();
    request.destination = TrimBoth(AsString(destination_.GetData()));
    request.generation.package_name = TrimBoth(AsString(package_.GetData()));
    request.generation.class_name = TrimBoth(AsString(class_.GetData()));
    request.generation.namespace_name = TrimBoth(AsString(namespace_.GetData()));
    request.generation.appearance_mode = AsString(appearance_.GetData());
    request.generation.include_source_design = (bool)include_design_.GetData();
    request.generation.include_theme = (bool)include_theme_.GetData();
    request.write.preserve_user_files = (bool)preserve_user_.GetData();
    const String overwrite = AsString(overwrite_.GetData());
    request.write.overwrite = overwrite == "refuse"
        ? UiDesignerOverwritePolicy::RefuseExisting
        : overwrite == "all" ? UiDesignerOverwritePolicy::ReplaceAll
                              : UiDesignerOverwritePolicy::ReplaceGenerated;

    if(request.destination.IsEmpty()) {
        error = "Choose an export destination.";
        return request;
    }
    if(request.profile == UiDesignerExportProfile::CompleteCppPackage ||
       request.profile == UiDesignerExportProfile::ComponentOnly) {
        if(!UiDesignerValidateGenerationOptions(request.generation, error))
            return request;
    }
    error.Clear();
    return request;
}

void UiDesignerExportDialog::RefreshPreview()
{
    String error;
    UiDesignerExportRequest request = BuildRequest(error);
    if(!error.IsEmpty()) {
        status_.SetText(error);
        inventory_.SetData(String());
        export_.Enable(false);
        return;
    }

    UiDesignerExportService service(session_.Catalog());
    UiDesignerExportResult preview = service.Preview(
        session_.Document(), session_.Theme(), request);
    String text;
    for(const String& file : preview.inventory) {
        const bool preserved = FindIndex(preview.preserved_files, file) >= 0;
        text << (preserved ? "PRESERVE  " : "WRITE     ") << file << "\n";
    }
    inventory_.SetData(text);
    status_.SetText(preview.success ? preview.diagnostic : preview.diagnostic);
    export_.Enable(preview.success);
}

void UiDesignerExportDialog::ExportNow()
{
    String error;
    UiDesignerExportRequest request = BuildRequest(error);
    if(!error.IsEmpty()) {
        Exclamation(error);
        return;
    }
    UiDesignerExportService service(session_.Catalog());
    result_ = service.Execute(session_.Document(), session_.Theme(), request);
    if(!result_.success) {
        Exclamation(result_.diagnostic);
        RefreshPreview();
        return;
    }
    status_.SetText(result_.diagnostic);
    Break(IDOK);
}

bool UiDesignerExportDialog::Execute()
{
    return Run() == IDOK;
}

void UiDesignerExportDialog::Layout()
{
    const Size sz = GetSize();
    const int margin = DPI(14);
    const int label_w = DPI(130);
    const int row_h = DPI(31);
    const int gap = DPI(8);
    int y = margin;

    auto PlaceRow = [&](Ctrl& label, Ctrl& editor) {
        label.SetRect(margin, y, label_w, row_h);
        editor.SetRect(margin + label_w + gap, y,
                       max(0, sz.cx - margin * 2 - label_w - gap), row_h);
        y += row_h + gap;
    };
    PlaceRow(profile_label_, profile_);

    destination_label_.SetRect(margin, y, label_w, row_h);
    browse_.SetRect(sz.cx - margin - DPI(92), y, DPI(92), row_h);
    destination_.SetRect(margin + label_w + gap, y,
        max(0, sz.cx - margin * 2 - label_w - gap - DPI(100)), row_h);
    y += row_h + gap;

    PlaceRow(package_label_, package_);
    PlaceRow(class_label_, class_);
    PlaceRow(namespace_label_, namespace_);
    PlaceRow(appearance_label_, appearance_);
    PlaceRow(overwrite_label_, overwrite_);

    include_design_.SetRect(margin + label_w + gap, y, DPI(220), row_h);
    include_theme_.SetRect(margin + label_w + DPI(235), y, DPI(180), row_h);
    y += row_h;
    preserve_user_.SetRect(margin + label_w + gap, y, DPI(300), row_h);
    y += row_h + gap;

    inventory_label_.SetRect(margin, y, sz.cx - margin * 2, row_h);
    y += row_h;
    const int button_y = sz.cy - margin - row_h;
    const int status_y = button_y - row_h - gap;
    inventory_.SetRect(margin, y, sz.cx - margin * 2,
                       max(DPI(80), status_y - y - gap));
    status_.SetRect(margin, status_y, sz.cx - margin * 2, row_h);

    cancel_.SetRect(sz.cx - margin - DPI(90), button_y, DPI(90), row_h);
    export_.SetRect(sz.cx - margin - DPI(188), button_y, DPI(90), row_h);
    refresh_.SetRect(margin, button_y, DPI(126), row_h);
}

}
