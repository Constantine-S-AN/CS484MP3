cmake_minimum_required (VERSION 3.5)
#project(mp3-part1 VERSION 1.0.0 LANGUAGES C CXX)

# Location of additional cmake modules
#
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

add_library(particle-sim-common OBJECT "simblock.h" "simblock.cpp" "datagen.h" "datagen.cpp")
target_include_directories(particle-sim-common PUBLIC ".")
target_link_libraries(particle-sim-common PUBLIC gassim2d)
target_include_directories(particle-sim-common PUBLIC ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(particle-sim-common PUBLIC ${MPI_CXX_LIBRARIES})
if(MPI_COMPILE_FLAGS)
  set_target_properties(particle-sim-common PROPERTIES
  COMPILE_FLAGS "${MPI_CXX_COMPILE_FLAGS}")
endif()
