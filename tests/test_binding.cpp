/// Unit tests for the Observable<T> data-binding system.
/// Run with: ctest -R test_binding  (after cmake build)

#include "../include/aigui/binding/observable.hpp"
#include "../include/aigui/binding/property.hpp"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>

using namespace aigui;

// ── Helpers ───────────────────────────────────────────────────────────────────

static int passed = 0;
static int failed = 0;

#define EXPECT_EQ(a, b) do { \
    if ((a) == (b)) { ++passed; } \
    else { ++failed; \
        std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                  << "  " << #a << " != " << #b << "\n"; } \
} while(0)

#define EXPECT_TRUE(x) EXPECT_EQ(true, (x))
#define EXPECT_FALSE(x) EXPECT_EQ(false, (x))

// ─────────────────────────────────────────────────────────────────────────────

void test_basic_get_set() {
    Observable<int> x{42};
    EXPECT_EQ(x.get(), 42);

    x.set(100);
    EXPECT_EQ(x.get(), 100);

    // Operator=
    x = 7;
    EXPECT_EQ(static_cast<const int&>(x), 7);
}

void test_listener_called_on_change() {
    Observable<int> x{0};
    int lastValue = -1;
    auto token = x.subscribe([&](const int& v){ lastValue = v; });

    x.set(5);
    EXPECT_EQ(lastValue, 5);

    x.set(10);
    EXPECT_EQ(lastValue, 10);
}

void test_listener_not_called_if_no_change() {
    Observable<int> x{42};
    int callCount = 0;
    auto token = x.subscribe([&](const int&){ ++callCount; });

    x.set(42); // same value → no notification
    EXPECT_EQ(callCount, 0);

    x.set(43);
    EXPECT_EQ(callCount, 1);
}

void test_token_unsubscribes_on_destroy() {
    Observable<int> x{0};
    int callCount = 0;
    {
        auto token = x.subscribe([&](const int&){ ++callCount; });
        x.set(1);
        EXPECT_EQ(callCount, 1);
    } // token destroyed → unsubscribed
    x.set(2);
    EXPECT_EQ(callCount, 1); // no more calls
}

void test_multiple_listeners() {
    Observable<std::string> s{"hello"};
    std::vector<std::string> log;
    auto t1 = s.subscribe([&](const std::string& v){ log.push_back("A:" + v); });
    auto t2 = s.subscribe([&](const std::string& v){ log.push_back("B:" + v); });

    s.set("world");
    EXPECT_EQ(log.size(), static_cast<size_t>(2));
    EXPECT_EQ(log[0], "A:world");
    EXPECT_EQ(log[1], "B:world");
}

void test_one_way_bind() {
    Observable<int> src{10};
    Observable<int> dst{0};

    auto token = dst.bindFrom(src);
    EXPECT_EQ(dst.get(), 10);

    src.set(99);
    EXPECT_EQ(dst.get(), 99);

    // dst changes don't flow back.
    dst.set(1);
    EXPECT_EQ(src.get(), 99);
}

void test_two_way_bind() {
    Observable<int> a{1};
    Observable<int> b{2};

    auto [t1, t2] = a.bindTwoWay(b);
    // After binding, a adopts b's value.
    EXPECT_EQ(a.get(), 2);

    b.set(50);
    EXPECT_EQ(a.get(), 50);

    a.set(99);
    EXPECT_EQ(b.get(), 99);
}

void test_computed_property() {
    Observable<int> x{2};
    Observable<int> y{3};
    ComputedProperty<int, int, int> sum([&]{ return x.get() + y.get(); }, x, y);

    EXPECT_EQ(sum.get(), 5);

    x = 10;
    EXPECT_EQ(sum.get(), 13);

    y = 7;
    EXPECT_EQ(sum.get(), 17);
}

void test_force_notify() {
    Observable<int> x{0};
    int calls = 0;
    auto token = x.subscribe([&](const int&){ ++calls; });

    x.forceNotify();
    EXPECT_EQ(calls, 1);

    x.forceNotify();
    EXPECT_EQ(calls, 2);
}

void test_observable_string() {
    Observable<std::string> s;
    EXPECT_EQ(s.get(), std::string{});

    s = "test";
    EXPECT_EQ(s.get(), "test");
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    test_basic_get_set();
    test_listener_called_on_change();
    test_listener_not_called_if_no_change();
    test_token_unsubscribes_on_destroy();
    test_multiple_listeners();
    test_one_way_bind();
    test_two_way_bind();
    test_computed_property();
    test_force_notify();
    test_observable_string();

    std::cout << "Binding tests: " << passed << " passed, "
              << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
