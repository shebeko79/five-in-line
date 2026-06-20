cmake_minimum_required (VERSION 4.2)

project(five_in_line)

set(ENV{BOOST} C:/src/boost_1_91_0)

add_subdirectory (db/cmake)
add_subdirectory (solver/cmake)

if(WIN32)
add_subdirectory (gui/cmake)
add_subdirectory (tests/cmake)
endif()
