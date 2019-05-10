cmake_minimum_required(VERSION 2.8.12)

project (meshx)

set(VERSION_MAJOR    0)
set(VERSION_MINOR    1)
set(REVISION_NUMBER  0)
set(BUILD_NUMBER     0)

option(BUILD_EXAMPLES "Build meshx examples" OFF)
option(BUILD_TESTS "Build meshx unit tests programs" ON)


#-------------------------------------------------
# compiler flags 
#-------------------------------------------------
set(COMMON_FLAGS "-Wall -Werror -fsigned-char")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=gnu99 -Wstrict-prototypes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
message(STATUS "Build Type: " ${CMAKE_BUILD_TYPE})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -g -ggdb -DMESHX_ENABLE_ASSERT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g -ggdb -DMESHX_ENABLE_ASSERT")
    message(STATUS "C Flags:" ${CMAKE_C_FLAGS_DEBUG})
    message(STATUS "CXX Flags:" ${CMAKE_CXX_FLAGS_DEBUG})
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
    message(STATUS "C Flags:" ${CMAKE_C_FLAGS_RELEASE})
    message(STATUS "CXX Flags:" ${CMAKE_CXX_FLAGS_RELEASE})
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

#-------------------------------------------------
# add main directories
#-------------------------------------------------

#-------------------------------------------------
# add directories
#-------------------------------------------------
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

if (BUILD_TESTS)
    add_subdirectory(tests)   
endif ()


