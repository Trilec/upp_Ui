            if(distance <= DPI(10) * DPI(10) && distance < best_distance) {
                best = i;
                best_distance = distance;
            }
        }
        return best;
    }

    int HitFamily(Point point, const Rect& wheel) const
    {
        double scale = GlobalSaturationScale(recipe_);
        for(int i = recipe_.families.GetCount() - 1; i >= 0; i--) {
            const PaletteFamily& family = recipe_.families[i];
            int offset = recipe_.free_angles ? family.custom_offset : family.canonical_offset;
            double saturation = family.locked ? family.authored_saturation
                                              : family.authored_saturation * scale;
            Point marker = WheelPoint_(recipe_.base_hue + offset, saturation, wheel);
            int dx = point.x - marker.x, dy = point.y - marker.y;
            if(dx * dx + dy * dy <= DPI(16) * DPI(16))
                return i;
        }
        return -1;
    }

    void DragAnchor(Point point, bool final_commit)
    {
        if(drag_family_ < 0 || drag_family_ >= recipe_.families.GetCount())
            return;
        Rect wheel = WheelRect();
        int hue = 0, saturation = 0;
        PointToWheel_(point, wheel, hue, saturation);
        const PaletteFamily& family = recipe_.families[drag_family_];
        int base_hue = recipe_.base_hue;
        int custom_offset = family.custom_offset;
        if(recipe_.free_angles)
            custom_offset = NormalizeHue(hue - base_hue);
        else
            base_hue = NormalizeHue(hue - family.canonical_offset);
        double scale = GlobalSaturationScale(recipe_);
        int authored = minmax(int(saturation / max(0.0001, scale) + 0.5), 0, 100);
        if(WhenAnchorChange)
            WhenAnchorChange(drag_family_, base_hue, custom_offset, authored, final_commit);
    }

    Mode mode_ = PASSIVE;
    Vector<DisplaySwatch_> passive_items_;
    Vector<int> passive_selected_;
    int active_index_ = -1;
    GeneratorRecipe recipe_;
    Vector<GeneratedSwatch> generated_;
    int active_family_ = 0;
    int drag_family_ = -1;
    Image cache_;
    uint64 cache_revision_ = 0;
};
