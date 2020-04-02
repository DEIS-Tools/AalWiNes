find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${TEST_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
        ${AALWINES_SOURCE_DIR}
        )
add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable (SyntacticNetwork SyntacticNetwork_test.cpp)
add_executable (FastRerouting FastRerouting_test.cpp)

target_link_libraries(SyntacticNetwork ${Boost_LIBRARIES} aalwines)
target_link_libraries(FastRerouting ${Boost_LIBRARIES} aalwines)
