cmake_minimum_required(VERSION 3.7)
project(mpls2pda C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost 1.66 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR} rapidxml)

add_executable(mpls2pda ${HEADER_FILES} main.cpp)
target_link_libraries(mpls2pda PRIVATE stdc++fs  ${Boost_LIBRARIES} rapidxml)


install(TARGETS mpls2pda
	RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
#install (FILES  DESTINATION include/)


