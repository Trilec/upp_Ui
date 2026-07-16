#ifndef _Utilities_UiDesigner_Theme_UiDesignerTheme_h_
#define _Utilities_UiDesigner_Theme_UiDesignerTheme_h_

#include <Utilities/PropertyEditorCore/PropertyEditorCore.h>
#include <Utilities/UiDesigner/Core/UiDesignerCore.h>

namespace Upp {

struct UiDesignerThemeSnapshot {
    String preset = "Minimal";
    String mode = "Light";
    Color accent = Color(58, 132, 255);
    int spacing = 8;
    int radius = 8;
    int pill_radius = 25;
    int border_width = 1;
    bool shadows = true;
    int shadow_distance = 6;
    int shadow_offset_y = 2;
    int shadow_alpha = 24;

    ValueMap ToValue() const;
    bool FromValue(const Value& value, String& error);
};

struct UiDesignerThemeHistoryEntry {
    String label;
    UiDesignerThemeSnapshot before;
    UiDesignerThemeSnapshot after;
};

class UiDesignerThemeDocument {
public:
    typedef UiDesignerThemeDocument CLASSNAME;

    UiDesignerThemeDocument();

    const UiDesignerThemeSnapshot& Get() const { return value_; }
    const UiDesignerThemeSnapshot& GetEffective() const
    {
        return preview_active_ ? preview_ : value_;
    }

    void BuildPropertyModel(PropertyEditorModel& model) const;
    bool Preview(const String& property, const Value& value, String& error);
    bool Commit(const String& property, const Value& value,
                const String& label, String& error);
    void CancelPreview();
    bool Reset(const String& property, String& error);

    bool Undo();
    bool Redo();
    bool CanUndo() const { return position_ > 0; }
    bool CanRedo() const { return position_ < history_.GetCount(); }
    bool IsDirty() const { return position_ != saved_position_; }
    void MarkSaved() { saved_position_ = position_; }

    String Serialize(bool pretty = true) const;
    bool Deserialize(const String& json, String& error);
    bool Replace(const UiDesignerThemeSnapshot& value, bool mark_saved = true);

    Event<> WhenPreview;
    Event<>& WhenPreviewChanged = WhenPreview;
    Event<> WhenChanged;
    Event<> WhenHistoryChanged;

private:
    bool SetProperty(UiDesignerThemeSnapshot& target,
                     const String& property, const Value& value,
                     String& error) const;
    Value GetProperty(const UiDesignerThemeSnapshot& source,
                      const String& property) const;
    void TruncateRedo();

    UiDesignerThemeSnapshot value_;
    UiDesignerThemeSnapshot preview_;
    bool preview_active_ = false;

    Array<UiDesignerThemeHistoryEntry> history_;
    int position_ = 0;
    int saved_position_ = 0;
};

}

#endif
