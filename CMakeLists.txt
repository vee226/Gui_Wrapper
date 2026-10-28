cmake_minimum_required(VERSION 4.2)
project(Plotter)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(plotter main.cpp graphs.cpp painter_private.cpp
)

if (MSVC)
    target_compile_options(plotter PRIVATE /EHsc /W4)
endif ()

find_package(Qt6 REQUIRED
        COMPONENTS Core Widgets GUI Graphs
)
target_link_libraries(plotter PRIVATE Qt6::Widgets Qt6::Gui Qt6::Graphs)