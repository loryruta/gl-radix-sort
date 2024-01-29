set(SHADERS
    resources/radix_sort_count.comp.glsl
    resources/radix_sort_local_offsets.comp.glsl
    resources/radix_sort_reorder.comp.glsl
    resources/sort_test_arr_compare.comp.glsl
    resources/sort_test_check_sorted.comp.glsl
    resources/sort_test_count_elements.comp.glsl
    )

# Shaders
#add_custom_target(
#        shaders
#        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/"
#        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
#        DEPENDS ${SHADERS}
#
#        COMMENT "Copying shaders..."
#)
#add_dependencies(glu_lib shaders)

add_custom_command(
    OUTPUT generated/radix_sort.hpp
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shader_injector.py ${CMAKE_CURRENT_SOURCE_DIR}/src/radix_sort.hpp ${CMAKE_CURRENT_SOURCE_DIR}/generated/radix_sort.hpp
    )
