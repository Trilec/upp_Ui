#ifndef _Ui_UiShapePath_h_
#define _Ui_UiShapePath_h_

/*
    UiShapePath
    ===========

    Purpose
    - Provide one backend-neutral authored vector-path vocabulary for upp_Ui.
    - Keep control/stock-shape code out of tessellation details.
    - Flatten explicit geometry only through UiGeometry's final-device-pixel
      error contract.

    Contract
    - Coordinates are final device/screen pixels.
    - Commands express authored geometry; they do not carry colour, stroke,
      theme, cache or control state.
    - Adaptive flattening may change vertex count without changing path or
      interaction semantics.
    - Normal controls use UiShapes/UiShapePath when they need reusable authored
      silhouettes. Dense scenes may bypass this layer and use UiGeometry
      directly when command storage would be pure overhead.
*/

#include <Ui/UiGeometry.h>

namespace Upp {

enum class UiShapeCommandType : byte {
    MoveTo = 0,
    LineTo,
    QuadraticTo,
    CubicTo,
    Arc,
    EllipseArc,
    Close,
};

enum class UiShapeSide : byte {
    Left = 0,
    Top,
    Right,
    Bottom,
};

struct UiShapeCommand : Moveable<UiShapeCommand> {
    UiShapeCommandType type = UiShapeCommandType::MoveTo;
    Pointf p1;
    Pointf p2;
    Pointf p3;
    double radius_x = 0.0;
    double radius_y = 0.0;
    double start_angle = 0.0;
    double sweep_angle = 0.0;
};

struct UiShapeContour : Moveable<UiShapeContour> {
    Vector<Pointf> points;
    bool closed = false;
};

class UiShapePath : Moveable<UiShapePath> {
public:
    UiShapePath& Clear();

    UiShapePath& MoveTo(Pointf point);
    UiShapePath& LineTo(Pointf point);
    UiShapePath& QuadraticTo(Pointf control, Pointf end);
    UiShapePath& CubicTo(Pointf control1, Pointf control2, Pointf end);
    UiShapePath& Arc(Pointf center, double radius_px,
                     double start_angle, double sweep_angle);
    UiShapePath& EllipseArc(Pointf center, double radius_x_px, double radius_y_px,
                            double start_angle, double sweep_angle);
    UiShapePath& Close();

    bool IsEmpty() const                         { return commands_.IsEmpty(); }
    int GetCommandCount() const                  { return commands_.GetCount(); }
    const Vector<UiShapeCommand>& GetCommands() const { return commands_; }

    Vector<UiShapeContour> Flatten() const;
    Vector<Pointf> FlattenSingleContour() const;

private:
    Vector<UiShapeCommand> commands_;
};

} // namespace Upp

#endif
