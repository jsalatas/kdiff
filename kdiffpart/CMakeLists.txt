#define TRANSLATION_DOMAIN "kdiff"

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../interface ${CMAKE_CURRENT_SOURCE_DIR}/../libkdiff ${CMAKE_CURRENT_SOURCE_DIR}/../settings)


set(kdiff_PART_SRCS
        kdiffpart.cpp
        fileview.cpp
        diffscrollbar.cpp
        shortcutsdialog.cpp
        kdiffview.cpp
        connectwidget.cpp
        filenavigator.cpp
        filenavigatoritem.cpp
        icons.cpp
        )

add_library(kdiffpart MODULE ${kdiff_PART_SRCS})

target_link_libraries(kdiffpart
        KF5::Parts
        KF5::IconThemes
        KF5::TextEditor
        kdiffinterface
        kdiffparser
        kdiffsettings
        )


add_subdirectory(icons)

install(TARGETS kdiffpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kdiffpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES kdiffpartui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kdiff)

