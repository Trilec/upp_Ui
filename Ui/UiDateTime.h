#ifndef _Ui_UiDateTime_h_
#define _Ui_UiDateTime_h_

/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    UiDateTime
    ========== 

    Purpose
    - Styled date, time, or combined date/time field for the Ui control family.

    Intent
    - Keep one authoritative U++ Time value while exposing Date, Time, and
      DateTime presentation modes.
    - Use U++ language information for locale display and U++ Calendar/Clock
      popups for direct picking, while applying Ui theme colours to the popup.
    - Support editable and presentation-only use without introducing a second
      value model.
    - Keep clipboard access explicit and independently switchable.

    V1 scope
    - Local/naive Date and Time values only; timezone conversion is deliberately
      outside this control.
    - Locale and ISO display styles, plus explicit 12/24-hour overrides.
    - Optional seconds, null values, min/max constraints, keyboard editing,
      copy/paste policy, and calendar/clock popup selection.
*/

#include <CtrlCore/CtrlCore.h>
#include <CtrlLib/CtrlLib.h>
#include <Ui/UiLineEdit.h>
#include <Ui/UiToolButton.h>
#include <Ui/UiTheme.h>

namespace Upp {

enum class UiDateTimeMode : byte {
    Date,
    Time,
    DateTime,
};

enum class UiDateTimeFormatStyle : byte {
    Locale,
    Iso,
};

enum class UiClockFormat : byte {
    Locale,
    Hour12,
    Hour24,
};

class UiDateTime : public Ctrl {
public:
    typedef UiDateTime CLASSNAME;

    struct Style : ChStyle<Style> {
        UiBaseEdit::Style editable;
        UiBaseEdit::Style presentation;
        UiToolButton::Style button;
        int button_width = DPI(28);
        int min_width = DPI(150);
        int min_height = DPI(28);
    };

    UiDateTime();

    static const Style& StyleDefault();
    UiDateTime& SetCustomStyle(const Style& style);
    UiDateTime& ClearCustomStyle();
    bool HasCustomStyle() const { return has_custom_style_; }
    const Style& GetStyle() const;

    UiDateTime& SetRole(UiRole role);
    UiDateTime& SetButtonRole(UiRole role);

    UiDateTime& SetMode(UiDateTimeMode mode);
    UiDateTimeMode GetMode() const { return mode_; }
    UiDateTime& DateMode() { return SetMode(UiDateTimeMode::Date); }
    UiDateTime& TimeMode() { return SetMode(UiDateTimeMode::Time); }
    UiDateTime& DateTimeMode() { return SetMode(UiDateTimeMode::DateTime); }

    UiDateTime& SetFormatStyle(UiDateTimeFormatStyle style);
    UiDateTimeFormatStyle GetFormatStyle() const { return format_style_; }
    UiDateTime& SetClockFormat(UiClockFormat format);
    UiClockFormat GetClockFormat() const { return clock_format_; }
    UiDateTime& ShowSeconds(bool on = true);
    bool IsSecondsShown() const { return show_seconds_; }

    UiDateTime& SetLanguage(int language);
    UiDateTime& SetLanguage(const char *language);
    int GetLanguage() const { return language_; }

    UiDateTime& SetEditable(bool on = true);
    bool IsValueEditable() const { return editable_; }
    UiDateTime& SetPresentation(bool on = true) { return SetEditable(!on); }
    bool IsPresentation() const { return !editable_; }
    UiDateTime& ShowPresentationFrame(bool on = true);
    bool IsPresentationFrameShown() const { return presentation_frame_; }

    UiDateTime& AllowCopy(bool on = true);
    UiDateTime& AllowPaste(bool on = true);
    bool IsCopyAllowed() const { return copy_allowed_; }
    bool IsPasteAllowed() const { return paste_allowed_; }
    bool CopyValueToClipboard() const;
    bool PasteValueFromClipboard(bool fire_action = true);

    UiDateTime& AllowNull(bool on = true);
    bool IsNullAllowed() const { return allow_null_; }
    UiDateTime& ClearValue(bool fire_action = false);
    bool IsNullValue() const { return IsNull(value_); }

    UiDateTime& SetValue(Time value, bool fire_action = false);
    Time GetValue() const { return value_; }
    UiDateTime& SetDate(Date date, bool fire_action = false);
    Date GetDate() const;
    UiDateTime& SetTime(int hour, int minute, int second = 0, bool fire_action = false);
    UiDateTime& SetNow(bool fire_action = false);
    UiDateTime& SetToday(bool fire_action = false);

    UiDateTime& SetRange(Time minimum, Time maximum);
    UiDateTime& SetDateRange(Date minimum, Date maximum);
    UiDateTime& ClearRange();
    bool HasMinimum() const { return !IsNull(minimum_); }
    bool HasMaximum() const { return !IsNull(maximum_); }
    Time GetMinimum() const { return minimum_; }
    Time GetMaximum() const { return maximum_; }

    UiDateTime& SetFirstDayOfWeek(int day);
    int GetFirstDayOfWeek() const { return first_day_; }

    String GetDisplayText() const;
    bool ParseText(const String& text, Time& value) const;
    bool CommitText(const String& text, bool fire_action = true);
    void OpenPicker();

    virtual void SetData(const Value& value) override;
    virtual Value GetData() const override;
    virtual Size GetMinSize() const override;
    virtual void Layout() override;
    virtual void State(int reason) override;

    Event<> WhenChanging;
    Event<> WhenAction;
    Event<String> WhenInvalid;
    Event<> WhenOpenPicker;

private:
    class ValueEdit : public UiLineEdit {
    public:
        typedef ValueEdit CLASSNAME;

        Event<> WhenCommit;

        void SetClipboardPolicy(bool copy_allowed, bool paste_allowed)
        {
            copy_allowed_ = copy_allowed;
            paste_allowed_ = paste_allowed;
        }

        virtual bool Key(dword key, int count) override;
        virtual void RightDown(Point p, dword flags) override;
        virtual void LostFocus() override;

    private:
        bool copy_allowed_ = true;
        bool paste_allowed_ = true;
    };

    void InvalidateThemeStyle();
    void SyncThemeStyle() const;
    Style ResolveThemeStyle() const;
    void ApplyStyle();
    void SyncText();
    void SyncPopupStyle();
    void SyncPopupValues();
    void HandleEditorChange();
    void HandleEditorCommit();
    void HandlePickedDate(Date date, bool final_action);
    void HandlePickedClock(Time time, bool final_action);

    Time NormalizeForMode(Time value) const;
    Time ClampValue(Time value) const;
    int64 CompareKey(Time value) const;
    Rect GetPopupRect(Size size) const;

    bool ParseDatePart(const String& text, Date& date) const;
    bool ParseTimePart(const String& text, int& hour, int& minute, int& second) const;
    bool ParseDateTimeParts(const String& text, Time& value) const;
    String FormatDatePart(Date date) const;
    String FormatTimePart(Time time) const;

private:
    mutable Style themed_style_;
    Style style_;
    mutable uint64 theme_revision_ = 0;
    bool has_custom_style_ = false;

    UiRole role_ = UiRole::Standard;
    UiRole button_role_ = UiRole::Subtle;
    UiDateTimeMode mode_ = UiDateTimeMode::DateTime;
    UiDateTimeFormatStyle format_style_ = UiDateTimeFormatStyle::Locale;
    UiClockFormat clock_format_ = UiClockFormat::Locale;
    int language_ = LNG_CURRENT;
    int first_day_ = MONDAY;

    bool editable_ = true;
    bool presentation_frame_ = false;
    bool show_seconds_ = false;
    bool copy_allowed_ = true;
    bool paste_allowed_ = true;
    bool allow_null_ = true;
    bool syncing_ = false;

    Time value_ = GetSysTime();
    Time minimum_ = Null;
    Time maximum_ = Null;

    ValueEdit editor_;
    UiToolButton picker_button_;

    Calendar date_popup_;
    Clock time_popup_;
    CalendarClock datetime_popup_;
    Calendar::Style calendar_style_;
    Clock::Style clock_style_;
};

} // namespace Upp

#endif
