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
            https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp
            ${EXTERNAL_INSTALL_LOCATION}/include/json.hpp
            EXPECTED_HASH SHA256=7804b38146921d03374549c9e2a5e3acda097814c43caf2b96a0278e58df26e0
    )
    ExternalProject_add(pdaaal-ext
        GIT_REPOSITORY git@github.com:MortenSchou/PDAAAL.git
        GIT_TAG 5091f704ceeb6e9011b5887213c180d78a8fddec
        #URL https://github.com/DEIS-Tools/PDAAAL/archive/v0.2.5.zip
        #URL_HASH SHA512=7c9e1418c491f693b1dbcfac8b20aa10cc0ecc8db1456000d1b8ea3c1376d9dbe545abbfe550d7d5020631c4dcb44fe2b6205654337c68a16227f0810e0f32a6
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPDAAAL_BuildTests=OFF -DCMAKE_BUILD_TYPE=Release -DPTRIE_INSTALL_DIR=${EXTERNAL_INSTALL_LOCATION}/include
    )

    # we can now include external libraries
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
endif (AALWINES_GetDependencies)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/)

set(BUILD_TESTING ON)

option(AALWINES_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)
if(BUILD_TESTING AND AALWINES_BuildTests)
    #testing
    add_subdirectory(${CMAKE_SOURCE_DIR}/test/)
    enable_testing()
    add_test(NAME NetworkTest         COMMAND NetworkTest)
    add_test(NAME SyntacticNetwork    COMMAND SyntacticNetwork)
    add_test(NAME RouteConstruction   COMMAND RouteConstruction)
    add_test(NAME QueryTest           COMMAND QueryTest)
    add_test(NAME JSONFormatTest      COMMAND JSONFormatTest)
endif()
