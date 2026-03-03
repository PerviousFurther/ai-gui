#pragma once
#include "../core/constraints.hpp"
#include "../core/rect.hpp"
#include "../renderer/canvas.hpp"
#include <memory>
#include <vector>

namespace aigui {

class Widget;
using WidgetPtr = std::shared_ptr<Widget>;

/// Base class for all UI elements (Flutter-inspired).
///
/// Lifecycle:
///   1. build()  – returns child widgets (override in concrete classes).
///   2. layout() – compute and store our own Size given parent Constraints.
///   3. paint()  – emit draw calls onto the Canvas.
class Widget {
public:
    virtual ~Widget() = default;

    // ── Widget tree ───────────────────────────────────────────────────────

    /// Return child widgets.  Stateless widgets override this to compose
    /// their UI from other widgets; leaf widgets return an empty vector.
    virtual std::vector<WidgetPtr> build() { return {}; }

    // ── Layout ────────────────────────────────────────────────────────────

    /// Compute the widget's own size given parent constraints.
    /// Implementations must store the result in size_ and position children.
    virtual Size layout(const Constraints& constraints) = 0;

    /// Return the size computed by the last layout() call.
    Size size() const { return size_; }

    /// Position of this widget relative to its parent (set by parent during layout).
    Point offset() const { return offset_; }
    void  setOffset(const Point& p) { offset_ = p; }

    // ── Painting ──────────────────────────────────────────────────────────

    /// Emit draw calls.  Default implementation paints child widgets.
    virtual void paint(Canvas& canvas) {
        paintChildren(canvas);
    }

    // ── Input ─────────────────────────────────────────────────────────────

    /// Return true if this widget (at its current layout position) handles
    /// a pointer event at the given window coordinate.
    virtual bool hitTest(const Point& /*windowPt*/, const Point& /*myOrigin*/) const {
        return false;
    }

    virtual void onPointerDown(const Point& /*localPt*/) {}
    virtual void onPointerUp  (const Point& /*localPt*/) {}
    virtual void onPointerMove(const Point& /*localPt*/) {}

protected:
    Size  size_{};
    Point offset_{};

    /// Children managed by this widget (populated by the layout engine).
    std::vector<WidgetPtr> children_;

    void paintChildren(Canvas& canvas) {
        for (auto& child : children_) {
            canvas.pushTranslate(child->offset().x, child->offset().y);
            child->paint(canvas);
            canvas.popTransform();
        }
    }
};

} // namespace aigui
