cmake_minimum_required (VERSION 3.5)
#project(mp3-part1 VERSION 1.0.0 LANGUAGES C CXX)

# Location of additional cmake modules
#

find_package(MPI REQUIRED)

add_executable(part1 "main.cpp" "solution.h" "solution.cpp")

target_link_libraries(part1 PUBLIC gassim2d particle-sim-common)

target_link_libraries(part1 PUBLIC Boost::program_options)

target_link_libraries(part1 PUBLIC MPI::MPI_CXX)
