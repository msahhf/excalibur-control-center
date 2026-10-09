cmake_minimum_required(VERSION 3.16)

project(excalibur-control-center
    VERSION 0.5.2
    DESCRIPTION "EXCALIBUR G870 Control Center (Qt6 Widgets)"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Dev-only CLI hooks (--once, --screenshot, --shot-delay, --page, --size).
# OFF by default so a normal build ships no dev hooks. Developers and automated
# screenshots enable them with -DEXCALIBUR_DEV_HOOKS=ON.
option(EXCALIBUR_DEV_HOOKS "Enable dev-only CLI hooks" OFF)

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
    src/app/systeminfo.cpp
    src/app/systeminfo.h
    src/app/diagnostics.cpp
    src/app/diagnostics.h
    src/app/usersettings.cpp
    src/app/usersettings.h
    src/util/units.h
    src/app/version.h
    src/app/identity.h
    src/components/animatednumber.cpp
    src/components/animatednumber.h
    src/components/actionbutton.cpp
    src/components/actionbutton.h
    src/components/statuspill.cpp
    src/components/statuspill.h
    src/components/fanstatus.cpp
    src/components/fanstatus.h
    src/components/hardwaremodule.cpp
    src/components/hardwaremodule.h
    src/components/sparkline.cpp
    src/components/sparkline.h
    src/components/thermalpanel.cpp
    src/components/thermalpanel.h
    src/components/herostatus.cpp
    src/components/herostatus.h
    src/components/surfacepanel.cpp
    src/components/surfacepanel.h
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
    src/pages/diagnosticspage.cpp
    src/pages/diagnosticspage.h
    src/pages/aboutpage.cpp
    src/pages/aboutpage.h
    src/pages/settingspage.cpp
    src/pages/settingspage.h
    src/pages/pageutils.h
    assets/resources.qrc
)

target_include_directories(excalibur-control-center PRIVATE src)

target_link_libraries(excalibur-control-center PRIVATE Qt6::Widgets)

target_compile_options(excalibur-control-center PRIVATE -Wall -Wextra)

# Single source of truth for the product version (project VERSION above).
target_compile_definitions(excalibur-control-center PRIVATE EXCALIBUR_VERSION="${PROJECT_VERSION}")

# Build type surfaced in diagnostics (empty when CMAKE_BUILD_TYPE is unset).
target_compile_definitions(excalibur-control-center PRIVATE EXCALIBUR_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

if(EXCALIBUR_DEV_HOOKS)
    target_compile_definitions(excalibur-control-center PRIVATE EXCALIBUR_DEV_HOOKS)
endif()

# --- Desktop integration (packaging-ready, standard freedesktop layout) -------
# Executable/package name (excalibur-control-center) stays separate from the
# user-facing display name (EXCALIBUR Control Center) in the .desktop entry.
install(TARGETS excalibur-control-center RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/excalibur-control-center.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/excalibur-control-center.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(DIRECTORY assets/icons/hicolor/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/excalibur-control-center
        RENAME LICENSE)
