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

int ValidateColorFormat(const char *api_name, ScriptColorFormat color_format)
{
    switch (color_format)
    {
    case kScColorFmt_8bit:
        return color_format;
    case kScColorFmt_Default:
        return game.GetColorDepth();
    default:
        debug_script_log("%s: unsupported color depth %d, will fallback to game default %d", api_name, color_format, game.GetColorDepth());
        return game.GetColorDepth();
    }
}

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
    ccRegisterManagedObject(surface, surface);
    return surface;
}

int DynamicSprite_GetGraphic(ScriptDynamicSprite *sds) {
    if (sds->slot == 0)
        quit("!DynamicSprite.Graphic: Cannot get graphic, sprite has been deleted");
    return sds->slot;
}

int DynamicSprite_GetWidth(ScriptDynamicSprite *sds) {
    return game.SpriteInfos[sds->slot].Width;
}

int DynamicSprite_GetHeight(ScriptDynamicSprite *sds) {
    return game.SpriteInfos[sds->slot].Height;
}

int DynamicSprite_GetColorDepth(ScriptDynamicSprite *sds) {
    // Dynamic sprite ensures the sprite exists always
    return spriteset[sds->slot]->GetColorDepth();
}

void DynamicSprite_Resize(ScriptDynamicSprite *sds, int width, int height) {
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Resize: width and height must be greater than zero");
    if (sds->slot == 0)
        quit("!DynamicSprite.Resize: sprite has been deleted");

    if (width * height >= 25000000)
        quitprintf("!DynamicSprite.Resize: new size is too large: %d x %d", width, height);

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth()));
    new_pic->StretchBlt(sprite,
        RectWH(0, 0, game.SpriteInfos[sds->slot].Width, game.SpriteInfos[sds->slot].Height),
        RectWH(0, 0, width, height));

    add_dynamic_sprite(sds->slot, std::move(new_pic));
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction) {
    if (sds->slot == 0)
        quit("!DynamicSprite.Flip: sprite has been deleted");
    GraphicFlip flip = ValidateFlip("DynamicSprite.Flip", direction);

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateTransparentBitmap(sprite->GetWidth(), sprite->GetHeight(), sprite->GetColorDepth()));

    // AGS script FlipDirection corresponds to internal GraphicFlip
    new_pic->FlipBlt(sprite, 0, 0, flip);

    add_dynamic_sprite(sds->slot, std::move(new_pic));
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
    BitmapHelper::CopyTransparency(target, source, target->GetColorDepth() == 32, source->GetColorDepth() == 32);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *sds, int width, int height, int x, int y) 
{
    if (sds->slot == 0)
        quit("!DynamicSprite.ChangeCanvasSize: sprite has been deleted");
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.ChangeCanvasSize: new size is too small");

    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth()));
    // blit it into the enlarged image
    new_pic->Blit(sprite, 0, 0, x, y, sprite->GetWidth(), sprite->GetHeight());

    add_dynamic_sprite(sds->slot, std::move(new_pic));
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Crop(ScriptDynamicSprite *sds, int x1, int y1, int width, int height) {
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Crop: co-ordinates do not make sense");
    if (sds->slot == 0)
        quit("!DynamicSprite.Crop: sprite has been deleted");

    if ((width > game.SpriteInfos[sds->slot].Width) || (height > game.SpriteInfos[sds->slot].Height))
        quit("!DynamicSprite.Crop: requested to crop an area larger than the source");

    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth()));
    new_pic->Blit(sprite, x1, y1, 0, 0, new_pic->GetWidth(), new_pic->GetHeight());

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, std::move(new_pic));
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Rotate(ScriptDynamicSprite *sds, int angle, int width, int height) {
    if ((angle < 1) || (angle > 359))
        quit("!DynamicSprite.Rotate: invalid angle (must be 1-359)");
    if (sds->slot == 0)
        quit("!DynamicSprite.Rotate: sprite has been deleted");

    const int src_width = game.SpriteInfos[sds->slot].Width;
    const int src_height = game.SpriteInfos[sds->slot].Height;
    if ((width == SCR_NO_VALUE) || (height == SCR_NO_VALUE)) {
        Size rot_sz = RotateSize(Size(src_width, src_height), angle);
        width = rot_sz.Width;
        height = rot_sz.Height;
    }

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth()));

    // rotate the sprite about its centre
    // (+ width%2 fixes one pixel offset problem)
    new_pic->RotateBlt(sprite, width / 2 + width % 2, height / 2,
        src_width / 2, src_height / 2, angle);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, std::move(new_pic));
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance) 
{
    Bitmap *source = spriteset[sds->slot];
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateBitmap(source->GetWidth(), source->GetHeight(), source->GetColorDepth()));

    tint_image(new_pic.get(), source, red, green, blue, saturation, GfxDef::Value100ToValue250(luminance));

    add_dynamic_sprite(sds->slot, std::move(new_pic));
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

ScriptDynamicSprite* DynamicSprite_CreateFromSaveGame(int sgslot, int width, int height)
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    auto screenshot = read_savedgame_screenshot(get_save_game_path(sgslot));
    if (!screenshot)
        return nullptr;

    // resize the sprite to the requested size
    if ((screenshot->GetWidth() != width) || (screenshot->GetHeight() != height))
    {
        std::unique_ptr<Bitmap> temp(BitmapHelper::CreateBitmap(width, height, screenshot->GetColorDepth()));
        temp->StretchBlt(screenshot.get(),
            RectWH(0, 0, screenshot->GetWidth(), screenshot->GetHeight()),
            RectWH(0, 0, width, height));
        screenshot = std::move(temp);
    }

    int new_slot = add_dynamic_sprite(std::move(screenshot));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromFile(const char *filename, int color_fmt)
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    std::unique_ptr<Stream> in(
        ResolveScriptPathAndOpen(filename, FileOpenMode::kFile_Open, StreamMode::kStream_Read));
    if (!in)
        return nullptr;

    String ext = Path::GetFileExtension(filename);
    int dst_color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));
    std::unique_ptr<Bitmap> image(BitmapHelper::LoadBitmap(in.get(), ext, dst_color_depth));
    if (!image)
        return nullptr;

    image.reset(PrepareSpriteForUse(image.release(), false /* no depth conv */, false /* not force opaque */));

    int new_slot = add_dynamic_sprite(std::move(image));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot(int width, int height, int layers)
{
    // TODO: refactor and merge with create_savegame_screenshot()
    if (!spriteset.HasFreeSlots())
        return nullptr;

    // NOTE: be aware that by the historical logic AGS makes a screenshot
    // of a "main viewport", that may be smaller in legacy "letterbox" mode.
    const Rect &viewport = play.GetMainViewport();
    if (width <= 0)
        width = viewport.GetWidth();

    if (height <= 0)
        height = viewport.GetHeight();

    // NOTE: if there will be a difference between script constants and internal
    // constants of Render Layers, or any necessity to adjust these, then convert flags here.
    std::unique_ptr<Bitmap> new_pic;
    if (layers != 0)
        new_pic.reset(CopyScreenIntoBitmap(width, height, &viewport, false, ~layers));
    else
        new_pic.reset(new Bitmap(width, height));

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot2(int width, int height) {
    return DynamicSprite_CreateFromScreenShot(width, height, RENDER_BATCH_ALL);
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot, int color_fmt)
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    if (!spriteset.DoesSpriteExist(slot))
        quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

    int dst_color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));

    // create a new sprite as a copy of the existing one
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmapCopy(spriteset[slot], dst_color_depth));
    if (!new_pic)
        return nullptr;

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *sds, int x, int y, int width, int height, int color_fmt) 
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    int dst_color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));

    if (width <= 0 || height <= 0)
    {
        debug_script_warn("WARNING: DynamicSprite.CreateFromDrawingSurface: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    Bitmap *ds = sds->StartDrawing();
    if ((x < 0) || (y < 0) || (x + width > ds->GetWidth()) || (y + height > ds->GetHeight()))
        quit("!DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");

    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, dst_color_depth));
    if (!new_pic)
    {
        sds->FinishedDrawingReadOnly();
        return nullptr;
    }

    new_pic->Blit(ds, x, y, 0, 0, width, height);
    sds->FinishedDrawingReadOnly();

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_Create(int width, int height, int color_fmt)
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    int color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));

    if (width <= 0 || height <= 0)
    {
        debug_script_warn("WARNING: DynamicSprite.Create: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    std::unique_ptr<Bitmap> new_pic(CreateCompatBitmap(width, height, color_depth));
    if (!new_pic)
        return nullptr;

    new_pic->ClearTransparent();

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height)
{
    if (!spriteset.HasFreeSlots())
        return nullptr;

    if (frame == SCR_NO_VALUE) {
        frame = play.bg_frame;
    }
    else if ((frame < 0) || ((size_t)frame >= thisroom.BgFrameCount))
        quit("!DynamicSprite.CreateFromBackground: invalid frame specified");

    if (x1 == SCR_NO_VALUE)
        x1 = 0;
    if (y1 == SCR_NO_VALUE)
        y1 = 0;
    if (width == SCR_NO_VALUE)
        width = play.room_width;
    if (height == SCR_NO_VALUE)
        height = play.room_height;

    if (width <= 0 || height <= 0)
    {
        debug_script_warn("WARNING: DynamicSprite.CreateFromBackground: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    if ((x1 < 0) || (y1 < 0) || (x1 + width > play.room_width) || (y1 + height > play.room_height))
        quit("!DynamicSprite.CreateFromBackground: invalid co-ordinates specified");

    // create a new sprite as a copy of the existing one
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, thisroom.BgFrames[frame].Graphic->GetColorDepth()));
    if (!new_pic)
        return nullptr;

    new_pic->Blit(thisroom.BgFrames[frame].Graphic.get(), x1, y1, 0, 0, width, height);

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

//=============================================================================

int add_dynamic_sprite(std::unique_ptr<Bitmap> image, uint32_t extra_flags)
{
    int slot = spriteset.GetFreeIndex();
    if (slot <= 0)
        return 0;

    return add_dynamic_sprite(slot, std::move(image), extra_flags);
}

int add_dynamic_sprite(int slot, std::unique_ptr<Bitmap> image, uint32_t extra_flags)
{
    assert(slot > 0 && !spriteset.IsAssetSprite(slot));
    if (slot <= 0 || spriteset.IsAssetSprite(slot))
        return 0; // invalid slot, or reserved for the static sprite

    uint32_t flags = SPF_DYNAMICALLOC | extra_flags;
    if (!spriteset.SetSprite(slot, std::move(image), flags))
        return 0; // failed to add the sprite, bad image or realloc failed
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
    API_SCALL_OBJAUTO_POBJ_PINT5(ScriptDynamicSprite, DynamicSprite_CreateFromDrawingSurface, ScriptDrawingSurface);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromExistingSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromExistingSprite);
}

// ScriptDynamicSprite* (const char *filename)
RuntimeScriptValue Sc_DynamicSprite_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT(ScriptDynamicSprite, DynamicSprite_CreateFromFile, const char);
}

// ScriptDynamicSprite* (int sgslot, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromSaveGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromSaveGame);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot2);
}


void RegisterDynamicSpriteAPI()
{
    // WARNING: DynamicSprite.Create and CreateFromExistingSprite have 1 param REPLACED since ags4
    ScFnRegister dynsprite_api[] = {
        { "DynamicSprite::Create^3",                  API_FN_PAIR(DynamicSprite_Create) },
        { "DynamicSprite::CreateFromBackground",      API_FN_PAIR(DynamicSprite_CreateFromBackground) },
        { "DynamicSprite::CreateFromDrawingSurface^6", API_FN_PAIR(DynamicSprite_CreateFromDrawingSurface) },
        { "DynamicSprite::CreateFromExistingSprite^2", API_FN_PAIR(DynamicSprite_CreateFromExistingSprite) },
        { "DynamicSprite::CreateFromFile^2",          API_FN_PAIR(DynamicSprite_CreateFromFile) },
        { "DynamicSprite::CreateFromSaveGame",        API_FN_PAIR(DynamicSprite_CreateFromSaveGame) },
        { "DynamicSprite::CreateFromScreenShot^2",    API_FN_PAIR(DynamicSprite_CreateFromScreenShot2) },
        { "DynamicSprite::CreateFromScreenShot^3",    API_FN_PAIR(DynamicSprite_CreateFromScreenShot) },
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
