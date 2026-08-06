            cache_ = Image();
            cache_key_ = UiRasterCacheKey();
        }
        Refresh();
    }

    Event<Color, bool> WhenColor;

    virtual Size GetMinSize() const override { return Size(DPI(220), DPI(210)); }

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        draw.DrawRect(rect, SurfaceColor_());
        const Image& image = EnsureCache();
        if(!image.IsEmpty())
            draw.DrawImage(rect.left, rect.top, image);
        Point marker = Marker();
        Rect outer = RectC(marker.x - DPI(5), marker.y - DPI(5), DPI(10), DPI(10));
        DrawFrame_(draw, outer, White(), DPI(2));
        DrawFrame_(draw, outer.Deflated(DPI(2)), Black(), DPI(1));
    }

    virtual void LeftDown(Point point, dword) override
    {
        SetCapture();
        Pick(point, false);
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(HasCapture() && (flags & K_MOUSELEFT))
            Pick(point, false);
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(HasCapture())
            ReleaseCapture();
        Pick(point, true);
    }

private:
    Color Sample(Point point) const
    {
        int width = max(1, GetSize().cx - 1);
        int height = max(1, GetSize().cy - 1);
        int x = minmax(point.x, 0, width);
        int y = minmax(point.y, 0, height);
        switch(mode_) {
        case UiColorPicker::SPECTRUM_HUE_STRIP: {
            int hue = int(x / double(width) * 359.0 + 0.5);
            int saturation = 100 - int(y / double(height) * 100.0 + 0.5);
            return HsvToColor(hue, saturation, gain_);
        }
        case UiColorPicker::SPECTRUM_RGB_SPECTRUM:
            return Color(ClampByte_(int(x / double(width) * 255.0 + 0.5)),
                         ClampByte_(255 - int(y / double(height) * 255.0 + 0.5)),
                         color_.GetB());
        case UiColorPicker::SPECTRUM_HSV_WHEEL: {
            Point centre(width / 2, height / 2);
            double dx = x - centre.x;
            double dy = centre.y - y;
            double radius = max(1.0, min(width, height) * 0.5);
            if(sqrt(dx * dx + dy * dy) > radius)
                return Null;
            int hue = NormalizeHue(int(atan2(dy, dx) * 180.0 / M_PI + 0.5));
            int saturation = minmax(int(sqrt(dx * dx + dy * dy) / radius * 100.0 + 0.5), 0, 100);
            return HsvToColor(hue, saturation, gain_);
        }
        case UiColorPicker::SPECTRUM_HSV_RECT:
        default: {
            int saturation = int(x / double(width) * 100.0 + 0.5);
            int value = 100 - int(y / double(height) * 100.0 + 0.5);
            return HsvToColor(remembered_hue_, saturation, value);
        }
        }
    }

    void Pick(Point point, bool final_commit)
    {
        Color color = Sample(point);
        if(!IsNull(color) && WhenColor)
            WhenColor(color, final_commit);
    }

    Point Marker() const
    {
        int width = max(1, GetSize().cx - 1);
        int height = max(1, GetSize().cy - 1);
        int hue = 0, saturation = 0, value = 0;
        ColorToHsv(color_, hue, saturation, value);
        if(saturation == 0 || value == 0)
            hue = remembered_hue_;
        switch(mode_) {
        case UiColorPicker::SPECTRUM_HUE_STRIP:
            return Point(int(hue / 359.0 * width + 0.5),
                         int((100 - saturation) / 100.0 * height + 0.5));
        case UiColorPicker::SPECTRUM_RGB_SPECTRUM:
            return Point(int(color_.GetR() / 255.0 * width + 0.5),
                         int((255 - color_.GetG()) / 255.0 * height + 0.5));
        case UiColorPicker::SPECTRUM_HSV_WHEEL: {
            double angle = hue * M_PI / 180.0;
            double radius = saturation / 100.0 * min(width, height) * 0.5;
            Point centre(width / 2, height / 2);
            return Point(centre.x + int(cos(angle) * radius + 0.5),
                         centre.y - int(sin(angle) * radius + 0.5));
        }
        case UiColorPicker::SPECTRUM_HSV_RECT:
        default:
            return Point(int(saturation / 100.0 * width + 0.5),
                         int((100 - value) / 100.0 * height + 0.5));
        }
    }

    const Image& EnsureCache() const
    {
        Size size = GetSize();
        if(size.IsEmpty())
            return cache_;
        UiRasterCacheKeyBuilder builder("palette-lab-color-field");
        builder.Add(size).Add((int)mode_).Add(remembered_hue_).Add(gain_).Add(color_.GetB());
        UiRasterCacheKey key = builder.Build();
        if(cache_.IsEmpty() || !(cache_key_ == key)) {
            UiRasterCachePolicy policy = UiRasterPolicyAA("palette-lab-color-field");
            policy.allow_scale_from_bucket = false;
            policy.max_single_image_bytes = 2 * 1024 * 1024;
            cache_ = UiRasterCache::Get(key, policy, [=] {
                ImageBuffer buffer(size);
                for(int y = 0; y < size.cy; y++) {
                    RGBA *row = buffer[y];
                    for(int x = 0; x < size.cx; x++) {
                        Color color = Sample(Point(x, y));
                        if(IsNull(color))
                            row[x] = RGBAZero();
                        else {
                            row[x] = RGBA(color);
                            row[x].a = 255;
                        }
                    }
                }
                return Image(buffer);
            });
            cache_key_ = key;
        }
        return cache_;
    }

    UiColorPicker::SpectrumMode mode_ = UiColorPicker::SPECTRUM_HUE_STRIP;
    Color color_ = Color(0, 120, 212);
    int remembered_hue_ = 200;
    int gain_ = 100;
    mutable UiRasterCacheKey cache_key_;
    mutable Image cache_;
};

class PaletteWheel_ : public Ctrl {
public:
    typedef PaletteWheel_ CLASSNAME;

    enum Mode {
        PASSIVE,
        GENERATOR
    };

    PaletteWheel_() { WantFocus(); }

    void SetPassive(const Vector<DisplaySwatch_>& items, const Vector<int>& selected, int active)
    {
        mode_ = PASSIVE;
        passive_items_ = clone(items);
        passive_selected_ = clone(selected);
        active_index_ = active;
        Refresh();
    }

    void SetGenerator(const GeneratorRecipe& recipe,
                      const Vector<GeneratedSwatch>& generated,
                      int active_family)
    {
        mode_ = GENERATOR;
        recipe_ = clone(recipe);
        generated_ = clone(generated);
        active_family_ = active_family;
        Refresh();
    }

    Event<int> WhenPassivePick;
    Event<int, int, int, int, bool> WhenAnchorChange;

    virtual Size GetMinSize() const override { return Size(DPI(180), DPI(180)); }

    virtual void Paint(Draw& draw) override
    {
        Rect outer(Point(0, 0), GetSize());
        draw.DrawRect(outer, SurfaceColor_());
        Rect wheel = WheelRect();
        if(wheel.IsEmpty())
            return;
        EnsureCache(wheel.GetSize());
        draw.DrawImage(wheel.left, wheel.top, cache_);
        DrawGuides(draw, wheel);
        if(mode_ == PASSIVE)
            DrawPassive(draw, wheel);
        else
            DrawGenerator(draw, wheel);
        DrawFrame_(draw, outer, FrameColor_());
    }

    virtual void LeftDown(Point point, dword) override
    {
        Rect wheel = WheelRect();
        if(mode_ == PASSIVE) {
            int index = HitPassive(point, wheel);
            if(index >= 0 && WhenPassivePick)
                WhenPassivePick(index);
            return;
        }
        drag_family_ = HitFamily(point, wheel);
