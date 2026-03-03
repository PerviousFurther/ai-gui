#pragma once
#include "observable.hpp"
#include <functional>

namespace aigui {

/// A computed / derived property.  Value is lazily re-evaluated whenever
/// any source Observable it depends upon changes.
///
/// Usage:
///   Observable<int>    a{2}, b{3};
///   ComputedProperty<int> sum([&]{ return a.get() + b.get(); }, a, b);
///   sum.get();   // → 5
///   a = 10;
///   sum.get();   // → 13
template<typename T, typename... Sources>
class ComputedProperty {
public:
    using ComputeFn = std::function<T()>;

    ComputedProperty(ComputeFn compute, Observable<Sources>&... sources)
        : compute_(std::move(compute))
        , value_(compute_())
    {
        // Subscribe to all source observables; re-compute on any change.
        tokens_ = subscribeAll(sources...);
    }

    const T& get() const { return value_; }
    operator const T&() const { return value_; }

    /// Allow downstream subscription.
    [[nodiscard]] SubscriptionToken subscribe(typename Observable<T>::Listener listener) {
        return inner_.subscribe(std::move(listener));
    }

private:
    ComputeFn              compute_;
    T                      value_;
    Observable<T>          inner_{value_};
    std::vector<SubscriptionToken> tokens_;

    void recompute() {
        value_ = compute_();
        inner_.set(value_);
    }

    // Variadic helper to subscribe to all sources.
    template<typename S, typename... Rest>
    std::vector<SubscriptionToken> subscribeAll(Observable<S>& src, Observable<Rest>&... rest) {
        std::vector<SubscriptionToken> ts;
        ts.push_back(src.subscribe([this](const S&) { recompute(); }));
        if constexpr (sizeof...(rest) > 0) {
            auto more = subscribeAll(rest...);
            for (auto& t : more) ts.push_back(std::move(t));
        }
        return ts;
    }
    std::vector<SubscriptionToken> subscribeAll() { return {}; }
};

/// Convenience alias for a single-source computed property.
template<typename T, typename Source>
using DerivedProperty = ComputedProperty<T, Source>;

} // namespace aigui
