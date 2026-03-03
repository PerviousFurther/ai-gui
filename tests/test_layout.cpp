/// Unit tests for the layout engine (Constraints, Column, Row, Container, etc.)
/// Uses the SoftwareCanvas to verify draw-call generation without a real GPU.

#include "../include/aigui/aigui.hpp"
#include "../src/renderer/software_canvas.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace aigui;

static int passed = 0;
static int failed = 0;

#define EXPECT_EQ(a, b) do { \
    if ((a) == (b)) { ++passed; } \
    else { ++failed; \
        std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                  << "  " << #a << " != " << #b << "\n"; } \
} while(0)

#define EXPECT_NEAR(a, b, eps) do { \
    if (std::fabs(static_cast<float>(a) - static_cast<float>(b)) <= (eps)) { ++passed; } \
    else { ++failed; \
        std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                  << "  |" << #a << " - " << #b << "| > " << (eps) \
                  << "  (got " << (a) << " vs " << (b) << ")\n"; } \
} while(0)

#define EXPECT_TRUE(x) EXPECT_EQ(true, (x))
#define EXPECT_FALSE(x) EXPECT_EQ(false, (x))

// ── Constraint tests ──────────────────────────────────────────────────────────

void test_constraints_tight() {
    auto c = Constraints::tight(100.0f, 200.0f);
    EXPECT_EQ(c.minWidth,  100.0f);
    EXPECT_EQ(c.maxWidth,  100.0f);
    EXPECT_EQ(c.minHeight, 200.0f);
    EXPECT_EQ(c.maxHeight, 200.0f);
    EXPECT_TRUE(c.isTight());
}

void test_constraints_loose() {
    auto c = Constraints::loose(400.0f, 300.0f);
    EXPECT_EQ(c.minWidth,  0.0f);
    EXPECT_EQ(c.maxWidth,  400.0f);
    EXPECT_EQ(c.minHeight, 0.0f);
    EXPECT_EQ(c.maxHeight, 300.0f);
    EXPECT_FALSE(c.isTight());
}

void test_constraints_constrain() {
    auto c = Constraints{50, 200, 50, 200};
    Size s1 = c.constrain({30, 30});   // below min → clamped to min
    EXPECT_EQ(s1.width,  50.0f);
    EXPECT_EQ(s1.height, 50.0f);

    Size s2 = c.constrain({250, 250}); // above max → clamped to max
    EXPECT_EQ(s2.width,  200.0f);
    EXPECT_EQ(s2.height, 200.0f);

    Size s3 = c.constrain({100, 100}); // within range
    EXPECT_EQ(s3.width,  100.0f);
    EXPECT_EQ(s3.height, 100.0f);
}

void test_constraints_deflate() {
    auto c  = Constraints{0, 400, 0, 300};
    auto c2 = c.deflate(20, 10);  // removes 20 from width, 10 from height
    EXPECT_EQ(c2.maxWidth,  380.0f);
    EXPECT_EQ(c2.maxHeight, 290.0f);
}

// ── Core type tests ───────────────────────────────────────────────────────────

void test_color() {
    auto c = Color::fromRGBA(0xFF8040FFu);
    EXPECT_EQ(c.r, 0xFF);
    EXPECT_EQ(c.g, 0x80);
    EXPECT_EQ(c.b, 0x40);
    EXPECT_EQ(c.a, 0xFF);
    EXPECT_EQ(c.toRGBA(), 0xFF8040FFu);

    auto w = Color::white();
    EXPECT_EQ(w.r, 255);
    EXPECT_EQ(w.g, 255);
    EXPECT_EQ(w.b, 255);

    EXPECT_NEAR(Color::red().rf(), 1.0f, 1e-3f);
    EXPECT_NEAR(Color::red().gf(), 0.0f, 1e-3f);
}

void test_rect() {
    Rect r{10, 20, 100, 50};
    EXPECT_EQ(r.right(),  110.0f);
    EXPECT_EQ(r.bottom(), 70.0f);
    EXPECT_TRUE(r.contains({50, 40}));
    EXPECT_FALSE(r.contains({5, 5}));

    Rect r2 = r.translated(5, -5);
    EXPECT_EQ(r2.x, 15.0f);
    EXPECT_EQ(r2.y, 15.0f);
}

void test_size() {
    Size s{100, 200};
    EXPECT_EQ(s.area(), 20000.0f);

    Size clamped = s.clamp({50, 50}, {150, 150});
    EXPECT_EQ(clamped.width,  100.0f);
    EXPECT_EQ(clamped.height, 150.0f);
}

// ── Widget layout tests ───────────────────────────────────────────────────────

void test_container_layout() {
    auto text = std::make_shared<Text>("Hello");
    auto cont = std::make_shared<Container>(text);
    cont->withPadding(EdgeInsets::all(10))
        .withDecoration(Decoration::filled(Color::blue()));

    Constraints c = Constraints::loose(500, 500);
    Size s = cont->layout(c);

    // Container width = text width + 2*padding
    EXPECT_TRUE(s.width  > 20.0f);
    EXPECT_TRUE(s.height > 20.0f);
}

void test_column_stacks_vertically() {
    std::vector<WidgetPtr> children = {
        std::make_shared<Text>("Line 1"),
        std::make_shared<Text>("Line 2"),
        std::make_shared<Text>("Line 3"),
    };

    auto col = std::make_shared<Column>(children);
    Constraints c = Constraints::loose(400, 600);
    Size s = col->layout(c);

    // Total height >= sum of children heights.
    float childH = children[0]->size().height;
    EXPECT_TRUE(s.height >= childH * 3.0f - 1e-3f);

    // Children are stacked vertically – y offsets should increase.
    EXPECT_EQ(children[0]->offset().y, 0.0f);
    EXPECT_TRUE(children[1]->offset().y >= children[0]->size().height - 1e-3f);
    EXPECT_TRUE(children[2]->offset().y >= children[1]->offset().y +
                                           children[1]->size().height - 1e-3f);
}

void test_row_lays_out_horizontally() {
    std::vector<WidgetPtr> children = {
        std::make_shared<Text>("A"),
        std::make_shared<Text>("B"),
    };

    auto row = std::make_shared<Row>(children);
    Constraints c = Constraints::loose(800, 600);
    Size s = row->layout(c);

    EXPECT_TRUE(s.width > 0.0f);
    EXPECT_EQ(children[0]->offset().x, 0.0f);
    EXPECT_TRUE(children[1]->offset().x >= children[0]->size().width - 1e-3f);
}

void test_center_widget() {
    auto text = std::make_shared<Text>("Center");
    auto center = std::make_shared<Center>(text);

    Constraints c = Constraints::tight(800.0f, 600.0f);
    center->layout(c);

    float cx = center->size().width  * 0.5f - text->size().width  * 0.5f;
    float cy = center->size().height * 0.5f - text->size().height * 0.5f;
    EXPECT_NEAR(text->offset().x, cx, 1.0f);
    EXPECT_NEAR(text->offset().y, cy, 1.0f);
}

void test_sized_box() {
    auto box = std::make_shared<SizedBox>(200, 100);
    Constraints c = Constraints::loose(800, 600);
    Size s = box->layout(c);
    EXPECT_EQ(s.width,  200.0f);
    EXPECT_EQ(s.height, 100.0f);
}

void test_padding_widget() {
    auto text = std::make_shared<Text>("Padded");
    auto pad  = std::make_shared<Padding>(EdgeInsets::all(16), text);

    Constraints c = Constraints::loose(500, 500);
    Size s = pad->layout(c);

    EXPECT_TRUE(s.width  >= text->size().width  + 32.0f - 1e-3f);
    EXPECT_TRUE(s.height >= text->size().height + 32.0f - 1e-3f);
    EXPECT_EQ(text->offset().x, 16.0f);
    EXPECT_EQ(text->offset().y, 16.0f);
}

// ── SoftwareCanvas rendering tests ───────────────────────────────────────────

void test_software_canvas_records_commands() {
    SoftwareCanvas canvas;
    canvas.beginFrame(800, 600);

    canvas.drawRect({10, 10, 100, 50}, Color::red());
    canvas.drawText("hello", {10, 70, 200, 30}, Font{}, Color::black());

    canvas.endFrame();

    EXPECT_EQ(canvas.countOf(DrawCommand::Kind::DrawRect), 1u);
    EXPECT_EQ(canvas.countOf(DrawCommand::Kind::DrawText), 1u);
    EXPECT_EQ(canvas.countOf(DrawCommand::Kind::BeginFrame), 1u);
    EXPECT_EQ(canvas.countOf(DrawCommand::Kind::EndFrame), 1u);
}

void test_software_canvas_translate() {
    SoftwareCanvas canvas;
    canvas.beginFrame(800, 600);
    canvas.pushTranslate(100, 50);
    canvas.drawRect({0, 0, 10, 10}, Color::blue());
    canvas.popTransform();
    canvas.endFrame();

    // The recorded rect should have its origin translated.
    const auto& cmds = canvas.commands();
    for (const auto& cmd : cmds) {
        if (cmd.kind == DrawCommand::Kind::DrawRect) {
            EXPECT_NEAR(cmd.rect.x, 100.0f, 1e-3f);
            EXPECT_NEAR(cmd.rect.y,  50.0f, 1e-3f);
        }
    }
}

void test_software_canvas_rasterizes_pixel() {
    SoftwareCanvas canvas;
    canvas.beginFrame(100, 100);
    canvas.drawRect({10, 10, 20, 20}, Color::red());
    canvas.endFrame();

    // Pixels inside the rectangle should be red.
    EXPECT_EQ(canvas.pixelAt(15, 15), Color::red());
    // Pixels outside should still be transparent.
    EXPECT_EQ(canvas.pixelAt(5, 5), Color::transparent());
}

// ── Widget paint integration test ─────────────────────────────────────────────

void test_container_paint_emits_draw_rect() {
    SoftwareCanvas canvas;
    canvas.beginFrame(800, 600);

    auto cont = std::make_shared<Container>();
    cont->withDecoration(Decoration::filled(Color::green()))
        .withSize(200, 100);

    cont->layout(Constraints::tight(200, 100));

    canvas.pushTranslate(0, 0);
    cont->paint(canvas);
    canvas.popTransform();

    canvas.endFrame();

    // Container should have emitted at least one DrawRoundedRect.
    EXPECT_TRUE(canvas.countOf(DrawCommand::Kind::DrawRoundedRect) >= 1u);
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    test_constraints_tight();
    test_constraints_loose();
    test_constraints_constrain();
    test_constraints_deflate();

    test_color();
    test_rect();
    test_size();

    test_container_layout();
    test_column_stacks_vertically();
    test_row_lays_out_horizontally();
    test_center_widget();
    test_sized_box();
    test_padding_widget();

    test_software_canvas_records_commands();
    test_software_canvas_translate();
    test_software_canvas_rasterizes_pixel();
    test_container_paint_emits_draw_rect();

    std::cout << "Layout tests: " << passed << " passed, "
              << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
