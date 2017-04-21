#define TRANSLATION_DOMAIN "kdiff"
set(kdiffparser_SRCS
        diffprocess.cpp
        modellist.cpp
        diffmodelmap.cpp
        diffmodel.cpp
        difference.cpp
        diffhunk.cpp
        diffsettings.cpp
        parser.cpp
        parserbase.cpp
        cvsdiffparser.cpp
        diffparser.cpp
        perforceparser.cpp
        stringlistpair.cpp
        info.cpp
        utils.cpp
        linemapper.cpp
        diffprocessor.cpp
        )


add_library(kdiffparser SHARED ${kdiffparser_SRCS})
generate_export_header(kdiffparser BASE_NAME KDIFFPARSER)

target_link_libraries(kdiffparser
        Qt5::Core
        Qt5::Widgets
        KF5::Codecs
        KF5::CoreAddons
        KF5::I18n
        KF5::KIOCore
        KF5::XmlGui
        )

set_target_properties(kdiffparser PROPERTIES VERSION ${KDIFF_LIB_VERSION} SOVERSION ${KDIFF_LIB_SOVERSION})

install(TARGETS kdiffparser ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
