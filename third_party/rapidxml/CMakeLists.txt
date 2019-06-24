cmake_minimum_required(VERSION 3.7)

project(rapidxml VERSION 1.13 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_library(rapidxml INTERFACE)
target_include_directories(rapidxml INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/rapidxml>
)

