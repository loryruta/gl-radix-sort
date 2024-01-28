add_executable(glu_test
    main.cpp
    reduce_tests.cpp
    blelloch_scan_tests.cpp
    radix_sort_tests.cpp
)

target_link_libraries(glu_test PRIVATE glu_hlib)

target_link_libraries(glu_test PRIVATE Catch2::Catch2)
target_link_libraries(glu_test PRIVATE glad)
target_link_libraries(glu_test PRIVATE glm)
target_link_libraries(glu_test PRIVATE glfw)
target_link_libraries(glu_test PRIVATE renderdoc)
