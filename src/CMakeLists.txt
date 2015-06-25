cmake_minimum_required(VERSION 2.8)
project(libhawk)

set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -pedantic -g -O0 -fPIC \
	 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE")

include_directories("include")

file(GLOB_RECURSE srcs "./*.cpp")
add_library(hawk_obj OBJECT ${srcs})
add_library(hawk_static STATIC $<TARGET_OBJECTS:hawk_obj>)
add_library(hawk SHARED $<TARGET_OBJECTS:hawk_obj>)

target_link_libraries(hawk magic pthread)

install(TARGETS hawk hawk_static DESTINATION lib)
install(DIRECTORY "include/" DESTINATION "include/hawk")
