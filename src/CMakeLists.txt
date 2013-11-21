cmake_minimum_required(VERSION 2.8)
project(libhawk)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pedantic -g -Og")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS system filesystem)

include_directories("include" ${Boost_INCLUDE_DIRS})

file(GLOB_RECURSE srcs "./*.cpp")
add_library(hawk SHARED ${srcs})
target_link_libraries(hawk ${Boost_LIBRARIES} magic)
