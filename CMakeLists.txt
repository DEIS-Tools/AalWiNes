cmake_minimum_required(VERSION 3.9)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project(aalwines VERSION 1.0.0 LANGUAGES CXX C)

option(AALWINES_BuildBundle "Includes all but system libraries within the binary." OFF)
if (AALWINES_BuildBundle)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif (AALWINES_BuildBundle)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wpedantic -I -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG -Wall -Wpedantic -fPIC")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(AALWINES_GetDependencies "Fetch external dependencies from web." ON)
if (AALWINES_GetDependencies)
    # setup for external imports
    include(ExternalProject)
    find_package(Git REQUIRED)
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

    ExternalProject_add(ptrie-ext
        GIT_REPOSITORY https://github.com/petergjoel/ptrie
        GIT_TAG 1.0.3
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPTRIE_BuildTest=OFF -DCMAKE_BUILD_TYPE=Release
    )
    
    ExternalProject_add(rapidxml-ext
        URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
        BUILD_COMMAND ""
        CONFIGURE_COMMAND ls ../rapidxml-ext
        INSTALL_COMMAND cp -f ../rapidxml-ext/{rapidxml,rapidxml_iterators,rapidxml_print,rapidxml_utils}.hpp ${EXTERNAL_INSTALL_LOCATION}/include 
    )

    # we can now include external libraries
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
endif (AALWINES_GetDependencies)


add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
