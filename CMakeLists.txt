cmake_minimum_required(VERSION 3.19)
project(parallel_radix_sort)

set(CMAKE_CXX_STANDARD 17)

find_package(Python COMPONENTS Interpreter)
if ((NOT Python_FOUND) OR (NOT Python_Interpreter_FOUND))
    message(FATAL_ERROR "Python interpreter isn't found, it is required to build.")
endif()

set(SRC
        src/main.cpp
        src/radix_sort.hpp
        src/renderdoc.hpp
        src/renderdoc.cpp
        src/sort_test.hpp
        src/sort_test.cpp

        generated/radix_sort.hpp

        third_party/glad/src/glad.c
        third_party/glad/include/glad/glad.h
        third_party/renderdoc/renderdoc_app.h
        )

set(SHADERS
        resources/radix_sort_count.comp
        resources/radix_sort_local_offsets.comp
        resources/radix_sort_reorder.comp
        resources/sort_test_arr_compare.comp
        resources/sort_test_check_sorted.comp
        resources/sort_test_count_elements.comp
    )

add_executable(parallel_radix_sort ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC third_party/glad/include)

# Shaders
add_custom_target(
        shaders
        COMMAND ${CMAKE_COMMAND} -E rm -r "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        DEPENDS ${SHADERS}

        COMMENT "Copying shaders..."
)
add_dependencies(${PROJECT_NAME} shaders)

add_custom_command(
        OUTPUT generated/radix_sort.hpp
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shader_injector.py ${CMAKE_CURRENT_SOURCE_DIR}/src/radix_sort.hpp ${CMAKE_CURRENT_SOURCE_DIR}/generated/radix_sort.hpp
)

# GLFW
add_subdirectory(third_party/glfw)

target_include_directories(${PROJECT_NAME} PUBLIC third_party/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)

# Renderdoc
target_include_directories(${PROJECT_NAME} PUBLIC third_party/renderdoc)
