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
#include "util/wgt2allg.h"
#include "ac/dynamicsprite.h"
#include "ac/common.h"
#include "ac/charactercache.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_dynamicsprite.h"
#include "ac/global_game.h"
#include "ac/file.h"
#include "ac/math.h"    // M_PI
#include "ac/objectcache.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "debug/debug_log.h"
#include "gui/dynamicarray.h"
#include "gui/guibutton.h"
#include "ac/spritecache.h"
#include "platform/base/override_defines.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern roomstruct thisroom;
extern DynamicArray<GUIButton> guibuts;
extern int numguibuts;
extern RoomObject*objs;
extern RoomStatus*croom;
extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_INIT_SPR];

extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;
extern color palette[256];
extern Bitmap *virtual_screen;
extern IGraphicsDriver *gfxDriver;
extern RuntimeScriptValue GlobalReturnValue;

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

    if ((game.spriteflags[dss->slot] & SPF_ALPHACHANNEL) != 0)
        surface->hasAlphaChannel = true;

    ccRegisterManagedObject(surface, surface);
    GlobalReturnValue.SetDynamicObject(surface, surface);
    return surface;
}

int DynamicSprite_GetGraphic(ScriptDynamicSprite *sds) {
    if (sds->slot == 0)
        quit("!DynamicSprite.Graphic: Cannot get graphic, sprite has been deleted");
    return sds->slot;
}

int DynamicSprite_GetWidth(ScriptDynamicSprite *sds) {
    return divide_down_coordinate(spritewidth[sds->slot]);
}

int DynamicSprite_GetHeight(ScriptDynamicSprite *sds) {
    return divide_down_coordinate(spriteheight[sds->slot]);
}

int DynamicSprite_GetColorDepth(ScriptDynamicSprite *sds) {
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

    multiply_up_coordinates(&width, &height);

    if (width * height >= 25000000)
        quitprintf("!DynamicSprite.Resize: new size is too large: %d x %d", width, height);

    // resize the sprite to the requested size
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, spriteset[sds->slot]->GetColorDepth());

    newPic->StretchBlt(spriteset[sds->slot],
        RectWH(0, 0, spritewidth[sds->slot], spriteheight[sds->slot]),
        RectWH(0, 0, width, height));

    delete spriteset[sds->slot];

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction) {
    if ((direction < 1) || (direction > 3))
        quit("!DynamicSprite.Flip: invalid direction");
    if (sds->slot == 0)
        quit("!DynamicSprite.Flip: sprite has been deleted");

    // resize the sprite to the requested size
    Bitmap *newPic = BitmapHelper::CreateBitmap(spritewidth[sds->slot], spriteheight[sds->slot], spriteset[sds->slot]->GetColorDepth());
    newPic->Clear(newPic->GetMaskColor());

    if (direction == 1)
        newPic->FlipBlt(spriteset[sds->slot], 0, 0, Common::kBitmap_HFlip);
    else if (direction == 2)
        newPic->FlipBlt(spriteset[sds->slot], 0, 0, Common::kBitmap_VFlip);
    else if (direction == 3)
        newPic->FlipBlt(spriteset[sds->slot], 0, 0, Common::kBitmap_HVFlip);

    delete spriteset[sds->slot];

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_CopyTransparencyMask(ScriptDynamicSprite *sds, int sourceSprite) {
    if (sds->slot == 0)
        quit("!DynamicSprite.CopyTransparencyMask: sprite has been deleted");

    if ((spritewidth[sds->slot] != spritewidth[sourceSprite]) ||
        (spriteheight[sds->slot] != spriteheight[sourceSprite]))
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
    bool sourceHasAlpha = (game.spriteflags[sourceSprite] & SPF_ALPHACHANNEL) != 0;
    game.spriteflags[sds->slot] &= ~SPF_ALPHACHANNEL;
    if (sourceHasAlpha)
    {
        game.spriteflags[sds->slot] |= SPF_ALPHACHANNEL;
    }

    unsigned int maskColor = source->GetMaskColor();
    int colDep = source->GetColorDepth();
    int bytesPerPixel = (colDep + 1) / 8;

    unsigned short *shortPtr;
    unsigned int *longPtr;
    for (int y = 0; y < target->GetHeight(); y++)
    {
        unsigned char * sourcePixel = source->GetScanLineForWriting(y);
        unsigned char * targetPixel = target->GetScanLineForWriting(y);
        for (int x = 0; x < target->GetWidth(); x++)
        {
            shortPtr = (unsigned short*)sourcePixel;
            longPtr = (unsigned int*)sourcePixel;

            if ((colDep == 8) && (sourcePixel[0] == maskColor))
            {
                targetPixel[0] = maskColor;
            }
            else if ((bytesPerPixel == 2) && (shortPtr[0] == maskColor))
            {
                ((unsigned short*)targetPixel)[0] = maskColor;
            }
            else if ((bytesPerPixel == 3) && (memcmp(sourcePixel, &maskColor, 3) == 0))
            {
                memcpy(targetPixel, sourcePixel, 3);
            }
            else if ((bytesPerPixel == 4) && (longPtr[0] == maskColor))
            {
                ((unsigned int*)targetPixel)[0] = maskColor;
            }
            else if ((bytesPerPixel == 4) && (sourceHasAlpha))
            {
                // the fourth byte is the alpha channel, copy it
                targetPixel[3] = sourcePixel[3];
            }
            else if (bytesPerPixel == 4)
            {
                // set the alpha channel byte to opaque
                targetPixel[3] = 0xff;
            }

            sourcePixel += bytesPerPixel;
            targetPixel += bytesPerPixel;
        }
    }
}

void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *sds, int width, int height, int x, int y) 
{
    if (sds->slot == 0)
        quit("!DynamicSprite.ChangeCanvasSize: sprite has been deleted");
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.ChangeCanvasSize: new size is too small");

    multiply_up_coordinates(&x, &y);
    multiply_up_coordinates(&width, &height);

    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, spriteset[sds->slot]->GetColorDepth());
    newPic->Clear(newPic->GetMaskColor());
    // blit it into the enlarged image
    newPic->Blit(spriteset[sds->slot], 0, 0, x, y, spritewidth[sds->slot], spriteheight[sds->slot]);

    delete spriteset[sds->slot];

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Crop(ScriptDynamicSprite *sds, int x1, int y1, int width, int height) {
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Crop: co-ordinates do not make sense");
    if (sds->slot == 0)
        quit("!DynamicSprite.Crop: sprite has been deleted");

    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates(&width, &height);

    if ((width > spritewidth[sds->slot]) || (height > spriteheight[sds->slot]))
        quit("!DynamicSprite.Crop: requested to crop an area larger than the source");

    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, spriteset[sds->slot]->GetColorDepth());
    // blit it cropped
    newPic->Blit(spriteset[sds->slot], x1, y1, 0, 0, newPic->GetWidth(), newPic->GetHeight());

    delete spriteset[sds->slot];

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
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

        width = (cosVal * (double)spritewidth[sds->slot] + sinVal * (double)spriteheight[sds->slot]);
        height = (sinVal * (double)spritewidth[sds->slot] + cosVal * (double)spriteheight[sds->slot]);
    }
    else {
        multiply_up_coordinates(&width, &height);
    }

    // convert to allegro angle
    angle = (angle * 256) / 360;

    // resize the sprite to the requested size
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, spriteset[sds->slot]->GetColorDepth());
    newPic->Clear(newPic->GetMaskColor());

    // rotate the sprite about its centre
    // (+ width%2 fixes one pixel offset problem)
    newPic->RotateBlt(spriteset[sds->slot], width / 2 + width % 2, height / 2,
        spritewidth[sds->slot] / 2, spriteheight[sds->slot] / 2, itofix(angle));

    delete spriteset[sds->slot];

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance) 
{
    Bitmap *source = spriteset[sds->slot];
    Bitmap *newPic = BitmapHelper::CreateBitmap(source->GetWidth(), source->GetHeight(), source->GetColorDepth());

    tint_image(source, newPic, red, green, blue, saturation, (luminance * 25) / 10);

    delete source;
    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

int DynamicSprite_SaveToFile(ScriptDynamicSprite *sds, const char* namm) {
    if (sds->slot == 0)
        quit("!DynamicSprite.SaveToFile: sprite has been deleted");

    char fileName[MAX_PATH];
    get_current_dir_path(fileName, namm);

    if (strchr(namm,'.') == NULL)
        strcat(fileName, ".bmp");

	if (!BitmapHelper::SaveToFile(spriteset[sds->slot], fileName, palette))
        return 0; // failed

    return 1;  // successful
}

ScriptDynamicSprite* DynamicSprite_CreateFromSaveGame(int sgslot, int width, int height) {
    int slotnum = LoadSaveSlotScreenshot(sgslot, width, height);
    if (slotnum) {
        ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(slotnum);
        GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
        return new_spr;
    }
    return NULL;
}

ScriptDynamicSprite* DynamicSprite_CreateFromFile(const char *filename) {
    int slotnum = LoadImageFile(filename);
    if (slotnum) {
        ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(slotnum);
        GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
        return new_spr;
    }
    return NULL;
}

ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot(int width, int height) {

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    if (width <= 0)
        width = virtual_screen->GetWidth();
    else
        width = multiply_up_coordinate(width);

    if (height <= 0)
        height = virtual_screen->GetHeight();
    else
        height = multiply_up_coordinate(height);

    Bitmap *newPic;
    if (!gfxDriver->UsesMemoryBackBuffer()) 
    {
        // D3D driver
        Bitmap *scrndump = BitmapHelper::CreateBitmap(scrnwid, scrnhit, final_col_dep);
        gfxDriver->GetCopyOfScreenIntoBitmap(scrndump);

        update_polled_stuff_if_runtime();

        if ((scrnwid != width) || (scrnhit != height))
        {
            newPic = BitmapHelper::CreateBitmap(width, height, final_col_dep);
            newPic->StretchBlt(scrndump,
                RectWH(0, 0, scrndump->GetWidth(), scrndump->GetHeight()),
                RectWH(0, 0, width, height));
            delete scrndump;
        }
        else
        {
            newPic = scrndump;
        }
    }
    else
    {
        // resize the sprite to the requested size
        newPic = BitmapHelper::CreateBitmap(width, height, virtual_screen->GetColorDepth());

        newPic->StretchBlt(virtual_screen,
            RectWH(0, 0, virtual_screen->GetWidth(), virtual_screen->GetHeight()),
            RectWH(0, 0, width, height));
    }

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(newPic));
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot, int preserveAlphaChannel) {

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    if (!spriteset.doesSpriteExist(slot))
        quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

    // create a new sprite as a copy of the existing one
    Bitmap *newPic = BitmapHelper::CreateBitmap(spritewidth[slot], spriteheight[slot], spriteset[slot]->GetColorDepth());
    if (newPic == NULL)
        return NULL;

    newPic->Blit(spriteset[slot], 0, 0, 0, 0, spritewidth[slot], spriteheight[slot]);

    bool hasAlpha = (preserveAlphaChannel) && ((game.spriteflags[slot] & SPF_ALPHACHANNEL) != 0);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic, hasAlpha);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *sds, int x, int y, int width, int height) 
{
    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    // use DrawingSurface resolution
    sds->MultiplyCoordinates(&x, &y);
    sds->MultiplyCoordinates(&width, &height);

    sds->StartDrawing();

    if ((x < 0) || (y < 0) || (x + width > abuf->GetWidth()) || (y + height > abuf->GetHeight()))
        quit("!DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");

    int colDepth = abuf->GetColorDepth();

    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, colDepth);
    if (newPic == NULL)
        return NULL;

    newPic->Blit(abuf, x, y, 0, 0, width, height);

    sds->FinishedDrawingReadOnly();

    add_dynamic_sprite(gotSlot, newPic, (sds->hasAlphaChannel != 0));
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_Create(int width, int height, int alphaChannel) 
{
    multiply_up_coordinates(&width, &height);

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, final_col_dep);
    if (newPic == NULL)
        return NULL;
    newPic->Clear(newPic->GetMaskColor());

    if ((alphaChannel) && (final_col_dep < 32))
        alphaChannel = false;

    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(newPic), alphaChannel != 0);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
    return new_spr;
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite_Old(int slot) 
{
    return DynamicSprite_CreateFromExistingSprite(slot, 0);
}

ScriptDynamicSprite* DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height) {

    if (frame == SCR_NO_VALUE) {
        frame = play.bg_frame;
    }
    else if ((frame < 0) || (frame >= thisroom.num_bscenes))
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

    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates(&width, &height);

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    // create a new sprite as a copy of the existing one
    Bitmap *newPic = BitmapHelper::CreateBitmap(width, height, thisroom.ebscene[frame]->GetColorDepth());
    if (newPic == NULL)
        return NULL;

    newPic->Blit(thisroom.ebscene[frame], x1, y1, 0, 0, width, height);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic);
    ScriptDynamicSprite *new_spr = new ScriptDynamicSprite(gotSlot);
    GlobalReturnValue.SetDynamicObject(new_spr, new_spr);
    return new_spr;
}

//=============================================================================

void add_dynamic_sprite(int gotSlot, Bitmap *redin, bool hasAlpha) {

  spriteset.set(gotSlot, redin);

  game.spriteflags[gotSlot] = SPF_DYNAMICALLOC;

  if (redin->GetColorDepth() > 8)
    game.spriteflags[gotSlot] |= SPF_HICOLOR;
  if (redin->GetColorDepth() > 16)
    game.spriteflags[gotSlot] |= SPF_TRUECOLOR;
  if (hasAlpha)
    game.spriteflags[gotSlot] |= SPF_ALPHACHANNEL;

  spritewidth[gotSlot] = redin->GetWidth();
  spriteheight[gotSlot] = redin->GetHeight();
}

void free_dynamic_sprite (int gotSlot) {
  int tt;

  if ((gotSlot < 0) || (gotSlot >= spriteset.elements))
    quit("!FreeDynamicSprite: invalid slot number");

  if ((game.spriteflags[gotSlot] & SPF_DYNAMICALLOC) == 0)
    quitprintf("!DeleteSprite: Attempted to free static sprite %d that was not loaded by the script", gotSlot);

  delete spriteset[gotSlot];
  spriteset.set(gotSlot, NULL);

  game.spriteflags[gotSlot] = 0;
  spritewidth[gotSlot] = 0;
  spriteheight[gotSlot] = 0;

  // ensure it isn't still on any GUI buttons
  for (tt = 0; tt < numguibuts; tt++) {
    if (guibuts[tt].IsDeleted())
      continue;
    if (guibuts[tt].pic == gotSlot)
      guibuts[tt].pic = 0;
    if (guibuts[tt].usepic == gotSlot)
      guibuts[tt].usepic = 0;
    if (guibuts[tt].overpic == gotSlot)
      guibuts[tt].overpic = 0;
    if (guibuts[tt].pushedpic == gotSlot)
      guibuts[tt].pushedpic = 0;
  }

  // force refresh of any object caches using the sprite
  if (croom != NULL) 
  {
    for (tt = 0; tt < croom->numobj; tt++) 
    {
      if (objs[tt].num == gotSlot)
      {
        objs[tt].num = 0;
        objcache[tt].sppic = -1;
      }
      else if (objcache[tt].sppic == gotSlot)
        objcache[tt].sppic = -1;
    }
  }
}
