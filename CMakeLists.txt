cmake_minimum_required(VERSION 3.19)
project(CML LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Charts)

qt_standard_project_setup()

qt_add_executable(CML
    WIN32 MACOSX_BUNDLE
    main.cpp
    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/mainwindow.ui
    models/multimedia.h
    models/platformy.h
    models/postep.h
    db/databasemanager.h
    db/databasemanager.cpp
    ui/kategoriedialog.h ui/kategoriedialog.cpp
    ui/platformydialog.h ui/platformydialog.cpp

    ui/statisticswidget.cpp
    ui/statisticswidget.h
    ui/statisticswidget.ui
    ui/multimediaformwidget.h ui/multimediaformwidget.cpp ui/multimediaformwidget.ui
    ui/szczegolywidget.h ui/szczegolywidget.cpp ui/szczegolywidget.ui
    ui/dashboardwidget.h ui/dashboardwidget.cpp ui/dashboardwidget.ui
    ui/panelnawigacjiwidget.h ui/panelnawigacjiwidget.cpp ui/panelnawigacjiwidget.ui
    models/aktywnosc_statystyki.h
    controllers/appcontroller.h
    controllers/appcontroller.cpp

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