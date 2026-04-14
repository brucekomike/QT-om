cmake_minimum_required(VERSION 3.20)
project(qt-om VERSION 0.1.0 LANGUAGES CXX)

# Detect target architecture for package naming
if(NOT DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    execute_process(COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE _DEB_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(_DEB_ARCH)
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${_DEB_ARCH}")
    else()
        # Fallback: map CMake system processor to package arch
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE    "aarch64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE    "x86_64")
        else()
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
            set(CPACK_RPM_PACKAGE_ARCHITECTURE    "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()
endif()

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
    install(FILES packaging/qt-om.desktop DESTINATION ${KDE_INSTALL_APPDIR})
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
    include(GNUInstallDirs)

    if(APPLE)
        # Create a proper .app bundle so macdeployqt can embed Qt frameworks
        set_target_properties(qt-om PROPERTIES
            MACOSX_BUNDLE                 TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER  "app.qt-om"
            MACOSX_BUNDLE_BUNDLE_NAME     "QT-om"
            MACOSX_BUNDLE_BUNDLE_VERSION  "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        )
        install(TARGETS qt-om BUNDLE DESTINATION .)
    else()
        install(TARGETS qt-om DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

    if(UNIX AND NOT APPLE)
        install(FILES packaging/qt-om.desktop
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    endif()
endif()

target_include_directories(qt-om PRIVATE src)

# ── CPack packaging ──────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME             "qt-om")
set(CPACK_PACKAGE_VERSION          "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR           "QT-om")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Cross-platform Qt DevOps server manager")
set(CPACK_PACKAGE_DESCRIPTION
    "QT-om is a Qt6/KDE application for managing remote servers.\n"
    "Features: SSH key auth setup, service deployment, cron management,\n"
    "automated backups, health monitoring and full config migration.")
set(CPACK_PACKAGE_HOMEPAGE_URL     "https://github.com/brucekomike/QT-om")
set(CPACK_RESOURCE_FILE_LICENSE    "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT          "qt-om@example.com")
set(CPACK_STRIP_FILES              TRUE)

# DEB
set(CPACK_DEBIAN_PACKAGE_SECTION   "net")
set(CPACK_DEBIAN_PACKAGE_PRIORITY  "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6 | libqt6core6t64, libqt6widgets6 | libqt6widgets6t64, libqt6network6 | libqt6network6t64, libqt6sql6 | libqt6sql6t64, openssh-client, sshpass")
set(CPACK_DEBIAN_FILE_NAME         DEB-DEFAULT)

# RPM
set(CPACK_RPM_PACKAGE_LICENSE      "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP        "Applications/Internet")
set(CPACK_RPM_PACKAGE_REQUIRES     "qt6-qtbase, openssh-clients, sshpass")
set(CPACK_RPM_FILE_NAME            RPM-DEFAULT)

# macOS DMG
set(CPACK_DMG_VOLUME_NAME          "QT-om ${PROJECT_VERSION}")
set(CPACK_DMG_FORMAT               "UDZO")

# Windows NSIS
set(CPACK_NSIS_PACKAGE_NAME        "QT-om")
set(CPACK_NSIS_DISPLAY_NAME        "QT-om ${PROJECT_VERSION}")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH         ON)

include(CPack)
