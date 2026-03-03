cmake_minimum_required(VERSION 3.20)

# Common include paths for all tests (header-only, no platform back-end needed)
set(AIGUI_TEST_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# ── Binding tests ──────────────────────────────────────────────────────────────
add_executable(test_binding test_binding.cpp)
target_include_directories(test_binding PRIVATE ${AIGUI_TEST_INCLUDES})
target_compile_features(test_binding PRIVATE cxx_std_17)
add_test(NAME binding_tests COMMAND test_binding)

# ── Layout / widget tests ──────────────────────────────────────────────────────
add_executable(test_layout test_layout.cpp)
target_include_directories(test_layout PRIVATE ${AIGUI_TEST_INCLUDES})
target_compile_features(test_layout PRIVATE cxx_std_17)
add_test(NAME layout_tests COMMAND test_layout)
