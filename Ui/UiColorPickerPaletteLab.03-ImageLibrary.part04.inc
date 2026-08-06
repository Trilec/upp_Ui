        bool duplicate = false;
        if(!allow_duplicates) {
            for(const UiColorPicker::SlotValue& existing : destination)
                duplicate = duplicate || (existing.color == value.color && existing.alpha == value.alpha);
            for(const UiColorPicker::SlotValue& existing : append)
                duplicate = duplicate || (existing.color == value.color && existing.alpha == value.alpha);
        }
        if(!duplicate)
            append.Add(value);
    }
    int overflow = max(0, destination.GetCount() + append.GetCount() - capacity);
    if(rejected)
        *rejected = overflow;
    if(overflow > 0)
        return false;
    for(const UiColorPicker::SlotValue& value : append)
        destination.Add(value);
    return true;
}

} // namespace UiColorPickerPaletteLab
} // namespace Upp
