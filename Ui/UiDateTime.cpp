#include <Ui/UiDateTime.h>
#include <Ui/UiDraw.h>

namespace Upp {

namespace {

static Vector<String> DateTimeNumbers_(const String& text)
{
    Vector<String> out;
    for(int i = 0; i < text.GetCount();) {
        while(i < text.GetCount() && !IsDigit((byte)text[i]))
            i++;
        if(i >= text.GetCount())
            break;
        int begin = i;
        while(i < text.GetCount() && IsDigit((byte)text[i]))
            i++;
        out.Add(text.Mid(begin, i - begin));
    }
    return out;
}

static int DateTimeInt_(const String& text)
{
    return text.IsEmpty() ? 0 : atoi(~text);
}

static Color SolidColor_(const UiFill& fill, Color fallback)
{
    return fill.IsSolid() ? fill.color : fallback;
}

static bool IsCurrentLanguage_(int language)
{
    return language == LNG_CURRENT || language == GetCurrentLanguage();
}

static Date DateFromTime_(Time time)
{
    return IsNull(time) ? Date(Null) : Date(time.year, time.month, time.day);
}

} // namespace

bool UiDateTime::ValueEdit::Key(dword key, int count)
{
    if((key == K_CTRL_C || key == K_CTRL_INSERT) && !copy_allowed_)
        return true;
    if((key == K_CTRL_V || key == K_SHIFT_INSERT) && !paste_allowed_)
        return true;
    if((key == K_CTRL_X || key == K_SHIFT_DELETE) && (!copy_allowed_ || !IsEditable()))
        return true;
    return UiLineEdit::Key(key, count);
}

void UiDateTime::ValueEdit::RightDown(Point p, dword flags)
{
    // The base edit menu exposes the complete edit clipboard set. When the
    // caller restricts either side of that policy, suppress the generic menu
    // rather than advertising an operation that the control has disabled.
    if(copy_allowed_ && paste_allowed_)
        UiLineEdit::RightDown(p, flags);
}

void UiDateTime::ValueEdit::LostFocus()
{
    UiLineEdit::LostFocus();
    if(WhenCommit)
        WhenCommit();
}

const UiDateTime::Style& UiDateTime::StyleDefault()
{
    static Style style;
    return style;
}

UiDateTime::UiDateTime()
    : style_(StyleDefault())
    , themed_style_(StyleDefault())
    , datetime_popup_(CalendarClock::MODE_TIME)
{
    Add(editor_);
    Add(picker_button_);

    picker_button_.SetText("▾")
                  .SetContentInset(0)
                  .ClickFocus(false);

    editor_.WhenChange = [=] { HandleEditorChange(); };
    editor_.WhenAction = [=] { HandleEditorCommit(); };
    editor_.WhenCommit = [=] { HandleEditorCommit(); };
    picker_button_.WhenAction = [=] { OpenPicker(); };

    date_popup_.WhenSelect = [=] {
        HandlePickedDate(date_popup_.GetDate(), true);
        date_popup_.Deactivate();
    };
    time_popup_.WhenAction = [=] {
        HandlePickedClock(time_popup_.GetTime(), true);
        time_popup_.Deactivate();
    };

    datetime_popup_.calendar.WhenSelect = [=] {
        HandlePickedDate(datetime_popup_.calendar.GetDate(), true);
    };
    datetime_popup_.clock.WhenAction = [=] {
        HandlePickedClock(datetime_popup_.clock.GetTime(), true);
    };

    ApplyStyle();
    SyncPopupStyle();
    SyncText();
}

UiDateTime::Style UiDateTime::ResolveThemeStyle() const
{
    Style out;
    out.editable = UiTheme::ResolveEdit(role_);
    out.presentation = out.editable;
    out.presentation.metrics.face_enabled = false;
    out.presentation.metrics.frame_enabled = false;
    out.presentation.metrics.focus_enabled = false;
    out.presentation.metrics.shadow.enabled = false;
    out.presentation.show_readonly_bg = false;
    out.button = UiTheme::ResolveToolButton(button_role_);
    out.button.metrics.content_margin = Rect(0, 0, 0, 0);
    out.button_width = DPI(28);
    out.min_width = DPI(150);
    out.min_height = max(DPI(28), UiStyledOuterSizeFromContent(Size(0, out.editable.font.GetCy()),
                                                               out.editable.metrics,
                                                               out.editable.skin).cy);
    return out;
}

void UiDateTime::InvalidateThemeStyle()
{
    theme_revision_ = 0;
    ApplyStyle();
    SyncPopupStyle();
    RefreshLayout();
    Refresh();
}

void UiDateTime::SyncThemeStyle() const
{
    if(has_custom_style_)
        return;
    uint64 revision = UiTheme::GetRevision();
    if(theme_revision_ == revision)
        return;
    themed_style_ = ResolveThemeStyle();
    theme_revision_ = revision;
}

const UiDateTime::Style& UiDateTime::GetStyle() const
{
    if(has_custom_style_)
        return style_;
    SyncThemeStyle();
    return themed_style_;
}

UiDateTime& UiDateTime::SetCustomStyle(const Style& style)
{
    style_ = style;
    has_custom_style_ = true;
    ApplyStyle();
    SyncPopupStyle();
    RefreshLayout();
    Refresh();
    return *this;
}

UiDateTime& UiDateTime::ClearCustomStyle()
{
    if(has_custom_style_) {
        has_custom_style_ = false;
        theme_revision_ = 0;
        ApplyStyle();
        SyncPopupStyle();
        RefreshLayout();
        Refresh();
    }
    return *this;
}

UiDateTime& UiDateTime::SetRole(UiRole role)
{
    role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

UiDateTime& UiDateTime::SetButtonRole(UiRole role)
{
    button_role_ = role;
    if(!has_custom_style_)
        InvalidateThemeStyle();
    return *this;
}

UiDateTime& UiDateTime::SetMode(UiDateTimeMode mode)
{
    if(mode_ == mode)
        return *this;
    mode_ = mode;
    if(!IsNull(value_))
        value_ = NormalizeForMode(value_);
    SyncText();
    SyncPopupValues();
    RefreshLayout();
    return *this;
}

UiDateTime& UiDateTime::SetFormatStyle(UiDateTimeFormatStyle style)
{
    if(format_style_ != style) {
        format_style_ = style;
        SyncText();
    }
    return *this;
}

UiDateTime& UiDateTime::SetClockFormat(UiClockFormat format)
{
    if(clock_format_ != format) {
        clock_format_ = format;
        SyncText();
    }
    return *this;
}

UiDateTime& UiDateTime::ShowSeconds(bool on)
{
    show_seconds_ = on;
    time_popup_.Seconds(on);
    datetime_popup_.clock.Seconds(on);
    SyncText();
    return *this;
}

UiDateTime& UiDateTime::SetLanguage(int language)
{
    language_ = language;
    SyncText();
    return *this;
}

UiDateTime& UiDateTime::SetLanguage(const char *language)
{
    if(language && *language)
        language_ = LNGFromText(language);
    else
        language_ = LNG_CURRENT;
    SyncText();
    return *this;
}

UiDateTime& UiDateTime::SetEditable(bool on)
{
    if(editable_ == on)
        return *this;
    if(!on)
        HandleEditorCommit();
    editable_ = on;
    ApplyStyle();
    RefreshLayout();
    return *this;
}

UiDateTime& UiDateTime::ShowPresentationFrame(bool on)
{
    if(presentation_frame_ != on) {
        presentation_frame_ = on;
        ApplyStyle();
        RefreshLayout();
    }
    return *this;
}

UiDateTime& UiDateTime::AllowCopy(bool on)
{
    copy_allowed_ = on;
    editor_.SetClipboardPolicy(copy_allowed_, paste_allowed_ && editable_);
    return *this;
}

UiDateTime& UiDateTime::AllowPaste(bool on)
{
    paste_allowed_ = on;
    editor_.SetClipboardPolicy(copy_allowed_, paste_allowed_ && editable_);
    return *this;
}

bool UiDateTime::CopyValueToClipboard() const
{
    if(!copy_allowed_)
        return false;
    WriteClipboardText(GetDisplayText());
    return true;
}

bool UiDateTime::PasteValueFromClipboard(bool fire_action)
{
    if(!editable_ || !paste_allowed_ || !IsClipboardAvailableText())
        return false;
    return CommitText(ReadClipboardText(), fire_action);
}

UiDateTime& UiDateTime::AllowNull(bool on)
{
    allow_null_ = on;
    if(!allow_null_ && IsNull(value_))
        SetNow(false);
    return *this;
}

UiDateTime& UiDateTime::ClearValue(bool fire_action)
{
    if(!allow_null_)
        return *this;
    if(IsNull(value_))
        return *this;
    value_ = Null;
    SyncText();
    if(WhenChanging)
        WhenChanging();
    if(fire_action && WhenAction)
        WhenAction();
    return *this;
}

UiDateTime& UiDateTime::SetValue(Time value, bool fire_action)
{
    if(IsNull(value))
        return ClearValue(fire_action);
    if(!value.IsValid())
        return *this;

    value = ClampValue(NormalizeForMode(value));
    if(value_ == value)
        return *this;

    value_ = value;
    SyncText();
    SyncPopupValues();
    if(WhenChanging)
        WhenChanging();
    if(fire_action && WhenAction)
        WhenAction();
    return *this;
}

UiDateTime& UiDateTime::SetDate(Date date, bool fire_action)
{
    if(IsNull(date))
        return ClearValue(fire_action);
    if(!date.IsValid())
        return *this;
    Time value = IsNull(value_) ? Time(date.year, date.month, date.day)
                                : value_;
    value.year = date.year;
    value.month = date.month;
    value.day = date.day;
    return SetValue(value, fire_action);
}

Date UiDateTime::GetDate() const
{
    return DateFromTime_(value_);
}

UiDateTime& UiDateTime::SetTime(int hour, int minute, int second, bool fire_action)
{
    if(hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
        return *this;
    Time value = IsNull(value_) ? Time(1970, 1, 1, hour, minute, second) : value_;
    value.hour = hour;
    value.minute = minute;
    value.second = second;
    return SetValue(value, fire_action);
}

UiDateTime& UiDateTime::SetNow(bool fire_action)
{
    return SetValue(GetSysTime(), fire_action);
}

UiDateTime& UiDateTime::SetToday(bool fire_action)
{
    Date today = GetSysDate();
    Time value(today.year, today.month, today.day);
    if(mode_ != UiDateTimeMode::Date && !IsNull(value_)) {
        value.hour = value_.hour;
        value.minute = value_.minute;
        value.second = value_.second;
    }
    return SetValue(value, fire_action);
}

UiDateTime& UiDateTime::SetRange(Time minimum, Time maximum)
{
    minimum_ = IsNull(minimum) ? Time(Null) : NormalizeForMode(minimum);
    maximum_ = IsNull(maximum) ? Time(Null) : NormalizeForMode(maximum);
    if(!IsNull(minimum_) && !IsNull(maximum_) && CompareKey(minimum_) > CompareKey(maximum_))
        Swap(minimum_, maximum_);
    if(!IsNull(value_))
        SetValue(value_, false);
    return *this;
}

UiDateTime& UiDateTime::SetDateRange(Date minimum, Date maximum)
{
    Time lo = IsNull(minimum) ? Time(Null) : ToTime(minimum);
    Time hi = IsNull(maximum) ? Time(Null) : ToTime(maximum);
    return SetRange(lo, hi);
}

UiDateTime& UiDateTime::ClearRange()
{
    minimum_ = maximum_ = Null;
    return *this;
}

UiDateTime& UiDateTime::SetFirstDayOfWeek(int day)
{
    first_day_ = minmax(day, (int)SUNDAY, (int)SATURDAY);
    date_popup_.FirstDay(first_day_);
    datetime_popup_.calendar.FirstDay(first_day_);
    return *this;
}

String UiDateTime::FormatDatePart(Date date) const
{
    if(IsNull(date))
        return String();
    if(format_style_ == UiDateTimeFormatStyle::Iso)
        return Format("%04d-%02d-%02d", date.year, date.month, date.day);
    return GetLanguageInfo(language_).FormatDate(date);
}

String UiDateTime::FormatTimePart(Time time) const
{
    if(IsNull(time))
        return String();

    if(format_style_ == UiDateTimeFormatStyle::Locale && clock_format_ == UiClockFormat::Locale)
        return GetLanguageInfo(language_).FormatTime(time);

    bool hour12 = clock_format_ == UiClockFormat::Hour12;
    if(format_style_ == UiDateTimeFormatStyle::Iso)
        hour12 = false;

    if(hour12) {
        int hour = time.hour % 12;
        if(hour == 0)
            hour = 12;
        return show_seconds_
             ? Format("%d:%02d:%02d %s", hour, time.minute, time.second, time.hour >= 12 ? "PM" : "AM")
             : Format("%d:%02d %s", hour, time.minute, time.hour >= 12 ? "PM" : "AM");
    }

    return show_seconds_
         ? Format("%02d:%02d:%02d", time.hour, time.minute, time.second)
         : Format("%02d:%02d", time.hour, time.minute);
}

String UiDateTime::GetDisplayText() const
{
    if(IsNull(value_))
        return String();
    if(mode_ == UiDateTimeMode::Date)
        return FormatDatePart(GetDate());
    if(mode_ == UiDateTimeMode::Time)
        return FormatTimePart(value_);
    String date = FormatDatePart(GetDate());
    String time = FormatTimePart(value_);
    return format_style_ == UiDateTimeFormatStyle::Iso ? date + "T" + time
                                                        : date + " " + time;
}

bool UiDateTime::ParseDatePart(const String& text, Date& date) const
{
    String s = TrimBoth(text);
    if(s.IsEmpty())
        return false;

    Vector<String> number = DateTimeNumbers_(s);
    if(number.GetCount() >= 3 && number[0].GetCount() == 4) {
        Date iso(DateTimeInt_(number[0]), DateTimeInt_(number[1]), DateTimeInt_(number[2]));
        if(iso.IsValid()) {
            date = iso;
            return true;
        }
    }

    if(IsCurrentLanguage_(language_)) {
        Date scanned = ScanDate(~s, Null);
        if(!IsNull(scanned) && scanned.IsValid()) {
            date = scanned;
            return true;
        }
    }

    if(number.GetCount() < 3)
        return false;

    const String& format = GetLanguageInfo(language_).date_format;
    int pos[3] = { format.Find("%1"), format.Find("%2"), format.Find("%3") };
    int order[3] = { 0, 1, 2 }; // year, month, day
    for(int i = 0; i < 3; i++)
        if(pos[i] < 0)
            pos[i] = 1000 + i;
    for(int i = 0; i < 2; i++)
        for(int j = i + 1; j < 3; j++)
            if(pos[order[j]] < pos[order[i]])
                Swap(order[i], order[j]);

    int field[3] = { 0, 0, 0 };
    for(int i = 0; i < 3; i++)
        field[order[i]] = DateTimeInt_(number[i]);

    int year = field[0];
    if(year >= 0 && year < 100)
        year += year < 70 ? 2000 : 1900;
    Date parsed(year, field[1], field[2]);
    if(!parsed.IsValid())
        return false;
    date = parsed;
    return true;
}

bool UiDateTime::ParseTimePart(const String& text, int& hour, int& minute, int& second) const
{
    String s = TrimBoth(text);
    Vector<String> number = DateTimeNumbers_(s);
    if(number.GetCount() < 2)
        return false;

    hour = DateTimeInt_(number[0]);
    minute = DateTimeInt_(number[1]);
    second = number.GetCount() >= 3 ? DateTimeInt_(number[2]) : 0;

    String lower = ToLower(s);
    bool pm = lower.Find("pm") >= 0 || lower.Find("p.m.") >= 0;
    bool am = lower.Find("am") >= 0 || lower.Find("a.m.") >= 0;
    if(am || pm) {
        if(hour < 1 || hour > 12)
            return false;
        if(hour == 12)
            hour = 0;
        if(pm)
            hour += 12;
    }

    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
}

bool UiDateTime::ParseDateTimeParts(const String& text, Time& value) const
{
    String s = TrimBoth(text);
    for(int i = 1; i < s.GetCount() - 1; i++) {
        if(s[i] != 'T' && s[i] != 't' && !IsSpace((byte)s[i]))
            continue;
        Date date;
        int hour = 0, minute = 0, second = 0;
        if(ParseDatePart(s.Left(i), date) && ParseTimePart(s.Mid(i + 1), hour, minute, second)) {
            value = Time(date.year, date.month, date.day, hour, minute, second);
            return true;
        }
    }

    Vector<String> number = DateTimeNumbers_(s);
    if(number.GetCount() < 5)
        return false;

    Date date;
    String date_text = number[0] + "/" + number[1] + "/" + number[2];
    if(number[0].GetCount() == 4)
        date_text = number[0] + "-" + number[1] + "-" + number[2];
    if(!ParseDatePart(date_text, date))
        return false;

    int hour = DateTimeInt_(number[3]);
    int minute = DateTimeInt_(number[4]);
    int second = number.GetCount() >= 6 ? DateTimeInt_(number[5]) : 0;
    String lower = ToLower(s);
    bool pm = lower.Find("pm") >= 0 || lower.Find("p.m.") >= 0;
    bool am = lower.Find("am") >= 0 || lower.Find("a.m.") >= 0;
    if(am || pm) {
        if(hour < 1 || hour > 12)
            return false;
        if(hour == 12)
            hour = 0;
        if(pm)
            hour += 12;
    }
    if(hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
        return false;
    value = Time(date.year, date.month, date.day, hour, minute, second);
    return value.IsValid();
}

bool UiDateTime::ParseText(const String& text, Time& value) const
{
    String s = TrimBoth(text);
    if(s.IsEmpty()) {
        value = Null;
        return allow_null_;
    }

    if(mode_ == UiDateTimeMode::Date) {
        Date date;
        if(!ParseDatePart(s, date))
            return false;
        value = ToTime(date);
        return true;
    }

    if(mode_ == UiDateTimeMode::Time) {
        int hour = 0, minute = 0, second = 0;
        if(!ParseTimePart(s, hour, minute, second))
            return false;
        value = Time(1970, 1, 1, hour, minute, second);
        return true;
    }

    return ParseDateTimeParts(s, value);
}

bool UiDateTime::CommitText(const String& text, bool fire_action)
{
    Time parsed;
    if(!ParseText(text, parsed)) {
        SyncText();
        if(WhenInvalid)
            WhenInvalid(text);
        return false;
    }
    if(IsNull(parsed)) {
        ClearValue(fire_action);
        return true;
    }
    SetValue(parsed, fire_action);
    return true;
}

void UiDateTime::HandleEditorChange()
{
    if(syncing_ || !editable_)
        return;
    Time parsed;
    if(ParseText(editor_.GetTextUtf8(), parsed) && !IsNull(parsed)) {
        parsed = ClampValue(NormalizeForMode(parsed));
        if(parsed != value_) {
            value_ = parsed;
            SyncPopupValues();
            if(WhenChanging)
                WhenChanging();
        }
    }
}

void UiDateTime::HandleEditorCommit()
{
    if(syncing_ || !editable_)
        return;
    CommitText(editor_.GetTextUtf8(), true);
}

void UiDateTime::HandlePickedDate(Date date, bool final_action)
{
    if(IsNull(date) || !date.IsValid())
        return;
    Time value = IsNull(value_) ? Time(date.year, date.month, date.day) : value_;
    value.year = date.year;
    value.month = date.month;
    value.day = date.day;
    SetValue(value, final_action);
}

void UiDateTime::HandlePickedClock(Time time, bool final_action)
{
    Time value = IsNull(value_) ? Time(1970, 1, 1) : value_;
    value.hour = time.hour;
    value.minute = time.minute;
    value.second = show_seconds_ ? time.second : 0;
    SetValue(value, final_action);
}

Time UiDateTime::NormalizeForMode(Time value) const
{
    if(IsNull(value))
        return value;
    if(mode_ == UiDateTimeMode::Date) {
        value.hour = value.minute = value.second = 0;
    }
    else if(mode_ == UiDateTimeMode::Time) {
        value.year = 1970;
        value.month = 1;
        value.day = 1;
        if(!show_seconds_)
            value.second = 0;
    }
    else if(!show_seconds_)
        value.second = 0;
    return value;
}

int64 UiDateTime::CompareKey(Time value) const
{
    if(IsNull(value))
        return INT64_MIN;
    if(mode_ == UiDateTimeMode::Date)
        return Date(value.year, value.month, value.day).Get();
    if(mode_ == UiDateTimeMode::Time)
        return int64(value.hour) * 3600 + value.minute * 60 + value.second;
    return value.Get();
}

Time UiDateTime::ClampValue(Time value) const
{
    if(IsNull(value))
        return value;
    if(!IsNull(minimum_) && CompareKey(value) < CompareKey(minimum_))
        value = minimum_;
    if(!IsNull(maximum_) && CompareKey(value) > CompareKey(maximum_))
        value = maximum_;
    return NormalizeForMode(value);
}

void UiDateTime::SyncText()
{
    bool old = syncing_;
    syncing_ = true;
    editor_.SetTextUtf8(GetDisplayText());
    syncing_ = old;
}

void UiDateTime::ApplyStyle()
{
    const Style& style = GetStyle();
    editor_.SetCustomStyle(editable_ || presentation_frame_ ? style.editable : style.presentation);
    editor_.SetEditable(editable_);
    editor_.SetClipboardPolicy(copy_allowed_, paste_allowed_ && editable_);
    picker_button_.SetCustomStyle(style.button);
    picker_button_.Show(editable_);
}

void UiDateTime::SyncPopupStyle()
{
    const Style& style = GetStyle();
    UiButton::Style accent = UiTheme::ResolveButton(UiRole::Accent);
    UiButton::Style subtle = UiTheme::ResolveButton(UiRole::Subtle);

    Color face = SolidColor_(style.editable.palette.face[ST_NORMAL], SColorPaper());
    Color ink = style.editable.palette.ink[ST_NORMAL];
    if(IsNull(ink))
        ink = SColorText();
    Color frame = style.editable.palette.frame[ST_NORMAL];
    if(IsNull(frame))
        frame = SColorShadow();
    Color accent_face = SolidColor_(accent.palette.face[ST_NORMAL], SColorHighlight());
    Color accent_ink = accent.palette.ink[ST_NORMAL];
    if(IsNull(accent_ink))
        accent_ink = SColorHighlightText();
    Color subtle_face = SolidColor_(subtle.palette.face[ST_NORMAL], face);
    Color subtle_ink = subtle.palette.ink[ST_NORMAL];
    if(IsNull(subtle_ink))
        subtle_ink = ink;

    calendar_style_ = Calendar::StyleDefault();
    calendar_style_.header = subtle_face;
    calendar_style_.bgmain = face;
    calendar_style_.bgtoday = Blend(face, accent_face, 28);
    calendar_style_.bgselect = accent_face;
    calendar_style_.fgmain = ink;
    calendar_style_.fgtoday = ink;
    calendar_style_.fgselect = accent_ink;
    calendar_style_.outofmonth = Blend(face, ink, 96);
    calendar_style_.curdate = accent_face;
    calendar_style_.today = accent_face;
    calendar_style_.selecttoday = accent_ink;
    calendar_style_.cursorday = accent_face;
    calendar_style_.selectday = accent_ink;
    calendar_style_.line = frame;
    calendar_style_.dayname = subtle_ink;
    calendar_style_.week = subtle_ink;
    calendar_style_.font = style.editable.font;
    calendar_style_.spinhighlight = true;

    clock_style_ = Clock::StyleDefault();
    clock_style_.header = subtle_face;
    clock_style_.bgmain = face;
    clock_style_.fgmain = ink;
    clock_style_.arrowhl = accent_face;
    clock_style_.arrowhour = ink;
    clock_style_.arrowminute = accent_face;
    clock_style_.arrowsecond = accent_face;
    clock_style_.font = style.editable.font;

    date_popup_.SetStyle(calendar_style_).FirstDay(first_day_);
    time_popup_.SetStyle(clock_style_).Seconds(show_seconds_);
    datetime_popup_.calendar.SetStyle(calendar_style_).FirstDay(first_day_);
    datetime_popup_.clock.SetStyle(clock_style_).Seconds(show_seconds_);
}

void UiDateTime::SyncPopupValues()
{
    if(IsNull(value_))
        return;
    date_popup_.SetDate(GetDate());
    time_popup_.SetTime(value_);
    datetime_popup_.calendar.SetTime(value_);
    datetime_popup_.clock.SetTime(value_);
}

Rect UiDateTime::GetPopupRect(Size size) const
{
    Rect work = GetWorkArea();
    Rect screen = GetScreenRect();
    int x = screen.left;
    int y = screen.bottom;
    if(y + size.cy > work.bottom)
        y = screen.top - size.cy;
    if(x + size.cx > work.right)
        x = work.right - size.cx;
    x = max(work.left, x);
    y = max(work.top, y);
    return RectC(x, y, size.cx, size.cy);
}

void UiDateTime::OpenPicker()
{
    if(!editable_)
        return;
    HandleEditorCommit();
    if(IsNull(value_))
        value_ = NormalizeForMode(GetSysTime());
    SyncPopupStyle();
    SyncPopupValues();
    if(WhenOpenPicker)
        WhenOpenPicker();

    if(mode_ == UiDateTimeMode::Date) {
        Size size = date_popup_.GetPopUpSize();
        Rect rect = GetPopupRect(size);
        date_popup_.PopUp(this, rect);
    }
    else if(mode_ == UiDateTimeMode::Time) {
        Size size = time_popup_.GetPopUpSize();
        time_popup_.PopUp(this, GetPopupRect(size));
    }
    else {
        Size size = datetime_popup_.GetCalendarClockSize();
        datetime_popup_.PopUp(this, GetPopupRect(size));
    }
}

void UiDateTime::SetData(const Value& value)
{
    if(IsNull(value)) {
        ClearValue(false);
        return;
    }
    if(value.Is<Date>()) {
        SetDate((Date)value, false);
        return;
    }
    if(value.Is<Time>())
        SetValue((Time)value, false);
}

Value UiDateTime::GetData() const
{
    if(IsNull(value_))
        return Value();
    return mode_ == UiDateTimeMode::Date ? Value(GetDate()) : Value(value_);
}

Size UiDateTime::GetMinSize() const
{
    const Style& style = GetStyle();
    Size editor = editor_.GetMinSize();
    int width = max(style.min_width, editor.cx + (editable_ ? style.button_width : 0));
    int height = max(style.min_height, editor.cy);
    return Size(width, height);
}

void UiDateTime::Layout()
{
    const Style& style = GetStyle();
    Rect rect = GetSize();
    int button = editable_ ? min(style.button_width, rect.GetWidth()) : 0;
    editor_.SetRect(rect.left, rect.top, max(0, rect.GetWidth() - button), rect.GetHeight());
    if(editable_)
        picker_button_.SetRect(rect.right - button, rect.top, button, rect.GetHeight());
}

void UiDateTime::State(int reason)
{
    Ctrl::State(reason);
    if(reason == FOCUS || reason == ENABLE || reason == SHOW || reason == LAYOUTPOS) {
        if(!has_custom_style_ && theme_revision_ != UiTheme::GetRevision()) {
            ApplyStyle();
            SyncPopupStyle();
        }
    }
}

} // namespace Upp
