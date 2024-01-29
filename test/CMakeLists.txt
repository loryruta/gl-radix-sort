add_executable(glu_test
    main.cpp
    reduce_tests.cpp
    blelloch_scan_tests.cpp
    radix_sort_tests.cpp

    # These source files test the correct generation of the dist/* files
    generated/test_include_BlellochScan.cpp
    generated/test_include_RadixSort.cpp
    generated/test_include_Reduce.cpp
)

target_link_libraries(glu_test PRIVATE glu)

target_link_libraries(glu_test PRIVATE Catch2::Catch2)
target_link_libraries(glu_test PRIVATE glad)
target_link_libraries(glu_test PRIVATE glm)
target_link_libraries(glu_test PRIVATE glfw)
target_link_libraries(glu_test PRIVATE renderdoc)
