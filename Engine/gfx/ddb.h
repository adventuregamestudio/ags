//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

#include <memory>
#include "gfx/gfxdefines.h"

namespace AGS
{
namespace Engine
{

// A base parent for the otherwise opaque texture object;
// Texture refers to the pixel data itself, with no additional
// properties. It may be shared between multiple sprites if necessary.
// FIXME: Both Texture and DDB structs record not a real texture's
// pixel fmt, but rather a requested one, corresponding to the source bitmap.
// This is used to safety test bitmap->texture sync, but may be confusing.
// Need to think this over, and adjust; e.g. store both src and texture fmt.
struct Texture
{
    uint32_t ID = UINT32_MAX; // optional ID, may refer to sprite ID
    const GraphicResolution Res;
    const bool RenderTarget = false; // TODO: replace with flags later

    virtual ~Texture() = default;
    virtual size_t GetMemSize() const = 0;

protected:
    Texture(const GraphicResolution &res, bool rt)
        : Res(res), RenderTarget(rt) {}
    Texture(uint32_t id, const GraphicResolution &res, bool rt)
        : ID(id), Res(res), RenderTarget(rt) {}
};


// The "texture sprite" object, contains Texture object ref,
// which may be either shared or exclusive to this sprite.
// Lets assign various effects and transformations which will be
// used when rendering the sprite.
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

  // Tells if this DDB has an actual render data assigned to it.
  virtual bool IsValid() = 0;
  // Attaches new texture data, sets basic render rules
  // FIXME: opaque should be either texture data's flag, - in which case same sprite_id
  // will be either opaque or not opaque, - or DDB's flag, but in that case it cannot
  // be applied to the shared texture data. Currently it's possible to share same
  // texture data, but update it with different "opaque" values, which breaks logic.
  virtual void AttachData(std::shared_ptr<Texture> txdata, bool opaque) = 0;
  // Detach any internal texture data from this DDB, make this an empty object.
  virtual void DetachData() = 0;

protected:
  IDriverDependantBitmap() = default;
  virtual ~IDriverDependantBitmap() = default;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__DDB_H
