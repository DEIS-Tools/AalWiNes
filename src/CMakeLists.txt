cmake_minimum_required(VERSION 3.7)
project(mpls2pda C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(BISON 3.0.0 REQUIRED)
find_package(FLEX 2.6.0 REQUIRED)

bison_target(bparser "parser/QueryParser.y" "${CMAKE_CURRENT_SOURCE_DIR}/generated_QueryParser.cc")
flex_target(flexer "parser/QueryLexer.l"  "${CMAKE_CURRENT_SOURCE_DIR}/generated_QueryLexer.cc")
add_flex_bison_dependency(flexer bparser)


find_package(Boost 1.66 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR} rapidxml)

add_executable(mpls2pda ${HEADER_FILES} main.cpp model/Router.cpp model/RoutingTable.cpp model/Query.cpp parser/QueryBuilder.cpp)
target_link_libraries(mpls2pda PRIVATE stdc++fs  ${Boost_LIBRARIES} rapidxml ptrie ${BISON_bparser_OUTPUTS} ${FLEX_flexer_OUTPUTS})

install(TARGETS mpls2pda
	RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
#install (FILES  DESTINATION include/)


