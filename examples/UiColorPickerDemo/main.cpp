/*
    UiColorPickerDemo
    -----------------

    Purpose
    - Active UiColorPicker demo used as a build smoke test, API round-trip check,
      and visual styling reference.

    Demo hygiene header
    - Keep this package compiling in the active demo sweep.
    - Use BuilderDemoSupport for header/version/theme/exit shell behavior.
    - Prefer UiTheme defaults; local painting here is limited to the API result
      snapshot and alpha swatch visualization.

    Changelog
    - 2026-05: migrated from dark-only standalone host to shared builder shell.
*/

#include "../BuilderDemoSupport.h"
#include <Ui/UiColorPicker.h>

using namespace Upp;
using namespace BuilderDemoSupport;

namespace {

Color AlphaComposite(Color fg, int alpha, Color bg)
{
    int a = max(0, min(255, alpha));
    int ia = 255 - a;
    return Color((fg.GetR() * a + bg.GetR() * ia + 127) / 255,
                 (fg.GetG() * a + bg.GetG() * ia + 127) / 255,
                 (fg.GetB() * a + bg.GetB() * ia + 127) / 255);
}

Color CheckerColor(int x, int y, int tile, const DemoPalette& p)
{
    bool on = ((x / max(1, tile)) + (y / max(1, tile))) & 1;
    return on ? Blend(p.paper, p.ink, p.dark ? 190 : 220)
              : Blend(p.paper, p.ink, p.dark ? 225 : 238);
}

String FormatRgb(Color c)
{
    return Format("%d, %d, %d", c.GetR(), c.GetG(), c.GetB());
}

class ApiResultsPanel : public Ctrl {
public:
    typedef ApiResultsPanel CLASSNAME;

    void SetPalette(const DemoPalette& p)
    {
        palette_ = p;
        Refresh();
    }

    void SetResults(const Vector<UiColorPicker::SlotValue>& slots, int active_slot, const String& active_hex, int active_alpha)
    {
        slots_ = clone(slots);
        active_slot_ = active_slot;
        active_hex_ = active_hex;
        active_alpha_ = active_alpha;
        Refresh();
    }

    virtual void Paint(Draw& w) override
    {
        Rect r(GetSize());
        w.DrawRect(r, palette_.paper);

        Font title = DemoSans(13, true);
        Font mono = DemoMono(10);
        Font body = DemoSans(10);
        Color ink = palette_.ink;
        Color muted = palette_.muted;
        Color frame = palette_.divider;
        Color row_face = palette_.dark ? Color(21, 25, 32) : Color(244, 248, 253);
        Color active_face = palette_.dark ? Color(26, 40, 58) : Color(230, 240, 255);

        int x = DPI(18);
        int y = DPI(16);
        w.DrawText(x, y, "API result snapshot", title, ink);
        y += DPI(26);
        w.DrawText(x, y, Format("Active slot: %d    HEX8: %s    Alpha: %d", active_slot_ + 1, active_hex_, active_alpha_), body, muted);
        y += DPI(30);

        int row_h = DPI(62);
        for(int i = 0; i < slots_.GetCount(); i++) {
            Rect row(x, y, r.right - DPI(18), y + row_h);
            w.DrawRect(row, i == active_slot_ ? active_face : row_face);
            DrawFrame(w, row, frame);

            Rect sw(row.left + DPI(12), row.top + DPI(12), row.left + DPI(72), row.bottom - DPI(12));
            DrawSwatch(w, sw, slots_[i].color, slots_[i].alpha);

            String label = slots_[i].label.IsEmpty() ? Format("Slot %d", i + 1) : slots_[i].label;
            String hex = Format("#%02X%02X%02X%02X", slots_[i].color.GetR(), slots_[i].color.GetG(), slots_[i].color.GetB(), slots_[i].alpha);
            int tx = sw.right + DPI(14);
            w.DrawText(tx, row.top + DPI(10), Format("%s  %s", label, i == active_slot_ ? "(active)" : ""), DemoSans(10, true), ink);
            w.DrawText(tx, row.top + DPI(31), Format("color={%s}  alpha=%d  hex8=%s", FormatRgb(slots_[i].color), slots_[i].alpha, hex), mono, muted);
            y += row_h + DPI(8);
        }
    }

private:
    void DrawFrame(Draw& w, const Rect& r, Color c)
    {
        w.DrawRect(r.left, r.top, r.GetWidth(), 1, c);
        w.DrawRect(r.left, r.bottom - 1, r.GetWidth(), 1, c);
        w.DrawRect(r.left, r.top, 1, r.GetHeight(), c);
        w.DrawRect(r.right - 1, r.top, 1, r.GetHeight(), c);
    }

    void DrawSwatch(Draw& w, const Rect& r, Color c, int alpha)
    {
        int tile = max(2, DPI(5));
        for(int y = r.top; y < r.bottom; y += tile)
            for(int x = r.left; x < r.right; x += tile)
                w.DrawRect(x, y, min(tile, r.right - x), min(tile, r.bottom - y), CheckerColor(x - r.left, y - r.top, tile, palette_));
        for(int y = r.top; y < r.bottom; y++) {
            Color bg = CheckerColor(0, y - r.top, tile, palette_);
            w.DrawRect(r.left, y, r.GetWidth(), 1, AlphaComposite(c, alpha, bg));
        }
        DrawFrame(w, r, palette_.preview_frame);
    }

    DemoPalette palette_ = ResolveDemoPalette(UiThemeMode::Light);
    Vector<UiColorPicker::SlotValue> slots_;
    int active_slot_ = 0;
    String active_hex_ = "#000000FF";
    int active_alpha_ = 255;
};

class ColorPickerDialog : public TopWindow {
public:
    typedef ColorPickerDialog CLASSNAME;

    ColorPickerDialog(const Vector<UiColorPicker::SlotValue>& slots, int active_slot)
    {
        Title("Brand color slots");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(800), DPI(610));
        SetMinSize(Size(DPI(760), DPI(580)));
        CenterOwner();

        Add(host_);
        host_.Add(picker_);
        host_.SetCustomStyle(UiTheme::ResolvePanel(UiPanelRole::Surface));
        picker_.WhenAccept = [=] { Break(IDOK); };
        picker_.WhenCancel = [=] { Break(IDCANCEL); };

        picker_.SetSlotCount(4).SetAlphaEnabled(true);
        for(int i = 0; i < min(4, slots.GetCount()); i++)
            picker_.SetSlot(i, slots[i].color, slots[i].alpha, false).SetSlotLabel(i, slots[i].label);
        picker_.SetActiveSlot(active_slot);
    }

    const UiColorPicker& Picker() const { return picker_; }

    virtual void Paint(Draw& w) override
    {
        w.DrawRect(GetSize(), SColorPaper());
    }

    virtual void Layout() override
    {
        Rect r = Rect(GetSize()).Deflated(DPI(14));
        host_.SetRect(r);
        picker_.SetRect(r.Deflated(DPI(12)));
    }

private:
    UiPanel host_;
    UiColorPicker picker_;
};

class ColorPickerBuilder : public BuilderWindowBase {
public:
    typedef ColorPickerBuilder CLASSNAME;

    ColorPickerBuilder()
        : BuilderWindowBase("UiColorPickerDemo", "U++ UiColorPicker Builder",
                            "Open the picker dialog, edit color slots, and verify alpha, labels, and HEX8 through the public API.")
    {
        Preview().Add(open_button_);
        Preview().Add(results_);

        AddStateRow(StateBox(), state_slot_row_, state_slot_label_, state_slot_value_, "Active slot");
        AddStateRow(StateBox(), state_hex_row_, state_hex_label_, state_hex_value_, "HEX8");
        AddStateRow(StateBox(), state_alpha_row_, state_alpha_label_, state_alpha_value_, "Alpha");
        AddStateRow(StateBox(), state_count_row_, state_count_label_, state_count_value_, "Slots");

        AddButtonRow(PropsBox(), action_row_, open_button_prop_, reset_button_);
        AddToggleRow(PropsBox(), alpha_row_, "Alpha enabled");

        open_button_.SetText("Open color picker");
        open_button_.WhenAction = [=] { OpenPicker(); };
        open_button_prop_.SetText("Open dialog");
        open_button_prop_.WhenAction = [=] { OpenPicker(); };
        reset_button_.SetText("Reset slots");
        reset_button_.WhenAction = [=] {
            InitSlots();
            RefreshResults();
        };
        alpha_row_.Toggle().WhenAction = [=] {
            alpha_enabled_ = alpha_row_.Toggle().IsOn();
            SetUsageCode(BuildUsageCode());
        };

        InitSlots();
        FinishInit();
        RefreshResults();
    }

protected:
    virtual void ApplyDemoTheme() override
    {
        open_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
        open_button_prop_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
        reset_button_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
        results_.SetPalette(Palette());
    }

    virtual void LayoutPreviewContent() override
    {
        Rect canvas = Preview().GetCanvasRect();
        open_button_.SetRect(canvas.left, canvas.top, DPI(180), DPI(34));
        results_.SetRect(canvas.left, canvas.top + DPI(50), canvas.GetWidth(), max(0, canvas.GetHeight() - DPI(50)));
    }

private:
    void InitSlots()
    {
        slots_.SetCount(4);
        slots_[0].color = Color(255, 59, 48); slots_[0].alpha = 255; slots_[0].label = "Red";
        slots_[1].color = Color(52, 199, 89); slots_[1].alpha = 255; slots_[1].label = "Green";
        slots_[2].color = Color(0, 122, 255); slots_[2].alpha = 255; slots_[2].label = "Blue";
        slots_[3].color = Color(255, 204, 0); slots_[3].alpha = 255; slots_[3].label = "Yellow";
        active_slot_ = 0;
        active_hex_ = "#FF3B30FF";
        active_alpha_ = 255;
    }

    void OpenPicker()
    {
        ColorPickerDialog dlg(slots_, active_slot_);
        if(dlg.Execute() != IDOK)
            return;

        const UiColorPicker& picker = dlg.Picker();
        slots_ = picker.GetSlots();
        active_slot_ = picker.GetActiveSlot();
        active_hex_ = picker.FormatActiveHex8();
        active_alpha_ = picker.GetAlpha();
        RefreshResults();
    }

    void RefreshResults()
    {
        results_.SetPalette(Palette());
        results_.SetResults(slots_, active_slot_, active_hex_, active_alpha_);
        state_slot_value_.SetText(AsString(active_slot_ + 1));
        state_hex_value_.SetText(active_hex_);
        state_alpha_value_.SetText(AsString(active_alpha_));
        state_count_value_.SetText(AsString(slots_.GetCount()));
        alpha_row_.Toggle().SetOn(alpha_enabled_);
        SetUsageCode(BuildUsageCode());
        Preview().Refresh();
    }

    String BuildUsageCode() const
    {
        String code;
        code << "UiColorPicker picker;\n";
        code << "picker.SetSlotCount(4)\n";
        code << "      .SetAlphaEnabled(" << (alpha_enabled_ ? "true" : "false") << ");\n";
        for(int i = 0; i < slots_.GetCount(); i++) {
            code << "picker.SetSlot(" << i << ", " << ColorCpp(slots_[i].color) << ", " << slots_[i].alpha << ", false)\n";
            code << "      .SetSlotLabel(" << i << ", " << QuoteCpp(slots_[i].label) << ");\n";
        }
        code << "picker.SetActiveSlot(" << active_slot_ << ");\n";
        code << "String hex8 = picker.FormatActiveHex8();\n";
        code << "int alpha = picker.GetAlpha();\n";
        return code;
    }

    Vector<UiColorPicker::SlotValue> slots_;
    int active_slot_ = 0;
    String active_hex_ = "#FF3B30FF";
    int active_alpha_ = 255;
    bool alpha_enabled_ = true;

    UiButton open_button_;
    ApiResultsPanel results_;

    UiBoxLayout state_slot_row_ { UiBoxLayout::Direction::H }, state_hex_row_ { UiBoxLayout::Direction::H }, state_alpha_row_ { UiBoxLayout::Direction::H }, state_count_row_ { UiBoxLayout::Direction::H };
    UiLabel state_slot_label_, state_slot_value_, state_hex_label_, state_hex_value_, state_alpha_label_, state_alpha_value_, state_count_label_, state_count_value_;

    UiBoxLayout action_row_ { UiBoxLayout::Direction::H };
    UiButton open_button_prop_, reset_button_;
    UiCompositeToggle alpha_row_;
};

}

GUI_APP_MAIN
{
    ColorPickerBuilder demo;
    demo.Run();
}
