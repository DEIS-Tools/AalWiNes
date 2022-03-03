find_package (Boost COMPONENTS unit_test_framework REQUIRED)
add_definitions (-DBOOST_TEST_DYN_LINK)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(AALWINES_test_sources
    Network_test.cpp
    SyntheticNetwork_test.cpp
    RouteConstruction_test.cpp
    Query_test.cpp
    JSONFormat_test.cpp
    more_algorithms_test.cpp
)

foreach(test_source_file ${AALWINES_test_sources})
    get_filename_component(exename ${test_source_file} NAME_WE)
    add_executable(${exename} ${test_source_file})
    target_link_libraries(${exename} PRIVATE Boost::unit_test_framework aalwines)
endforeach()
