#include <QTextEdit>

#include <QPushButton>
#include <QScrollbar>

#include "bennett_parser.h"
#include "graphs.h"
#include <QApplication>
#include <QMainWindow>
#include <QLayout>
#include <QMessageBox>

#include <fstream>
#include <iostream>

const std::string filename = "ventilator_data (2).txt";

int main(int argc, char* argv[]) {
    std::string input = "ventilator_data (2).txt";
    std::string path = R"(C:\Users\vwann\OneDrive\Desktop\Ventilator Data\)";

    QApplication app(argc, argv);
    using namespace shape;
    using namespace widgets;

    //set up data
    Line_Graph flow{"flow"};
    flow.set_style(Qt::black, 2);
    Line_Graph pressure{"pressure"};
    pressure.set_style(Qt::black, 2);
    Line_Graph breath{"breath_number"};
    std::ifstream ifs{path+input, std::ifstream::in|std::ifstream::binary};
    if (!ifs) {
        return 1;
    }
    for (double i = 0; ifs.good(); ) {
        Bennett::Data data;

        ifs>>data;
        if (data.type()==Bennett::Data::flow_pressure) {
            flow.add_point({i, data.flow()});
            pressure.add_point({i, data.pressure()});
            i += Bennett::interval;
        }
        if (data.type()==Bennett::Data::breath_no) {
            breath.add_point({i, static_cast<double>(data.breath())});
        }
    }

    //set up widget
    const Point flow_stretch{50,1};
    const Point pressure_stretch{50,3};
    Plot_View plot("Ventilator Plot");
    plot.setMinimumSize(400,600);
    QPalette palette;
    palette.setColor(QPalette::Window, Qt::white);
    plot.setPalette(palette);
    plot.setAutoFillBackground(true);
    plot.add_series(std::move(flow), {0, 200}, flow_stretch);
    plot.add_series(std::move(pressure), {0, 150}, pressure_stretch);

    //message box
    Message_Box message_box{};
    message_box.setMaximumSize(200,400);
    QObject::connect(&plot, &Plot_View::on_mouse_clicked, &message_box, &Message_Box::add_and_update);

    //layout
    QMainWindow* main_window = new QMainWindow();
    main_window->resize(800,600);
    QWidget *central_widget = new QWidget();
    main_window->setCentralWidget(central_widget);

    QGridLayout* layout = new QGridLayout(central_widget);
    layout->setColumnMinimumWidth(1, 10);
    layout->addWidget(&plot, 1, 2, 3, 1);
    plot.setMouseTracking(true);
    layout->addWidget(&message_box, 1, 0, 2, 1);

    main_window->show();
    return app.exec();
}


