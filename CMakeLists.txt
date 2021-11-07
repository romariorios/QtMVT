cmake_minimum_required(VERSION 3.12)
project(examplesuite)

find_package(Qt5 5.12.0 COMPONENTS Core Gui Widgets)
include_directories(${CMAKE_SOURCE_DIR})

add_executable(examplesuite examplesuite.cpp)
target_link_libraries(examplesuite Qt5::Core Qt5::Gui Qt5::Widgets)
set_target_properties(examplesuite PROPERTIES
  CMAKE_CXX_STANDARD 11
  CMAKE_CXX_STANDARD_REQUIRED TRUE
  AUTOUIC ON)

