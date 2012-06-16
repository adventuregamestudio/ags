
#include "acmain/ac_maindefines.h"



int LoadImageFile(const char *filename) {

    char loadFromPath[MAX_PATH];
    get_current_dir_path(loadFromPath, filename);

    block loadedFile = load_bitmap(loadFromPath, NULL);

    if (loadedFile == NULL)
        return 0;

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return 0;

    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(loadedFile));

    return gotSlot;
}



// ** SCRIPT DYNAMIC SPRITE

int ScriptDynamicSprite::Dispose(const char *address, bool force) {
    // always dispose
    if ((slot) && (!force))
        free_dynamic_sprite(slot);

    delete this;
    return 1;
}

const char *ScriptDynamicSprite::GetType() {
    return "DynamicSprite";
}

int ScriptDynamicSprite::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(slot);
    return EndSerialize();
}

void ScriptDynamicSprite::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    slot = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite(int theSlot) {
    slot = theSlot;
    ccRegisterManagedObject(this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite() {
    slot = 0;
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

    if ((game.spriteflags[dss->slot] & SPF_ALPHACHANNEL) != 0)
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
    return divide_down_coordinate(spritewidth[sds->slot]);
}

int DynamicSprite_GetHeight(ScriptDynamicSprite *sds) {
    return divide_down_coordinate(spriteheight[sds->slot]);
}

int DynamicSprite_GetColorDepth(ScriptDynamicSprite *sds) {
    int depth = bitmap_color_depth(spriteset[sds->slot]);
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
    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);

    stretch_blit(spriteset[sds->slot], newPic,
        0, 0, spritewidth[sds->slot], spriteheight[sds->slot],
        0, 0, width, height);

    wfreeblock(spriteset[sds->slot]);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction) {
    if ((direction < 1) || (direction > 3))
        quit("!DynamicSprite.Flip: invalid direction");
    if (sds->slot == 0)
        quit("!DynamicSprite.Flip: sprite has been deleted");

    // resize the sprite to the requested size
    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), spritewidth[sds->slot], spriteheight[sds->slot]);
    clear_to_color(newPic, bitmap_mask_color(newPic));

    if (direction == 1)
        draw_sprite_h_flip(newPic, spriteset[sds->slot], 0, 0);
    else if (direction == 2)
        draw_sprite_v_flip(newPic, spriteset[sds->slot], 0, 0);
    else if (direction == 3)
        draw_sprite_vh_flip(newPic, spriteset[sds->slot], 0, 0);

    wfreeblock(spriteset[sds->slot]);

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

    block target = spriteset[sds->slot];
    block source = spriteset[sourceSprite];

    if (bitmap_color_depth(target) != bitmap_color_depth(source))
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

    unsigned int maskColor = bitmap_mask_color(source);
    int colDep = bitmap_color_depth(source);
    int bytesPerPixel = (colDep + 1) / 8;

    unsigned short *shortPtr;
    unsigned long *longPtr;
    for (int y = 0; y < target->h; y++)
    {
        unsigned char * sourcePixel = source->line[y];
        unsigned char * targetPixel = target->line[y];
        for (int x = 0; x < target->w; x++)
        {
            shortPtr = (unsigned short*)sourcePixel;
            longPtr = (unsigned long*)sourcePixel;

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
                ((unsigned long*)targetPixel)[0] = maskColor;
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

    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);
    clear_to_color(newPic, bitmap_mask_color(newPic));
    // blit it into the enlarged image
    blit(spriteset[sds->slot], newPic, 0, 0, x, y, spritewidth[sds->slot], spriteheight[sds->slot]);

    wfreeblock(spriteset[sds->slot]);

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

    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);
    // blit it cropped
    blit(spriteset[sds->slot], newPic, x1, y1, 0, 0, newPic->w, newPic->h);

    wfreeblock(spriteset[sds->slot]);

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
    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);
    clear_to_color(newPic, bitmap_mask_color(newPic));

    // rotate the sprite about its centre
    // (+ width%2 fixes one pixel offset problem)
    pivot_sprite(newPic, spriteset[sds->slot], width / 2 + width % 2, height / 2,
        spritewidth[sds->slot] / 2, spriteheight[sds->slot] / 2, itofix(angle));

    wfreeblock(spriteset[sds->slot]);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance) 
{
    block source = spriteset[sds->slot];
    block newPic = create_bitmap_ex(bitmap_color_depth(source), source->w, source->h);

    tint_image(source, newPic, red, green, blue, saturation, (luminance * 25) / 10);

    destroy_bitmap(source);
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

    if (save_bitmap(fileName, spriteset[sds->slot], palette))
        return 0; // failed

    return 1;  // successful
}

ScriptDynamicSprite* DynamicSprite_CreateFromSaveGame(int sgslot, int width, int height) {
    int slotnum = LoadSaveSlotScreenshot(sgslot, width, height);
    if (slotnum) {
        return new ScriptDynamicSprite(slotnum);
    }
    return NULL;
}

ScriptDynamicSprite* DynamicSprite_CreateFromFile(const char *filename) {
    int slotnum = LoadImageFile(filename);
    if (slotnum) {
        return new ScriptDynamicSprite(slotnum);
    }
    return NULL;
}

ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot(int width, int height) {

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    if (width <= 0)
        width = virtual_screen->w;
    else
        width = multiply_up_coordinate(width);

    if (height <= 0)
        height = virtual_screen->h;
    else
        height = multiply_up_coordinate(height);

    BITMAP *newPic;
    if (!gfxDriver->UsesMemoryBackBuffer()) 
    {
        // D3D driver
        BITMAP *scrndump = create_bitmap_ex(final_col_dep, scrnwid, scrnhit);
        gfxDriver->GetCopyOfScreenIntoBitmap(scrndump);

        update_polled_stuff_if_runtime();

        if ((scrnwid != width) || (scrnhit != height))
        {
            newPic = create_bitmap_ex(final_col_dep, width, height);
            stretch_blit(scrndump, newPic,
                0, 0, scrndump->w, scrndump->h,
                0, 0, width, height);
            destroy_bitmap(scrndump);
        }
        else
        {
            newPic = scrndump;
        }
    }
    else
    {
        // resize the sprite to the requested size
        newPic = create_bitmap_ex(bitmap_color_depth(virtual_screen), width, height);

        stretch_blit(virtual_screen, newPic,
            0, 0, virtual_screen->w, virtual_screen->h,
            0, 0, width, height);
    }

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(newPic));
    return new ScriptDynamicSprite(gotSlot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot, int preserveAlphaChannel) {

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    if (!spriteset.doesSpriteExist(slot))
        quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

    // create a new sprite as a copy of the existing one
    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[slot]), spritewidth[slot], spriteheight[slot]);
    if (newPic == NULL)
        return NULL;

    blit(spriteset[slot], newPic, 0, 0, 0, 0, spritewidth[slot], spriteheight[slot]);

    bool hasAlpha = (preserveAlphaChannel) && ((game.spriteflags[slot] & SPF_ALPHACHANNEL) != 0);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic, hasAlpha);
    return new ScriptDynamicSprite(gotSlot);
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

    if ((x < 0) || (y < 0) || (x + width > abuf->w) || (y + height > abuf->h))
        quit("!DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");

    int colDepth = bitmap_color_depth(abuf);

    block newPic = create_bitmap_ex(colDepth, width, height);
    if (newPic == NULL)
        return NULL;

    blit(abuf, newPic, x, y, 0, 0, width, height);

    sds->FinishedDrawingReadOnly();

    add_dynamic_sprite(gotSlot, newPic, (sds->hasAlphaChannel != 0));
    return new ScriptDynamicSprite(gotSlot);
}

ScriptDynamicSprite* DynamicSprite_Create(int width, int height, int alphaChannel) 
{
    multiply_up_coordinates(&width, &height);

    int gotSlot = spriteset.findFreeSlot();
    if (gotSlot <= 0)
        return NULL;

    block newPic = create_bitmap_ex(final_col_dep, width, height);
    if (newPic == NULL)
        return NULL;
    clear_to_color(newPic, bitmap_mask_color(newPic));

    if ((alphaChannel) && (final_col_dep < 32))
        alphaChannel = false;

    add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(newPic), alphaChannel != 0);
    return new ScriptDynamicSprite(gotSlot);
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
    block newPic = create_bitmap_ex(bitmap_color_depth(thisroom.ebscene[frame]), width, height);
    if (newPic == NULL)
        return NULL;

    blit(thisroom.ebscene[frame], newPic, x1, y1, 0, 0, width, height);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(gotSlot, newPic);
    return new ScriptDynamicSprite(gotSlot);
}



void add_dynamic_sprite(int gotSlot, block redin, bool hasAlpha) {

  spriteset.set(gotSlot, redin);

  game.spriteflags[gotSlot] = SPF_DYNAMICALLOC;

  if (bitmap_color_depth(redin) > 8)
    game.spriteflags[gotSlot] |= SPF_HICOLOR;
  if (bitmap_color_depth(redin) > 16)
    game.spriteflags[gotSlot] |= SPF_TRUECOLOR;
  if (hasAlpha)
    game.spriteflags[gotSlot] |= SPF_ALPHACHANNEL;

  spritewidth[gotSlot] = redin->w;
  spriteheight[gotSlot] = redin->h;
}

void free_dynamic_sprite (int gotSlot) {
  int tt;

  if ((gotSlot < 0) || (gotSlot >= spriteset.elements))
    quit("!FreeDynamicSprite: invalid slot number");

  if ((game.spriteflags[gotSlot] & SPF_DYNAMICALLOC) == 0)
    quitprintf("!DeleteSprite: Attempted to free static sprite %d that was not loaded by the script", gotSlot);

  wfreeblock(spriteset[gotSlot]);
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
