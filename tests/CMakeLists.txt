
enable_testing()

add_executable(tests "tests_liquid.cpp" "main.cpp")
add_dependencies(tests processor)
target_include_directories(tests PUBLIC "../include")
target_link_libraries(tests processor)
