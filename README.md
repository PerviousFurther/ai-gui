# ai-gui

A modern C++ cross-platform GPU-accelerated GUI library.

## Features

| Feature | Description |
|---|---|
| **Cross-platform** | Windows → DirectX 12 · Linux → Vulkan |
| **Self-draw engine** | All UI elements rendered by the GPU; pixel-perfect consistent style across platforms |
| **Declarative UI** | Flutter-inspired `Widget` / `StatelessWidget` / `StatefulWidget` composition model |
| **Data binding** | XAML-inspired `Observable<T>` with one-way / two-way binding and computed properties |
| **Minimal dependencies** | Windows: D3D12 + Win32 · Linux: Vulkan + X11 – no third-party UI toolkit required |
| **High performance** | GPU geometry batching, clip-rect scissoring, efficient constraint-based layout |

---

## Architecture Overview

```
include/aigui/
├── core/           Color · Point · Size · Rect · Constraints
├── binding/        Observable<T> · ComputedProperty<T>
├── renderer/       Canvas (abstract GPU canvas interface)
├── widget/         Widget · StatelessWidget · StatefulWidget
├── widgets/        Text · Button · Container · Padding
│                   Column · Row · Center · SizedBox · Stack · Expanded
└── app/            Application · Window (platform back-end selection)

src/
├── app/            Application (event-loop + window factory)
├── renderer/       SoftwareCanvas (headless, used in tests)
└── platform/
    ├── windows/    DX12Canvas + Win32Window   (DirectX 12)
    └── linux/      VulkanCanvas + X11Window   (Vulkan)
```

### Rendering Pipeline

```
Widget.build()          ← declarative composition (widget tree)
    ↓
Layout engine           ← constraint propagation (Flutter box model)
    ↓
Widget.paint(Canvas&)   ← platform-agnostic draw calls
    ↓
Canvas implementation   ← GPU command recording
    ↓
GPU (DX12 / Vulkan)     ← hardware-accelerated rasterization
```

### Data Binding

```cpp
Observable<int> count{0};

// One-way binding
auto token = label.bindFrom(count);

// Two-way binding
auto [t1, t2] = a.bindTwoWay(b);

// Computed / derived property
ComputedProperty<int, int, int> sum(
    [&]{ return x.get() + y.get(); }, x, y);

// Subscribe to changes
auto sub = count.subscribe([](const int& v){
    std::cout << "count changed to " << v << '\n';
});
```

### Declarative UI (Flutter-style)

```cpp
// Stateless widget – pure function of its config
class MyCard : public StatelessWidget {
    std::vector<WidgetPtr> build() override {
        return { std::make_shared<Container>(
            std::make_shared<Text>("Hello, AI-GUI!")
        )->withDecoration(Decoration::rounded(Color::blue(), 8.0f))
         .withPadding(EdgeInsets::all(16)) };
    }
};

// Stateful widget – mutable state with reactive rebuilds
class CounterState : public State {
    std::vector<WidgetPtr> build(const Constraints&) override {
        return { std::make_shared<Button>(
            "Clicked " + std::to_string(count_) + " times",
            [this]{ setState([this]{ ++count_; }); }) };
    }
    int count_{0};
};
```

---

## Building

### Requirements

| Platform | Toolchain | GPU SDK |
|---|---|---|
| Windows | MSVC 2022 or Clang-cl | Windows 10 SDK (D3D12) |
| Linux | GCC ≥ 11 or Clang ≥ 14 | Vulkan SDK + X11 headers |

### CMake

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Run tests
cd build && ctest --output-on-failure
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `AIGUI_BUILD_TESTS` | `ON` | Build unit tests |
| `AIGUI_BUILD_EXAMPLES` | `ON` | Build example applications |

---

## Hello World Example

```cpp
#include <aigui/aigui.hpp>
using namespace aigui;

int main(int argc, char** argv) {
    Application app(argc, argv);

    Observable<int>         count{0};
    Observable<std::string> label{"Click me!"};

    // Bind label text to count value
    auto token = count.subscribe([&](const int& v){
        label = "Clicked " + std::to_string(v) + " times";
    });

    auto win = app.createWindow({.title="Hello AI-GUI", .width=800, .height=600});

    win->setRoot(std::make_shared<Center>(
        std::make_shared<Column>(
            std::vector<WidgetPtr>{
                std::make_shared<Text>("AI-GUI Demo",
                    Font{"sans-serif", 32.f, FontWeight::Bold}, Color::white()),
                std::make_shared<SizedBox>(0, 24),
                std::make_shared<Button>(label.get(),
                    [&count]{ count = count.get() + 1; }),
            },
            MainAxisAlignment::Center,
            CrossAxisAlignment::Center
        )
    ));

    return app.run();
}
```

---

## License

MIT © PerviousFurther
