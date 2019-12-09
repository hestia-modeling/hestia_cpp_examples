cmake_minimum_required(VERSION 3.15)

project(hestia_examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

SET(HESTIA_SHARED ON)


add_subdirectory(external/hestia)

add_subdirectory(src)
