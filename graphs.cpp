//
// Created by vwann on 10/22/2026.
//

#include "graphs.h"
#include "widgets.h"
#include "cassert"

#ifndef BENNETT_PARSER_H
#define BENNETT_PARSER_H
namespace Bennett {
    constexpr double interval = 0.02;
}
#endif

static constexpr int y_flip = -1;

//===========================================================================
//namespace shape
//ABC
void shape::Shape::set_style(QColor color, double width, Qt::PenStyle style, bool unexpandable) {
    s.pen.setColor(color);
    s.pen.setWidthF(width);
    s.pen.setStyle(style);
    s.pen.setCosmetic(unexpandable);
}
void shape::Shape::set_style(QColor color, Qt::BrushStyle style) {
    s.brush.setColor(color);
    s.brush.setStyle(style);
}

void shape::Shape::draw(const Point& reference, Painter& la_Peintresse) {
    la_Peintresse.set_offset(reference - pts[0]);
    la_Peintresse.get_impl()->translate(la_Peintresse.get_offset());
    draw_specific(la_Peintresse);
}

//===========================================================================
//polyline converts back to ABC with implicit type conversion
//              constexpr   unique_ptr<T> make_unique( Args&&... args );    (since c++23)
//                          unique_ptr( unique_ptr&& u ) noexcept;          (since c++23)
std::unique_ptr<shape::Shape> shape::Polyline::convert_to_ABC() {
    return std::make_unique<shape::Polyline>(*this);
}

shape::Polyline::Polyline(std::initializer_list<Point> points) : Shape{ } {
    for (const auto& p: points) this->pts.push_back(p);
}

void shape::Polyline::add_points(std::initializer_list<Point> points) {
    for (auto& p: points) {
        pts.push_back(p);
    }
}

void shape::Polyline::draw_specific(Painter& la_Peintresse) {
    // numeric_limits<int>::max() is 32 bits, if size is exceeded, those naughty users deserve their error.
    la_Peintresse.get_impl()->drawPolyline(pts.data(), pts.size());
}

//===========================================================================
//linegraph inherited from polyline
std::unique_ptr<shape::Shape> shape::Line_Graph::convert_to_ABC() {
    return std::make_unique<shape::Line_Graph>(*this);
}

shape::Line_Graph::Line_Graph(std::initializer_list<Point> points) {
    for (const auto& p: points) {
        pts.push_back(p);
    }
    std::sort(pts.begin(), pts.end(),
        [](const Point& p1, const Point& p2) {return p1.x()<p2.x();});
}

void shape::Line_Graph::draw_specific(Painter& la_Peintresse) {
    std::sort(pts.begin(), pts.end(),
        [](const Point& a, const Point& b) {return a.x()<b.x();});
    la_Peintresse.get_impl()->scale(1, y_flip);
    la_Peintresse.get_impl()->drawPolyline(pts.data(), pts.size());
}


//===========================================================================
void shape::Plot_View::add_series(Shape&& series, const Point& reference_point, const Point& stretch_factor) {
    std::unique_ptr<Shape> s{series.convert_to_ABC()};
    points_repr repr;
    if (s->points().empty()) repr.str = "empty series";
    data_private d{std::move(s), reference_point, stretch_factor, repr};
    collections.push_back(std::move(d));
}

void shape::Plot_View::set_style(size_t i, QColor color, double width, Qt::PenStyle pen_style) {
    if (i >= collections.size()) throw std::out_of_range("Series index out of range");
    collections[i].series->set_style(color, width, pen_style);
}
void shape::Plot_View::set_style(size_t i, QColor color, Qt::BrushStyle brush_style) {
    if (i >= collections.size()) throw std::out_of_range("Series index out of range");
    collections[i].series->set_style(color, brush_style);
}

void shape::Plot_View::set_series_title(size_t i, const std::string &title) {
    if (i >= collections.size()) throw std::out_of_range("Series index out of range");
    collections[i].series->set_title(title);
}
std::string shape::Plot_View::series_title(size_t i) {
    if (i >= collections.size()) throw std::out_of_range("Series index out of range");
    return collections[i].series->title();
}

//repr functions are private and called mainly after draw() and assume sorted state.
static Point repr_y(const double& x, const std::unique_ptr<shape::Shape>& u_ptr, Point offset, Point stretch) {
    const auto& data = u_ptr->points();
    if (data.empty()) throw std::out_of_range("empty series");
    double t_x = (x-offset.x())/stretch.x();
    if (t_x < data.front().x()) {
        return *data.begin();
    }
    if (t_x > data.back().x()) return *data.end();

    size_t high = data.size();
    size_t low = 0;
    while (low < high) {
        size_t mid = (high+low)/2;
        if (data[mid].x() > t_x) high = mid;
        if (data[mid].x() < t_x) low = mid+1;
        if (data[mid].x() == t_x) return {t_x, data[mid].y()};
    }
    const Point& higher = data[low];
    const Point& lower = data[low-1];

    auto p = higher.x()-t_x < t_x-lower.x()? higher: lower;
    //transformed-back value (from position) must be within 0.01 of original 0.02 interval
    assert(abs(t_x-p.x())<Bennett::interval/2);
    return {higher.x()-t_x < t_x-lower.x()? higher: lower};
}

//---------------------------------------------------------------------------
//events handler
//plot_view paintEvent

void shape::Plot_View::paintEvent(QPaintEvent *event) {
    QPainter impl{this};

    //draw each series according to their own rules
    for (auto& data : collections) {
        painter.set_stretch(data.stretch);
        painter.draw(*data.series, data.ref_point, &impl);
        //place holder, to be refactored
        impl.setPen(Qt::black);
        impl.drawText(data.repr.display_at, QString::fromStdString(data.repr.str));
    }
}


//mouse position and click
void shape::Plot_View::mouseMoveEvent(QMouseEvent* event) {
    Point mouse_pos{event->position().x(), event->position().y()};
    std::vector<std::string> display_strs;
    for (data_private& data: collections) {
        if (!data.series->points().empty()) {
            data.repr.value = repr_y(mouse_pos.x(), data.series, data.ref_point-data.series->points()[0],data.stretch);

            //widget-level states
            data.repr.display_at = {mouse_pos.x(),
                (data.repr.value.y()*y_flip + data.ref_point.y() - data.series->points()[0].y())*data.stretch.y()};
            // std::cout<<(data.repr.value.y() + (data.ref_point.y() - data.series->points()[0].y()))*y_flip;
            data.repr.str = std::format("({:.3f}, {:.3f}", data.repr.value.x(), data.repr.value.y());
        }
    }
    update();
}
void shape::Plot_View::mousePressEvent(QMouseEvent *event) {
    std::vector<std::string> messages;
    if (event->button() == Qt::LeftButton) {
        for (const auto& data : collections) {
            messages.push_back(data.series->title()+
                std::format(": {:.2f}, time {:.2f}ms\n",data.repr.value.y(), data.repr.value.x())
                );
        }
        emit on_mouse_clicked(messages);
    }
}

//===========================================================================

void widgets::Message_Box::add_messages(const std::vector<std::string>& messages) {
    collections.push_back(messages);
}
void widgets::Message_Box::add_and_update(const std::vector<std::string>& messages) {
    add_messages(messages);
    update();
}

void widgets::Message_Box::clear() {
    collections.clear();
}

void widgets::Message_Box::paintEvent(QPaintEvent *event) {
    QPainter impl{this};
    impl.setPen(QPen(Qt::white));
    Point spacing = offst + Point{horizontal_spacing, vertical_spacing};
    for (const auto& data: collections) {
        spacing += {0, vertical_spacing};
        for (const auto& msg: data) {
            spacing += {0, vertical_spacing};
            impl.drawText(spacing, QString::fromStdString(msg));
        }
    }
}