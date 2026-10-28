//
// Created by vwann on 10/25/2026.
//

#include "painter_private.h"
#include "graphs.h"

Point& Point::operator-=(const Point& rhs) {
    this->rx() -= rhs.x();
    this->ry() -= rhs.y();
    return *this;
}
Point& Point::operator+=(const Point& rhs) {
    this->rx() += rhs.x();
    this->ry() += rhs.y();
    return *this;
}
Point operator+(const Point& lhs, const Point& rhs) {
    return Point{lhs.x()+rhs.x(), lhs.y()+rhs.y()};
}
Point operator-(const Point& lhs, const Point& rhs) {
    return Point{lhs.x()-rhs.x(), lhs.y()-rhs.y()};
}


//===========================================================================
Style::Style() {
    pen.setCosmetic(true);
}

Style::Style(const QColor& color, const double& width, const Qt::PenStyle& style,
    bool unexpandable) : pen{color, width, style}
{
    pen.setCosmetic(unexpandable);
}
Style::Style(const QColor& color, const Qt::BrushStyle& style) : brush{color, style} { }


//===========================================================================
//painter

void Painter::set(const Style& style) {
    impl->setPen(style.pen);
    impl->setBrush(style.brush);
    impl->setRenderHint(QPainter::Antialiasing);
    impl->translate(offset);
    impl->scale(stretch.x(), stretch.y());
}
void Painter::save() {impl->save();}
void Painter::restore() {impl->restore();}
void Painter::clear() {
    offset = {0,0};
    stretch = {1,1};
    impl = nullptr;
}

void Painter::draw(shape::Shape& shape, const Point& reference_point, QPainter* impl_painter) {
    impl = impl_painter;
    save();
    set(shape.style());
    shape.draw(reference_point, *this);
    restore();
    clear();
}

void Painter::redraw(shape::Shape& shape, const Point& reference_point, QPainter* impl_painter) {
    impl = impl_painter;
    save();
    set(shape.style());
    shape.draw(reference_point, *this);
    restore();
}


//===========================================================================
Widget::~Widget() {
    //to be implemented, onion style
};