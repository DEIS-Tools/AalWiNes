cmake_minimum_required(VERSION 3.9)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project(mpls2pda VERSION 1.0.0 LANGUAGES CXX C)

option(MPLS2PDA_BuildBundle "Includes all but system libraries within the binary." OFF)
if (MPLS2PDA_BuildBundle)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif (MPLS2PDA_BuildBundle)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wpedantic -I -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG -Wall -Wpedantic -fPIC")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/rapidxml)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/ptrie)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
