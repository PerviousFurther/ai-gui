#include <aigui/app/application.hpp>
#include <aigui/core/platform.hpp>

#ifdef AIGUI_PLATFORM_WINDOWS
#   include "../platform/windows/dx12_window.hpp"
#endif
#ifdef AIGUI_PLATFORM_LINUX
#   include "../platform/linux/vulkan_window.hpp"
#endif

#include <stdexcept>
#include <algorithm>

namespace aigui {

Application* Application::instance_ = nullptr;

Application::Application(int /*argc*/, char** /*argv*/) {
    if (instance_) throw std::logic_error("Only one Application may exist at a time");
    instance_ = this;
}

Application::~Application() {
    instance_ = nullptr;
}

std::shared_ptr<Window> Application::createWindow(const WindowConfig& config) {
    std::shared_ptr<Window> win;

#ifdef AIGUI_PLATFORM_WINDOWS
    win = std::make_shared<Win32Window>();
#elif defined(AIGUI_PLATFORM_LINUX)
    win = std::make_shared<X11VulkanWindow>();
#else
    throw std::runtime_error("No platform window implementation available");
#endif

    if (!win->open(config))
        throw std::runtime_error("Failed to open window: " + config.title);

    windows_.push_back(win);
    return win;
}

int Application::run() {
    running_ = true;
    while (running_) {
        bool anyOpen = false;
        for (auto& win : windows_) {
            if (!win->isOpen()) continue;
            anyOpen = true;
            if (!win->pollEvents()) continue;

            // Render frame.
            auto& c = win->canvas();
            // Determine actual window size (simplified – use initial size).
            c.beginFrame(1280, 720);
            // root widget rendering is driven by Window::render().
            c.endFrame();
            win->present();
        }
        // Remove closed windows.
        windows_.erase(
            std::remove_if(windows_.begin(), windows_.end(),
                           [](const auto& w){ return !w->isOpen(); }),
            windows_.end());

        if (!anyOpen) break;
    }
    return exitCode_;
}

void Application::quit(int exitCode) {
    exitCode_ = exitCode;
    running_  = false;
}

} // namespace aigui
