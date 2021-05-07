find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${TEST_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
        ${AALWINES_SOURCE_DIR}
        )
add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable (NetworkTest Network_test.cpp)
add_executable (SyntheticNetwork SyntheticNetwork_test.cpp)
add_executable (RouteConstruction RouteConstruction_test.cpp)
add_executable (QueryTest Query_test.cpp)
add_executable (JSONFormatTest JSONFormat_test.cpp)

target_link_libraries(NetworkTest ${Boost_LIBRARIES} aalwines)
target_link_libraries(SyntheticNetwork ${Boost_LIBRARIES} aalwines)
target_link_libraries(RouteConstruction ${Boost_LIBRARIES} aalwines)
target_link_libraries(QueryTest ${Boost_LIBRARIES} aalwines)
target_link_libraries(JSONFormatTest ${Boost_LIBRARIES} aalwines)
