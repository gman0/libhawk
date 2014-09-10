cmake_minimum_required(VERSION 2.8)
project(libhawk)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pedantic -g -Og -fPIC")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS system filesystem)

include_directories("include" ${Boost_INCLUDE_DIRS})

file(GLOB_RECURSE srcs "./*.cpp")
add_library(hawk_obj OBJECT ${srcs})
add_library(hawk_static STATIC $<TARGET_OBJECTS:hawk_obj>)
add_library(hawk SHARED $<TARGET_OBJECTS:hawk_obj>)

target_link_libraries(hawk ${Boost_LIBRARIES} magic pthread)

install(TARGETS hawk hawk_static DESTINATION lib)
install(DIRECTORY "include/" DESTINATION "include/hawk")
