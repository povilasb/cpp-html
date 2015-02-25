cmake_minimum_required(VERSION 2.6)


set(test_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE test_src_files ${test_src_dir}/*_test.cpp)


set(gmock_dir "${CMAKE_CURRENT_SOURCE_DIR}/lib/gmock")
set(gtest_dir "${gmock_dir}/gtest")
add_subdirectory("${gmock_dir}" "${CMAKE_CURRENT_BINARY_DIR}/lib/gmock")
include_directories("${gtest_dir}/include")
include_directories("${gmock_dir}/include")

add_definitions(-DTEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixture")


set(all_libraries gmock_main gmock gtest ${lib_cpp_html})


add_custom_target(run-tests COMMAND "")
set(test_bin_dir "${CMAKE_CURRENT_BINARY_DIR}/test-bin")

foreach(test_src_file ${test_src_files})
	get_filename_component(test_name "${test_src_file}" NAME_WE)
	add_executable("${test_name}" "${test_src_file}")
	add_dependencies("${test_name}" ${all_libraries})
	target_link_libraries("${test_name}" ${all_libraries})

	add_custom_command(TARGET run-tests
		COMMAND "${test_bin_dir}/${test_name}")
	add_dependencies(run-tests "${test_name}")

	set_target_properties("${test_name}" PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${test_bin_dir}")
endforeach()
