cmake_minimum_required(VERSION 3.20)

add_executable(hello_world main.cpp)

target_link_libraries(hello_world PRIVATE aigui)

target_include_directories(hello_world PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Designate as GUI application on Windows (no console window).
if(WIN32)
    set_target_properties(hello_world PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
