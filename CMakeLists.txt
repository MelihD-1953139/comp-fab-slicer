cmake_minimum_required(VERSION 3.15)
project(Slicer)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_executable(Slicer ${SOURCES} ${HEADERS})
target_include_directories(Slicer PRIVATE include)

add_subdirectory(vendor/assimp)
target_link_libraries(Slicer PRIVATE assimp)

add_subdirectory(vendor/Nexus)
target_link_libraries(Slicer PRIVATE Nexus)

