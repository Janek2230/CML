cmake_minimum_required(VERSION 3.19)
project(CML LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql)

qt_standard_project_setup()

qt_add_executable(CML
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    multimedia.h
    platformy.h
    postep_ksiazki.h
    databasemanager.h
    databasemanager.cpp
)

target_link_libraries(CML
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
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
