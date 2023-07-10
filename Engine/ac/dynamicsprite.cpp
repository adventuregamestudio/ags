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

// CLNUP take a look
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

    if (width * height >= 25000000)
        quitprintf("!DynamicSprite.Resize: new size is too large: %d x %d", width, height);

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth());
    newPic->StretchBlt(sprite,
        RectWH(0, 0, game.SpriteInfos[sds->slot].Width, game.SpriteInfos[sds->slot].Height),
        RectWH(0, 0, width, height));

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction) {
    if ((direction < 1) || (direction > 3))
        quit("!DynamicSprite.Flip: invalid direction");
    if (sds->slot == 0)
        quit("!DynamicSprite.Flip: sprite has been deleted");

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[sds->slot];
    Bitmap *newPic = BitmapHelper::CreateTransparentBitmap(sprite->GetWidth(), sprite->GetHeight(), sprite->GetColorDepth());

    // AGS script FlipDirection corresponds to internal GraphicFlip
    newPic->FlipBlt(sprite, 0, 0, static_cast<GraphicFlip>(direction));

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic);
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
    Bitmap *newPic = BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth());
    // blit it into the enlarged image
    newPic->Blit(sprite, 0, 0, x, y, sprite->GetWidth(), sprite->GetHeight());

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic);
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
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth());
    // blit it cropped
    newPic->Blit(sprite, x1, y1, 0, 0, newPic->GetWidth(), newPic->GetHeight());

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic);
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
    Bitmap *newPic = BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth());

    // rotate the sprite about its centre
    // (+ width%2 fixes one pixel offset problem)
    newPic->RotateBlt(sprite, width / 2 + width % 2, height / 2,
        src_width / 2, src_height / 2, angle);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic);
    game_sprite_updated(sds->slot);
}

void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance) 
{
    Bitmap *source = spriteset[sds->slot];
    Bitmap *newPic = BitmapHelper::CreateBitmap(source->GetWidth(), source->GetHeight(), source->GetColorDepth());

    tint_image(newPic, source, red, green, blue, saturation, (luminance * 25) / 10);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic);
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

    int gotSlot = spriteset.GetFreeIndex();
    if (gotSlot <= 0)
        return nullptr;

    const Rect &viewport = play.GetMainViewport();
    if (width <= 0)
        width = viewport.GetWidth();

    if (height <= 0)
        height = viewport.GetHeight();

    Bitmap *newPic = CopyScreenIntoBitmap(width, height);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot)
{
    int gotSlot = spriteset.GetFreeIndex();
    if (gotSlot <= 0)
        return nullptr;

    if (!spriteset.DoesSpriteExist(slot))
        quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

    // create a new sprite as a copy of the existing one
    Bitmap *newPic = BitmapHelper::CreateBitmapCopy(spriteset[slot]);
    if (newPic == nullptr)
        return nullptr;

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *sds, int x, int y, int width, int height) 
{
    int gotSlot = spriteset.GetFreeIndex();
    if (gotSlot <= 0)
        return nullptr;

    Bitmap *ds = sds->StartDrawing();

    if ((x < 0) || (y < 0) || (x + width > ds->GetWidth()) || (y + height > ds->GetHeight()))
        quit("!DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");

    int colDepth = ds->GetColorDepth();

    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, colDepth);
    if (newPic == nullptr)
        return nullptr;

    newPic->Blit(ds, x, y, 0, 0, width, height);

    sds->FinishedDrawingReadOnly();

    add_dynamic_sprite(gotSlot, newPic);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_Create(int width, int height) 
{
    if (width <= 0 || height <= 0)
    {
        debug_script_warn("WARNING: DynamicSprite.Create: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    int gotSlot = spriteset.GetFreeIndex();
    if (gotSlot <= 0)
        return nullptr;

    Bitmap *newPic = CreateCompatBitmap(width, height);
    if (newPic == nullptr)
        return nullptr;

    newPic->ClearTransparent();

    add_dynamic_sprite(gotSlot, newPic);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    return new_spr;
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

    int gotSlot = spriteset.GetFreeIndex();
    if (gotSlot <= 0)
        return nullptr;

    // create a new sprite as a copy of the existing one
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, thisroom.BgFrames[frame].Graphic->GetColorDepth());
    if (newPic == nullptr)
        return nullptr;

    newPic->Blit(thisroom.BgFrames[frame].Graphic.get(), x1, y1, 0, 0, width, height);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    return new_spr;
}

//=============================================================================

void add_dynamic_sprite(int gotSlot, Bitmap *redin) {
  spriteset.SetSprite(gotSlot, redin);
  game.SpriteInfos[gotSlot].Flags = SPF_DYNAMICALLOC;
  game.SpriteInfos[gotSlot].Width = redin->GetWidth();
  game.SpriteInfos[gotSlot].Height = redin->GetHeight();
}

void free_dynamic_sprite (int gotSlot) {

  if ((gotSlot < 0) || ((size_t)gotSlot >= spriteset.GetSpriteSlotCount()))
    quit("!FreeDynamicSprite: invalid slot number");

  if ((game.SpriteInfos[gotSlot].Flags & SPF_DYNAMICALLOC) == 0)
    quitprintf("!DeleteSprite: Attempted to free static sprite %d that was not loaded by the script", gotSlot);

  spriteset.DisposeSprite(gotSlot);

  game.SpriteInfos[gotSlot].Flags = 0;
  game.SpriteInfos[gotSlot].Width = 0;
  game.SpriteInfos[gotSlot].Height = 0;

  game_sprite_deleted(gotSlot);
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
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_Create);
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

RuntimeScriptValue Sc_DynamicSprite_CreateFromExistingSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDynamicSprite, DynamicSprite_CreateFromExistingSprite);
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
    // WARNING: DynamicSprite.Create and CreateFromExistingSprite have 1 param REMOVED since ags4
    ScFnRegister dynsprite_api[] = {
        { "DynamicSprite::Create^2",                  API_FN_PAIR(DynamicSprite_Create) },
        { "DynamicSprite::CreateFromBackground",      API_FN_PAIR(DynamicSprite_CreateFromBackground) },
        { "DynamicSprite::CreateFromDrawingSurface^5", API_FN_PAIR(DynamicSprite_CreateFromDrawingSurface) },
        { "DynamicSprite::CreateFromExistingSprite^1", API_FN_PAIR(DynamicSprite_CreateFromExistingSprite) },
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
