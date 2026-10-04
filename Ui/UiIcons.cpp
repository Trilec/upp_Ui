#include "UiIcons.h"
#include <Painter/Painter.h>

namespace Upp {

#define IMAGECLASS UiIconsImg
#define IMAGEFILE <Ui/UiIcons.iml>
#include <Draw/iml_source.h>

// Shared catalogue factories; cached monochrome masks, tinted by Ui controls.
static Image UiMediaIcon_(int kind)
{
    ImagePainter painter(Size(48, 48), MODE_ANTIALIASED);
    painter.Clear(RGBAZero());
    auto triangle = [&](double x, bool reverse) {
        painter.Move(reverse ? x + 16 : x, 10).Line(reverse ? x : x + 16, 24)
               .Line(reverse ? x + 16 : x, 38).Close().Fill(Black());
    };
    switch(kind) {
    case 0: triangle(19, true); painter.Rectangle(10,10,4,28).Fill(Black()); break;
    case 1: triangle(19, true); painter.Rectangle(10,16,4,16).Fill(Black()); break;
    case 2: triangle(14, true); break;
    case 3: painter.Rectangle(13,10,7,28).Fill(Black()); painter.Rectangle(28,10,7,28).Fill(Black()); break;
    case 4: triangle(18, false); break;
    case 5: triangle(13, false); painter.Rectangle(34,16,4,16).Fill(Black()); break;
    case 6: triangle(13, false); painter.Rectangle(34,10,4,28).Fill(Black()); break;
    case 7: painter.Circle(24,24,10).Fill(Black()); break;
    case 8: painter.Rectangle(10,10,28,28).Stroke(5,Black()); break;
    }
    return painter;
}
Image ICON_MEDIA_FIRST_48() { static Image image = UiMediaIcon_(0); return image; }
Image ICON_MEDIA_STEP_BACK_48() { static Image image = UiMediaIcon_(1); return image; }
Image ICON_MEDIA_REVERSE_48() { static Image image = UiMediaIcon_(2); return image; }
Image ICON_MEDIA_PAUSE_48() { static Image image = UiMediaIcon_(3); return image; }
Image ICON_MEDIA_PLAY_48() { static Image image = UiMediaIcon_(4); return image; }
Image ICON_MEDIA_STEP_FORWARD_48() { static Image image = UiMediaIcon_(5); return image; }
Image ICON_MEDIA_LAST_48() { static Image image = UiMediaIcon_(6); return image; }
Image ICON_SAMPLE_POINT_48() { static Image image = UiMediaIcon_(7); return image; }
Image ICON_SAMPLE_AREA_48() { static Image image = UiMediaIcon_(8); return image; }

}
