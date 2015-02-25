cmake_minimum_required(VERSION 2.6)

option(CPPHTML_ENABLE_TESTS "Enables or disables tests. Disabled by default."
	OFF)

project(cpp-html CXX)
set(lib_cpp_html "${PROJECT_NAME}")


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}.")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fstack-protector")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic")

set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(include_dir "${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB_RECURSE src_files "${src_dir}/*.cpp")
add_library("${lib_cpp_html}" STATIC ${src_files})

include_directories(${include_dir})
include_directories(${src_dir})


#
# Testing rules.
#

if (CPPHTML_ENABLE_TESTS)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
endif()
