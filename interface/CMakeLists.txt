set(kdiffinterface_LIB_SRCS kdiffinterface.cpp )

add_library(kdiffinterface SHARED ${kdiffinterface_LIB_SRCS})

generate_export_header(kdiffinterface BASE_NAME KDIFFINTERFACE)

target_link_libraries(kdiffinterface
        Qt5::Core
)

set_target_properties(kdiffinterface PROPERTIES VERSION ${KDIFF_LIB_VERSION} SOVERSION ${KDIFF_LIB_SOVERSION} )
install(TARGETS kdiffinterface ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES kdiffinterface.h DESTINATION ${INCLUDE_INSTALL_DIR}/kdiff COMPONENT Devel )


