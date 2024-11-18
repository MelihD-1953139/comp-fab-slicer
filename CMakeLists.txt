cmake_minimum_required(VERSION 3.15)
project(Slicer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_executable(Slicer ${SOURCES} ${HEADERS})
target_include_directories(Slicer PUBLIC include)

FIND_PACKAGE(assimp 5.4 REQUIRED)
IF(assimp_FOUND)
  MESSAGE(STATUS "assimp found")
  target_include_directories(Slicer PUBLIC ${ASSIMP_INCLUDE_DIRS})
  target_link_libraries(Slicer ${ASSIMP_LIBRARIES})
ELSE()
  MESSAGE(FATAL_ERROR "assimp not found")
ENDIF()

add_subdirectory(vendor/Nexus)
target_link_libraries(Slicer Nexus)

add_subdirectory(vendor/Clipper2/CPP)
target_link_libraries(Slicer Clipper2)

