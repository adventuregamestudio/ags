//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Graphics class encapsulates a drawing toolset, including output surface
// and settings for vector and text drawing.
//
//=============================================================================
#ifndef __AGS_CN_GFX__GRAPHICS_H
#define __AGS_CN_GFX__GRAPHICS_H

#include "gfx/bitmap.h"

namespace AGS
{
namespace Common
{

class Graphics
{
public:
    Graphics();
    Graphics(Bitmap *bitmap);
    ~Graphics();

    void    SetBitmap(Bitmap *bitmap);
    void    ReleaseBitmap();

    inline Bitmap *GetBitmap()
    {
        return _bitmap;
    }

    void    SetDrawColor(color_t color);
    void    SetTextColor(color_t color);
    void    SetDrawColorExact(color_t color);
    void    SetTextColorExact(color_t color);
    color_t GetDrawColor();
    color_t GetTextColor();

    void    SetClip(const Rect &rc);
    Rect    GetClip() const;

    //=========================================================================
    // Blitting operations (drawing one bitmap over another)
    //=========================================================================
    // Draw other bitmap over current one
    void    Blit(Bitmap *src, int dst_x, int dst_y, BitmapMaskOption mask = kBitmap_Copy);
    void    Blit(Bitmap *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, BitmapMaskOption mask = kBitmap_Copy);
    // Copy other bitmap, stretching or shrinking its size to given values
    void    StretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    void    StretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    // Antia-aliased stretch-blit
    void    AAStretchBlt(Bitmap *src, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    void    AAStretchBlt(Bitmap *src, const Rect &src_rc, const Rect &dst_rc, BitmapMaskOption mask = kBitmap_Copy);
    // TODO: find more general way to call these operations, probably require pointer to Blending data struct?
    // Draw bitmap using translucency preset
    void    TransBlendBlt(Bitmap *src, int dst_x, int dst_y);
    // Draw bitmap using lighting preset
    void    LitBlendBlt(Bitmap *src, int dst_x, int dst_y, int light_amount);
    // TODO: generic "draw transformed" function? What about mask option?
    void    FlipBlt(Bitmap *src, int dst_x, int dst_y, BitmapFlip flip);
    void    RotateBlt(Bitmap *src, int dst_x, int dst_y, fixed_t angle);
    void    RotateBlt(Bitmap *src, int dst_x, int dst_y, int pivot_x, int pivot_y, fixed_t angle);

    //=========================================================================
    // Pixel operations
    //=========================================================================
    // The PutPixel and GetPixel are supposed to be safe and therefore
    // relatively slow operations. They should not be used for changing large
    // blocks of bitmap memory - reading/writing from/to scan lines should be
    // done in such cases.
    void    PutPixel(int x, int y);
    void    PutPixel(int x, int y, color_t color);
    int	    GetPixel(int x, int y) const;

    //=========================================================================
    // Vector drawing operations
    //=========================================================================
    void    DrawLine(const Line &ln);
    void    DrawLine(const Line &ln, color_t color);
    void    DrawTriangle(const Triangle &tr);
    void    DrawTriangle(const Triangle &tr, color_t color);
    void    DrawRect(const Rect &rc);
    void    DrawRect(const Rect &rc, color_t color);
    void    FillRect(const Rect &rc);
    void    FillRect(const Rect &rc, color_t color);
    void    FillCircle(const Circle &circle);
    void    FillCircle(const Circle &circle, color_t color);
    // Fills the whole bitmap with given color
    void    Fill(color_t color);
    void    FillTransparent();
    // Floodfills an enclosed area, starting at point
    void    FloodFill(int x, int y);
    void    FloodFill(int x, int y, color_t color);

private:
    Bitmap  *_bitmap;
    BITMAP  *_alBitmap;
    color_t _drawColor;
    color_t _textColor;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__GRAPHICS_H
