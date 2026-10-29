//
// Created by vwann on 10/22/2026.
//

#include "graphs.h"
#include "widgets.h"
#include "cassert"

#include "custom_error.h"

#include <algorithm>

#ifndef BENNETT_PARSER_H
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
//derived of class shape
//line
shape::Line::Line(std::initializer_list<Point> points) : Shape{} {
    if (points.size() > 2) throw custom_error(std::format("Line class: failed initialization with {} points", points.size()));

    for (const Point& p: points) {
        pts.push_back(p);
    }
}

//line::virtuals
//MEMO      constexpr   unique_ptr<T> make_unique( Args&&... args );    (since c++23)
//          unique_ptr( unique_ptr&& u ) noexcept;                      (since c++23)
std::unique_ptr<shape::Shape> shape::Line::convert_to_ABC() {
    return std::make_unique<Line>(*this);
}
void shape::Line::add_point(const Point& point) {
    if (!check()) {
        throw custom_error(std::format("Line class: failed add_point: Line already contains {} points", pts.size()));
    }
    pts.push_back(point);
}
void shape::Line::add_points(std::initializer_list<Point> points) {
    for (const Point& p: points) {
        if (check()) pts.push_back(p);
        else throw custom_error(std::format("Line class: failed add_point: Line already contains {} points", pts.size()));
    }
}
void shape::Line::draw_specific(Painter& la_Peintresse) {
    la_Peintresse.get_impl()->drawLine(pts[1], pts[2]);
}

void shape::Line::delete_point(size_t i) {
    if (i > pts.size()) {
        throw out_of_range(std::format("Line class: failed delete_point: {} exceeds total {} points stored", i, pts.size()));
    }
    pts.erase(pts.begin()+i);
}
bool shape::Line::check() {
    return pts.size() < 2;
}


//polyline
shape::Polyline::Polyline(std::initializer_list<Point> points) : Shape{ } {
    for (const auto& p: points) this->pts.push_back(p);
}

//polyline::virtuals
std::unique_ptr<shape::Shape> shape::Polyline::convert_to_ABC() {
    return std::make_unique<Polyline>(*this);
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
//line graph inherited from polyline
std::unique_ptr<shape::Shape> shape::plot_2D::Line_Graph::convert_to_ABC() {
    return std::make_unique<Line_Graph>(*this);
}

shape::plot_2D::Line_Graph::Line_Graph(const std::string& title, axis orientation): Polyline{title} {
    using namespace shape::plot_2D;
    if (!check_axis(orientation)) {
        throw custom_error("Line_Graph: failed initialization with invalid axis setup");
    }
}
shape::plot_2D::Line_Graph::Line_Graph(std::initializer_list<Point> points, axis orientation) : Polyline{points} {
    main_ax = orientation;
    if (!check_axis(orientation)) {
        throw custom_error("Line_Graph: failed initialization with invalid axis setup");
    }

    auto key = [this](const Point& p1, const Point& p2)
    {return main_ax==axis::x_axis? p1.x()<p2.x(): p1.y()<p2.y();};
    if (!std::is_sorted(pts.begin(), pts.end(), key)) {
        std::sort(pts.begin(), pts.end(), key);
    }
}

void shape::plot_2D::Line_Graph::draw_specific(Painter& la_Peintresse) {
    la_Peintresse.get_impl()->scale(1, y_flip);
    la_Peintresse.get_impl()->drawPolyline(pts.data(), pts.size());
}
void shape::plot_2D::Line_Graph::add_point(const Point& point) {
    //middle insertion with bisection search:
    //          O(n) = n + log(n)
    if (pts.empty()) {
        pts.push_back(point); return;
    }

    auto key = [this](const Point& p) {return main_ax==axis::x_axis? p.x() : p.y();};
    double val = key(point);
    if (val > key(pts.back())) {
        pts.push_back(point); return;
    }
    if (val < key(pts.front())) {
        pts.insert(pts.begin(), point); return;
    }
    //ForwardIt upper_bound( ForwardIt first, ForwardIt last, const T& value, Compare comp );
    //returns first elements which is ordered after value (i.e., at e > value)
    //          default:    bool(value < e)
    //          comp:       bool(comp(value, e))
    auto iter = std::upper_bound(pts.begin(), pts.end(), val,
        [&key](double val, const Point& p){return val<key(p);});

    pts.insert(iter, point);        //arbitrary plot for (x1,y1), (x1, y2)
}

void shape::plot_2D::Line_Graph::add_points(std::initializer_list<Point> points) {
    if (points.empty()) return;

    //batch insertion then sort
    //          O = k + (k+n)log(k+n)

    //check for sorted k first then merge and sort
    //Back insertion
    //          O = 2k + (k+n)(log(k+n)
    //Front insertion
    //          O = (k+n)(1+log(k+n)) + k
    //for n in 100000s range and k in 5000s range with front insertion,
    //checking adds little overhead compared to 100000 (105000 vs 1785000)
    auto key = [this](const Point& point) {return main_ax==axis::x_axis? point.x(): point.y();};
    auto less = [&key](const Point& p1, const Point& p2) {return key(p1)<key(p2);};

    bool batch_is_sorted = std::is_sorted(points.begin(), points.end(), less);

    if (batch_is_sorted && !pts.empty()) {
        const Point& batch_front = *points.begin(), batch_back = *(points.end()-1);

        //if batch_back < pts.front(), front insertion O = k + n
        //back insertion with merge_sort = k + (k + n)
        if (less(batch_back, pts.front())) {
            pts.insert(pts.begin(), points); return;
        }

        size_t sf_start = pts.size();
        pts.insert(pts.end(), points);
        if (less(pts[sf_start-1], batch_front)) return;     //pts.back() < batch_front, all is good, return!
        //two sections are separately sorted but not all good (i.e., overlapped)
        //inplace_merge two consecutive sorted ranges: [first, middle) and [middle, last).
        //          O(merge_sort) = (n + k)
        std::inplace_merge(pts.begin(), pts.begin()+sf_start, pts.end(), less);
        return;
    }
    pts.insert(pts.end(), points);
    if (!batch_is_sorted) std::sort(pts.begin(), pts.end(), less);
}

void shape::plot_2D::Line_Graph::set_axis(axis axis) {
    if (!check_axis(axis)) throw custom_error("Line_Graph: failed set_axis: invalid axis");

    if (main_ax != axis) {
        auto key = [this](const Point& p1, const Point& p2)
        {return main_ax==axis::x_axis? p1.x()<p2.x(): p1.y()<p2.y();};
        std::sort(pts.begin(), pts.end(), key);
    }
}

//===========================================================================
void shape::plot_2D::Plot_View::add_series(Shape&& series, const Point& reference_point, const Point& stretch_factor) {
    std::unique_ptr<Shape> s{series.convert_to_ABC()};
    points_repr repr;
    if (s->points().empty()) repr.str = "empty series";
    data_private d{std::move(s), reference_point, stretch_factor, repr};
    collections.push_back(std::move(d));
}

void shape::plot_2D::Plot_View::set_style(size_t i, QColor color, double width, Qt::PenStyle pen_style) {
    if (i >= collections.size()) throw custom_error("Plot_View: failed set_style: index out of range");
    collections[i].series->set_style(color, width, pen_style);
}
void shape::plot_2D::Plot_View::set_style(size_t i, QColor color, Qt::BrushStyle brush_style) {
    if (i >= collections.size()) throw custom_error("Plot_View: failed set_style: index out of range");
    collections[i].series->set_style(color, brush_style);
}

void shape::plot_2D::Plot_View::set_series_title(size_t i, const std::string &title) {
    if (i >= collections.size()) throw custom_error("Plot_View: failed set_series_title: index out of range");
    collections[i].series->set_title(title);
}
std::string shape::plot_2D::Plot_View::series_title(size_t i) {
    if (i >= collections.size()) throw out_of_range("Plot_View: failed series_title: index out of range");
    return collections[i].series->title();
}

//repr functions are private and called mainly after draw() and assume sorted state.
static Point repr(const Point& index_point, const std::unique_ptr<shape::Shape>& u_ptr,
    const Point& offset, const Point& stretch
    )
{
    const auto& data = u_ptr->points();
    if (data.empty()) throw custom_error("Failed repr: cannot get a representation for empty Series\n");

    Point idx = index_point-offset;
    auto* l_ptr = dynamic_cast<shape::plot_2D::Line_Graph*>(u_ptr.get());
    if (l_ptr) {
        auto key = [&l_ptr](const Point& point) {
            return l_ptr->main_axis()==shape::plot_2D::axis::x_axis? point.x(): point.y();
        };
        auto val = key(idx)/key(stretch);
        //std::lower_bound returns first element which is NOT ordered before value
        //i.e., index at which !(e<v)
        //          default:    bool(e < value)
        //          comp:       bool((e, value))
        auto iter = std::lower_bound(l_ptr->points().begin(), l_ptr->points().end(), val,
            [&key](const Point& p1, double value) {return key(p1)<value;}
            );

        if (iter == l_ptr->points().begin()) return l_ptr->points().front();
        if (iter == l_ptr->points().end()) return l_ptr->points().back();
        Point p1 = *iter, p2 = *(iter-1);
        return key(p1)-val < val-key(p2)? p1: p2;
    }
    throw invalid_type("Failed repr: cannot get a representation for non Graph objects");
}

//---------------------------------------------------------------------------
//events handler
//plot_view paintEvent

void shape::plot_2D::Plot_View::paintEvent(QPaintEvent *event) {
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
void shape::plot_2D::Plot_View::mouseMoveEvent(QMouseEvent* event) {
    Point mouse_pos{event->position().x(), event->position().y()};
    std::vector<std::string> display_strs;
    for (data_private& data: collections) {
        if (!data.series->points().empty()) {
            data.repr.value = repr(mouse_pos, data.series, data.ref_point-data.series->points()[0],data.stretch);

            //widget-level states
            data.repr.display_at = {mouse_pos.x(),
                (data.repr.value.y()*y_flip + data.ref_point.y() - data.series->points()[0].y())*data.stretch.y()};
            data.repr.str = std::format("({:.3f}, {:.3f}", data.repr.value.x(), data.repr.value.y());
        }
    }
    update();
}
void shape::plot_2D::Plot_View::mousePressEvent(QMouseEvent *event) {
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


//===========================================================================