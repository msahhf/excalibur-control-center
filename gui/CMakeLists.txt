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
    src/app/appstate.cpp
    src/app/appstate.h
    src/app/telemetrymodel.cpp
    src/app/telemetrymodel.h
    src/components/animatednumber.cpp
    src/components/animatednumber.h
    src/components/statuspill.cpp
    src/components/statuspill.h
    src/components/fanstatus.cpp
    src/components/fanstatus.h
    src/components/hardwaremodule.cpp
    src/components/hardwaremodule.h
    src/components/herostatus.cpp
    src/components/herostatus.h
    src/components/emptystate.cpp
    src/components/emptystate.h
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
