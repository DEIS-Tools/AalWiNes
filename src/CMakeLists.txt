cmake_minimum_required(VERSION 3.7)
project(aalwines C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.5 REQUIRED)

flex_target(flexer "aalwines/query/QueryLexer.l" "${CMAKE_CURRENT_SOURCE_DIR}/aalwines/query/generated_QueryLexer.cc")
bison_target(bparser "aalwines/query/QueryParser.y" "${CMAKE_CURRENT_SOURCE_DIR}/aalwines/query/generated_QueryParser.cc")
add_flex_bison_dependency(flexer bparser)

find_package(Boost 1.66 COMPONENTS program_options regex filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_library(aalwines ${HEADER_FILES} aalwines/model/builders/JuniperBuilder.cpp aalwines/model/builders/PRexBuilder.cpp
		aalwines/model/Router.cpp aalwines/model/RoutingTable.cpp aalwines/model/Query.cpp aalwines/model/Network.cpp
		aalwines/model/filter.cpp ${BISON_bparser_OUTPUTS} ${FLEX_flexer_OUTPUTS} aalwines/query/QueryBuilder.cpp
		aalwines/utils/parsing.cpp aalwines/utils/system.cpp aalwines/synthesis/FastRerouting.cpp)
add_dependencies(aalwines ptrie-ext rapidxml-ext pdaaal-ext)
target_link_libraries(aalwines PRIVATE ${Boost_LIBRARIES} pdaaal)
target_include_directories(aalwines PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS aalwines
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include/aalwines)

add_executable(aalwines_exe main.cpp)
target_link_libraries(aalwines_exe PRIVATE aalwines)

install(TARGETS aalwines_exe
		RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
