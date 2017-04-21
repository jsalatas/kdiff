cmake_minimum_required(VERSION 2.8.12)
project (kdiff)

# search packages used by KDE

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(FeatureSummary)
include(GenerateExportHeader)

find_package(Qt5 CONFIG REQUIRED COMPONENTS
        Core
)

find_package(KF5 REQUIRED COMPONENTS
        KIO
        IconThemes
        TextEditor
        CoreAddons
        Parts
        I18n
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

if(IS_ABSOLUTE ${DATA_INSTALL_DIR})
  set(KDIFF_DATA_INSTALL_DIR ${DATA_INSTALL_DIR}/kdiff)
else()
  set(KDIFF_DATA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}/kdiff)
endif()

set(KDIFF_LIB_VERSION 1)
set(KDIFF_LIB_SOVERSION 1)

add_subdirectory(interface)
add_subdirectory(libkdiff)
add_subdirectory(settings)
add_subdirectory(kdiffpart)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/interface
        ${CMAKE_CURRENT_SOURCE_DIR}/settings
        ${CMAKE_CURRENT_SOURCE_DIR}/kdiffpart
)


set(kdiff_SRCS
        main.cpp
        mainwindow.cpp
        opendialog.cpp
)

add_executable(kdiff ${kdiff_SRCS} )

target_link_libraries(kdiff
        kdiffinterface
        kdiffsettings
        KF5::TextEditor
)

install(TARGETS kdiff  ${INSTALL_TARGETS_DEFAULT_ARGS})

install( PROGRAMS org.kde.kdiff.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

install( FILES kdiffviewpart.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

install(FILES kdiffui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kdiff)
