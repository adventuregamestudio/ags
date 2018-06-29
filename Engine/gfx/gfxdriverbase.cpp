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

#include "util/wgt2allg.h"
#include "gfx/ali3dexception.h"
#include "gfx/bitmap.h"
#include "gfx/gfxfilter.h"
#include "gfx/gfxdriverbase.h"
#include "gfx/gfx_util.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{

GraphicsDriverBase::GraphicsDriverBase()
    : _global_x_offset(0)
    , _global_y_offset(0)
    , _loopTimer(NULL)
    , _pollingCallback(NULL)
    , _drawScreenCallback(NULL)
    , _nullSpriteCallback(NULL)
    , _initGfxCallback(NULL)
    , _initSurfaceUpdateCallback(NULL)
{
}

bool GraphicsDriverBase::IsModeSet() const
{
    return _mode.Width != 0 && _mode.Height != 0 && _mode.ColorDepth != 0;
}

bool GraphicsDriverBase::IsNativeSizeValid() const
{
    return !_srcRect.IsEmpty();
}

bool GraphicsDriverBase::IsRenderFrameValid() const
{
    return !_srcRect.IsEmpty() && !_dstRect.IsEmpty();
}

DisplayMode GraphicsDriverBase::GetDisplayMode() const
{
    return _mode;
}

Size GraphicsDriverBase::GetNativeSize() const
{
    return _srcRect.GetSize();
}

Rect GraphicsDriverBase::GetRenderDestination() const
{
    return _dstRect;
}

void GraphicsDriverBase::SetRenderOffset(int x, int y)
{
    _global_x_offset = x;
    _global_y_offset = y;
}

void GraphicsDriverBase::OnInit(volatile int *loopTimer)
{
    _loopTimer = loopTimer;
}

void GraphicsDriverBase::OnUnInit()
{
}

void GraphicsDriverBase::OnModeSet(const DisplayMode &mode)
{
    _mode = mode;
}

void GraphicsDriverBase::OnModeReleased()
{
    _mode = DisplayMode();
    _dstRect = Rect();
}

void GraphicsDriverBase::OnScalingChanged()
{
    PGfxFilter filter = GetGraphicsFilter();
    if (filter)
        _filterRect = filter->SetTranslation(_srcRect.GetSize(), _dstRect);
    else
        _filterRect = Rect();
    _scaling.Init(_srcRect.GetSize(), _dstRect);
}

void GraphicsDriverBase::OnSetNativeSize(const Size &src_size)
{
    _srcRect = RectWH(0, 0, src_size.Width, src_size.Height);
    OnScalingChanged();
}

void GraphicsDriverBase::OnSetRenderFrame(const Rect &dst_rect)
{
    _dstRect = dst_rect;
    OnScalingChanged();
}

void GraphicsDriverBase::OnSetFilter()
{
    _filterRect = GetGraphicsFilter()->SetTranslation(Size(_srcRect.GetSize()), _dstRect);
}

Bitmap *GraphicsDriverBase::ReplaceBitmapWithSupportedFormat(Bitmap *old_bmp)
{
    Bitmap *new_bitmap = GfxUtil::ConvertBitmap(old_bmp, GetCompatibleBitmapFormat(old_bmp->GetColorDepth()));
    if (new_bitmap != old_bmp)
        delete old_bmp;
    return new_bitmap;
}


VideoMemoryGraphicsDriver::VideoMemoryGraphicsDriver()
    : _stageVirtualScreen(NULL)
    , _stageVirtualScreenDDB(NULL)
    , _stageScreenDirty(false)
{
}

VideoMemoryGraphicsDriver::~VideoMemoryGraphicsDriver()
{
    DestroyStageScreen();
}

bool VideoMemoryGraphicsDriver::UsesMemoryBackBuffer()
{
    // Although we do use ours, we do not let engine draw upon it;
    // only plugin handling are allowed to request our mem buffer.
    // TODO: find better workaround?
    return false;
}

Bitmap *VideoMemoryGraphicsDriver::GetMemoryBackBuffer()
{
    _stageScreenDirty = true;
    return _stageVirtualScreen;
}

void VideoMemoryGraphicsDriver::SetMemoryBackBuffer(Bitmap *backBuffer)
{
    // TODO: support this under certain circumstances?
}

void VideoMemoryGraphicsDriver::CreateStageScreen()
{
    if (_stageVirtualScreenDDB)
        this->DestroyDDB(_stageVirtualScreenDDB);
    _stageVirtualScreenDDB = NULL;
    delete _stageVirtualScreen;
    _stageVirtualScreen = ReplaceBitmapWithSupportedFormat(
    BitmapHelper::CreateBitmap(_srcRect.GetWidth(), _srcRect.GetHeight(), _mode.ColorDepth));
    BitmapHelper::SetScreenBitmap(_stageVirtualScreen);
}

void VideoMemoryGraphicsDriver::DestroyStageScreen()
{
    if (_stageVirtualScreenDDB)
        this->DestroyDDB(_stageVirtualScreenDDB);
    _stageVirtualScreenDDB = NULL;
    delete _stageVirtualScreen;
    _stageVirtualScreen = NULL;
}

bool VideoMemoryGraphicsDriver::DoNullSpriteCallback(int x, int y)
{
    if (!_nullSpriteCallback)
        throw Ali3DException("Unhandled attempt to draw null sprite");
    _stageScreenDirty = false;
    _stageVirtualScreen->ClearTransparent();
    // NOTE: this is not clear whether return value of callback may be
    // relied on. Existing plugins do not seem to return anything but 0,
    // even if they handle this event.
    _nullSpriteCallback(x, y);
    if (_stageScreenDirty)
    {
        if (_stageVirtualScreenDDB)
            UpdateDDBFromBitmap(_stageVirtualScreenDDB, _stageVirtualScreen, true);
        else
            _stageVirtualScreenDDB = CreateDDBFromBitmap(_stageVirtualScreen, true);
        return true;
    }
    return false;
}

#define algetr32(xx) getr32(xx)
#define algetg32(xx) getg32(xx)
#define algetb32(xx) getb32(xx)
#define algeta32(xx) geta32(xx)

#define algetr16(xx) getr16(xx)
#define algetg16(xx) getg16(xx)
#define algetb16(xx) getb16(xx)


__inline void get_pixel_if_not_transparent16(unsigned short *pixel, unsigned short *red, unsigned short *green, unsigned short *blue, unsigned short *divisor)
{
  if (pixel[0] != MASK_COLOR_16)
  {
    *red += algetr16(pixel[0]);
    *green += algetg16(pixel[0]);
    *blue += algetb16(pixel[0]);
    divisor[0]++;
  }
}

__inline void get_pixel_if_not_transparent32(unsigned int *pixel, unsigned int *red, unsigned int *green, unsigned int *blue, unsigned int *divisor)
{
  if (pixel[0] != MASK_COLOR_32)
  {
    *red += algetr32(pixel[0]);
    *green += algetg32(pixel[0]);
    *blue += algetb32(pixel[0]);
    divisor[0]++;
  }
}

// OPENGL:
/*
#define D3DCOLOR_RGBA(r,g,b,a) \
  (((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))
*/
// D3D9:
#define D3DCOLOR_ARGB(a,r,g,b) \
    (((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)


void VideoMemoryGraphicsDriver::BitmapToVideoMem(const Bitmap *bitmap, const bool has_alpha, const TextureTile *tile, const VideoMemDDB *target,
                                                 char *dst_ptr, const int dst_pitch, const bool usingLinearFiltering)
{
  bool lastPixelWasTransparent = false;
  for (int y = 0; y < tile->height; y++)
  {
    lastPixelWasTransparent = false;
    const uint8_t *scanline_before = bitmap->GetScanLine(y + tile->y - 1);
    const uint8_t *scanline_at     = bitmap->GetScanLine(y + tile->y);
    const uint8_t *scanline_after  = bitmap->GetScanLine(y + tile->y + 1);
    unsigned int* memPtrLong = (unsigned int*)dst_ptr;

    for (int x = 0; x < tile->width; x++)
    {
      if (bitmap->GetColorDepth() == 16)
      {
        unsigned short* srcData = (unsigned short*)&scanline_at[(x + tile->x) * sizeof(short)];
        if (*srcData == MASK_COLOR_16) 
        {
          if (target->_opaque)  // set to black if opaque
            memPtrLong[x] = 0xFF000000;
          else if (!usingLinearFiltering)
            memPtrLong[x] = 0;
          // set to transparent, but use the colour from the neighbouring 
          // pixel to stop the linear filter doing black outlines
          else
          {
            unsigned short red = 0, green = 0, blue = 0, divisor = 0;
            if (x > 0)
              get_pixel_if_not_transparent16(&srcData[-1], &red, &green, &blue, &divisor);
            if (x < tile->width - 1)
              get_pixel_if_not_transparent16(&srcData[1], &red, &green, &blue, &divisor);
            if (y > 0)
              get_pixel_if_not_transparent16((unsigned short*)&scanline_before[(x + tile->x) * sizeof(short)], &red, &green, &blue, &divisor);
            if (y < tile->height - 1)
              get_pixel_if_not_transparent16((unsigned short*)&scanline_after[(x + tile->x) * sizeof(short)], &red, &green, &blue, &divisor);
            if (divisor > 0)
              memPtrLong[x] = D3DCOLOR_RGBA(red / divisor, green / divisor, blue / divisor, 0);
            else
              memPtrLong[x] = 0;
          }
          lastPixelWasTransparent = true;
        }
        else
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr16(*srcData), algetg16(*srcData), algetb16(*srcData), 0xff);
          if (lastPixelWasTransparent)
          {
            // update the colour of the previous tranparent pixel, to
            // stop black outlines when linear filtering
            memPtrLong[x - 1] = memPtrLong[x] & 0x00FFFFFF;
            lastPixelWasTransparent = false;
          }
        }
      }
      else if (bitmap->GetColorDepth() == 32)
      {
        unsigned int* memPtrLong = (unsigned int*)dst_ptr;
        unsigned int* srcData = (unsigned int*)&scanline_at[(x + tile->x) * sizeof(int)];
        if (*srcData == MASK_COLOR_32)
        {
          if (target->_opaque)  // set to black if opaque
            memPtrLong[x] = 0xFF000000;
          else if (!usingLinearFiltering)
            memPtrLong[x] = 0;
          // set to transparent, but use the colour from the neighbouring 
          // pixel to stop the linear filter doing black outlines
          else
          {
            unsigned int red = 0, green = 0, blue = 0, divisor = 0;
            if (x > 0)
              get_pixel_if_not_transparent32(&srcData[-1], &red, &green, &blue, &divisor);
            if (x < tile->width - 1)
              get_pixel_if_not_transparent32(&srcData[1], &red, &green, &blue, &divisor);
            if (y > 0)
              get_pixel_if_not_transparent32((unsigned int*)&scanline_before[(x + tile->x) * sizeof(int)], &red, &green, &blue, &divisor);
            if (y < tile->height - 1)
              get_pixel_if_not_transparent32((unsigned int*)&scanline_after[(x + tile->x) * sizeof(int)], &red, &green, &blue, &divisor);
            if (divisor > 0)
              memPtrLong[x] = D3DCOLOR_RGBA(red / divisor, green / divisor, blue / divisor, 0);
            else
              memPtrLong[x] = 0;
          }
          lastPixelWasTransparent = true;
        }
        else if (has_alpha)
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr32(*srcData), algetg32(*srcData), algetb32(*srcData), algeta32(*srcData));
        }
        else
        {
          memPtrLong[x] = D3DCOLOR_RGBA(algetr32(*srcData), algetg32(*srcData), algetb32(*srcData), 0xff);
          if (lastPixelWasTransparent)
          {
            // update the colour of the previous tranparent pixel, to
            // stop black outlines when linear filtering
            memPtrLong[x - 1] = memPtrLong[x] & 0x00FFFFFF;
            lastPixelWasTransparent = false;
          }
        }
      }
    }

    dst_ptr += dst_pitch;
  }
}

} // namespace Engine
} // namespace AGS
