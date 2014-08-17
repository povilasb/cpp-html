cmake_minimum_required(VERSION 2.6)

if (NOT DEFINED PROJECT_NAME)
	set(PROJECT_NAME "cpp-html")
endif(NOT DEFINED PROJECT_NAME)

option(PUGIHTML_ENABLE_TESTS "Enables or disables tests. Disabled by default."
	OFF)

project(${PROJECT_NAME} CXX)

set(PROJECT_LIB "${PROJECT_NAME}")


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}.")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fstack-protector -s")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic")

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

#file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
set(SRC_FILES "${SRC_DIR}/node.cpp" "${SRC_DIR}/attribute.cpp"
	"${SRC_DIR}/document.cpp" "${SRC_DIR}/parser.cpp")
add_library("${PROJECT_LIB}" STATIC ${SRC_FILES})

include_directories("${INCLUDE_DIR}")
include_directories("${SRC_DIR}")


#
# Testing rules.
#

if (PUGIHTML_ENABLE_TESTS)

	set(TEST_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
	#file(GLOB_RECURSE TEST_SRC_FILES "${TEST_SRC_DIR}/*.cpp")
	set(TEST_SRC_FILES "${TEST_SRC_DIR}/node_test.cpp"
		"${TEST_SRC_DIR}/attribute_test.cpp"
		"${TEST_SRC_DIR}/document_test.cpp"
		"${TEST_SRC_DIR}/parser_test.cpp")


	set(GTEST_DIR "${LIBS_DIR}/gtest")
	add_subdirectory(${GTEST_DIR})
	include_directories("${GTEST_DIR}/include")


	add_custom_target(run-tests COMMAND "")
	add_custom_target(run-memleak-tests COMMAND "")

	find_program(VALGRIND "valgrind")

	foreach(TEST_SRC ${TEST_SRC_FILES})
		get_filename_component(TEST_NAME "${TEST_SRC}" NAME_WE)

		add_executable("${TEST_NAME}" ${TEST_SRC})
		add_dependencies("${TEST_NAME}" "${PROJECT_LIB}")
		target_link_libraries(${TEST_NAME} "gtest" "gtest_main"
			${PROJECT_LIB})

		set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Testing/bin")
		set_target_properties("${TEST_NAME}" PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_DIR}")

		add_custom_command(TARGET run-tests
			COMMAND "${TEST_OUTPUT_DIR}/${TEST_NAME}")

		add_custom_command(TARGET run-memleak-tests
			COMMAND "${VALGRIND}"
			ARGS "--leak-check=full"
				"${TEST_OUTPUT_DIR}/${TEST_NAME}"
		)
	endforeach()
endif()
