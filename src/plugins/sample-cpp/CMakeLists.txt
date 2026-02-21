cmake_minimum_required(VERSION 3.12)
project(cutter-sample-plugin)

set(CMAKE_CXX_STANDARD 20)

find_package(Cutter REQUIRED)
find_package(Rizin REQUIRED)
set(CUTTER_INSTALL_PLUGDIR "${Cutter_USER_PLUGINDIR}" CACHE STRING "Directory to install Cutter plugin into")

set(CMAKE_AUTOMOC ON)

add_library(sample_plugin MODULE
    CutterSamplePlugin.h
    CutterSamplePlugin.cpp)
target_link_libraries(sample_plugin PRIVATE Cutter::Cutter Rizin::Core)
install(TARGETS sample_plugin DESTINATION "${CUTTER_INSTALL_PLUGDIR}")
