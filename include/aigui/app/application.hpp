#pragma once
#include "window.hpp"
#include "../core/platform.hpp"
#include <memory>
#include <string>
#include <functional>

namespace aigui {

/// Entry point for an AI-GUI application.
///
/// Usage:
///   int main(int argc, char** argv) {
///       aigui::Application app(argc, argv);
///       auto win = app.createWindow({.title="Hello", .width=800, .height=600});
///       win->setRoot(std::make_shared<MyRootWidget>());
///       return app.run();
///   }
class Application {
public:
    Application(int argc, char** argv);
    ~Application();

    // Non-copyable, non-movable.
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    /// Create a platform window (Win32+DX12 or X11+Vulkan).
    std::shared_ptr<Window> createWindow(const WindowConfig& config = WindowConfig{});

    /// Run the event loop.  Returns the exit code when the last window closes.
    int run();

    /// Request the application to quit after the current frame.
    void quit(int exitCode = 0);

    /// Global app instance (set by the constructor).
    static Application* instance() { return instance_; }

private:
    std::vector<std::shared_ptr<Window>> windows_;
    bool   running_{false};
    int    exitCode_{0};

    static Application* instance_;
};

} // namespace aigui
