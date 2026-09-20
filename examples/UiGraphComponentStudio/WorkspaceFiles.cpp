#include "WorkspaceWindow.h"

namespace Upp {
namespace GraphWorkspace {
bool NodeWorkspace::AllowDiscard()
{
    FinishProperty();
    if(!document_.dirty) return true;
    int result = PromptYesNoCancel("Save changes to this template family?");
    if(result < 0) return false;
    return result == 0 || Save(false);
}
bool NodeWorkspace::Save(bool as)
{
    FinishProperty();
    String path = document_.file;
    if(as || path.IsEmpty()) {
        FileSel fs; fs.Type("UiGraph family", "*.uigraph.json").AllFilesType().DefaultExt("uigraph.json");
        if(!path.IsEmpty()) fs.Set(path);
        if(!fs.ExecuteSaveAs("Save template family")) return false;
        path = ~fs;
    }
    try {
        document_.zoom = preview_.GetZoom(); document_.pan = preview_.GetPan();
        String error;
        if(!SaveAtomic(path, AsJSON(Encode(document_)), error)) { Exclamation(error); return false; }
        document_.file = path; document_.dirty = false; Reports(); status_.SetText("Saved " + path); return true;
    }
    catch(const Exc& e) { Exclamation(e); return false; }
}
void NodeWorkspace::Open()
{
    if(!AllowDiscard()) return;
    FileSel fs; fs.Type("UiGraph family", "*.uigraph.json").AllFilesType();
    if(!fs.ExecuteOpen("Open template family")) return;
    FileIn file(~fs);
    if(!file.IsOpen() || file.GetSize() > 8 * 1024 * 1024) { Exclamation("Could not read file, or file exceeds 8 MiB."); return; }
    Document next = document_; String error;
    if(!Load(file.Get((int)file.GetSize()), next, error)) { Exclamation(error); return; }
    next.file = ~fs; next.dirty = false;
    document_ = next; expanded_ = false; compact_size_ = document_.size; expand_.SetText("Expand specimen"); undo_.Clear(); selection_.id.Clear(); Changed();
    camera_action_ = true;
    double z = document_.zoom; Pointf pan = document_.pan;
    applying_ = true; preview_.SetZoom(z, Point(0, 0)); preview_.SetPan(pan); applying_ = false;
    camera_action_ = false; jump_ = -1; Reports();
}
void NodeWorkspace::NewFamily(int kind, bool empty)
{
    if(!AllowDiscard()) { SyncLeft(); return; }
    int revision = document_.revision + 1;
    document_ = MakeDocument((UiGraphNodeTemplateKind)minmax(kind, 1, 7));
    if(empty) { document_.family.name = "Untitled"; document_.family.base_layout.slot_count = 0; }
    expanded_ = false; compact_size_ = document_.size; expand_.SetText("Expand specimen");
    document_.revision = revision; document_.dirty = empty; undo_.Clear(); selection_.id.Clear();
    Changed(); ResetCamera(true);
}
void NodeWorkspace::CloneFamily()
{
    if(!AllowDiscard()) return;
    document_.family.name = document_.family.name.Left(115) + " Copy";
    document_.file.Clear(); document_.dirty = true; document_.revision++; undo_.Clear(); Changed();
}
void NodeWorkspace::Detach(bool style)
{
    FinishProperty(); if(document_.edit_base) return;
    int i = document_.shape; bool active = style ? document_.family.style_override[i] : document_.family.layout_override[i];
    if(active && !PromptYesNo("Discard this shape's section override and inherit Base again?")) return;
    PushUndo(document_);
    if(style) { if(active) document_.family.style_override[i] = false; else document_.family.DetachStyle(i); }
    else { if(active) document_.family.layout_override[i] = false; else document_.family.DetachLayout(i); }
    document_.dirty = true; document_.revision++; Changed();
}
void NodeWorkspace::CopyAll()
{
    FinishProperty();
    if(!PromptYesNo("Replace Base layout and style with this preview, and reset all shape overrides? This is undoable.")) return;
    auto layout = EffectiveLayout(); auto style = document_.family.Style(Scope());
    PushUndo(document_); document_.family.base_layout = layout; document_.family.base_style = style;
    for(int i = 0; i < SHAPE_COUNT; i++) document_.family.layout_override[i] = document_.family.style_override[i] = false;
    document_.edit_base = true; document_.dirty = true; document_.revision++; Changed();
}
void NodeWorkspace::ExportCode()
{
    FinishProperty(); String error, text = GenerateCpp(document_, error);
    if(!error.IsEmpty()) { Exclamation(error); return; }
    FileSel fs; fs.Type("C++ source", "*.cpp").DefaultExt("cpp");
    if(fs.ExecuteSaveAs("Export layout and style factories")) {
        if(!SaveAtomic(~fs, text, error)) Exclamation(error);
        else status_.SetText("Exported C++: " + String(~fs));
    }
}
bool NodeWorkspace::PickImage(Value& value, Ctrl*)
{
    FileSel fs; fs.Type("Image", "*.png *.jpg *.jpeg"); if(!fs.ExecuteOpen("Import static thumbnail")) return false;
    FileIn file(~fs);
    if(!file.IsOpen() || file.GetSize() > 8 * 1024 * 1024) { Exclamation("Image file limit is 8 MiB."); return false; }
    auto raster = StreamRaster::OpenAny(file);
    if(!raster) { Exclamation("Unsupported image."); return false; }
    Size size = raster->GetSize();
    if(size.cx <= 0 || size.cy <= 0 || size.cx > 4096 || size.cy > 4096) { Exclamation("Decode dimensions must be at most 4096 x 4096."); return false; }
    Image image = raster->GetImage();
    if(IsNull(image)) return false;
    double scale = min(1.0, min(256.0 / size.cx, 256.0 / size.cy));
    if(scale < 1) image = CachedRescale(image, Size(max(1, fround(size.cx * scale)), max(1, fround(size.cy * scale))));
    value = image; return true;
}
void NodeWorkspace::Close()
{
    if(AllowDiscard()) TopWindow::Close();
}
bool NodeWorkspace::Key(dword key, int count)
{
    if(key == K_CTRL_S) { Save(false); return true; }
    if(key == K_CTRL_Z) { Undo(); return true; }
    if(key == K_DELETE && (table_.HasFocus() || region_.HasFocus()
                          || overlay_.HasFocus() || preview_.HasFocus())) {
        // Never intercept Delete from a text/value editor or its filter. The
        // button and keyboard share the same transaction, undo and scope guard.
        if(!selection_.id.IsEmpty()) {
            if(!EditableLayout(document_))
                status_.SetText("Inherited layout: edit Base or create a layout override to remove this component.");
            else
                remove_.WhenAction();
        }
        return true;
    }
    return TopWindow::Key(key, count);
}
} // namespace GraphWorkspace
} // namespace Upp
