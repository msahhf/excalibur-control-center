cmake_minimum_required(VERSION 3.16)

project(excalibur-control-center
    VERSION 0.5.8
    DESCRIPTION "EXCALIBUR G870 Control Center (Qt6 Widgets)"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

# LinguistTools provides lupdate/lrelease and the qt_add_translations function
# (new build dependency, tracked by the plan's A1 step).
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui LinguistTools)

# Dev-only CLI hooks (--once, --screenshot, --shot-delay, --page, --size).
# OFF by default so a normal build ships no dev hooks. Developers and automated
# screenshots enable them with -DEXCALIBUR_DEV_HOOKS=ON.
option(EXCALIBUR_DEV_HOOKS "Enable dev-only CLI hooks" OFF)

# Unit tests for the pure logic (temperature conversion, settings parsing).
# OFF by default so a normal release build compiles only the application.
option(EXCALIBUR_BUILD_TESTS "Build the unit tests" OFF)

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
    src/util/sensorrange.h
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

# --- Translations (i18n) -----------------------------------------------------
# .ts sources are committed (gui/i18n/); lrelease compiles them to .qm and
# qt_add_translations embeds the .qm into the binary under :/i18n (the default
# RESOURCE_PREFIX), so no packaging path or install rule has to change.
#   cmake --build <build> --target update_translations   # refresh .ts from source
#   cmake --build <build> --target release_translations   # compile .qm only
# Only the Turkish catalog is produced; English is the source language (A5).
# The committed .ts declares language="tr_TR"; lupdate preserves that attribute
# across refreshes (do not pass -target-language, it makes lupdate warn).
#
# Portability: the leading positional argument is the target whose sources are
# scanned. Qt 6.4-6.6 only understand this positional form (they infer the
# sources from it); the SOURCE_TARGETS keyword used here previously was added in
# Qt 6.7, where the call below still works via automatic source collection.
# Both lines of Qt end up embedding the compiled .qm under :/i18n.
qt_add_translations(excalibur-control-center
    TS_FILES i18n/excalibur-control-center_tr.ts)

target_compile_options(excalibur-control-center PRIVATE -Wall -Wextra)

# Single source of truth for the product version (project VERSION above).
target_compile_definitions(excalibur-control-center PRIVATE EXCALIBUR_VERSION="${PROJECT_VERSION}")

# Build type surfaced in diagnostics (empty when CMAKE_BUILD_TYPE is unset).
target_compile_definitions(excalibur-control-center PRIVATE EXCALIBUR_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

if(EXCALIBUR_DEV_HOOKS)
    target_compile_definitions(excalibur-control-center PRIVATE EXCALIBUR_DEV_HOOKS)
endif()

if(EXCALIBUR_BUILD_TESTS)
    enable_testing()
    add_executable(excalibur-tests
        tests/tests.cpp
        src/util/units.h
        src/util/sensorrange.h
        src/app/appstate.cpp
        src/app/appstate.h
        src/app/usersettings.cpp
        src/app/usersettings.h
        src/theme/theme.cpp
        src/theme/theme.h)
    target_include_directories(excalibur-tests PRIVATE src)
    target_link_libraries(excalibur-tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    add_test(NAME units-and-settings COMMAND excalibur-tests)
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
