cmake_minimum_required(VERSION 3.7)
project(mpls2pda C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.5 REQUIRED)

flex_target(flexer "query/QueryLexer.l"  "${CMAKE_CURRENT_SOURCE_DIR}/query/generated_QueryLexer.cc")
bison_target(bparser "query/QueryParser.y" "${CMAKE_CURRENT_SOURCE_DIR}/query/generated_QueryParser.cc")
add_flex_bison_dependency(flexer bparser)


find_package(Boost 1.66 COMPONENTS program_options regex filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR} rapidxml)

add_executable(mpls2pda ${HEADER_FILES} main.cpp model/builders/JuniperBuilder.cpp model/builders/PRexBuilder.cpp
    model/Router.cpp model/RoutingTable.cpp model/Query.cpp model/Network.cpp model/filter.cpp model/NetworkPDAFactory.cpp
    ${BISON_bparser_OUTPUTS} ${FLEX_flexer_OUTPUTS} query/QueryBuilder.cpp 
    pdaaal/model/PDA.cpp pdaaal/engine/Moped.cpp
    utils/parsing.cpp utils/system.cpp)
target_link_libraries(mpls2pda PRIVATE stdc++fs  ${Boost_LIBRARIES} rapidxml ptrie)

install(TARGETS mpls2pda
	RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
#install (FILES  DESTINATION include/)


