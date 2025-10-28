cmake_minimum_required (VERSION 3.5)
project(mp3-part2 VERSION 1.0.0 LANGUAGES C CXX)

# Location of additional cmake modules
#
# Location of additional cmake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake
    )

#find_package(MPI REQUIRED)
#find_package(AMPI REQUIRED)

find_package(Charm REQUIRED)

#This is really ugly, I'm setting the compiler directly.
set(CMAKE_C_COMPILER "${AMPI_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${AMPI_CXX_COMPILER}")
#set(CMAKE_LINKER "/opt/charm-6.9.0/bin/ampicxx")
set(CMAKE_EXE_LINKER_FLAGS "-L${CHARMLIB}") #absolutely disgusting, but leave for now

add_executable(part2 "../part1/main.cpp" "../part1/solution.h" "../part1/solution.cpp")
target_link_libraries(part2 PUBLIC gassim2d particle-sim-common)
target_link_libraries(part2 PUBLIC Boost::program_options)
#target_link_libraries(part2 PUBLIC MPI::MPI_CXX) #automatically linked by AMPI

#set_property(TARGET part2 APPEND_STRING PROPERTY COMPILE_FLAGS " -modules CommonLBs -memory isomalloc")
set_property(TARGET part2 APPEND_STRING PROPERTY LINK_FLAGS " -modules CommonLBs -memory isomalloc")
