cmake_minimum_required (VERSION 3.5)
project(mp3 VERSION 1.0.0 LANGUAGES C CXX)

option(TA_PROJECT_TESTS "Turned on when testing code to provide to students." OFF)
option(DOWNLOAD_BOOST "Use this to download boost into this directory, instead of searching the system." OFF)


# Location of additional cmake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake
    )
include("PreventInTreeBuilds")
include("cs484common")

if(DOWNLOAD_BOOST)
    add_subdirectory("boost-cmake")
    #set(BOOST_ROOT "boost-cmake/boost")
else()
    message("Searching for a sytem-installed BOOST. Use the option 'DOWNLOAD_BOOST' if you want to download the Boost code into this directory.")
    find_package(Boost REQUIRED COMPONENTS program_options)
endif()

#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

add_subdirectory("gas")
add_subdirectory("common")

add_subdirectory("part1")
add_subdirectory("part2")
add_subdirectory("part3")

add_subdirectory("tests")
