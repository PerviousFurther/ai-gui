/// AI-GUI Hello-World Example
/// ===========================
/// Demonstrates:
///   - Declarative UI composition (Flutter-style widgets)
///   - Data binding (XAML-style Observable<T>)
///   - Stateful widgets with reactive updates
///   - Column / Row / Center layout
///   - Container styling (color, border-radius, padding)

#include <aigui/aigui.hpp>
#include <iostream>
#include <string>
#include <memory>

using namespace aigui;

// ─────────────────────────────────────────────────────────────────────────────
// View-model: data-binding layer (similar to XAML view-model)
// ─────────────────────────────────────────────────────────────────────────────

struct CounterViewModel {
    Observable<int>         count{0};
    Observable<std::string> message{"Click the button!"};

    // Computed: double the count
    ComputedProperty<int, int> doubleCount{
        [this]{ return count.get() * 2; }, count};

    void increment() {
        count = count.get() + 1;
        if (count.get() == 1)
            message = "You clicked once!";
        else
            message = "Count is " + std::to_string(count.get());
    }

    void reset() {
        count   = 0;
        message = "Reset!";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Counter widget (stateful, bound to ViewModel)
// ─────────────────────────────────────────────────────────────────────────────

class CounterWidget : public StatefulWidget {
public:
    explicit CounterWidget(std::shared_ptr<CounterViewModel> vm)
        : vm_(std::move(vm)) {}

    std::unique_ptr<State> createState() override;

private:
    std::shared_ptr<CounterViewModel> vm_;
    friend class CounterState;
};

class CounterState : public State {
public:
    explicit CounterState(CounterWidget* owner) : owner_(owner) {}

    void initState() override {
        // Subscribe so the widget rebuilds whenever the count changes.
        token_ = owner_->vm_->count.subscribe([this](const int&) {
            setState([]{}); // mark dirty → repaint
        });
    }

    std::vector<WidgetPtr> build(const Constraints& constraints) override {
        auto& vm = *owner_->vm_;

        // --- Title bar ---
        auto title = std::make_shared<Container>(
            std::make_shared<Text>(
                "AI-GUI Counter Demo",
                Font{"sans-serif", 24.0f, FontWeight::Bold},
                Color::white()
            )
        );
        title->withPadding(EdgeInsets::symmetric(16, 20))
              .withDecoration(Decoration::filled(Color(30, 50, 120)));

        // --- Count display ---
        auto countText = std::make_shared<Container>(
            std::make_shared<Center>(
                std::make_shared<Text>(
                    std::to_string(vm.count.get()),
                    Font{"sans-serif", 64.0f, FontWeight::Bold},
                    Color::white()
                )
            )
        );
        countText->withSize(200, 120)
                  .withDecoration(Decoration::rounded(Color(50, 80, 180), 16.0f));

        // --- Message label ---
        auto msgLabel = std::make_shared<Text>(
            vm.message.get(),
            Font{"sans-serif", 16.0f},
            Color::lightGray()
        );

        // --- Double count info ---
        auto doubleLabel = std::make_shared<Text>(
            "Double: " + std::to_string(vm.doubleCount.get()),
            Font{"sans-serif", 14.0f},
            Color::gray()
        );

        // --- Buttons ---
        auto incrementBtn = std::make_shared<Button>(
            "  +  Increment  ",
            [&vm]{ vm.increment(); }
        );
        incrementBtn->withFont(Font{"sans-serif", 16.0f})
                     .withColors(Color(70, 130, 220),
                                 Color::white(),
                                 Color(90, 150, 240),
                                 Color(50, 110, 200));

        auto resetBtn = std::make_shared<Button>(
            "  Reset  ",
            [&vm]{ vm.reset(); }
        );
        resetBtn->withFont(Font{"sans-serif", 14.0f})
                 .withColors(Color(160, 40, 40),
                              Color::white(),
                              Color(180, 60, 60),
                              Color(140, 20, 20));

        auto buttons = std::make_shared<Row>(
            std::vector<WidgetPtr>{incrementBtn, resetBtn},
            MainAxisAlignment::Center,
            CrossAxisAlignment::Center
        );

        // --- Main body (vertical column, centered) ---
        auto body = std::make_shared<Column>(
            std::vector<WidgetPtr>{
                countText,
                std::make_shared<SizedBox>(0, 20),
                msgLabel,
                std::make_shared<SizedBox>(0, 8),
                doubleLabel,
                std::make_shared<SizedBox>(0, 32),
                buttons,
            },
            MainAxisAlignment::Center,
            CrossAxisAlignment::Center
        );

        auto centeredBody = std::make_shared<Container>(
            std::make_shared<Center>(body)
        );
        centeredBody->withDecoration(Decoration::filled(Color(20, 25, 50)));

        // --- Full layout: title + body ---
        auto root = std::make_shared<Column>(
            std::vector<WidgetPtr>{title, centeredBody},
            MainAxisAlignment::Start,
            CrossAxisAlignment::Stretch
        );

        return {root};
    }

private:
    CounterWidget*   owner_;
    SubscriptionToken token_;
};

std::unique_ptr<State> CounterWidget::createState() {
    return std::make_unique<CounterState>(this);
}

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    try {
        Application app(argc, argv);

        auto vm  = std::make_shared<CounterViewModel>();
        auto win = app.createWindow({
            .title    = "AI-GUI Hello World",
            .width    = 800,
            .height   = 600,
            .resizable= true,
        });

        win->setRoot(std::make_shared<CounterWidget>(vm));

        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << '\n';
        return 1;
    }
}
