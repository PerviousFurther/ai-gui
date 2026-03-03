#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility>

namespace aigui {

/// A change-notification token.  Destroy it to unsubscribe.
class SubscriptionToken {
public:
    using CancelFn = std::function<void()>;

    SubscriptionToken() = default;
    explicit SubscriptionToken(CancelFn cancel) : cancel_(std::move(cancel)) {}

    ~SubscriptionToken() { if (cancel_) cancel_(); }

    // Move-only
    SubscriptionToken(SubscriptionToken&&) = default;
    SubscriptionToken& operator=(SubscriptionToken&&) = default;
    SubscriptionToken(const SubscriptionToken&) = delete;
    SubscriptionToken& operator=(const SubscriptionToken&) = delete;

private:
    CancelFn cancel_;
};

/// Observable<T>: holds a value and broadcasts changes (XAML-style property).
///
/// Usage:
///   Observable<int> count{0};
///   auto token = count.subscribe([](int v){ std::cout << v; });
///   count.set(42);   // subscriber called with 42
///   count = 100;     // same via operator=
template<typename T>
class Observable {
public:
    using Listener   = std::function<void(const T&)>;
    using ListenerID = uint64_t;

    Observable() = default;
    explicit Observable(T value) : value_(std::move(value)) {}

    /// Read current value.
    const T& get() const { return value_; }
    operator const T&() const { return value_; }

    /// Write a new value (notifies listeners only if value changed).
    void set(T value) {
        if (!(value_ == value)) {
            value_ = std::move(value);
            notify();
        }
    }
    Observable& operator=(T value) { set(std::move(value)); return *this; }

    /// Force notification even if value is unchanged.
    void forceNotify() { notify(); }

    /// Subscribe to value changes.  Returns an RAII token.
    [[nodiscard]] SubscriptionToken subscribe(Listener listener) {
        ListenerID id = nextID_++;
        listeners_.push_back({id, std::move(listener)});

        // Capture weak pointer to listener list so the cancel is safe even
        // after the Observable is moved or destroyed.
        auto* self = this;
        return SubscriptionToken([self, id]() {
            auto& ls = self->listeners_;
            ls.erase(std::remove_if(ls.begin(), ls.end(),
                                    [id](const Entry& e) { return e.id == id; }),
                     ls.end());
        });
    }

    /// One-way bind: whenever `source` changes, this observable updates too.
    [[nodiscard]] SubscriptionToken bindFrom(Observable<T>& source) {
        set(source.get());
        return source.subscribe([this](const T& v) { set(v); });
    }

    /// Two-way bind: keeps this and `other` in sync.
    /// Returns a pair of tokens – keep both alive.
    std::pair<SubscriptionToken, SubscriptionToken>
    bindTwoWay(Observable<T>& other) {
        set(other.get());
        auto t1 = other.subscribe([this](const T& v) { set(v); });
        auto t2 = subscribe([&other](const T& v) { other.set(v); });
        return {std::move(t1), std::move(t2)};
    }

private:
    T value_{};

    struct Entry {
        ListenerID id;
        Listener   fn;
    };
    std::vector<Entry> listeners_;
    ListenerID nextID_{0};

    void notify() {
        // Snapshot to allow re-entrant mutations.
        auto snapshot = listeners_;
        for (auto& e : snapshot) e.fn(value_);
    }
};

} // namespace aigui
