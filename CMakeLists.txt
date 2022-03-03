cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(AALWINES_VERSION 1.0.0)
project(aalwines VERSION ${AALWINES_VERSION} LANGUAGES CXX C)

option(AALWINES_BuildBundle "Includes all but system libraries within the binary." OFF)
if (AALWINES_BuildBundle)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif (AALWINES_BuildBundle)
option(AALWINES_BuildStatic "Includes all (incl. system libraries) within the binary." OFF)
if (AALWINES_BuildStatic)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif (AALWINES_BuildStatic)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++")

option(AALWINES_GetDependencies "Fetch external dependencies from web." ON)
if (AALWINES_GetDependencies)
    include(FetchContent)
    FetchContent_Declare(pdaaal
        URL https://github.com/DEIS-Tools/PDAAAL/archive/v1.0.0.tar.gz
        URL_HASH SHA512=843f5ff1906783d62b809406ebd32dcab08826dab9310adb4ca3326343381723230a83a6a13f83535318fd6f81cc86417547f14ce1d26d04010451e8c0a15249
    )
    FetchContent_MakeAvailable(pdaaal)
    if(IS_DIRECTORY "${pdaaal_SOURCE_DIR}")
        set_property(DIRECTORY ${pdaaal_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
    endif()

endif (AALWINES_GetDependencies)

add_subdirectory(src)

set(BUILD_TESTING ON)

option(AALWINES_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)
if(BUILD_TESTING AND AALWINES_BuildTests)
    #testing
    add_subdirectory(test)
    enable_testing()
    add_test(NAME Network_test              COMMAND Network_test)
    add_test(NAME SyntheticNetwork_test     COMMAND SyntheticNetwork_test)
    add_test(NAME RouteConstruction_test    COMMAND RouteConstruction_test)
    add_test(NAME Query_test                COMMAND Query_test)
    add_test(NAME JSONFormat_test           COMMAND JSONFormat_test)
    add_test(NAME more_algorithms_test      COMMAND more_algorithms_test)
endif()
