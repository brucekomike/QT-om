cmake_minimum_required(VERSION 3.20)
project(qt-om VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Try to find KF6, fall back to Qt6-only build
find_package(ECM 5.240 QUIET)

if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)
    include(FeatureSummary)

    find_package(KF6 QUIET COMPONENTS
        CoreAddons
        XmlGui
        WidgetsAddons
        Config
        ConfigWidgets
        Notifications
        I18n
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Concurrent
)

set(SOURCES
    src/main.cpp
    src/app/mainwindow.cpp
    src/models/server.cpp
    src/models/serverlistmodel.cpp
    src/ssh/sshclient.cpp
    src/managers/servermanager.cpp
    src/managers/servicemanager.cpp
    src/managers/cronmanager.cpp
    src/managers/backupmanager.cpp
    src/managers/healthcheckmanager.cpp
    src/managers/configmanager.cpp
    src/panels/serverspanel.cpp
    src/panels/servicespanel.cpp
    src/panels/cronpanel.cpp
    src/panels/backuppanel.cpp
    src/panels/healthpanel.cpp
    src/dialogs/addserverdialog.cpp
    src/dialogs/servicedeploydialog.cpp
    src/dialogs/configmigrationdialog.cpp
    src/dialogs/cronjobeditdialog.cpp
    resources/resources.qrc
)

if(KF6_FOUND)
    add_executable(qt-om ${SOURCES})
    target_compile_definitions(qt-om PRIVATE USE_KF6)
    target_link_libraries(qt-om
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::Concurrent
        KF6::CoreAddons
        KF6::XmlGui
        KF6::WidgetsAddons
        KF6::Config
        KF6::ConfigWidgets
        KF6::Notifications
        KF6::I18n
    )
    install(TARGETS qt-om DESTINATION ${KDE_INSTALL_BINDIR})
else()
    message(STATUS "KF6 not found, building with Qt6 only")
    add_executable(qt-om ${SOURCES})
    target_link_libraries(qt-om
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::Concurrent
    )
    install(TARGETS qt-om DESTINATION bin)
endif()

target_include_directories(qt-om PRIVATE src)
