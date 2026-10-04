#include <Utilities/PropertyEditor/PropertyEditor.h>
using namespace Upp;

static int checks = 0, failures = 0;
static String report;
static void Check(bool valid, const char* message)
{
    ++checks;
    if(!valid) { ++failures; report << "FAIL: " << message << "\n"; }
}
static UiDropdown* FindDropdown(Ctrl& root)
{
    for(Ctrl* child=root.GetFirstChild(); child; child=child->GetNext()) {
        if(auto* dropdown=dynamic_cast<UiDropdown*>(child)) return dropdown;
        if(auto* dropdown=FindDropdown(*child)) return dropdown;
    }
    return nullptr;
}
static void PumpDeferred()
{
    // Windows dispatches U++ PostCallback through its native timer window.
    for(int i=0;i<3;++i) { Sleep(15); Ctrl::ProcessEvents(); }
}
static void AddChoice(PropertyEditorModel& model, bool inline_editor, int value=0)
{
    model.AddChoice("mode","Mode",value).AddChoice(0,"A").AddChoice(1,"B")
         .SetInlineEditor(inline_editor);
}

static void StructuralChange(bool inline_editor, int notification, bool replace_schema)
{
    PropertyEditorModel model;
    AddChoice(model,inline_editor);
    model.AddText("dependent","Dependent","visible");
    PropertyEditor editor;
    editor.SetRect(0,0,400,240);
    editor.SetModel(&model);
    editor.Layout();
    Check(editor.SelectProperty("mode",true),"real Choice activates");
    Ptr<UiDropdown> dropdown=FindDropdown(editor);
    Check(dropdown,"real Choice contains UiDropdown");
    int values=0, previews=0, commits=0;
    model.WhenValueChanged=[&](String id) {
        if(id!="mode" || ++values!=notification) return;
        if(replace_schema) {
            model.Clear(false);
            AddChoice(model,inline_editor,1);
            model.StructureChanged();
        }
        else model.SetVisible("dependent",false);
        // Coalesce repeated supported structure notifications.
        model.StructureChanged();
        Check(dropdown,"structural notification retains the active sender");
        Check(editor.GetDisplayRowCount()==2,"row replacement waits for editor event unwind");
        PumpDeferred();
        Check(dropdown,"nested event pump does not destroy the dispatching sender");
    };
    editor.WhenPreview=[&](String id,Value value) {
        ++previews; Check(id=="mode" && (int)value==1,"GUI preview retains id and authored value");
    };
    editor.WhenCommit=[&](String id,Value value) {
        ++commits; Check(id=="mode" && (int)value==1,"GUI commit retains id and authored value");
    };
    if(dropdown) dropdown->SelectByData(1);
    Check(values==2 && previews==1 && commits==1,"Choice preview and commit dispatch exactly once");
    Check(model.Find("mode") && (int)model.Find("mode")->value==1,"authored value survives synchronous schema notification");
    Check(dropdown,"sender remains alive until the full Choice event returns");
    PumpDeferred();
    Check(editor.GetDisplayRowCount()==1,"deferred rows match the final schema");
    Check(!dropdown,"deferred rebuild safely retires the original sender");
    if(inline_editor) Check(editor.GetInlineEditorCount()==1,"visible inline Choice is rebuilt once");
    else Check(editor.SelectProperty("mode",true),"Choice can reopen after deferred rebuild");
}

static void Rebind(bool inline_editor, int notification)
{
    PropertyEditorModel original,replacement;
    AddChoice(original,inline_editor);
    AddChoice(replacement,inline_editor);
    PropertyEditor editor;
    editor.SetRect(0,0,400,240);
    editor.SetModel(&original);
    editor.Layout();
    Check(editor.SelectProperty("mode",true),"Choice activates before rebinding");
    Ptr<UiDropdown> dropdown=FindDropdown(editor);
    int original_values=0, replacement_values=0;
    replacement.WhenValueChanged=[&](String) { ++replacement_values; };
    original.WhenValueChanged=[&](String) {
        if(++original_values==notification) {
            editor.SetModel(&replacement);
            Check(dropdown,"rebinding retains sender until its event returns");
        }
    };
    if(dropdown) dropdown->SelectByData(1);
    Check((int)original.Find("mode")->value==1,"original binding receives its selected value");
    Check((int)replacement.Find("mode")->value==0 && replacement_values==0,
          "old Choice event never previews or commits into a matching id in the new binding");
    PumpDeferred();
    Check(!dropdown,"rebound editor safely retires the old Choice");
    Check(editor.SelectProperty("mode",true),"new binding activates normally");
    UiDropdown* fresh=FindDropdown(editor);
    Check(fresh && (int)fresh->GetSelectedData()==0,"new Choice displays its own model value");
}

static void NestedDispatch()
{
    PropertyEditorModel model;
    AddChoice(model,true);
    model.AddChoice("nested","Nested",0).AddChoice(0,"A").AddChoice(1,"B").SetInlineEditor();
    model.AddText("dependent","Dependent","visible");
    PropertyEditor editor;
    editor.SetRect(0,0,400,240); editor.SetModel(&model); editor.Layout();
    Ptr<UiDropdown> outer,nested;
    for(Ctrl* child=editor.GetFirstChild();child;child=child->GetNext()) {
        UiDropdown* found=FindDropdown(*child);
        if(found) { if(!outer) outer=found; else nested=found; }
    }
    Check(outer && nested,"nested regression finds two actual inline Choice editors");
    int commits=0;
    editor.WhenCommit=[&](String,Value) { ++commits; };
    bool started=false;
    model.WhenValueChanged=[&](String id) {
        if(id=="mode" && !started) {
            started=true;
            if(nested) nested->SelectByData(1);
            model.SetVisible("dependent",false);
            PumpDeferred();
            Check(outer && nested && editor.GetDisplayRowCount()==3,
                  "nested editor callbacks restore the enclosing structural guard");
        }
    };
    if(outer) outer->SelectByData(1);
    Check(commits==2 && (int)model.Find("mode")->value==1 && (int)model.Find("nested")->value==1,
          "nested choices each commit the correct model property once");
    PumpDeferred();
    Check(!outer && !nested && editor.GetDisplayRowCount()==2,
          "outermost event completion publishes the final nested schema safely");
}

template<class T> static T* FindDirect(Ctrl& root)
{
    for(Ctrl* child=root.GetFirstChild(); child; child=child->GetNext())
        if(auto* typed=dynamic_cast<T*>(child)) return typed;
    return nullptr;
}
static void NumericPolicy()
{
    PropertyEditorModel model;
    auto& bounded=model.AddInteger("bounded","Bounded",3).SetRange(1,10,1).SetDefault(3);
    auto& negative=model.AddInteger("negative","Negative",-4).SetRange(-10,10,2);
    auto& unbounded=model.AddInteger("unbounded","Unbounded",1000000000);
    auto& slider_item=model.AddSliderInt("slider","Slider",4,-10,10,2);
    for(PropertyEditorItem* item : {&bounded,&negative,&slider_item}) {
        One<PropertyValueEditor> value_editor=PropertyEditorFactory::Global().Create(*item);
        value_editor->Configure(*item);
        value_editor->SetEditorValue(item->value,false);
        value_editor->SetRect(0,0,260,30);
        UiSlider* slider=FindDirect<UiSlider>(*value_editor);
        UiToolButton* toggle=FindDirect<UiToolButton>(*value_editor);
        Check(slider && toggle && toggle->IsShown(),"bounded integer presentations expose the number/slider toggle");
        Check(slider && slider->GetMin()==(int)item->minimum && slider->GetMax()==(int)item->maximum &&
              slider->GetStep()==(int)item->step,"numeric slider preserves authored positive/negative domain and step");
        bool started_as_slider=item->kind==PropertyEditorKind::SliderInt;
        Check(slider && slider->IsShown()==started_as_slider,"SliderInt begins as slider; bounded Integer begins as numeric entry");
        if(toggle) toggle->WhenAction();
        Check(slider && slider->IsShown()!=started_as_slider,"toggle switches presentation without schema conversion");
        Value before=value_editor->GetEditorValue();
        value_editor->Configure(*item);
        value_editor->SetEditorValue(before,false);
        Check(slider && slider->IsShown()!=started_as_slider && value_editor->GetEditorValue()==before,
              "canonical value refresh retains the chosen presentation and value");
        Check(item->kind==(started_as_slider ? PropertyEditorKind::SliderInt : PropertyEditorKind::Integer),
              "presentation leaves Integer and SliderInt schema kinds intact");
    }
    One<PropertyValueEditor> unlimited=PropertyEditorFactory::Global().Create(unbounded);
    unlimited->Configure(unbounded);
    unlimited->SetEditorValue(unbounded.value,false);
    Check(!FindDirect<UiSlider>(*unlimited) && (int)unlimited->GetEditorValue()==1000000000,
          "unbounded Integer retains full numeric semantics without a fabricated slider domain");
    unlimited->SetEditorValue(-1000000000,false);
    Check((int)unlimited->GetEditorValue()==-1000000000,"unbounded negative integers are not clamped");

    PropertyEditor editor;
    editor.SetRect(0,0,400,240); editor.SetModel(&model); editor.Layout();
    Check(editor.SelectProperty("bounded",true),"bounded Integer activates in production PropertyEditor");
    PropertyValueEditor* active=FindDirect<PropertyValueEditor>(editor);
    UiToolButton* toggle=active ? FindDirect<UiToolButton>(*active) : nullptr;
    if(toggle) toggle->WhenAction();
    model.Commit("bounded",9);
    UiSlider* slider=active ? FindDirect<UiSlider>(*active) : nullptr;
    Check(slider && slider->IsShown() && slider->GetValue()==9,"model commit preserves slider presentation and canonical value");
    model.Reset("bounded");
    Check(slider && slider->IsShown() && slider->GetValue()==3,"reset preserves authored domain and slider presentation");
    editor.SetPaletteMode(PropertyEditorPaletteMode::Dark);
    Check(editor.SelectProperty("bounded",true),"numeric editor reopens after theme rebuild");
    active=FindDirect<PropertyValueEditor>(editor);
    slider=active ? FindDirect<UiSlider>(*active) : nullptr;
    toggle=active ? FindDirect<UiToolButton>(*active) : nullptr;
    Check(slider && toggle && toggle->IsShown() && slider->GetMin()==1 && slider->GetMax()==10 &&
          slider->GetStep()==1 && slider->GetValue()==3,"theme rebuild preserves bounded slider capability, range, step and reset value");
    Check(bounded.kind==PropertyEditorKind::Integer && (int)bounded.minimum==1 && (int)bounded.maximum==10,
          "toggle, reset and theme never rewrite the public integer schema metadata");
}

GUI_APP_MAIN
{
    NumericPolicy();
    NestedDispatch();
    for(bool inline_editor : {false,true}) {
        for(int notification : {1,2}) {
            StructuralChange(inline_editor,notification,false);
            StructuralChange(inline_editor,notification,true);
            Rebind(inline_editor,notification);
        }
    }
    report << "PROPERTYEDITOR_CALLBACK_LIFECYCLE_SUMMARY checks=" << checks
           << " failed=" << failures << "\n";
    SaveFile(GetExeDirFile("PropertyEditorCallbackLifecycleTest-results.txt"),report);
    SetExitCode(failures ? 1 : 0);
}
