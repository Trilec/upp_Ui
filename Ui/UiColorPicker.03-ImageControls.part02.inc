                          marker.top + (marker.GetHeight() - size.cy) / 2,
                          text, font, SColorText());
        }
        for(int i = 0; i < manual_points_.GetCount(); i++) {
            Point point = ScreenPoint(manual_points_[i], image_rect);
            int radius = DPI(8);
            Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
            Color color = SampleImage_(image_, manual_points_[i], 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, color, DPI(2), White());
            String text = AsString(i + 1);
            Font font = SansSerif().Height(DPI(8)).Bold();
            Size size = GetTextSize(text, font);
            draw.DrawText(marker.left + (marker.GetWidth() - size.cx) / 2,
                          marker.top + (marker.GetHeight() - size.cy) / 2,
                          text, font, SColorText());
        }
        if(manual_points_.IsEmpty()) {
            int count = min(result_.representative_positions.GetCount(), result_.swatches.GetCount());
            for(int i = 0; i < count; i++) {
                Point point = ScreenPoint(result_.representative_positions[i], image_rect);
                int radius = DPI(7);
                Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
                Color color = result_.swatches[i].value.color;
                draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
                draw.DrawEllipse(marker, color, DPI(2), White());
                String text = AsString(i + 1);
                Font font = SansSerif().Height(DPI(7)).Bold();
                Size size = GetTextSize(text, font);
                Color ink = IsDark(color) ? White() : Black();
                draw.DrawText(marker.left + (marker.GetWidth() - size.cx) / 2,
                              marker.top + (marker.GetHeight() - size.cy) / 2,
                              text, font, ink);
            }
        }
    }

    int HitSeed(Point point, const Rect& image_rect) const
    {
        for(int i = 1; i >= 0; i--) {
            if(!seed_[i].enabled || !seed_[i].placed)
                continue;
            Point marker = ScreenPoint(seed_[i].position, image_rect);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            if(dx * dx + dy * dy <= DPI(16) * DPI(16))
                return i;
        }
        return -1;
    }

    int HitManual(Point point, const Rect& image_rect) const
    {
        for(int i = manual_points_.GetCount() - 1; i >= 0; i--) {
            Point marker = ScreenPoint(manual_points_[i], image_rect);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            if(dx * dx + dy * dy <= DPI(15) * DPI(15))
                return i;
        }
        return -1;
    }

    void MoveSeed(Point point, const Rect& image_rect, bool final_commit)
    {
        Pointf normalized = NormalizePoint(point, image_rect);
        if(WhenSeedMoved)
            WhenSeedMoved(drag_seed_, normalized, final_commit);
    }

    void MoveManual(Point point, const Rect& image_rect, bool final_commit)
    {
        Pointf normalized = NormalizePoint(point, image_rect);
        if(drag_manual_ >= 0 && drag_manual_ < manual_points_.GetCount())
            manual_points_[drag_manual_] = normalized;
        if(WhenManualMoved)
            WhenManualMoved(drag_manual_, normalized, final_commit);
        Refresh();
    }

    Image image_;
    ImageAnalysisResult result_;
    ImageExclusionSeed seed_[2];
    Vector<Pointf> manual_points_;
    int armed_seed_ = -1;
    int drag_seed_ = -1;
    int drag_manual_ = -1;
    bool show_mask_ = false;
    bool drop_hot_ = false;
    bool panning_ = false;
    Point pan_anchor_;
    Point pan_;
    double zoom_ = 1.0;
};

struct SharedSession_ {
    bool initialized = false;
    Vector<UiColorPicker::SlotValue> slots;
    Vector<UiColorPicker::SlotValue> previous;
    Vector<UiColorPicker::SlotValue> recent;
    Vector<UiColorPicker::SlotValue> stash;
    int active_slot = 0;
    bool alpha_enabled = true;
    int page = UiColorPicker::PAGE_COLOR;
    int spectrum = UiColorPicker::SPECTRUM_HUE_STRIP;
    int channel = UiColorPicker::CHANNEL_RGB_FLOAT;
    int palette_index = 0;
    GeneratorRecipe recipe;
    Image image;
    ImageAnalysisSettings image_settings;
    int image_gain = 0;
    int image_saturation = 100;
    int image_hero = -1;
    int image_hero_gain = 0;
    int stash_drop_mode = UiColorPicker::STASH_REPLACE;
};

static SharedSession_& SharedSessionState_()
{
    static SharedSession_ session;
    return session;
}

} // namespace