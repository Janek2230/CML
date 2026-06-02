cmake_minimum_required(VERSION 3.19)
project(CML LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Charts)

qt_standard_project_setup()

qt_add_executable(CML
    WIN32 MACOSX_BUNDLE
    main.cpp
    ui/mainwindow.cpp ui/mainwindow.h ui/mainwindow.ui
    db/databasemanager.h db/databasemanager.cpp
    ui/kategoriedialog.h ui/kategoriedialog.cpp ui/kategoriedialog.ui
    ui/platformydialog.h ui/platformydialog.cpp ui/platformydialog.ui
    ui/tagidialog.h ui/tagidialog.cpp ui/tagidialog.ui
    ui/statisticswidget.h ui/statisticswidget.cpp ui/statisticswidget.ui
    ui/multimediaformwidget.h ui/multimediaformwidget.cpp ui/multimediaformwidget.ui
    ui/szczegolywidget.h ui/szczegolywidget.cpp ui/szczegolywidget.ui
    ui/dashboardwidget.h ui/dashboardwidget.cpp ui/dashboardwidget.ui
    ui/panelnawigacjiwidget.h ui/panelnawigacjiwidget.cpp ui/panelnawigacjiwidget.ui
    ui/timelineview.h ui/timelineview.cpp ui/timelineview.ui
    controllers/appcontroller.h controllers/appcontroller.cpp
    models/multimedia.h models/platformy.h models/postep.h
    models/historia.h models/aktywnosc_statystyki.h
)

qt_add_resources(CML "resources"
    PREFIX "/icons"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons"
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/heart-filled.svg
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/heart-outline.svg
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/play.svg
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/pause.svg
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/reset.svg
)

target_include_directories(CML
    PRIVATE
        ui
        db
        models
        controllers
)

target_link_libraries(CML
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
        Qt6::Charts
)

install(TARGETS CML
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET CML
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
