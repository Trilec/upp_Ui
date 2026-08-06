        if(drag_family_ >= 0 && drag_family_ < recipe_.families.GetCount() && !recipe_.families[drag_family_].locked) {
            SetCapture();
            DragAnchor(point, false);
        }
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(mode_ == GENERATOR && HasCapture() && (flags & K_MOUSELEFT))
            DragAnchor(point, false);
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(mode_ == GENERATOR && HasCapture()) {
            DragAnchor(point, true);
            ReleaseCapture();
        }
        drag_family_ = -1;
    }

private:
    Rect WheelRect() const
    {
        Rect rect(Point(0, 0), GetSize());
        int side = max(0, min(rect.GetWidth(), rect.GetHeight()) - DPI(18));
        return RectC(rect.left + (rect.GetWidth() - side) / 2,
                     rect.top + (rect.GetHeight() - side) / 2, side, side);
    }

    void EnsureCache(Size size)
    {
        if(!cache_.IsEmpty() && cache_.GetSize() == size && cache_revision_ == UiTheme::GetRevision())
            return;
        ImageBuffer buffer(size);
        Point centre(size.cx / 2, size.cy / 2);
        double radius = max(1.0, min(size.cx, size.cy) * 0.5);
        Color neutral = SurfaceColor_();
        for(int y = 0; y < size.cy; y++) {
            RGBA *row = buffer[y];
            for(int x = 0; x < size.cx; x++) {
                double dx = x - centre.x;
                double dy = y - centre.y;
                double distance = sqrt(dx * dx + dy * dy);
                if(distance > radius) {
                    row[x] = RGBAZero();
                    continue;
                }
                int hue = NormalizeHue(int(atan2(dy, dx) * 180.0 / M_PI + 90.0 + 0.5));
                double saturation = distance / radius * 100.0;
                Color hue_color = HsvToColor(hue, saturation, 94);
                Color color = Blend(hue_color, neutral, int((1.0 - distance / radius) * 90.0));
                row[x] = RGBA(color);
                row[x].a = 255;
            }
        }
        cache_ = Image(buffer);
        cache_revision_ = UiTheme::GetRevision();
    }

    void DrawGuides(Draw& draw, const Rect& wheel)
    {
        Point centre = wheel.CenterPoint();
        Color guide = Blend(SColorText(), SurfaceColor_(), 185);
        for(int percent = 25; percent <= 75; percent += 25) {
            int radius = wheel.GetWidth() * percent / 200;
            draw.DrawEllipse(RectC(centre.x - radius, centre.y - radius, radius * 2, radius * 2),
                             Null, DPI(1), guide);
        }
        for(int angle = 0; angle < 360; angle += 45) {
            Point edge = WheelPoint_(angle, 100, wheel);
            draw.DrawLine(centre.x, centre.y, edge.x, edge.y, DPI(1), guide);
            Point label = WheelPoint_(angle, 88, wheel);
            String text = AsString(angle);
            Font font = SansSerif().Height(DPI(8));
            Size size = GetTextSize(text, font);
            draw.DrawText(label.x - size.cx / 2, label.y - size.cy / 2,
                          text, font, Blend(SColorText(), SurfaceColor_(), 130));
        }
    }

    void DrawPassive(Draw& draw, const Rect& wheel)
    {
        for(int pass = 0; pass < 2; pass++) {
            for(int i = 0; i < passive_items_.GetCount(); i++) {
                bool active = i == active_index_;
                if((pass == 0 && active) || (pass == 1 && !active))
                    continue;
                int hue = 0, saturation = 0, value = 0;
                ColorToHsv(passive_items_[i].value.color, hue, saturation, value);
                Point point = WheelPoint_(hue, saturation, wheel);
                int radius = active ? DPI(6) : DPI(4);
                Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
                draw.DrawEllipse(marker, passive_items_[i].value.color, DPI(1), White());
                if(active)
                    draw.DrawEllipse(marker.Inflated(DPI(3)), Null, DPI(2), SColorHighlight());
                else if(FindIndex(passive_selected_, i) >= 0)
                    draw.DrawEllipse(marker.Inflated(DPI(2)), Null, DPI(1), SColorHighlight());
            }
        }
    }

    void DrawGenerator(Draw& draw, const Rect& wheel)
    {
        Point centre = wheel.CenterPoint();
        double scale = GlobalSaturationScale(recipe_);
        Vector<Point> anchors;
        for(int i = 0; i < recipe_.families.GetCount(); i++) {
            const PaletteFamily& family = recipe_.families[i];
            int offset = recipe_.free_angles ? family.custom_offset : family.canonical_offset;
            double saturation = family.locked ? family.authored_saturation
                                              : family.authored_saturation * scale;
            anchors.Add(WheelPoint_(recipe_.base_hue + offset, saturation, wheel));
        }
        for(const Point& point : anchors) {
            draw.DrawLine(centre.x, centre.y, point.x, point.y, DPI(2), White());
            draw.DrawLine(centre.x, centre.y + DPI(1), point.x, point.y + DPI(1), DPI(1), SColorShadow());
        }
        for(const GeneratedSwatch& swatch : generated_) {
            int hue = 0, saturation = 0, value = 0;
            ColorToHsv(swatch.value.color, hue, saturation, value);
            Point point = WheelPoint_(hue, saturation, wheel);
            Rect marker = RectC(point.x - DPI(2), point.y - DPI(2), DPI(4), DPI(4));
            draw.DrawEllipse(marker, swatch.value.color, DPI(1), White());
        }
        for(int i = 0; i < anchors.GetCount(); i++) {
            const PaletteFamily& family = recipe_.families[i];
            int offset = recipe_.free_angles ? family.custom_offset : family.canonical_offset;
            double saturation = family.locked ? family.authored_saturation
                                              : family.authored_saturation * scale;
            Color color = HsvToColor(recipe_.base_hue + offset, minmax(saturation, 0.0, 100.0), 84 + family.gain);
            int radius = DPI(10);
            Rect marker = RectC(anchors[i].x - radius, anchors[i].y - radius, radius * 2, radius * 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, color, DPI(2), White());
            if(i == active_family_)
                draw.DrawEllipse(marker.Inflated(DPI(4)), Null, DPI(2), SColorHighlight());
            String number = AsString(i + 1);
            Font font = SansSerif().Height(DPI(9)).Bold();
            Size text = GetTextSize(number, font);
            draw.DrawText(marker.left + (marker.GetWidth() - text.cx) / 2,
                          marker.top + (marker.GetHeight() - text.cy) / 2,
                          number, font, SColorText());
            if(family.locked) {
                String lock = "L";
                Font small = SansSerif().Height(DPI(7)).Bold();
                draw.DrawText(marker.right - DPI(2), marker.bottom - DPI(7), lock, small, SColorText());
            }
        }
    }

    int HitPassive(Point point, const Rect& wheel) const
    {
        int best = -1;
        int best_distance = INT_MAX;
        for(int i = 0; i < passive_items_.GetCount(); i++) {
            int hue = 0, saturation = 0, value = 0;
            ColorToHsv(passive_items_[i].value.color, hue, saturation, value);
            Point marker = WheelPoint_(hue, saturation, wheel);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            int distance = dx * dx + dy * dy;
