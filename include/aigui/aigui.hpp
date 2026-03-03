#pragma once
/// AI-GUI: Modern C++ Cross-Platform GPU-Accelerated GUI Library
/// ==============================================================
/// Include this single header to access the full public API.
///
/// Rendering back-ends:
///   Windows  → DirectX 12  (AIGUI_RENDERER_DX12)
///   Linux    → Vulkan       (AIGUI_RENDERER_VULKAN)

// Core types
#include "core/platform.hpp"
#include "core/color.hpp"
#include "core/point.hpp"
#include "core/size.hpp"
#include "core/rect.hpp"
#include "core/constraints.hpp"

// Data binding (XAML-inspired)
#include "binding/observable.hpp"
#include "binding/property.hpp"

// Renderer abstraction
#include "renderer/canvas.hpp"

// Widget base classes (Flutter-inspired)
#include "widget/widget.hpp"
#include "widget/stateful_widget.hpp"

// Built-in widgets
#include "widgets/text.hpp"
#include "widgets/container.hpp"
#include "widgets/flex.hpp"
#include "widgets/button.hpp"
#include "widgets/layout_widgets.hpp"

// Application / Window
#include "app/window.hpp"
#include "app/application.hpp"
