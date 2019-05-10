set(SRC_LIST
    )

#-------------------------------------------------
# build and install tlog
#-------------------------------------------------
if (BUILD_SHARED)
    add_library(tlog SHARED ${SRC_LIST})
    set_target_properties(tlog PROPERTIES VERSION 
        ${VERSION_MAJOR}.${VERSION_MINOR}.${REVISION_NUMBER}.${BUILD_NUMBER} SOVERSION ${VERSION_MAJOR})
    install(TARGETS tlog
        LIBRARY DESTINATION "lib")
else (BUILD_SHARED)
    add_library(tlog STATIC ${SRC_LIST})
    install(TARGETS tlog
        ARCHIVE DESTINATION "lib")
endif (BUILD_SHARED)

#-------------------------------------------------
# generate and install pc
#-------------------------------------------------
configure_file(
    cmake/tlog.pc.in
    "${CMAKE_BINARY_DIR}/tlog.pc"
    @ONLY)

install(FILES "${CMAKE_BINARY_DIR}/tlog.pc" DESTINATION "/usr/lib/pkgconfig")


