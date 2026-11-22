cmake_minimum_required(VERSION 3.26)
project(Wulkan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WULKAN_TRACY "Enable Tracy profiler" OFF) # needs to be public if tracy is moved into its own lib
add_subdirectory("Wulkan")