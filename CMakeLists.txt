cmake_minimum_required(VERSION 3.20)
project(ai-gui VERSION 0.1.0 LANGUAGES CXX)

# ── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Platform detection ────────────────────────────────────────────────────────
if(WIN32)
    set(AIGUI_PLATFORM_WINDOWS ON)
    message(STATUS "Platform: Windows  →  DirectX 12 renderer")
elseif(UNIX AND NOT APPLE)
    set(AIGUI_PLATFORM_LINUX ON)
    message(STATUS "Platform: Linux    →  Vulkan renderer")
else()
    message(FATAL_ERROR "Unsupported platform (only Windows and Linux are supported)")
endif()

# ── Library sources ───────────────────────────────────────────────────────────
set(AIGUI_HEADERS
    # Core
    include/aigui/core/platform.hpp
    include/aigui/core/color.hpp
    include/aigui/core/point.hpp
    include/aigui/core/size.hpp
    include/aigui/core/rect.hpp
    include/aigui/core/constraints.hpp
    # Binding
    include/aigui/binding/observable.hpp
    include/aigui/binding/property.hpp
    # Renderer abstraction
    include/aigui/renderer/canvas.hpp
    # Widget
    include/aigui/widget/widget.hpp
    include/aigui/widget/stateful_widget.hpp
    # Concrete widgets
    include/aigui/widgets/text.hpp
    include/aigui/widgets/container.hpp
    include/aigui/widgets/flex.hpp
    include/aigui/widgets/button.hpp
    include/aigui/widgets/layout_widgets.hpp
    # App
    include/aigui/app/window.hpp
    include/aigui/app/application.hpp
    include/aigui/aigui.hpp
)

set(AIGUI_SOURCES
    src/app/application.cpp
)

# Platform back-end sources
if(AIGUI_PLATFORM_WINDOWS)
    list(APPEND AIGUI_SOURCES
        src/platform/windows/dx12_window.cpp
    )
elseif(AIGUI_PLATFORM_LINUX)
    list(APPEND AIGUI_SOURCES
        src/platform/linux/vulkan_window.cpp
    )
endif()

# ── Static library target ─────────────────────────────────────────────────────
add_library(aigui STATIC ${AIGUI_SOURCES} ${AIGUI_HEADERS})

target_include_directories(aigui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ── Platform-specific link dependencies ───────────────────────────────────────
if(AIGUI_PLATFORM_WINDOWS)
    target_compile_definitions(aigui PRIVATE AIGUI_BUILD_DLL)
    target_link_libraries(aigui PUBLIC
        d3d12 dxgi dxguid d3dcompiler
    )
    # Require Windows 10+
    target_compile_definitions(aigui PUBLIC
        WINVER=0x0A00 _WIN32_WINNT=0x0A00
    )
elseif(AIGUI_PLATFORM_LINUX)
    find_package(Vulkan QUIET)
    find_package(X11 QUIET)
    if(Vulkan_FOUND AND X11_FOUND)
        target_link_libraries(aigui PUBLIC Vulkan::Vulkan ${X11_LIBRARIES})
        target_compile_definitions(aigui PRIVATE AIGUI_HAVE_VULKAN AIGUI_HAVE_X11)
        message(STATUS "Vulkan + X11 found – full Linux renderer enabled")
    else()
        message(STATUS "Vulkan or X11 not found – platform renderer disabled (tests still work)")
        # Remove the platform back-end source that needs Vulkan/X11.
        get_target_property(_srcs aigui SOURCES)
        list(REMOVE_ITEM _srcs src/platform/linux/vulkan_window.cpp)
        set_target_properties(aigui PROPERTIES SOURCES "${_srcs}")
    endif()
endif()

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(aigui PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(aigui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(AIGUI_BUILD_TESTS "Build unit tests" ON)
if(AIGUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Examples ──────────────────────────────────────────────────────────────────
option(AIGUI_BUILD_EXAMPLES "Build example applications" ON)
if(AIGUI_BUILD_EXAMPLES)
    add_subdirectory(examples/hello_world)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS aigui EXPORT aigui-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/aigui DESTINATION include)
install(EXPORT aigui-targets
    FILE aigui-config.cmake
    DESTINATION lib/cmake/aigui
)
