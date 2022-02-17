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
#    FetchContent_Declare(json
#            URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
#            URL_HASH SHA512=feb7dbdd1f1b6318131821f55d1411f0435262601c65eca368ccf3418750fd6348a37a3cd9d0694b31ccacfc91d02fbbecf94af52429005f8898490a5233c37d
#            )
#    FetchContent_GetProperties(json)
#    if(NOT json_POPULATED)
#        FetchContent_Populate(json)
#        set(JSON_Install ON)
#        add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
#    endif()

    FetchContent_Declare(pdaaal
        #URL https://github.com/DEIS-Tools/PDAAAL/archive/v0.3.0.zip
        #URL_HASH SHA512=81eb135be9fd2687a48f526fb303c8a7cdc03a59ec0309717fa5af40c5807ed0eecf07f326692fe9f0cb11214b7d3488fd2e2ef5853d245e328509b02b20cbd7
        GIT_REPOSITORY https://github.com/MortenSchou/PDAAAL.git
        GIT_TAG db583b46c8fd70e9ef29bf645f6f7715997f42e2
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
