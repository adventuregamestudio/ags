// AGS Editor ImGui - Application window icon
// Provides a simple embedded window icon for the AGS Editor
#pragma once

#include <SDL.h>

// Creates a 32x32 SDL_Surface* with the AGS Editor icon.
// Caller must call SDL_FreeSurface() when done.
inline SDL_Surface* CreateAGSEditorIcon()
{
    // 32x32 RGBA pixel data — simple "AGS" branding icon
    // Dark blue background (#1e1e3f) with a lighter blue "A" shape (#5c7cfa)
    static const int SIZE = 32;
    static uint32_t pixels[SIZE * SIZE];

    const uint32_t bg       = 0xFF3F1E1E;  // ABGR: dark blue-purple
    const uint32_t accent   = 0xFFFA7C5C;  // ABGR: light blue
    const uint32_t border   = 0xFF5A3A2A;  // ABGR: darker edge
    const uint32_t white    = 0xFFFFFFFF;  // ABGR: white

    // Fill background
    for (int i = 0; i < SIZE * SIZE; i++)
        pixels[i] = bg;

    // Draw border (2px)
    for (int x = 0; x < SIZE; x++)
    {
        for (int y = 0; y < SIZE; y++)
        {
            if (x < 2 || x >= SIZE - 2 || y < 2 || y >= SIZE - 2)
                pixels[y * SIZE + x] = border;
        }
    }

    // Draw a simple "A" glyph centered (columns 10-21, rows 5-26)
    // This represents the "A" in AGS
    auto setPixel = [&](int x, int y, uint32_t color) {
        if (x >= 0 && x < SIZE && y >= 0 && y < SIZE)
            pixels[y * SIZE + x] = color;
    };

    auto fillRect = [&](int x0, int y0, int w, int h, uint32_t color) {
        for (int dy = 0; dy < h; dy++)
            for (int dx = 0; dx < w; dx++)
                setPixel(x0 + dx, y0 + dy, color);
    };

    // "A" shape
    // Top apex
    fillRect(14, 5, 4, 3, accent);

    // Left diagonal (going down-left)
    fillRect(12, 8, 4, 3, accent);
    fillRect(10, 11, 4, 3, accent);
    fillRect(8, 14, 4, 3, accent);
    fillRect(7, 17, 4, 3, accent);
    fillRect(6, 20, 4, 3, accent);
    fillRect(5, 23, 5, 3, accent);

    // Right diagonal (going down-right)
    fillRect(16, 8, 4, 3, accent);
    fillRect(18, 11, 4, 3, accent);
    fillRect(20, 14, 4, 3, accent);
    fillRect(21, 17, 4, 3, accent);
    fillRect(22, 20, 4, 3, accent);
    fillRect(22, 23, 5, 3, accent);

    // Horizontal bar of A
    fillRect(9, 18, 14, 3, accent);

    // Small "GS" hint as white dots at bottom
    fillRect(11, 27, 2, 2, white);
    fillRect(15, 27, 2, 2, white);
    fillRect(19, 27, 2, 2, white);

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        pixels, SIZE, SIZE, 32, SIZE * 4,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    return surface;
}
