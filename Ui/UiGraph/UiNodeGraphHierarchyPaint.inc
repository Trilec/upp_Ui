namespace Upp {

void UiNodeGraph::Paint(Draw& w)
{
    int64 started = usecs();
    Rect outer(Point(0, 0), GetSize());
    const Style& style = GetEffectiveStyle();
    UiPaintStyledBackground(w, outer, style.canvas_palette, style.canvas_metrics,
                            style.canvas_skin, ST_NORMAL, HasFocus());

    if(style.show_grid && style.grid_size > 0) {
        Rect grid_paint = w.GetPaintRect() & outer;
        const int hierarchy = max(2, style.major_grid_every);
        const double base_spacing = style.grid_size * zoom_;
        if(!grid_paint.IsEmpty() && base_spacing > 0.0) {
            Color paper = ResolveFace(style.canvas_palette.face[ST_NORMAL], SColorPaper());
            Color minor = BlendGraphColor(paper, style.grid_minor, 0.45);
            Color major = BlendGraphColor(paper, style.grid_major, 0.58);

            int level = 0;
            double anchor_spacing = base_spacing;
            while(anchor_spacing < 50.0 && level < 8) {
                anchor_spacing *= hierarchy;
                level++;
            }
            double minor_spacing = level == 0 ? base_spacing : anchor_spacing / hierarchy;
            double minor_factor = level == 0 ? 1.0 : SmoothUnit(minor_spacing, 8.0, 18.0);
            bool draw_minor = minor_factor > 0.02;
            if(level > 0)
                minor = BlendGraphColor(paper, minor, minor_factor);

            int minor_step = max(1, fround(minor_spacing));
            int major_step = minor_step * hierarchy;
            Image tile = GetGraphGridTile(minor_step, major_step, draw_minor, minor, major);
            Size ts = tile.GetSize();
            if(ts.cx > 0 && ts.cy > 0) {
                double phase_x = std::fmod(pan_.x, (double)major_step);
                double phase_y = std::fmod(pan_.y, (double)major_step);
                if(phase_x < 0) phase_x += major_step;
                if(phase_y < 0) phase_y += major_step;
                int start_x = outer.left + fround(phase_x);
                int start_y = outer.top + fround(phase_y);
                while(start_x > grid_paint.left) start_x -= ts.cx;
                while(start_y > grid_paint.top) start_y -= ts.cy;
                while(start_x + ts.cx <= grid_paint.left) start_x += ts.cx;
                while(start_y + ts.cy <= grid_paint.top) start_y += ts.cy;
                for(int y = start_y; y < grid_paint.bottom; y += ts.cy)
                    for(int x = start_x; x < grid_paint.right; x += ts.cx)
                        w.DrawImage(x, y, tile);
            }
        }

        if(style.show_origin) {
            Point origin = WorldToScreen(Pointf(0, 0));
            w.DrawLine(origin.x, outer.top, origin.x, outer.bottom, 1, Color(248, 113, 113));
            w.DrawLine(outer.left, origin.y, outer.right, origin.y, 1, Color(96, 165, 250));
        }
    }

    // Fixed H2 paint order: canvas/grid -> Backdrops -> edges/nodes -> overlays.
    PaintBackdrops(w);
    if(model_)
        PaintGraphGeometry(w);
    PaintMarquee(w);
    UiPaintStyledForeground(w, outer, style.canvas_palette, style.canvas_metrics,
                            style.canvas_skin, ST_NORMAL, HasFocus());
    last_paint_usecs_ = max<int64>(0, usecs() - started);
}

} // namespace Upp
