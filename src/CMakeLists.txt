cmake_minimum_required(VERSION 2.8)
project(libhawk)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pedantic -g -Og")

include_directories(include)

file(GLOB_RECURSE srcs "./*.cpp")
add_library(hawk SHARED ${srcs})
