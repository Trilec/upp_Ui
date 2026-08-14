TITLE("UiGallery")
TOPIC_TEXT(
"UiGallery is the model-backed, fluid wrapping item view for large visual collections. "
"It uses uniform cell geometry so scrolling, hit testing and painting remain bounded by "
"the viewport rather than total model size. Bind UiListModel with SetModel(); the built-in "
"vertical UiItemRenderImage works by default, while SetItemRender() swaps the presentation "
"without changing model or selection ownership. Configure SetItemSize(), SetGap(), SetInset() "
"and SetOverscanRows(), and use WhenVisibleRange for lazy asset preparation outside Paint(). "
"The Gallery recycles only visible/overscan UiItemRender instances. SetZoom()/ZoomBy() change "
"uniform tile scale; on Windows Ctrl+wheel invokes Gallery zoom. Multi-selection supports "
"background marquee selection with Ctrl/Shift modifiers and Escape cancellation."
)
