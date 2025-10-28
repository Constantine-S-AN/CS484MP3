
#Output sorter needed for unit and regression tests.
add_executable(xyz_sorter "xyz_sorter.cpp")
target_link_libraries(xyz_sorter PUBLIC gassim2d particle-sim-common)
target_link_libraries(xyz_sorter PUBLIC Boost::program_options)

#copy the data directory to the build directory
add_custom_command(TARGET xyz_sorter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/data ${CMAKE_DATA_OUTPUT_DIRECTORY}
)

#Tests and grading
include(CTest)
include(FindGTest)

if(NOT GTEST_FOUND)
	message(FATAL_ERROR "DANGER: Could not find GoogleTest")
else(NOT GTEST_FOUND)
	include(GoogleTest)

	#unit tests for student code.
	add_executable(run_tests "mpi_bounded_tests.cpp" "gtest-mpi-listener.hpp" "../part1/solution.cpp" "../part1/solution.h")
	#include_directories(run_tests "gas/gassim2d/include")
	target_include_directories(run_tests PUBLIC "../part1")
	target_link_libraries(run_tests gassim2d particle-sim-common)
	target_link_libraries(run_tests pthread)
	target_include_directories(run_tests PUBLIC ${GTEST_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_PATH})
	target_link_libraries(run_tests ${GTEST_BOTH_LIBRARIES} ${MPI_CXX_LIBRARIES})
endif(NOT GTEST_FOUND)
