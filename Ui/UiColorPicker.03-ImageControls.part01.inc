
class ImageCanvas_ : public Ctrl {
public:
    typedef ImageCanvas_ CLASSNAME;

    ImageCanvas_() { WantFocus(); }

    void SetImage(const Image& image)
    {
        image_ = image;
        zoom_ = 1.0;
        pan_ = Point(0, 0);
        drop_hot_ = false;
        Refresh();
    }

    void SetResult(const ImageAnalysisResult& result, bool show_mask)
    {
        result_ = clone(result);
        show_mask_ = show_mask;
        Refresh();
    }

    void SetSeeds(const ImageExclusionSeed seed[2])
    {
        seed_[0] = seed[0];
        seed_[1] = seed[1];
        Refresh();
    }

    void SetManualPoints(const Vector<Pointf>& points)
    {
        manual_points_ = clone(points);
        Refresh();
    }

    void ArmSeed(int index)
    {
        armed_seed_ = index >= 0 && index < 2 ? index : -1;
        SetFocus();
        Refresh();
    }

    int GetArmedSeed() const { return armed_seed_; }

    void Fit()
    {
        zoom_ = 1.0;
        pan_ = Point(0, 0);
        Refresh();
    }

    void ZoomBy(double factor)
    {
        zoom_ = minmax(zoom_ * factor, 0.25, 8.0);
        Refresh();
    }

    int GetZoomPercent() const { return int(zoom_ * 100.0 + 0.5); }

    Event<> WhenLoadRequest;
    Event<Image> WhenImageDrop;
    Event<int, Pointf> WhenSeedPlaced;
    Event<int, Pointf, bool> WhenSeedMoved;
    Event<int, Pointf, bool> WhenManualMoved;

    virtual Size GetMinSize() const override { return Size(DPI(260), DPI(210)); }

    virtual void Paint(Draw& draw) override
    {
        Rect rect(Point(0, 0), GetSize());
        draw.DrawRect(rect, SurfaceColor_());
        DrawFrame_(draw, rect, FrameColor_());
        if(image_.IsEmpty()) {
            String text = "Click, drop or paste an image";
            Font font = SansSerif().Height(DPI(11));
            Size size = GetTextSize(text, font);
            draw.DrawText(rect.left + (rect.GetWidth() - size.cx) / 2,
                          rect.top + (rect.GetHeight() - size.cy) / 2,
                          text, font, SColorDisabled());
            if(drop_hot_)
                DrawFrame_(draw, rect.Deflated(DPI(2)), SColorHighlight(), DPI(2));
            return;
        }
        Rect image_rect = ImageRect();
        draw.Clip(rect.Deflated(DPI(1)));
        draw.DrawImage(image_rect.left, image_rect.top,
                       image_rect.GetWidth(), image_rect.GetHeight(), image_);
        if(show_mask_ && result_.exclusion_mask.GetCount() == result_.proxy_size.cx * result_.proxy_size.cy &&
           result_.proxy_size.cx > 0 && result_.proxy_size.cy > 0)
            DrawMask(draw, image_rect);
        DrawMarkers(draw, image_rect);
        draw.End();
        if(armed_seed_ >= 0)
            DrawFrame_(draw, rect.Deflated(DPI(2)), SColorHighlight(), DPI(2));
        else if(drop_hot_)
            DrawFrame_(draw, rect.Deflated(DPI(2)), SColorHighlight(), DPI(2));
    }

    virtual void LeftDown(Point point, dword) override
    {
        SetFocus();
        if(image_.IsEmpty()) {
            if(WhenLoadRequest)
                WhenLoadRequest();
            return;
        }
        Rect image_rect = ImageRect();
        if(!image_rect.Contains(point))
            return;
        if(armed_seed_ >= 0) {
            Pointf normalized = NormalizePoint(point, image_rect);
            if(WhenSeedPlaced)
                WhenSeedPlaced(armed_seed_, normalized);
            armed_seed_ = -1;
            Refresh();
            return;
        }
        drag_seed_ = HitSeed(point, image_rect);
        if(drag_seed_ >= 0) {
            SetCapture();
            MoveSeed(point, image_rect, false);
            return;
        }
        drag_manual_ = HitManual(point, image_rect);
        if(drag_manual_ >= 0) {
            SetCapture();
            MoveManual(point, image_rect, false);
            return;
        }
        panning_ = true;
        pan_anchor_ = point;
        SetCapture();
    }

    virtual void MouseMove(Point point, dword flags) override
    {
        if(!HasCapture() || !(flags & K_MOUSELEFT))
            return;
        Rect image_rect = ImageRect();
        if(drag_seed_ >= 0)
            MoveSeed(point, image_rect, false);
        else if(drag_manual_ >= 0)
            MoveManual(point, image_rect, false);
        else if(panning_) {
            pan_ += point - pan_anchor_;
            pan_anchor_ = point;
            Refresh();
        }
    }

    virtual void LeftUp(Point point, dword) override
    {
        if(!HasCapture())
            return;
        Rect image_rect = ImageRect();
        if(drag_seed_ >= 0)
            MoveSeed(point, image_rect, true);
        else if(drag_manual_ >= 0)
            MoveManual(point, image_rect, true);
        ReleaseCapture();
        drag_seed_ = -1;
        drag_manual_ = -1;
        panning_ = false;
    }

    virtual void MouseWheel(Point, int zdelta, dword) override
    {
        if(zdelta)
            ZoomBy(zdelta > 0 ? 1.15 : 1.0 / 1.15);
    }

    virtual void DragEnter() override
    {
        drop_hot_ = true;
        Refresh();
    }

    virtual void DragAndDrop(Point, PasteClip& clip) override
    {
        if(IsAvailableImage(clip)) {
            AcceptImage(clip);
            clip.SetAction(DND_COPY);
            if(clip.IsPaste()) {
                Image dropped = GetImage(clip);
                if(!dropped.IsEmpty() && WhenImageDrop)
                    WhenImageDrop(dropped);
                drop_hot_ = false;
                Refresh();
            }
            return;
        }
        if(IsAvailableFiles(clip)) {
            AcceptFiles(clip);
            if(clip.IsPaste()) {
                Vector<String> files = GetFiles(clip);
                if(!files.IsEmpty()) {
                    Image dropped = StreamRaster::LoadFileAny(files[0]);
                    if(!dropped.IsEmpty() && WhenImageDrop)
                        WhenImageDrop(dropped);
                }
                drop_hot_ = false;
                Refresh();
            }
            return;
        }
        clip.Reject();
        drop_hot_ = false;
        Refresh();
    }

    virtual void DragLeave() override
    {
        drop_hot_ = false;
        Refresh();
    }

private:
    Rect ImageRect() const
    {
        Rect rect(Point(0, 0), GetSize());
        Size source = image_.GetSize();
        if(source.IsEmpty())
            return Rect(0, 0, 0, 0);
        double scale = min(rect.GetWidth() / double(max(1, source.cx)),
                           rect.GetHeight() / double(max(1, source.cy))) * zoom_;
        Size target(max(1, int(source.cx * scale + 0.5)),
                    max(1, int(source.cy * scale + 0.5)));
        return RectC(rect.left + (rect.GetWidth() - target.cx) / 2 + pan_.x,
                     rect.top + (rect.GetHeight() - target.cy) / 2 + pan_.y,
                     target.cx, target.cy);
    }

    Pointf NormalizePoint(Point point, const Rect& image_rect) const
    {
        return Pointf(minmax((point.x - image_rect.left) / double(max(1, image_rect.GetWidth())), 0.0, 1.0),
                      minmax((point.y - image_rect.top) / double(max(1, image_rect.GetHeight())), 0.0, 1.0));
    }

    Point ScreenPoint(Pointf point, const Rect& image_rect) const
    {
        return Point(image_rect.left + int(point.x * image_rect.GetWidth() + 0.5),
                     image_rect.top + int(point.y * image_rect.GetHeight() + 0.5));
    }

    void DrawMask(Draw& draw, const Rect& image_rect)
    {
        Size proxy = result_.proxy_size;
        int stripe = max(2, DPI(4));
        for(int y = 0; y < proxy.cy; y += 2) {
            int sy0 = image_rect.top + y * image_rect.GetHeight() / proxy.cy;
            int sy1 = image_rect.top + min(proxy.cy, y + 2) * image_rect.GetHeight() / proxy.cy;
            for(int x = 0; x < proxy.cx; x += 2) {
                int index = y * proxy.cx + x;
                if(index < 0 || index >= result_.exclusion_mask.GetCount() || !result_.exclusion_mask[index])
                    continue;
                int sx0 = image_rect.left + x * image_rect.GetWidth() / proxy.cx;
                int sx1 = image_rect.left + min(proxy.cx, x + 2) * image_rect.GetWidth() / proxy.cx;
                Color mask = ((sx0 + sy0) / stripe) & 1 ? Color(36, 39, 44) : Color(72, 76, 84);
                draw.DrawRect(sx0, sy0, max(1, sx1 - sx0), max(1, sy1 - sy0), mask);
            }
        }
    }

    void DrawMarkers(Draw& draw, const Rect& image_rect)
    {
        for(int i = 0; i < 2; i++) {
            if(!seed_[i].enabled || !seed_[i].placed)
                continue;
            Point point = ScreenPoint(seed_[i].position, image_rect);
            int radius = DPI(9);
            Rect marker = RectC(point.x - radius, point.y - radius, radius * 2, radius * 2);
            draw.DrawEllipse(marker.Inflated(DPI(2)), SColorPaper(), DPI(1), SColorShadow());
            draw.DrawEllipse(marker, seed_[i].color, DPI(2), i == 0 ? Color(255, 220, 40) : Color(80, 190, 255));
            String text = i == 0 ? "A" : "B";
            Font font = SansSerif().Height(DPI(8)).Bold();
            Size size = GetTextSize(text, font);
            draw.DrawText(marker.left + (marker.GetWidth() - size.cx) / 2,