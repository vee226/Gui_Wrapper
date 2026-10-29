//
// Created by vwann on 10/22/2026.
//

#ifndef GRAPHS_H
#define GRAPHS_H

#include <QObject>
#include <algorithm>
#include <iostream>

#include "painter_private.h"
#include "custom_error.h"


//===========================================================================
//Series section declares class associated with shape information
//Series is designed to preserve the original value and is external-space agnostic
//i.e., outside of Painter (which is not space-agnostic),
//the class only contains coordinates in relation to itself
namespace shape {

    class Shape {
    public:
        virtual ~Shape() = default;
        //for a Series that allows add_point, also sets origin as a reference point.
        virtual void add_point(const Point& point) = 0;
        virtual void add_points(std::initializer_list<Point> points) = 0;

        void draw(const Point& reference, Painter& la_Peintresse);

        void set_title(const std::string& title) {name = title;}
        std::string title() const {return name;}
        Style& style() {return s;}
        const std::vector<Point>& points() const {return pts;}

        //Pen and brush overloads
        void set_style(QColor color, double width, Qt::PenStyle style = Qt::SolidLine, bool unexpandable = true);
        void set_style(QColor color, Qt::BrushStyle style = Qt::SolidPattern);


        virtual std::unique_ptr<Shape> convert_to_ABC() = 0;
    protected:
        Shape() = default;
        Shape(const std::string& title) : name{title} { }

        //reference point for drawing may or may not be pts[0]
        virtual void draw_specific(Painter& la_Peintresse) = 0;

        //container
        std::string name;
        std::vector<Point> pts;
        //render settings
        Style s;
    };


    //===========================================================================
    //Line
    class Line : public Shape {
    public:
        Line() = default;
        explicit Line(const std::string& title) : Shape{title} { }
        Line(std::initializer_list<Point> points);

        virtual std::unique_ptr<Shape> convert_to_ABC() override;

        //all things virtual
        void add_point(const Point& point) override;
        void add_points(std::initializer_list<Point> points) override;
        void delete_point(size_t i);
    private:
        void draw_specific(Painter& la_Peintresse) override;

        bool check();
    };

    //Polyline
    class Polyline : public Shape {
    public:
        Polyline() = default;
        explicit Polyline(const std::string& title): Shape{title} { }
        Polyline(std::initializer_list<Point> points);

        virtual std::unique_ptr<Shape> convert_to_ABC() override;

        //all things virtual
        void add_point(const Point &point) override {pts.push_back(point);}
        void add_points(std::initializer_list<Point> points) override;
    private:
        void draw_specific(Painter& la_Peintresse) override;
    };

    namespace plot_2D {
        //axis
        enum class axis{undefined, x_axis, y_axis};
        class Plane : public Line {
        public:
            enum type{x_axis, y_axis, xy_axis};
            Plane() = delete;
            explicit Plane(type=xy_axis);
            explicit Plane(const std::string& title, type=xy_axis);
            Plane(const Point& start, const Point& end, type=xy_axis);

        private:
            type t;
        };

        //Line_Graph
        class Line_Graph : public Polyline {
        public:
            Line_Graph() = default;
            explicit Line_Graph(const std::string& title, axis orientation = axis::x_axis);
            Line_Graph(std::initializer_list<Point> points, axis orientation = axis::x_axis);

            std::unique_ptr<Shape> convert_to_ABC() override;
            //add points does not mutate original data but y-axis will be inverted by default draw_specifics
            void add_point(const Point& point) override;
            void add_points(std::initializer_list<Point> points) override;
            void set_axis(axis axis);
            axis main_axis() {return main_ax;}

        private:
            axis main_ax = axis::x_axis;
            //draw_specific inverts y-axis
            void draw_specific(Painter& la_Peintresse) override;

            static constexpr int y_flip = -1;
        };

        //===========================================================================
        //Plot_View offer means to translate space agnostic series into viewable charts
        class Plot_View : public QWidget {
            Q_OBJECT
            //dummy data is stored here to improve efficiency
            struct points_repr {
                Point value;
                Point display_at;
                std::string str;
            };
            struct data_private {                           //movable, unique_ptr(unique_ptr) is allowed
                std::unique_ptr<Shape> series;
                Point ref_point;
                Point stretch{1,1};
                points_repr repr;
            };

        public:
            Plot_View() = default;
            Plot_View(const std::string& title) : name{title} { }

            void set_title(const std::string& title) {name = title;}
            std::string title() {return name;}
            void add_series(
                Shape&& series,
                const Point& reference_point = {0,0},  //reference points may or may not be pts[0]
                const Point& stretch_factor ={1,1});
            void add_point(size_t i, const Point& p);
            void add_points(size_t i, std::initializer_list<Point> points);

            void set_style(size_t i, QColor color, double width, Qt::PenStyle pen_style);
            void set_style(size_t i, QColor color, Qt::BrushStyle brush_style);

            size_t series_count() const {return collections.size();}
            void set_series_title(size_t i, const std::string &title);
            std::string series_title(size_t i);

            signals:
            void on_mouse_clicked(std::vector<std::string> messages);

        protected:
            void paintEvent(QPaintEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mousePressEvent(QMouseEvent *event) override;

        private:
            std::string name;
            Painter painter;

            Point mouse;
            std::vector<data_private> collections;
        };

        inline bool check_axis(const axis& axis) {
            return static_cast<int>(axis)==1||static_cast<int>(axis)==2;
        }
    }
}


//===========================================================================
//widgets
namespace widgets {
    class Message_Box : public QWidget {
        Q_OBJECT
    public:
        Message_Box() = default;
        Message_Box(Point offset) : offst{offset} { }

        void set_title(const std::string& title) {name = title;}
        std::string title() const {return name;}

        void set_h_spacing(double spacing) {horizontal_spacing = spacing;}
        void set_v_spacing(double spacing) {vertical_spacing = spacing;}
        double h_spacing() const {return horizontal_spacing;}
        double v_spacing() const {return vertical_spacing;}
        void sett_offset(Point offset) {offst = offset;}
        Point offset() const {return {offst};}

        // void add_message(const std::string& message);
        //add messages in paket for now, while AWAITING Text class
        void add_messages(const std::vector<std::string>& messages);

        //slot
        void add_and_update(const std::vector<std::string>& messages);
    protected:
        void paintEvent(QPaintEvent* event) override;
        // void mousePressEvent(QMouseEvent* event) override;

    private:
        std::string name;
        //PATCHWORK... IMPLEMENT class Text later
        std::vector<std::vector<std::string>> collections;
        Point offst{15,15};
        double horizontal_spacing = 15;
        double vertical_spacing = 15;

        void clear();
    };
}

#endif //GRAPHS_H
