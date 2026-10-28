//
// Created by vwann on 10/25/2026.
//

#ifndef PAINTER_PRIVATE_H
#define PAINTER_PRIVATE_H

//private painter means that this class should never be constructed or accessed by user.

#include <QWidget>
#include <QPainter>
#include <QPointF>

namespace shape {
    class Shape;
}


struct Point : QPointF{
    Point() = default;
    Point(double x, double y) : QPointF(x, y) {}
    Point(const Point& other) = default;
    Point& operator=(const Point& other) = default;

    Point& operator+=(const Point& rhs);
    Point& operator-=(const Point& rhs);
};
Point operator+(const Point& lhs, const Point& rhs);
Point operator-(const Point& lhs, const Point& rhs);

struct Style {
    Style();
    Style(const QColor& color, const double& width, const Qt::PenStyle& style, bool unexpandable = true);
    Style(const QColor& color, const Qt::BrushStyle& style);
    QPen pen;
    QBrush brush;
};


//style and connections to QPen and Brush belongs to shape
//painter is style agnostic.
struct Painter : QPen, QBrush {
    Painter() = default;

    Point get_offset() {return offset;}
    void set_offset(const Point& distance) {offset = distance;}
    Point get_stretch() {return stretch;}
    void set_stretch(const Point& stretch_factor) {stretch = stretch_factor;}

    //do not call these functions from outside Widget class to prevent double delete and invalid pointer
    //Widget will save(), restore() and clear() during the draw process.
    QPainter* get_impl() {return impl;}
    void save();
    void restore();
    void clear();
    void draw(shape::Shape& shape, const Point& reference_point, QPainter* impl_painter);
    void set(const Style& style);

    //redraw allows style state to persist
    void redraw(shape::Shape& shape, const Point& reference_point, QPainter* impl_painter);
    //pen and brush are public for easy access through Qt mechanism
    //REMEMBER TO ENCAPSULATE LATER <<<=============================================================
    QPen pen{Qt::black, 2, Qt::SolidLine};
    QBrush brush{};

private:
    Point offset;
    Point stretch;
    QPainter* impl = nullptr;
};


//===========================================================================
class Widget : public QWidget {
    Q_OBJECT
public:
    Widget() = default;
    ~Widget();

private:
    QWidget* impl;
};

#endif //PAINTER_PRIVATE_H
