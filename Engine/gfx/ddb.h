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
// Driver-dependant bitmap interface
//
// TODO: split into texture object that has only tex data
// and object describing a drawing operation, with ref to texture and
// drawing parameters (modes, shaders, etc).
// Then we will also be able to share one texture among multiple game entities.
//=============================================================================
#ifndef __AGS_EE_GFX__DDB_H
#define __AGS_EE_GFX__DDB_H

#include "gfx/gfx_def.h"

namespace AGS
{
namespace Engine
{

class IDriverDependantBitmap
{
public:
  virtual int  GetTransparency() const = 0;
  virtual void SetTransparency(int transparency) = 0;  // 0-255
  virtual void SetFlippedLeftRight(bool isFlipped) = 0;
  virtual void SetStretch(int width, int height, bool useResampler = true) = 0;
  virtual void SetRotation(float rotation) = 0; // degrees
  virtual void SetLightLevel(int light_level) = 0;   // 0-255
  virtual void SetTint(int red, int green, int blue, int tintSaturation) = 0;  // 0-255
  virtual void SetBlendMode(Common::BlendMode blendMode) = 0;

  virtual int GetWidth() const = 0;
  virtual int GetHeight() const = 0;
  virtual int GetColorDepth() const = 0;

protected:
  IDriverDependantBitmap() = default;
  ~IDriverDependantBitmap() = default;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__DDB_H
