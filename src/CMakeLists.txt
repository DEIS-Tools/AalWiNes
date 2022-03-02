cmake_minimum_required(VERSION 3.14)
project(aalwines VERSION ${AALWINES_VERSION} LANGUAGES C CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.5 REQUIRED)

flex_target(flexer "aalwines/query/QueryLexer.l" "${CMAKE_CURRENT_SOURCE_DIR}/aalwines/query/generated_QueryLexer.cc")
bison_target(bparser "aalwines/query/QueryParser.y" "${CMAKE_CURRENT_SOURCE_DIR}/aalwines/query/generated_QueryParser.cc")
add_flex_bison_dependency(flexer bparser)

find_package(Boost 1.70 COMPONENTS headers program_options regex REQUIRED)

include(GNUInstallDirs) # With GNUInstallDirs we use platform-independent macros to get the correct install directory names.  (CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_INCLUDEDIR)

############################
# Setup aalwines (library) #

add_library(aalwines)
add_library(aalwines::aalwines ALIAS aalwines)
target_sources(aalwines
	PRIVATE aalwines/model/builders/AalWiNesBuilder.cpp
			aalwines/model/builders/NetworkParsing.cpp
			aalwines/model/builders/TopologyBuilder.cpp
			aalwines/model/builders/NetworkSAXHandler.cpp
			aalwines/model/Router.cpp
			aalwines/model/RoutingTable.cpp
			aalwines/model/Query.cpp
			aalwines/model/Network.cpp
			aalwines/model/filter.cpp
			${BISON_bparser_OUTPUTS} ${FLEX_flexer_OUTPUTS}
			aalwines/query/QueryBuilder.cpp
			aalwines/utils/coordinate.cpp
			aalwines/utils/system.cpp
			aalwines/synthesis/RouteConstruction.cpp
	PUBLIC  ${HEADER_FILES}
)
target_compile_features(aalwines PUBLIC cxx_std_17) # Require C++17 features.
target_compile_definitions(aalwines PUBLIC $<$<CONFIG:Release>:NDEBUG>) # We can use "#ifndef NDEBUG\n <debug only code> \n#endif" to define code that is only used in debug mode.
target_include_directories(aalwines PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
) # The correct include directory depends on whether we are building or installing aalwines.
#target_link_libraries(aalwines PUBLIC m) # Uncomment in case we need <math.h>
target_link_libraries(aalwines
	PUBLIC  Boost::headers
			Boost::program_options
			Boost::regex
			pdaaal::pdaaal
			nlohmann_json::nlohmann_json
)

# Define which directories to install with the aalwines library.
install(DIRECTORY	aalwines/
					aalwines/model/
					aalwines/query
					aalwines/synthesis
					aalwines/utils/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aalwines)

# More or less boilerplate stuff that ensures everything is installed correctly
install(TARGETS  aalwines
		EXPORT   aalwines-targets
		RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT      aalwines-targets
		FILE        aalwines-targets.cmake
		NAMESPACE   aalwines::
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aalwines)
# aalwines-config.cmake.in defines dependencies that should be (transitively) available in aalwines's parent project.
configure_file(aalwines-config.cmake.in aalwines-config.cmake @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/aalwines-config.cmake"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aalwines)
# Setup creation of versioned package. AALWINES_VERSION is defined in the top-level cmake file.
include(CMakePackageConfigHelpers)
write_basic_package_version_file("aalwines-config-version.cmake"
		VERSION ${AALWINES_VERSION}
		COMPATIBILITY SameMajorVersion
		)
install(FILES "aalwines-config-version.cmake"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aalwines)


###########################################
# Setup aalwines-bin (stand-alone binary) #

if(DEFINED ENV{GITHUB_SHA}) # Workaround for Github CI workflow, where the current directory (for some odd reason) is not a git repository.
	add_custom_command(OUTPUT version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define STRINGIFY(x) #x" > version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define STRINGIFY_MACRO(x) STRINGIFY(x)" >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo_append "#define AALWINES_GIT_HASH " >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "$ENV{GITHUB_SHA}" >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define AALWINES_GIT_HASH_STR STRINGIFY_MACRO(AALWINES_GIT_HASH) " >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define AALWINES_VERSION ${AALWINES_VERSION}" >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define AALWINES_VERSION_STR STRINGIFY_MACRO(AALWINES_VERSION)" >> version.h
			DEPENDS aalwines main.cpp
			VERBATIM)
else()
	find_package(Git)
	add_custom_command(OUTPUT version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define STRINGIFY(x) #x" > version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define STRINGIFY_MACRO(x) STRINGIFY(x)" >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo_append "#define AALWINES_GIT_HASH " >> version.h
			COMMAND ${GIT_EXECUTABLE} rev-parse HEAD >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define AALWINES_GIT_HASH_STR STRINGIFY_MACRO(AALWINES_GIT_HASH) " >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define AALWINES_VERSION ${AALWINES_VERSION}" >> version.h
			COMMAND ${CMAKE_COMMAND} -E echo "#define AALWINES_VERSION_STR STRINGIFY_MACRO(AALWINES_VERSION)" >> version.h
			DEPENDS aalwines main.cpp
			VERBATIM)
endif()
add_custom_target(generate_hash DEPENDS "version.h")
add_executable(aalwines-bin main.cpp)
target_link_libraries(aalwines-bin PRIVATE aalwines)
add_dependencies(aalwines-bin generate_hash)
set_target_properties(aalwines-bin PROPERTIES OUTPUT_NAME aalwines)
