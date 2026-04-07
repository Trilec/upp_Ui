#ifndef _Ui_UiIndicatorBase_h_
#define _Ui_UiIndicatorBase_h_

/*
    UiIndicatorBase
    ===============

    Purpose
    - Internal shared base for indicator-bearing controls such as UiCheckBox
      and UiRadioButton.

    Intent
    - Centralize common hover/press/focus/text/layout-cache behavior while
      keeping style resolution and control-specific semantics in derived
      controls.

    Thread context
    - GUI thread only.

    Usage
    - Derive narrow indicator controls from this class and keep actual paint,
      style, and value semantics in the derived type.

    Changelog
    - 2026-03-31: introduced as the narrow shared behavior base for checkbox
      and radio controls.
    - 2026-03-31: layout cache now keys on geometry/alignment inputs, not only
      the content rect.
*/

#include <CtrlCore/CtrlCore.h>
#include <Ui/UiIndicatorSupport.h>

namespace Upp {

class UiIndicatorBase : public Ctrl {
public:
    typedef UiIndicatorBase CLASSNAME;

    UiIndicatorBase();

protected:
    void InvalidateIndicatorCaches();
    void OnIndicatorStyleChanged(const Font& font);
    void SetIndicatorTextValue(const String& text, const Font& font);
    const String& GetIndicatorTextValue() const { return text_; }
    Size GetIndicatorTextSizeCached(const Font& font) const;

    void LayoutIndicatorBlocks(const StyledMetrics& metrics,
                               const StyledSkin& skin,
                               const Font& font,
                               Size support_natural,
                               UiAlign align_h,
                               UiAlign align_v,
                               UiAlign indicator_side,
                               int gap,
                               int min_support_side);
    Size GetIndicatorMinSize(const StyledMetrics& metrics,
                             const StyledSkin& skin,
                             const Font& font,
                             Size support_natural,
                             UiAlign indicator_side,
                             int gap,
                             int empty_w,
                             int empty_h,
                             int min_support_side) const;
    void SetIndicatorUserMinSize(Size sz);
    Size GetIndicatorUserMinSize() const { return user_min_size_; }
    const UiBlocksLayout& GetIndicatorLayoutCache() const { return layout_cache_; }

    StyledState GetIndicatorStyledState() const;
    bool IsIndicatorActivationKey(dword key) const;

    void BeginIndicatorPress();
    void EndIndicatorPress();
    void IndicatorGotFocus();
    void IndicatorLostFocus();
    void IndicatorMouseEnter();
    void IndicatorMouseLeave();
    void SetIndicatorHover(bool on);

private:
    void RebuildIndicatorLayoutCache(const Rect& content,
                                     const Font& font,
                                     Size support_natural,
                                     UiAlign align_h,
                                     UiAlign align_v,
                                     UiAlign indicator_side,
                                     int gap,
                                     int min_support_side) const;

private:
    String text_;
    bool hover_ = false;
    bool pressed_ = false;

    Size user_min_size_ = Size(0, 0);
    mutable bool text_size_dirty_ = true;
    mutable Size text_size_cache_ = Size(0, 0);
    mutable UiBlocksLayout layout_cache_;
    mutable Rect           layout_content_cache_ = Rect(0, 0, 0, 0);
    mutable Size           layout_support_cache_ = Size(0, 0);
    mutable UiAlign        layout_align_h_cache_ = UiAlign::LEFT;
    mutable UiAlign        layout_align_v_cache_ = UiAlign::CENTER;
    mutable UiAlign        layout_side_cache_ = UiAlign::LEFT;
    mutable int            layout_gap_cache_ = 0;
    mutable int            layout_min_support_cache_ = 0;
    mutable bool           layout_dirty_ = true;
};

}

#endif
