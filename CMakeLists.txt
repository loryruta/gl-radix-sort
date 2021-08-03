cmake_minimum_required(VERSION 3.19)
project(parallel_radix_sort)

set(CMAKE_CXX_STANDARD 17)

set(SRC
        src/main.cpp

        src/radix_sort.hpp
        src/radix_sort.cpp
        src/renderdoc.hpp
        src/renderdoc.cpp

        third_party/glad/src/glad.c
        third_party/glad/include/glad/glad.h
        third_party/renderdoc/renderdoc_app.h
        )

set(SHADERS
        resources/radix_sort__scan.comp
        resources/radix_sort__sum.comp
        resources/radix_sort__write.comp
        )

add_executable(parallel_radix_sort ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC third_party/glad/include)

# shaders
add_custom_target(
        shaders
        COMMAND ${CMAKE_COMMAND} -E rm -r "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        DEPENDS ${SHADERS}

        COMMENT "Copying shaders..."
)
add_dependencies(${PROJECT_NAME} shaders)

# GLFW
add_subdirectory(third_party/glfw)

target_include_directories(${PROJECT_NAME} PUBLIC third_party/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)

# Renderdoc
target_include_directories(${PROJECT_NAME} PUBLIC third_party/renderdoc)
