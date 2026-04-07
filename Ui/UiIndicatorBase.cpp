#include <Ui/UiIndicatorBase.h>

namespace Upp {

UiIndicatorBase::UiIndicatorBase()
{
    BackPaint();
    WantFocus();
}

void UiIndicatorBase::InvalidateIndicatorCaches()
{
    text_size_dirty_ = true;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
    layout_support_cache_ = Size(0, 0);
}

void UiIndicatorBase::OnIndicatorStyleChanged(const Font& font)
{
    text_size_cache_ = GetTextSize(text_, font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
}

void UiIndicatorBase::SetIndicatorTextValue(const String& text, const Font& font)
{
    if(text_ == text)
        return;

    text_ = text;
    text_size_cache_ = GetTextSize(text_, font);
    text_size_dirty_ = false;
    layout_dirty_ = true;
    layout_content_cache_ = Rect(0, 0, 0, 0);
    RefreshLayout();
    Refresh();
}

Size UiIndicatorBase::GetIndicatorTextSizeCached(const Font& font) const
{
    if(text_size_dirty_) {
        text_size_cache_ = GetTextSize(text_, font);
        text_size_dirty_ = false;
    }
    return text_size_cache_;
}

void UiIndicatorBase::RebuildIndicatorLayoutCache(const Rect& content,
                                                  const Font& font,
                                                  Size support_natural,
                                                  UiAlign align_h,
                                                  UiAlign align_v,
                                                  UiAlign indicator_side,
                                                  int gap,
                                                  int min_support_side) const
{
    if(!layout_dirty_
       && layout_content_cache_ == content
       && layout_support_cache_ == support_natural
       && layout_align_h_cache_ == align_h
       && layout_align_v_cache_ == align_v
       && layout_side_cache_ == indicator_side
       && layout_gap_cache_ == gap
       && layout_min_support_cache_ == min_support_side)
        return;

    layout_cache_ = UiComputeIndicatorBlocksLayout(content,
                                                   support_natural,
                                                   GetIndicatorTextSizeCached(font),
                                                   align_h,
                                                   align_v,
                                                   indicator_side,
                                                   gap,
                                                   min_support_side);
    layout_content_cache_ = content;
    layout_support_cache_ = support_natural;
    layout_align_h_cache_ = align_h;
    layout_align_v_cache_ = align_v;
    layout_side_cache_ = indicator_side;
    layout_gap_cache_ = gap;
    layout_min_support_cache_ = min_support_side;
    layout_dirty_ = false;
}

void UiIndicatorBase::LayoutIndicatorBlocks(const StyledMetrics& metrics,
                                            const StyledSkin& skin,
                                            const Font& font,
                                            Size support_natural,
                                            UiAlign align_h,
                                            UiAlign align_v,
                                            UiAlign indicator_side,
                                            int gap,
                                            int min_support_side)
{
    Rect content = UiStyledInnerRect(GetSize(), metrics, skin);
    RebuildIndicatorLayoutCache(content,
                                font,
                                support_natural,
                                align_h,
                                align_v,
                                indicator_side,
                                gap,
                                min_support_side);
}

Size UiIndicatorBase::GetIndicatorMinSize(const StyledMetrics& metrics,
                                          const StyledSkin& skin,
                                          const Font& font,
                                          Size support_natural,
                                          UiAlign indicator_side,
                                          int gap,
                                          int empty_w,
                                          int empty_h,
                                          int min_support_side) const
{
    if(user_min_size_.cx > 0 && user_min_size_.cy > 0)
        return user_min_size_;

    Size main_natural = GetIndicatorTextSizeCached(font);
    Size content = UiMeasureIndicatorBlocksContent(support_natural,
                                                   main_natural,
                                                   indicator_side,
                                                   gap,
                                                   empty_w,
                                                   empty_h,
                                                   min_support_side,
                                                   !text_.IsEmpty());
    return UiStyledOuterSizeFromContent(content, metrics, skin);
}

void UiIndicatorBase::SetIndicatorUserMinSize(Size sz)
{
    user_min_size_ = Size(max(0, sz.cx), max(0, sz.cy));
    layout_dirty_ = true;
    RefreshLayout();
}

StyledState UiIndicatorBase::GetIndicatorStyledState() const
{
    return UiIndicatorStyledState(IsEnabled() && IsShowEnabled(), pressed_, hover_);
}

bool UiIndicatorBase::IsIndicatorActivationKey(dword key) const
{
    return key == K_SPACE || key == K_ENTER;
}

void UiIndicatorBase::BeginIndicatorPress()
{
    if(!pressed_) {
        pressed_ = true;
        Refresh();
    }
}

void UiIndicatorBase::EndIndicatorPress()
{
    if(pressed_) {
        pressed_ = false;
        Refresh();
    }
}

void UiIndicatorBase::IndicatorGotFocus()
{
    Refresh();
}

void UiIndicatorBase::IndicatorLostFocus()
{
    Refresh();
}

void UiIndicatorBase::IndicatorMouseEnter()
{
    SetIndicatorHover(true);
}

void UiIndicatorBase::IndicatorMouseLeave()
{
    SetIndicatorHover(false);
}

void UiIndicatorBase::SetIndicatorHover(bool on)
{
    if(hover_ == on)
        return;
    hover_ = on;
    Refresh();
}

}
