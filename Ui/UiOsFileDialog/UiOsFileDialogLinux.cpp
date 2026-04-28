#include "UiOsFileDialog.h"

#if defined(PLATFORM_POSIX) && !defined(PLATFORM_COCOA)

#include <gtk/gtk.h>

namespace Upp {
namespace {

static bool EnsureGtkReady()
{
    static bool attempted = false;
    static bool ok = false;

    if(!attempted) {
        attempted = true;
        int argc = 0;
        char** argv = nullptr;
        ok = gtk_init_check(&argc, &argv);
    }
    return ok;
}

static GtkFileChooserAction ToGtkAction(UiOsFileDialog::Mode mode)
{
    switch(mode) {
    case UiOsFileDialog::Mode::SaveFile:
        return GTK_FILE_CHOOSER_ACTION_SAVE;
    case UiOsFileDialog::Mode::PickFolder:
        return GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    case UiOsFileDialog::Mode::OpenFile:
    case UiOsFileDialog::Mode::OpenFiles:
    default:
        return GTK_FILE_CHOOSER_ACTION_OPEN;
    }
}

static void ApplyFilters(GtkFileChooser* chooser,
                         const Vector<UiOsFileDialog::Filter>& filters,
                         int selected_index)
{
    Vector<GtkFileFilter*> built;
    built.Reserve(filters.GetCount());

    for(const auto& f : filters) {
        GtkFileFilter* ff = gtk_file_filter_new();
        gtk_file_filter_set_name(ff, f.label.ToStd().c_str());

        for(const String& p : f.patterns)
            gtk_file_filter_add_pattern(ff, p.ToStd().c_str());

        gtk_file_chooser_add_filter(chooser, ff);
        built.Add(ff);
    }

    if(selected_index >= 0 && selected_index < built.GetCount())
        gtk_file_chooser_set_filter(chooser, built[selected_index]);
}

static int ResolveFilterIndex(GtkFileChooser* chooser, const Vector<UiOsFileDialog::Filter>& filters)
{
    GtkFileFilter* current = gtk_file_chooser_get_filter(chooser);
    if(!current || filters.IsEmpty())
        return 0;

    GSList* all = gtk_file_chooser_list_filters(chooser);
    int found = 0;
    int i = 0;
    for(GSList* p = all; p; p = p->next, ++i) {
        if(p->data == current) {
            found = i;
            break;
        }
    }
    g_slist_free(all);

    return minmax(found, 0, filters.GetCount() - 1);
}

}

bool UiOsFileDialog::ExecuteLinux(Ctrl*)
{
    result_paths_.Clear();

    if(!EnsureGtkReady())
        return false;

    GtkFileChooserAction action = ToGtkAction(mode_);
    const char* accept_text = mode_ == Mode::SaveFile ? "Save" : "Open";

    GtkFileChooserNative* native = gtk_file_chooser_native_new(
        title_.IsEmpty() ? "Select File" : title_.ToStd().c_str(),
        nullptr,
        action,
        accept_text,
        "Cancel"
    );
    if(!native)
        return false;

    GtkFileChooser* chooser = GTK_FILE_CHOOSER(native);

    if(!initial_directory_.IsEmpty())
        gtk_file_chooser_set_current_folder(chooser, initial_directory_.ToStd().c_str());

    if(mode_ == Mode::SaveFile && !suggested_name_.IsEmpty())
        gtk_file_chooser_set_current_name(chooser, suggested_name_.ToStd().c_str());

    gtk_file_chooser_set_select_multiple(chooser, mode_ == Mode::OpenFiles);
    gtk_file_chooser_set_show_hidden(chooser, show_hidden_);
    gtk_file_chooser_set_local_only(chooser, TRUE);
    gtk_file_chooser_set_create_folders(chooser, create_prompt_);

#if GTK_CHECK_VERSION(3,22,0)
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, confirm_overwrite_);
#endif

    if(mode_ != Mode::PickFolder)
        ApplyFilters(chooser, filters_, filter_index_);

    int rc = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if(rc == GTK_RESPONSE_ACCEPT) {
        if(mode_ == Mode::OpenFiles) {
            GSList* files = gtk_file_chooser_get_filenames(chooser);
            for(GSList* p = files; p; p = p->next) {
                char* fn = (char*)p->data;
                if(fn) {
                    AddResult(String(fn));
                    g_free(fn);
                }
            }
            g_slist_free(files);
        }
        else {
            char* fn = gtk_file_chooser_get_filename(chooser);
            if(fn) {
                SetSingleResult(String(fn));
                g_free(fn);
            }
        }

        if(mode_ != Mode::PickFolder && !filters_.IsEmpty())
            filter_index_ = ResolveFilterIndex(chooser, filters_);
    }

    g_object_unref(native);
    return !result_paths_.IsEmpty();
}

}
#endif