cmake_minimum_required (VERSION 3.5)
#project(mp3-part3 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS "-g -Wfatal-errors")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wfatal-errors")
# Location of additional cmake modules
#

find_package(Charm)

#
# IMPORTANT: Charm modules must have the same name as their .ci files.
# There may only be one charm module per .ci file.
#
# Look at https://github.com/bryan-lunt-supercomputing/charmpp_cmake_findpackage for examples
# also https://github.com/bryan-lunt-supercomputing/charmpp_simple_examples
#
add_charm_module(charmblock "charmblock.ci")
add_charm_module(simmain "simmain.ci" LINK_MODULES "CommonLBs")

add_library(part3-block STATIC "charmblock.cpp" "charmblock.h")
target_link_libraries(part3-block charmblock)
#target_include_directories(part3-block PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}) #somehow needed to make sure that the rest finds "Particle.h"
target_link_libraries(part3-block gassim2d particle-sim-common)

add_executable(part3 "main.cpp" "simmain.h" "simmain_modinit.C")
target_link_libraries(part3-block charmblock simmain charm_all)
#target_include_directories(part3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}) #somehow needed to make sure that the rest finds "Particle.h"
#set_charm_target(part3)

target_link_libraries(part3 part3-block gassim2d particle-sim-common)
target_link_libraries(part3 Boost::program_options)
