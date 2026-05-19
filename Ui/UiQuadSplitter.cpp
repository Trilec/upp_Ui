#include <Ui/UiQuadSplitter.h>

namespace Upp {

UiQuadSplitter::UiQuadSplitter()
{
    Add(root_.SizePos());
    root_.Vert(top_, bottom_);
    top_.Horz();
    bottom_.Horz();
    root_.SetSplitPercent(row_percent_);
    top_.SetSplitPercent(column_percent_);
    bottom_.SetSplitPercent(column_percent_);
    Wire();
}

void UiQuadSplitter::Wire()
{
    root_.WhenSplitFinish = [=] { SyncRowFromRoot(); };
    top_.WhenSplitFinish = [=] { SyncColumnFromTop(); };
    bottom_.WhenSplitFinish = [=] { SyncColumnFromBottom(); };
}

void UiQuadSplitter::SyncColumnFromTop()
{
    column_percent_ = top_.GetSplitPercent();
    bottom_.SetSplitPercent(column_percent_);
    WhenSplitFinish();
}

void UiQuadSplitter::SyncColumnFromBottom()
{
    column_percent_ = bottom_.GetSplitPercent();
    top_.SetSplitPercent(column_percent_);
    WhenSplitFinish();
}

void UiQuadSplitter::SyncRowFromRoot()
{
    row_percent_ = root_.GetSplitPercent();
    WhenSplitFinish();
}

UiQuadSplitter& UiQuadSplitter::Set(Ctrl& top_left, Ctrl& top_right, Ctrl& bottom_left, Ctrl& bottom_right)
{
    Clear();
    top_.Horz(top_left, top_right);
    bottom_.Horz(bottom_left, bottom_right);
    panes_[0] = &top_left;
    panes_[1] = &top_right;
    panes_[2] = &bottom_left;
    panes_[3] = &bottom_right;
    top_.SetSplitPercent(column_percent_);
    bottom_.SetSplitPercent(column_percent_);
    root_.SetSplitPercent(row_percent_);
    Layout();
    Refresh();
    return *this;
}

void UiQuadSplitter::Add(Ctrl& pane)
{
    for(int i = 0; i < 4; i++) {
        if(!panes_[i]) {
            panes_[i] = &pane;
            if(i < 2)
                top_.Add(pane);
            else
                bottom_.Add(pane);
            top_.SetSplitPercent(column_percent_);
            bottom_.SetSplitPercent(column_percent_);
            root_.SetSplitPercent(row_percent_);
            Layout();
            Refresh();
            return;
        }
    }
}

void UiQuadSplitter::Clear()
{
    top_.Clear();
    bottom_.Clear();
    for(int i = 0; i < 4; i++)
        panes_[i] = nullptr;
}

UiQuadSplitter& UiQuadSplitter::SetColumnSplitPercent(double percent)
{
    column_percent_ = minmax(percent, 0.0, 100.0);
    top_.SetSplitPercent(column_percent_);
    bottom_.SetSplitPercent(column_percent_);
    return *this;
}

UiQuadSplitter& UiQuadSplitter::SetRowSplitPercent(double percent)
{
    row_percent_ = minmax(percent, 0.0, 100.0);
    root_.SetSplitPercent(row_percent_);
    return *this;
}

UiQuadSplitter& UiQuadSplitter::SetMinPixels(int pane, int px)
{
    switch(pane) {
    case 0: top_.SetMinPixels(0, px); break;
    case 1: top_.SetMinPixels(1, px); break;
    case 2: bottom_.SetMinPixels(0, px); break;
    case 3: bottom_.SetMinPixels(1, px); break;
    default: break;
    }
    return *this;
}

UiQuadSplitter& UiQuadSplitter::SetSplitterStyle(const UiSplitter::Style& style)
{
    root_.SetCustomStyle(style);
    top_.SetCustomStyle(style);
    bottom_.SetCustomStyle(style);
    return *this;
}

UiQuadSplitter& UiQuadSplitter::ClearSplitterStyle()
{
    root_.ClearCustomStyle();
    top_.ClearCustomStyle();
    bottom_.ClearCustomStyle();
    return *this;
}

void UiQuadSplitter::Layout()
{
    root_.SetRect(GetSize());
}

Size UiQuadSplitter::GetMinSize() const
{
    return GetContentSize();
}

Size UiQuadSplitter::GetContentSize() const
{
    Size top = top_.GetContentSize();
    Size bottom = bottom_.GetContentSize();
    int split = root_.GetSplitWidth();
    return Size(max(top.cx, bottom.cx), top.cy + bottom.cy + split);
}

} // namespace Upp
