include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../libkdiff)

set(kdiffsettings_LIB_SRCS
        settings.cpp
        diffpage.cpp
        applicationpage.cpp
)

add_library(kdiffsettings SHARED ${kdiffsettings_LIB_SRCS})

generate_export_header(kdiffsettings BASE_NAME KDIFFSETTINGS)

target_link_libraries(kdiffsettings
        kdiffparser
        Qt5::Core
        KF5::KIOWidgets
        KF5::ConfigWidgets
        KF5::I18n
)

set_target_properties(kdiffsettings PROPERTIES VERSION ${KDIFF_LIB_VERSION} SOVERSION ${KDIFF_LIB_SOVERSION} )

install(TARGETS kdiffsettings ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)



