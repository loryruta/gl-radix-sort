cmake_minimum_required(VERSION 3.19)
project(glu VERSION 2.0 DESCRIPTION "GL Utilities by loryruta")

set(CMAKE_CXX_STANDARD 17)

# Catch2
add_subdirectory(third_party/Catch2)

# GLFW
add_subdirectory(third_party/glfw)

# glad
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE third_party/glm)

# Renderdoc
add_library(renderdoc INTERFACE)
target_include_directories(renderdoc INTERFACE third_party/renderdoc)

#
add_library(glu INTERFACE)
target_include_directories(glu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(test)
