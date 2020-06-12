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
        URL https://github.com/petergjoel/ptrie/archive/v1.1.0.zip
        URL_HASH SHA512=092a8f50ca21d1199b19a10c4e0273c93a717a9f6491998a16bf21d21d37e6537ffd8a06ac41a2b623241da6036546d44b754567441944565e2a16646378cf29
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
    )
    file(MAKE_DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/include)
    ExternalProject_add(rapidxml-ext
        URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
        URL_HASH SHA512=6c10583e6631ccdb0217d0a5381172cb4c1046226de6ef1acf398d85e81d145228e14c3016aefcd7b70a1db8631505b048d8b4f5d4b0dbf1811d2482eefdd265
        BUILD_COMMAND ""
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND cd ../rapidxml-ext && ${CMAKE_COMMAND} -E copy rapidxml.hpp rapidxml_iterators.hpp rapidxml_print.hpp rapidxml_utils.hpp ${EXTERNAL_INSTALL_LOCATION}/include 
    )
    file(DOWNLOAD
            https://github.com/nlohmann/json/releases/download/v3.7.3/json.hpp
            ${EXTERNAL_INSTALL_LOCATION}/include/json.hpp
            EXPECTED_HASH SHA256=3b5d2b8f8282b80557091514d8ab97e27f9574336c804ee666fda673a9b59926
    )
    ExternalProject_add(pdaaal-ext
        URL https://github.com/DEIS-Tools/PDAAAL/archive/v0.2.3.zip
        URL_HASH SHA512=40c6abf65c8664e91513584b6a50feb6731d393e3b3f635c4057f6f7097e195c9a35e8ac6d50bf69983ade3be2ba691fcb4989b1d646b539ed22649828af60a1
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPDAAAL_BuildTests=OFF -DCMAKE_BUILD_TYPE=Release -DPTRIE_INSTALL_DIR=${EXTERNAL_INSTALL_LOCATION}/include
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

set(BUILD_TESTING ON)

option(AALWINES_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)
if(BUILD_TESTING AND AALWINES_BuildTests)
    #testing
    add_subdirectory(${CMAKE_SOURCE_DIR}/test/)
    enable_testing()
    add_test(NAME SyntacticNetwork    COMMAND SyntacticNetwork)
    add_test(NAME RouteConstruction   COMMAND RouteConstruction)
    add_test(NAME QueryTest           COMMAND QueryTest)
endif()
