cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -Wpedantic")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++")

option(AALWINES_GetDependencies "Fetch external dependencies from web." ON)
if (AALWINES_GetDependencies)
    include(FetchContent)
    FetchContent_Declare(json
            URL https://github.com/nlohmann/json/archive/refs/tags/v3.10.0.tar.gz
            URL_HASH SHA512=1531780110260ea4cd8fbe79d860f1ea46fa8377d520669f938ddb5f8a4b11fe550fc7d2d978a281d5ddcc511447d160a2f68c06583672496c57c6672a4036a1
            )
    FetchContent_GetProperties(json)
    if(NOT json_POPULATED)
        FetchContent_Populate(json)
        set(JSON_Install ON CACHE INTERNAL "")
        add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    FetchContent_Declare(pdaaal
        #URL https://github.com/DEIS-Tools/PDAAAL/archive/v0.3.0.zip
        #URL_HASH SHA512=81eb135be9fd2687a48f526fb303c8a7cdc03a59ec0309717fa5af40c5807ed0eecf07f326692fe9f0cb11214b7d3488fd2e2ef5853d245e328509b02b20cbd7
        GIT_REPOSITORY https://github.com/MortenSchou/PDAAAL.git
        GIT_TAG 5f79667e60fadf700db81eeb2175ef6b8dfaf6f0
    )
    FetchContent_MakeAvailable(pdaaal)

endif (AALWINES_GetDependencies)

add_subdirectory(src)

set(BUILD_TESTING ON)

option(AALWINES_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)
if(BUILD_TESTING AND AALWINES_BuildTests)
    #testing
    add_subdirectory(test)
    enable_testing()
    add_test(NAME NetworkTest         COMMAND NetworkTest)
    add_test(NAME SyntheticNetwork    COMMAND SyntheticNetwork)
    add_test(NAME RouteConstruction   COMMAND RouteConstruction)
    add_test(NAME QueryTest           COMMAND QueryTest)
    add_test(NAME JSONFormatTest      COMMAND JSONFormatTest)
endif()
