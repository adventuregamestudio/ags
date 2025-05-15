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

#include <memory>
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/gfxdefines.h"


namespace AGS
{
namespace Engine
{

using AGS::Common::GraphicResolution;

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


// TextureFlags mark special texture behavior
enum TextureFlags
{
    kTxFlags_None = 0,
    // Texture is used as a render target
    kTxFlags_RenderTarget = 0x0001,
    // Texture should be treated as "opaque", which affects how the input bitmap
    // is converted to texture pixels.
    kTxFlags_Opaque = 0x0002,
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

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetColorDepth() const = 0;
    virtual bool IsOpaque() const = 0;
    virtual bool MatchesFormat(AGS::Common::Bitmap *other) const = 0;

    // Sprite's origin [0,1] is a relative position of texture around sprite's position;
    // E.g. (0.0, 0.0) means the texture will be aligned to sprite's position by its
    // left-top corner, (0.5, 0.5) means the texture will be centered around sprite's pos.
    virtual Pointf GetOrigin() const = 0;
    virtual void SetOrigin(float originx, float originy) = 0;
    virtual Size GetStretch() const = 0;
    virtual bool GetUseResampler() const = 0;
    virtual void SetStretch(int width, int height, bool useResampler = true) = 0;
    virtual Common::GraphicFlip GetFlip() const = 0;
    virtual void SetFlip(Common::GraphicFlip flip) = 0;
    virtual float GetRotation() const = 0; // in degrees
    virtual void SetRotation(float rotation) = 0; // in degrees
    virtual int  GetAlpha() const = 0;
    virtual void SetAlpha(int alpha) = 0; // 0-255
    virtual int  GetLightLevel() const = 0; // 0-255
    virtual void SetLightLevel(int light_level) = 0; // 0-255
    virtual void GetTint(int &red, int &green, int &blue, int &tintSaturation) const = 0; // 0-255
    virtual void SetTint(int red, int green, int blue, int tintSaturation) = 0; // 0-255
    virtual Common::BlendMode GetBlendMode() const = 0;
    virtual void SetBlendMode(Common::BlendMode blendMode) = 0;
    virtual uint32_t GetShader() const = 0;
    virtual void SetShader(uint32_t shader_id) = 0;

    // Tells if this DDB has an actual render data assigned to it.
    virtual bool IsValid() const = 0;
    // Attaches new texture data, sets basic render rules
    // FIXME: opaque should be either texture data's flag, - in which case same sprite_id
    // will be either opaque or not opaque, - or DDB's flag, but in that case it cannot
    // be applied to the shared texture data. Currently it's possible to share same
    // texture data, but update it with different "opaque" values, which breaks logic.
    virtual void AttachData(std::shared_ptr<Texture> txdata, int txflags) = 0;
    // Detach any internal texture data from this DDB, make this an empty object.
    virtual void DetachData() = 0;

protected:
    IDriverDependantBitmap() = default;
    virtual ~IDriverDependantBitmap() = default;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__DDB_H
