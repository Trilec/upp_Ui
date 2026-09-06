#include <Core/Core.h>
#include <Ui/Ui.h>

using namespace Upp;

struct TestCtx {
    int checks = 0;
    int fails = 0;

    void Expect(bool condition, const String& text)
    {
        checks++;
        if(!condition) {
            fails++;
            Cout() << "[FAIL] " << text << "\n";
        }
    }

    void Section(const char *title)
    {
        Cout() << "\n=== " << title << " ===\n";
    }
};

static void TestModesAndData(TestCtx& t)
{
    t.Section("Modes and value contract");
    UiDateTime c;
    c.SetFormatStyle(UiDateTimeFormatStyle::Iso)
     .ShowSeconds(true)
     .SetValue(Time(2026, 8, 9, 16, 37, 22));

    t.Expect(c.GetMode() == UiDateTimeMode::DateTime, "default mode is DateTime");
    t.Expect(c.GetDisplayText() == "2026-08-09T16:37:22", "ISO DateTime display");
    t.Expect(c.GetData().Is<Time>(), "DateTime GetData returns Time");

    c.DateMode();
    t.Expect(c.GetDate() == Date(2026, 8, 9), "Date mode retains date");
    t.Expect(c.GetValue().hour == 0 && c.GetValue().minute == 0 && c.GetValue().second == 0,
             "Date mode normalizes time");
    t.Expect(c.GetDisplayText() == "2026-08-09", "ISO Date display");
    t.Expect(c.GetData().Is<Date>(), "Date GetData returns Date");

    c.TimeMode().ShowSeconds(false).SetTime(9, 5, 47);
    t.Expect(c.GetValue().year == 1970 && c.GetValue().month == 1 && c.GetValue().day == 1,
             "Time mode uses neutral date");
    t.Expect(c.GetValue().second == 0, "hidden seconds normalize to zero");
    t.Expect(c.GetDisplayText() == "09:05", "ISO Time display");
}

static void TestParsing(TestCtx& t)
{
    t.Section("Parsing and clock formats");
    UiDateTime c;
    Time value;

    c.SetFormatStyle(UiDateTimeFormatStyle::Iso).DateMode();
    t.Expect(c.ParseText("2024-02-29", value) && value.year == 2024 && value.month == 2 && value.day == 29,
             "valid ISO leap date parses");
    t.Expect(!c.ParseText("2023-02-29", value), "invalid date rejected");

    c.TimeMode().ShowSeconds(true).SetClockFormat(UiClockFormat::Hour24);
    t.Expect(c.ParseText("23:14:59", value) && value.hour == 23 && value.minute == 14 && value.second == 59,
             "24-hour time parses");

    c.SetClockFormat(UiClockFormat::Hour12);
    t.Expect(c.ParseText("11:14 PM", value) && value.hour == 23 && value.minute == 14,
             "12-hour PM parses");
    t.Expect(c.ParseText("12:04 AM", value) && value.hour == 0 && value.minute == 4,
             "12-hour midnight parses");

    c.DateTimeMode().SetClockFormat(UiClockFormat::Hour24);
    t.Expect(c.ParseText("2026-08-09T16:37:22", value) &&
             value == Time(2026, 8, 9, 16, 37, 22), "ISO DateTime parses");
}

static void TestRangeAndNull(TestCtx& t)
{
    t.Section("Range and null policy");
    UiDateTime c;
    c.SetFormatStyle(UiDateTimeFormatStyle::Iso)
     .DateMode()
     .SetDateRange(Date(2026, 1, 10), Date(2026, 1, 20));

    c.SetDate(Date(2026, 1, 2));
    t.Expect(c.GetDate() == Date(2026, 1, 10), "date clamps to minimum");
    c.SetDate(Date(2026, 1, 30));
    t.Expect(c.GetDate() == Date(2026, 1, 20), "date clamps to maximum");

    c.AllowNull(true).ClearValue();
    t.Expect(c.IsNullValue() && IsNull(c.GetData()), "null value supported");
    c.AllowNull(false);
    t.Expect(!c.IsNullValue(), "disabling null resolves a value");
}

static void TestPresentationAndClipboardPolicy(TestCtx& t)
{
    t.Section("Presentation and clipboard policy");
    UiDateTime c;
    c.SetPresentation(true).AllowCopy(false).AllowPaste(false);
    t.Expect(c.IsPresentation() && !c.IsValueEditable(), "presentation is read-only");
    t.Expect(!c.IsCopyAllowed() && !c.IsPasteAllowed(), "clipboard policy is explicit");

    c.SetEditable(true).AllowCopy(true).AllowPaste(true);
    t.Expect(c.IsValueEditable() && c.IsCopyAllowed() && c.IsPasteAllowed(),
             "editable clipboard policy restored");
}

static void TestEvents(TestCtx& t)
{
    t.Section("Event contract");
    UiDateTime c;
    c.SetFormatStyle(UiDateTimeFormatStyle::Iso).DateMode();
    c.SetValue(Time(2026, 8, 8));
    int changing = 0;
    int action = 0;
    int invalid = 0;
    c.WhenChanging = [&] { changing++; };
    c.WhenAction = [&] { action++; };
    c.WhenInvalid = [&](String) { invalid++; };

    c.CommitText("2026-08-09", true);
    t.Expect(changing == 1 && action == 1, "committed value fires change and action");
    t.Expect(!c.CommitText("2026-99-99", true) && invalid == 1,
             "invalid final text reports invalid without action");
}

int RunDateTimeSuite()
{
    TestCtx t;
    TestModesAndData(t);
    TestParsing(t);
    TestRangeAndNull(t);
    TestPresentationAndClipboardPolicy(t);
    TestEvents(t);

    Cout() << "\nChecks: " << t.checks << ", Fails: " << t.fails << "\n";
    return t.fails ? 1 : 0;
}
