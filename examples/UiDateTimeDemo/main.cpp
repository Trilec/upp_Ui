#include <Ui/Ui.h>

using namespace Upp;

namespace {

class UiDateTimeDemo : public TopWindow {
public:
    typedef UiDateTimeDemo CLASSNAME;

    UiDateTimeDemo()
    {
        Title("UiDateTime Demo");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(720), DPI(430));

        UiThemeContext context = UiTheme::GetContext();
        context.preset = UiThemePreset::Minimal;
        context.mode = UiThemeMode::Light;
        UiTheme::Set(context);

        Add(root_.SizePos());
        root_.SetGap(DPI(10)).SetInset(DPI(14));

        header_.SetTitle("UiDateTime")
               .SetSubTitle("Locale-aware date, time and combined fields with editable and presentation modes");
        root_.Add(header_).Fixed(DPI(58));

        BuildRow(date_row_, date_label_, date_, "Date");
        BuildRow(time_row_, time_label_, time_, "Time");
        BuildRow(datetime_row_, datetime_label_, datetime_, "Date + time");
        BuildRow(presentation_row_, presentation_label_, presentation_, "Presentation");

        root_.Add(options_).Fixed(DPI(34));
        options_.SetGap(DPI(12));
        options_.Add(editable_toggle_).Fixed(DPI(120));
        options_.Add(copy_toggle_).Fixed(DPI(105));
        options_.Add(paste_toggle_).Fixed(DPI(105));
        options_.Add(seconds_toggle_).Fixed(DPI(120));
        options_.AddSpacer(1);
        options_.Add(dark_toggle_).Fixed(DPI(95));

        root_.Add(status_).Fixed(DPI(32));

        Time sample(2026, 8, 9, 16, 37, 22);
        date_.DateMode().SetValue(sample);
        time_.TimeMode().SetValue(sample);
        datetime_.DateTimeMode().SetValue(sample);
        presentation_.DateTimeMode()
                     .SetFormatStyle(UiDateTimeFormatStyle::Iso)
                     .ShowSeconds(true)
                     .SetPresentation(true)
                     .SetValue(sample);

        editable_toggle_.SetText("Editable").SetOn(true);
        copy_toggle_.SetText("Copy").SetOn(true);
        paste_toggle_.SetText("Paste").SetOn(true);
        seconds_toggle_.SetText("Seconds").SetOn(false);
        dark_toggle_.SetText("Dark").SetOn(false);
        status_.SetText("Edit the fields or use their picker buttons. Presentation remains selectable and read-only.");

        editable_toggle_.WhenAction = [=] {
            bool on = editable_toggle_.IsOn();
            date_.SetEditable(on);
            time_.SetEditable(on);
            datetime_.SetEditable(on);
            SyncStatus();
        };
        copy_toggle_.WhenAction = [=] {
            bool on = copy_toggle_.IsOn();
            date_.AllowCopy(on);
            time_.AllowCopy(on);
            datetime_.AllowCopy(on);
            presentation_.AllowCopy(on);
            SyncStatus();
        };
        paste_toggle_.WhenAction = [=] {
            bool on = paste_toggle_.IsOn();
            date_.AllowPaste(on);
            time_.AllowPaste(on);
            datetime_.AllowPaste(on);
            SyncStatus();
        };
        seconds_toggle_.WhenAction = [=] {
            bool on = seconds_toggle_.IsOn();
            time_.ShowSeconds(on);
            datetime_.ShowSeconds(on);
            SyncStatus();
        };
        dark_toggle_.WhenAction = [=] {
            UiThemeContext ctx = UiTheme::GetContext();
            ctx.mode = dark_toggle_.IsOn() ? UiThemeMode::Dark : UiThemeMode::Light;
            UiTheme::Set(ctx);
            Ctrl::SwapDarkLight();
            Refresh();
        };

        date_.WhenAction = [=] { SyncStatus("Date"); };
        time_.WhenAction = [=] { SyncStatus("Time"); };
        datetime_.WhenAction = [=] { SyncStatus("Date + time"); };
        date_.WhenInvalid = [=](String text) { Invalid("Date", text); };
        time_.WhenInvalid = [=](String text) { Invalid("Time", text); };
        datetime_.WhenInvalid = [=](String text) { Invalid("Date + time", text); };
    }

private:
    void BuildRow(UiBoxLayout& row, UiLabel& label, UiDateTime& value, const char *text)
    {
        root_.Add(row).Fixed(DPI(42));
        row.SetGap(DPI(10));
        row.Add(label).Fixed(DPI(120));
        row.Add(value).Expand(1);
        label.SetText(text);
    }

    void SyncStatus(const char *source = nullptr)
    {
        String prefix = source ? String(source) + ": " : String();
        status_.SetText(prefix + datetime_.GetDisplayText());
    }

    void Invalid(const char *source, const String& text)
    {
        status_.SetText(Format("%s rejected: %s", source, text));
    }

    UiBoxLayout root_ { UiDirection::V };
    UiTitleCard header_;

    UiBoxLayout date_row_ { UiDirection::H };
    UiLabel date_label_;
    UiDateTime date_;

    UiBoxLayout time_row_ { UiDirection::H };
    UiLabel time_label_;
    UiDateTime time_;

    UiBoxLayout datetime_row_ { UiDirection::H };
    UiLabel datetime_label_;
    UiDateTime datetime_;

    UiBoxLayout presentation_row_ { UiDirection::H };
    UiLabel presentation_label_;
    UiDateTime presentation_;

    UiBoxLayout options_ { UiDirection::H };
    UiToggle editable_toggle_;
    UiToggle copy_toggle_;
    UiToggle paste_toggle_;
    UiToggle seconds_toggle_;
    UiToggle dark_toggle_;
    UiLabel status_;
};

} // namespace

GUI_APP_MAIN
{
    UiDateTimeDemo().Run();
}
