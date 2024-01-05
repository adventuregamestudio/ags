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
#include <math.h>
#include "ac/dynamicsprite.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_dynamicsprite.h"
#include "ac/global_game.h"
#include "ac/math.h"    // M_PI
#include "ac/path_helper.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/system.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "gui/guibutton.h"
#include "ac/spritecache.h"
#include "gfx/graphicsdriver.h"
#include "script/runtimescriptvalue.h"

using namespace Common;
using namespace Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStruct thisroom;
extern RoomObject*objs;
extern RoomStatus*croom;

extern RGB palette[256];
extern AGS::Engine::IGraphicsDriver *gfxDriver;

char check_dynamic_sprites_at_exit = 1;

// ** SCRIPT DYNAMIC SPRITE

void DynamicSprite_Delete(ScriptDynamicSprite *sds) {
    if (sds->slot) {
        free_dynamic_sprite(sds->slot);
        sds->slot = 0;
    }
}

ScriptDrawingSurface* DynamicSprite_GetDrawingSurface(ScriptDynamicSprite *dss)
{
    ScriptDrawingSurface *surface = new ScriptDrawingSurface();
    surface->dynamicSpriteNumber = dss->slot;

    if ((game.SpriteInfos[dss->slot].Flags & SPF_ALPHACHANNEL) != 0)
        surface->hasAlphaChannel = true;

    ccRegisterManagedObject(surface, surface);
    return surface;
}

int DynamicSprite_GetGraphic(ScriptDynamicSprite *sds) {
    if (sds->slot == 0)
        quit("!DynamicSprite.Graphic: Cannot get graphic, sprite has been deleted");
    return sds->slot;
}

int DynamicSprite_GetWidth(ScriptDynamicSprite *sds) {
    return game_to_data_coord(game.SpriteInfos[sds->slot].Width);
}

int DynamicSprite_GetHeight(ScriptDynamicSprite *sds) {
    return game_to_data_coord(game.SpriteInfos[sds->slot].Height);
}

int DynamicSprite_GetColorDepth(ScriptDynamicSprite *sds) {
    // Dynamic sprite ensures the sprite exists always
    int depth = spriteset[sds->slot]->GetColorDepth();
    if (depth == 15)
        depth = 16;
    if (depth == 24)
        depth = 32;
    return depth;
}

void DynamicSprite_Resize(ScriptDynamicSprite *sds, int width, int height) {
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Resize: width and height must be greater than zero");
    if (sds->slot == 0)
        quit("!DynamicSprite.Resize: sprite has been deleted");

    data_to_game_coords(&width, &height);

    if (width * height >= 25000000)
        quitprintf("!DynamicSprite.Resize: new size is too large: %d x %d", width, height);

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth()));
    new_pic->StretchBlt(sprite,
        RectWH(0, 0, game.SpriteInfos[sds->slot].Width, game.SpriteInfos[sds->slot].Height),
        RectWH(0, 0, width, height));

    add_dynamic_sprite(sds->slot, std::move(new_pic), (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction) {
    if ((direction < 1) || (direction > 3))
        quit("!DynamicSprite.Flip: invalid direction");
    if (sds->slot == 0)
        quit("!DynamicSprite.Flip: sprite has been deleted");

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateTransparentBitmap(sprite->GetWidth(), sprite->GetHeight(), sprite->GetColorDepth()));

    // AGS script FlipDirection corresponds to internal GraphicFlip
    new_pic->FlipBlt(sprite, 0, 0, static_cast<GraphicFlip>(direction));

    add_dynamic_sprite(sds->slot, std::move(new_pic), (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_CopyTransparencyMask(ScriptDynamicSprite *sds, int sourceSprite) {
    if (sds->slot == 0)
        quit("!DynamicSprite.CopyTransparencyMask: sprite has been deleted");

    if ((game.SpriteInfos[sds->slot].Width != game.SpriteInfos[sourceSprite].Width) ||
        (game.SpriteInfos[sds->slot].Height != game.SpriteInfos[sourceSprite].Height))
    {
        quit("!DynamicSprite.CopyTransparencyMask: sprites are not the same size");
    }

    Bitmap *target = spriteset[sds->slot];
    Bitmap *source = spriteset[sourceSprite];

    if (target->GetColorDepth() != source->GetColorDepth())
    {
        quit("!DynamicSprite.CopyTransparencyMask: sprites are not the same colour depth");
    }

    // set the target's alpha channel depending on the source
    bool dst_has_alpha = (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0;
    bool src_has_alpha = (game.SpriteInfos[sourceSprite].Flags & SPF_ALPHACHANNEL) != 0;
    game.SpriteInfos[sds->slot].Flags &= ~SPF_ALPHACHANNEL;
    if (src_has_alpha)
    {
        game.SpriteInfos[sds->slot].Flags |= SPF_ALPHACHANNEL;
    }

    BitmapHelper::CopyTransparency(target, source, dst_has_alpha, src_has_alpha);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *sds, int width, int height, int x, int y) 
{
    if (sds->slot == 0)
        quit("!DynamicSprite.ChangeCanvasSize: sprite has been deleted");
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.ChangeCanvasSize: new size is too small");

    data_to_game_coords(&x, &y);
    data_to_game_coords(&width, &height);

    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth()));
    // blit it into the enlarged image
    new_pic->Blit(sprite, 0, 0, x, y, sprite->GetWidth(), sprite->GetHeight());

    add_dynamic_sprite(sds->slot, std::move(new_pic), (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Crop(ScriptDynamicSprite *sds, int x1, int y1, int width, int height) {
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Crop: co-ordinates do not make sense");
    if (sds->slot == 0)
        quit("!DynamicSprite.Crop: sprite has been deleted");

    data_to_game_coords(&x1, &y1);
    data_to_game_coords(&width, &height);

    if ((width > game.SpriteInfos[sds->slot].Width) || (height > game.SpriteInfos[sds->slot].Height))
        quit("!DynamicSprite.Crop: requested to crop an area larger than the source");

    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth()));
    new_pic->Blit(sprite, x1, y1, 0, 0, new_pic->GetWidth(), new_pic->GetHeight());

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, std::move(new_pic), (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Rotate(ScriptDynamicSprite *sds, int angle, int width, int height) {
    if ((angle < 1) || (angle > 359))
        quit("!DynamicSprite.Rotate: invalid angle (must be 1-359)");
    if (sds->slot == 0)
        quit("!DynamicSprite.Rotate: sprite has been deleted");

    if ((width == SCR_NO_VALUE) || (height == SCR_NO_VALUE)) {
        // calculate the new image size automatically
        // 1 degree = 181 degrees in terms of x/y size, so % 180
        int useAngle = angle % 180;
        // and 0..90 is the same as 180..90
        if (useAngle > 90)
            useAngle = 180 - useAngle;
        // useAngle is now between 0 and 90 (otherwise the sin/cos stuff doesn't work)
        double angleInRadians = (double)useAngle * (M_PI / 180.0);
        double sinVal = sin(angleInRadians);
        double cosVal = cos(angleInRadians);

        width = (cosVal * (double)game.SpriteInfos[sds->slot].Width + sinVal * (double)game.SpriteInfos[sds->slot].Height);
        height = (sinVal * (double)game.SpriteInfos[sds->slot].Width + cosVal * (double)game.SpriteInfos[sds->slot].Height);
    }
    else {
        data_to_game_coords(&width, &height);
    }

    // convert to allegro angle
    angle = (angle * 256) / 360;

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth()));

    // rotate the sprite about its centre
    // (+ width%2 fixes one pixel offset problem)
    new_pic->RotateBlt(sprite, width / 2 + width % 2, height / 2,
        sprite->GetWidth() / 2, sprite->GetHeight() / 2, itofix(angle));

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, std::move(new_pic), (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance) 
{
    Bitmap *source = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateBitmap(source->GetWidth(), source->GetHeight(), source->GetColorDepth()));

    tint_image(new_pic.get(), source, red, green, blue, saturation, (luminance * 25) / 10);

    add_dynamic_sprite(sds->slot, std::move(new_pic), (game.SpriteInfos[sds->slot].Flags & SPF_ALPHACHANNEL) != 0);
    game_sprite_updated(sds->slot);
}

int DynamicSprite_SaveToFile(ScriptDynamicSprite *sds, const char* namm)
{
    if (sds->slot == 0)
        quit("!DynamicSprite.SaveToFile: sprite has been deleted");

    auto filename = String(namm);
    if (filename.FindChar('.') == String::NoIndex)
        filename.Append(".bmp");

    ResolvedPath rp = ResolveWritePathAndCreateDirs(filename);
    if (!rp)
        return 0;
    return spriteset[sds->slot]->SaveToFile(rp.FullPath, palette) ? 1 : 0;
}

ScriptDynamicSprite* DynamicSprite_CreateFromSaveGame(int sgslot, int width, int height) {
    int slotnum = LoadSaveSlotScreenshot(sgslot, width, height);
    if (slotnum) {
        ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(slotnum);
        return new_spr;
    }
    return nullptr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromFile(const char *filename) {
    int slotnum = LoadImageFile(filename);
    if (slotnum) {
        ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(slotnum);
        return new_spr;
    }
    return nullptr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot(int width, int height) {

    // TODO: refactor and merge with create_savegame_screenshot()
    if (!spriteset.HasFreeSlots())
        return nullptr;

    const Rect &viewport = play.GetMainViewport();
    if (width <= 0)
        width = viewport.GetWidth();
    else
        width = data_to_game_coord(width);

    if (height <= 0)
        height = viewport.GetHeight();
    else
        height = data_to_game_coord(height);

    std::unique_ptr<Bitmap> new_pic(CopyScreenIntoBitmap(width, height));
    int new_slot = add_dynamic_sprite(std::move(new_pic));
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot, int preserveAlphaChannel) {

    if (!spriteset.HasFreeSlots())
        return nullptr;

    if (!spriteset.DoesSpriteExist(slot))
        quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

    // create a new sprite as a copy of the existing one
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmapCopy(spriteset[slot]));
    if (!new_pic)
        return nullptr;

    bool has_alpha = (preserveAlphaChannel) && ((game.SpriteInfos[slot].Flags & SPF_ALPHACHANNEL) != 0);
    int new_slot = add_dynamic_sprite(std::move(new_pic), has_alpha);
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *sds, int x, int y, int width, int height) 
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    // use DrawingSurface resolution
    sds->PointToGameResolution(&x, &y);
    sds->SizeToGameResolution(&width, &height);

    Bitmap *ds = sds->StartDrawing();

    if ((x < 0) || (y < 0) || (x + width > ds->GetWidth()) || (y + height > ds->GetHeight()))
        quit("!DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");

    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, ds->GetColorDepth()));
    if (!new_pic)
        return nullptr;

    new_pic->Blit(ds, x, y, 0, 0, width, height);
    sds->FinishedDrawingReadOnly();

    int new_slot = add_dynamic_sprite(std::move(new_pic), (sds->hasAlphaChannel != 0));
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_Create(int width, int height, int alphaChannel) 
{
    if (width <= 0 || height <= 0)
    {
        debug_script_warn("WARNING: DynamicSprite.Create: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    data_to_game_coords(&width, &height);

    if (!spriteset.HasFreeSlots())
        return nullptr;

    std::unique_ptr<Bitmap> new_pic(CreateCompatBitmap(width, height));
    if (!new_pic)
        return nullptr;

    new_pic->ClearTransparent();
    if ((alphaChannel) && (game.GetColorDepth() < 32))
        alphaChannel = false;

    int new_slot = add_dynamic_sprite(std::move(new_pic), alphaChannel != 0);
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite_Old(int slot) 
{
    return DynamicSprite_CreateFromExistingSprite(slot, 0);
}

ScriptDynamicSprite* DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height) {

    if (frame == SCR_NO_VALUE) {
        frame = play.bg_frame;
    }
    else if ((frame < 0) || ((size_t)frame >= thisroom.BgFrameCount))
        quit("!DynamicSprite.CreateFromBackground: invalid frame specified");

    if (x1 == SCR_NO_VALUE) {
        x1 = 0;
        y1 = 0;
        width = play.room_width;
        height = play.room_height;
    }
    else if ((x1 < 0) || (y1 < 0) || (width < 1) || (height < 1) ||
        (x1 + width > play.room_width) || (y1 + height > play.room_height))
        quit("!DynamicSprite.CreateFromBackground: invalid co-ordinates specified");

    data_to_game_coords(&x1, &y1);
    data_to_game_coords(&width, &height);

    if (!spriteset.HasFreeSlots())
        return nullptr;

    // create a new sprite as a copy of the existing one
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, thisroom.BgFrames[frame].Graphic->GetColorDepth()));
    if (!new_pic)
        return nullptr;

    new_pic->Blit(thisroom.BgFrames[frame].Graphic.get(), x1, y1, 0, 0, width, height);

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    return new ScriptDynamicSprite(new_slot);
}

//=============================================================================

int add_dynamic_sprite(std::unique_ptr<Bitmap> image, bool has_alpha) {
    int slot = spriteset.GetFreeIndex();
    if (slot <= 0)
        return 0;

    return add_dynamic_sprite(slot, std::move(image), has_alpha);
}

int add_dynamic_sprite(int slot, std::unique_ptr<Bitmap> image, bool has_alpha) {
    assert(slot > 0 && !spriteset.IsAssetSprite(slot));
    if (slot <= 0 || spriteset.IsAssetSprite(slot))
        return 0; // invalid slot, or reserved for the static sprite

    spriteset.SetSprite(slot, std::move(image), SPF_DYNAMICALLOC | (SPF_ALPHACHANNEL * has_alpha));
    if (play.spritemodified.size() < game.SpriteInfos.size())
        play.spritemodified.resize(game.SpriteInfos.size());
    return slot;
}

void free_dynamic_sprite(int slot, bool notify_all) {
    assert((slot > 0) && (static_cast<size_t>(slot) < game.SpriteInfos.size()) &&
        (game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC));
    if ((slot <= 0) || (static_cast<size_t>(slot) >= game.SpriteInfos.size()) ||
        (game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) == 0)
        return;

    spriteset.DisposeSprite(slot);
    if (notify_all)
        game_sprite_updated(slot, true);
    else
        notify_sprite_changed(slot, true);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void (ScriptDynamicSprite *sds, int width, int height, int x, int y)
RuntimeScriptValue Sc_DynamicSprite_ChangeCanvasSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDynamicSprite, DynamicSprite_ChangeCanvasSize);
}

// void (ScriptDynamicSprite *sds, int sourceSprite)
RuntimeScriptValue Sc_DynamicSprite_CopyTransparencyMask(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDynamicSprite, DynamicSprite_CopyTransparencyMask);
}

// void (ScriptDynamicSprite *sds, int x1, int y1, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Crop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDynamicSprite, DynamicSprite_Crop);
}

// void (ScriptDynamicSprite *sds)
RuntimeScriptValue Sc_DynamicSprite_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDynamicSprite, DynamicSprite_Delete);
}

// void (ScriptDynamicSprite *sds, int direction)
RuntimeScriptValue Sc_DynamicSprite_Flip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDynamicSprite, DynamicSprite_Flip);
}

// ScriptDrawingSurface* (ScriptDynamicSprite *dss)
RuntimeScriptValue Sc_DynamicSprite_GetDrawingSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDynamicSprite, ScriptDrawingSurface, DynamicSprite_GetDrawingSurface);
}

// void (ScriptDynamicSprite *sds, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Resize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDynamicSprite, DynamicSprite_Resize);
}

// void (ScriptDynamicSprite *sds, int angle, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Rotate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDynamicSprite, DynamicSprite_Rotate);
}

// int (ScriptDynamicSprite *sds, const char* namm)
RuntimeScriptValue Sc_DynamicSprite_SaveToFile(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptDynamicSprite, DynamicSprite_SaveToFile, const char);
}

// void (ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance)
RuntimeScriptValue Sc_DynamicSprite_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDynamicSprite, DynamicSprite_Tint);
}

// int (ScriptDynamicSprite *sds)
RuntimeScriptValue Sc_DynamicSprite_GetColorDepth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetColorDepth);
}

// int (ScriptDynamicSprite *sds)
RuntimeScriptValue Sc_DynamicSprite_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetGraphic);
}

// int (ScriptDynamicSprite *sds)
RuntimeScriptValue Sc_DynamicSprite_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetHeight);
}

// int (ScriptDynamicSprite *sds)
RuntimeScriptValue Sc_DynamicSprite_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetWidth);
}

// ScriptDynamicSprite* (int width, int height, int alphaChannel)
RuntimeScriptValue Sc_DynamicSprite_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_Create);
}

// ScriptDynamicSprite* (int frame, int x1, int y1, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT5(ScriptDynamicSprite, DynamicSprite_CreateFromBackground);
}

// ScriptDynamicSprite* (ScriptDrawingSurface *sds, int x, int y, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT4(ScriptDynamicSprite, DynamicSprite_CreateFromDrawingSurface, ScriptDrawingSurface);
}

// ScriptDynamicSprite* (int slot)
RuntimeScriptValue Sc_DynamicSprite_CreateFromExistingSprite_Old(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDynamicSprite, DynamicSprite_CreateFromExistingSprite_Old);
}

// ScriptDynamicSprite* (int slot, int preserveAlphaChannel)
RuntimeScriptValue Sc_DynamicSprite_CreateFromExistingSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromExistingSprite);
}

// ScriptDynamicSprite* (const char *filename)
RuntimeScriptValue Sc_DynamicSprite_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptDynamicSprite, DynamicSprite_CreateFromFile, const char);
}

// ScriptDynamicSprite* (int sgslot, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromSaveGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromSaveGame);
}

// ScriptDynamicSprite* (int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot);
}


void RegisterDynamicSpriteAPI()
{
    ScFnRegister dynsprite_api[] = {
        { "DynamicSprite::Create^3",                  API_FN_PAIR(DynamicSprite_Create) },
        { "DynamicSprite::CreateFromBackground",      API_FN_PAIR(DynamicSprite_CreateFromBackground) },
        { "DynamicSprite::CreateFromDrawingSurface^5", API_FN_PAIR(DynamicSprite_CreateFromDrawingSurface) },
        { "DynamicSprite::CreateFromExistingSprite^1", API_FN_PAIR(DynamicSprite_CreateFromExistingSprite_Old) },
        { "DynamicSprite::CreateFromExistingSprite^2", API_FN_PAIR(DynamicSprite_CreateFromExistingSprite) },
        { "DynamicSprite::CreateFromFile",            API_FN_PAIR(DynamicSprite_CreateFromFile) },
        { "DynamicSprite::CreateFromSaveGame",        API_FN_PAIR(DynamicSprite_CreateFromSaveGame) },
        { "DynamicSprite::CreateFromScreenShot",      API_FN_PAIR(DynamicSprite_CreateFromScreenShot) },

        { "DynamicSprite::ChangeCanvasSize^4",        API_FN_PAIR(DynamicSprite_ChangeCanvasSize) },
        { "DynamicSprite::CopyTransparencyMask^1",    API_FN_PAIR(DynamicSprite_CopyTransparencyMask) },
        { "DynamicSprite::Crop^4",                    API_FN_PAIR(DynamicSprite_Crop) },
        { "DynamicSprite::Delete",                    API_FN_PAIR(DynamicSprite_Delete) },
        { "DynamicSprite::Flip^1",                    API_FN_PAIR(DynamicSprite_Flip) },
        { "DynamicSprite::GetDrawingSurface^0",       API_FN_PAIR(DynamicSprite_GetDrawingSurface) },
        { "DynamicSprite::Resize^2",                  API_FN_PAIR(DynamicSprite_Resize) },
        { "DynamicSprite::Rotate^3",                  API_FN_PAIR(DynamicSprite_Rotate) },
        { "DynamicSprite::SaveToFile^1",              API_FN_PAIR(DynamicSprite_SaveToFile) },
        { "DynamicSprite::Tint^5",                    API_FN_PAIR(DynamicSprite_Tint) },
        { "DynamicSprite::get_ColorDepth",            API_FN_PAIR(DynamicSprite_GetColorDepth) },
        { "DynamicSprite::get_Graphic",               API_FN_PAIR(DynamicSprite_GetGraphic) },
        { "DynamicSprite::get_Height",                API_FN_PAIR(DynamicSprite_GetHeight) },
        { "DynamicSprite::get_Width",                 API_FN_PAIR(DynamicSprite_GetWidth) },
    };

    ccAddExternalFunctions(dynsprite_api);
}
