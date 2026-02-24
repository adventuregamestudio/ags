// AGS Editor ImGui - DPI Scaling Helpers
// Provides functions to scale hardcoded pixel sizes by the current DPI factor.
// Usage: replace `ImVec2(200, 100)` with `ImVec2(Dpi(200), Dpi(100))`
//        replace `float w = 180.0f` with `float w = Dpi(180)`
#pragma once

#include "imgui.h"

namespace AGSEditor
{

// Global DPI scale factor, set once during app init from AGSEditorApp::GetDpiScale()
inline float g_dpi_scale = 1.0f;

// Scale a pixel value by the current DPI factor.
// Example: Dpi(200) returns 200 at 1x, 300 at 1.5x, 400 at 2x.
inline float Dpi(float value) { return value * g_dpi_scale; }

// Scale an integer pixel value, returning float.
inline float Dpi(int value) { return static_cast<float>(value) * g_dpi_scale; }

// Scale an ImVec2 by DPI.
inline ImVec2 DpiVec(float x, float y) { return ImVec2(x * g_dpi_scale, y * g_dpi_scale); }

// Scale only if the value is positive (preserves 0 and -1 sentinel values used by ImGui).
inline float DpiPos(float value) { return value > 0.0f ? value * g_dpi_scale : value; }

} // namespace AGSEditor
