//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Driver-dependant bitmap interface.
//
// This interface describes an individual sprite object. The actual texture
// data (pixel data) may be shared among multiple DDBs, while DDB define
// additional settings telling how to present the texture: transform, colorize,
// and so on.
//=============================================================================
#ifndef __AGS_EE_GFX__DDB_H
#define __AGS_EE_GFX__DDB_H

namespace AGS
{
namespace Engine
{

class IDriverDependantBitmap
{
public:
  // Get an arbitrary sprite ID, returns UINT32_MAX if does not have one
  virtual uint32_t GetRefID() const = 0;

  virtual int  GetAlpha() const = 0;
  virtual void SetAlpha(int alpha) = 0;  // 0-255
  virtual void SetFlippedLeftRight(bool isFlipped) = 0;
  virtual void SetStretch(int width, int height, bool useResampler = true) = 0;
  virtual void SetLightLevel(int light_level) = 0;   // 0-255
  virtual void SetTint(int red, int green, int blue, int tintSaturation) = 0;  // 0-255

  virtual int GetWidth() const = 0;
  virtual int GetHeight() const = 0;
  virtual int GetColorDepth() const = 0;

protected:
  IDriverDependantBitmap() = default;
  virtual ~IDriverDependantBitmap() = default;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__DDB_H
