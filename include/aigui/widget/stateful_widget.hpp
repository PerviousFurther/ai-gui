#pragma once
#include "widget.hpp"

namespace aigui {

/// A widget whose appearance is fully determined by its configuration.
/// Override build() to compose the UI from child widgets.
class StatelessWidget : public Widget {
public:
    /// Subclasses describe their subtree here.
    std::vector<WidgetPtr> build() override = 0;

    Size layout(const Constraints& constraints) override {
        children_ = build();
        if (children_.empty()) {
            size_ = constraints.smallest();
            return size_;
        }
        // Layout first child and adopt its size.
        size_ = children_[0]->layout(constraints);
        return size_;
    }

    void paint(Canvas& canvas) override {
        paintChildren(canvas);
    }
};

/// Internal state for a StatefulWidget.
class State {
public:
    virtual ~State() = default;

    /// Called once after the state is created.
    virtual void initState() {}

    /// Called when setState() triggers a rebuild.
    virtual std::vector<WidgetPtr> build(const Constraints& constraints) = 0;

    /// Trigger a rebuild of the widget tree rooted here.
    void setState(std::function<void()> fn) {
        fn();
        dirty_ = true;
    }

    bool isDirty() const    { return dirty_; }
    void clearDirty()       { dirty_ = false; }

private:
    bool dirty_{true};
};

/// A widget that holds mutable state.  Subclass State and override build().
class StatefulWidget : public Widget {
public:
    virtual std::unique_ptr<State> createState() = 0;

    Size layout(const Constraints& constraints) override {
        if (!state_) {
            state_ = createState();
            state_->initState();
        }
        constraints_ = constraints;
        children_    = state_->build(constraints);
        if (children_.empty()) {
            size_ = constraints.smallest();
        } else {
            size_ = children_[0]->layout(constraints);
        }
        state_->clearDirty();
        return size_;
    }

    void paint(Canvas& canvas) override {
        // Rebuild if state has changed since last layout.
        if (state_ && state_->isDirty()) {
            children_ = state_->build(constraints_);
            if (!children_.empty()) children_[0]->layout(constraints_);
            state_->clearDirty();
        }
        paintChildren(canvas);
    }

protected:
    std::unique_ptr<State> state_;
    Constraints            constraints_;
};

} // namespace aigui
