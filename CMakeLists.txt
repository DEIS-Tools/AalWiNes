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
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
    
    ExternalProject_add(ptrie-ext
        GIT_REPOSITORY https://github.com/petergjoel/ptrie
        GIT_TAG e5f9eeedf3da4560265e4fca503ac00e8bbde4fc
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPTRIE_BuildTest=OFF -DCMAKE_BUILD_TYPE=Release
    )
    #ExternalProject_add(ptrie-ext
    #    URL https://github.com/petergjoel/ptrie/archive/1.0.3.zip
    #    URL_HASH SHA512=3f8285a0a7008dd0a8850753b748ad2d076df3f02574005f6580fa750f9a4aaa808e297a60c2b91d705e2ca63b1eb83ecd4e4504ae420fb2fd94d8f10ea42dba
    #    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
    #)
    file(MAKE_DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/include)
    ExternalProject_add(rapidxml-ext
        URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
        URL_HASH SHA512=6c10583e6631ccdb0217d0a5381172cb4c1046226de6ef1acf398d85e81d145228e14c3016aefcd7b70a1db8631505b048d8b4f5d4b0dbf1811d2482eefdd265
        BUILD_COMMAND ""
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND cp -f ../rapidxml-ext/{rapidxml,rapidxml_iterators,rapidxml_print,rapidxml_utils}.hpp ${EXTERNAL_INSTALL_LOCATION}/include 
    )

    # we can now include external libraries
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

    # download moped
    file(MAKE_DIRECTORY bin)
    file(DOWNLOAD https://github.com/petergjoel/P-Rex/raw/master/bin/moped bin/moped EXPECTED_HASH SHA512=f8a3bfc1914b425a9272544f639325c7799b76a5e6bf39d0500ae2453c7ccce3ddfb7a6db0d54ed5a84e623fea4002f2c09f2eb42235d3bb7cd7d52d78b73f5a)
    execute_process(COMMAND chmod +x bin/moped)
endif (AALWINES_GetDependencies)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
