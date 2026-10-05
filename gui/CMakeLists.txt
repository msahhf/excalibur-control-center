cmake_minimum_required(VERSION 3.16)

project(excalibur-control-center
    VERSION 0.1.0
    DESCRIPTION "EXCALIBUR G870 Control Center (Qt6 Widgets)"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(excalibur-control-center
    src/main.cpp
    src/hwmonclient.cpp
    src/hwmonclient.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/theme/theme.cpp
    src/theme/theme.h
    src/navigation/sidebar.cpp
    src/navigation/sidebar.h
    src/navigation/pagecontainer.cpp
    src/navigation/pagecontainer.h
    src/pages/dashboardpage.cpp
    src/pages/dashboardpage.h
    src/pages/coolingpage.cpp
    src/pages/coolingpage.h
    src/pages/devicepage.cpp
    src/pages/devicepage.h
    src/pages/aboutpage.cpp
    src/pages/aboutpage.h
    src/pages/pageutils.h
)

target_include_directories(excalibur-control-center PRIVATE src)

target_link_libraries(excalibur-control-center PRIVATE Qt6::Widgets)

target_compile_options(excalibur-control-center PRIVATE -Wall -Wextra)

install(TARGETS excalibur-control-center RUNTIME DESTINATION bin)
