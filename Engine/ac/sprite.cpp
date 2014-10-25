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

#include "ac/common.h"
#include "ac/draw.h"            // USE_15BIT_FIX
#include "ac/gamesetupstruct.h"
#include "ac/sprite.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "main/graphics_mode.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int current_screen_resolution_multiplier;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int our_eip, eip_guinum, eip_guiobj;
extern color palette[256];
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern int convert_16bit_bgr;

void get_new_size_for_sprite (int ee, int ww, int hh, int &newwid, int &newhit) {
    newwid = ww * current_screen_resolution_multiplier;
    newhit = hh * current_screen_resolution_multiplier;
    if (game.spriteflags[ee] & SPF_640x400) 
    {
        if (current_screen_resolution_multiplier == 2) {
            newwid = ww;
            newhit = hh;
        }
        else {
            newwid=(ww/2) * current_screen_resolution_multiplier;
            newhit=(hh/2) * current_screen_resolution_multiplier;
            // just make sure - could crash if wid or hit is 0
            if (newwid < 1)
                newwid = 1;
            if (newhit < 1)
                newhit = 1;
        }
    }
}

// set any alpha-transparent pixels in the image to the appropriate
// RGB mask value so that the blit calls work correctly
void set_rgb_mask_using_alpha_channel(Bitmap *image)
{
    int x, y;

    for (y=0; y < image->GetHeight(); y++) 
    {
        unsigned int*psrc = (unsigned int *)image->GetScanLine(y);

        for (x=0; x < image->GetWidth(); x++) 
        {
            if ((psrc[x] & 0xff000000) == 0x00000000)
                psrc[x] = MASK_COLOR_32;
        }
    }
}

// from is a 32-bit RGBA image, to is a 15/16/24-bit destination image
Bitmap *remove_alpha_channel(Bitmap *from) {
    int depth = ScreenResolution.ColorDepth;

    Bitmap *to = BitmapHelper::CreateBitmap(from->GetWidth(), from->GetHeight(),depth);
    int maskcol = to->GetMaskColor();
    int y,x;
    unsigned int c,b,g,r;

    if (depth == 24) {
        // 32-to-24
        for (y=0; y < from->GetHeight(); y++) {
            unsigned int*psrc = (unsigned int *)from->GetScanLine(y);
            unsigned char*pdest = (unsigned char*)to->GetScanLine(y);

            for (x=0; x < from->GetWidth(); x++) {
                c = psrc[x];
                // less than 50% opaque, remove the pixel
                if (((c >> 24) & 0x00ff) < 128)
                    c = maskcol;

                // copy the RGB values across
                memcpy(&pdest[x * 3], &c, 3);
            }
        }
    }
    else {  // 32 to 15 or 16

        for (y=0; y < from->GetHeight(); y++) {
            unsigned int*psrc = (unsigned int *)from->GetScanLine(y);
            unsigned short*pdest = (unsigned short *)to->GetScanLine(y);

            for (x=0; x < from->GetWidth(); x++) {
                c = psrc[x];
                // less than 50% opaque, remove the pixel
                if (((c >> 24) & 0x00ff) < 128)
                    pdest[x] = maskcol;
                else {
                    // otherwise, copy it across
                    r = (c >> 16) & 0x00ff;
                    g = (c >> 8) & 0x00ff;
                    b = c & 0x00ff;
                    pdest[x] = makecol_depth(depth, r, g, b);
                }
            }
        }
    }

    return to;
}

void pre_save_sprite(int ee) {
    // not used, we don't save
}

// these vars are global to help with debugging
Bitmap *tmpdbl, *curspr;
int newwid, newhit;
void initialize_sprite (int ee) {

    if ((ee < 0) || (ee > spriteset.elements))
        quit("initialize_sprite: invalid sprite number");

    if ((spriteset[ee] == NULL) && (ee > 0)) {
        // replace empty sprites with blue cups, to avoid crashes
        //spriteset[ee] = spriteset[0];
        spriteset.set (ee, spriteset[0]);
        spritewidth[ee] = spritewidth[0];
        spriteheight[ee] = spriteheight[0];
    }
    else if (spriteset[ee]==NULL) {
        spritewidth[ee]=0;
        spriteheight[ee]=0;
    }
    else {
        // stretch sprites to correct resolution
        int oldeip = our_eip;
        our_eip = 4300;

        if (game.spriteflags[ee] & SPF_HADALPHACHANNEL) {
            // we stripped the alpha channel out last time, put
            // it back so that we can remove it properly again
            game.spriteflags[ee] |= SPF_ALPHACHANNEL;
        }

        curspr = spriteset[ee];
        get_new_size_for_sprite (ee, curspr->GetWidth(), curspr->GetHeight(), newwid, newhit);

        eip_guinum = ee;
        eip_guiobj = newwid;

        if ((newwid != curspr->GetWidth()) || (newhit != curspr->GetHeight())) {
            tmpdbl = BitmapHelper::CreateTransparentBitmap(newwid,newhit,curspr->GetColorDepth());
            if (tmpdbl == NULL)
                quit("Not enough memory to load sprite graphics");
            tmpdbl->Acquire ();
            curspr->Acquire ();
            /*#ifdef USE_CUSTOM_EXCEPTION_HANDLER
            __try {
            #endif*/
            tmpdbl->StretchBlt(curspr,RectWH(0,0,tmpdbl->GetWidth(),tmpdbl->GetHeight()), Common::kBitmap_Transparency);
            /*#ifdef USE_CUSTOM_EXCEPTION_HANDLER
            } __except (1) {
            // I can't trace this fault, but occasionally stretch_sprite
            // crashes, even with valid source and dest bitmaps. So,
            // for now, just ignore the exception, since the stretch
            // looks successful
            //MessageBox (allegro_wnd, "ERROR", "FATAL ERROR", MB_OK);
            }
            #endif*/
            curspr->Release ();
            tmpdbl->Release ();
            delete curspr;
            spriteset.set (ee, tmpdbl);
        }

        spritewidth[ee]=spriteset[ee]->GetWidth();
        spriteheight[ee]=spriteset[ee]->GetHeight();

        int spcoldep = spriteset[ee]->GetColorDepth();

        if (((spcoldep > 16) && (ScreenResolution.ColorDepth <= 16)) ||
            ((spcoldep == 16) && (ScreenResolution.ColorDepth > 16))) {
                // 16-bit sprite in 32-bit game or vice versa - convert
                // so that scaling and blit calls work properly
                Bitmap *oldSprite = spriteset[ee];
                Bitmap *newSprite;

                if (game.spriteflags[ee] & SPF_ALPHACHANNEL)
                    newSprite = remove_alpha_channel(oldSprite);
                else {
                    newSprite = BitmapHelper::CreateBitmapCopy(oldSprite, ScreenResolution.ColorDepth);
                }
                spriteset.set(ee, newSprite);
                delete oldSprite;
                spcoldep = ScreenResolution.ColorDepth;
        }
        else if ((spcoldep == 32) && (ScreenResolution.ColorDepth == 32))
        {
#if defined (AGS_INVERTED_COLOR_ORDER)
            // PSP: Convert to BGR color order.
            spriteset.set(ee, convert_32_to_32bgr(spriteset[ee]));
#endif
            if ((game.spriteflags[ee] & SPF_ALPHACHANNEL) != 0)
            {
                set_rgb_mask_using_alpha_channel(spriteset[ee]);
            }
        }

#ifdef USE_15BIT_FIX
        else if ((ScreenResolution.ColorDepth != game.color_depth*8) && (spcoldep == game.color_depth*8)) {
            // running in 15-bit mode with a 16-bit game, convert sprites
            Bitmap *oldsprite = spriteset[ee];

            if (game.spriteflags[ee] & SPF_ALPHACHANNEL)
                // 32-to-24 with alpha channel
                spriteset.set (ee, remove_alpha_channel(oldsprite));
            else
                spriteset.set (ee, convert_16_to_15(oldsprite));

            delete oldsprite;
        }
        if ((convert_16bit_bgr == 1) && (spriteset[ee]->GetColorDepth() == 16))
            spriteset.set (ee, convert_16_to_16bgr (spriteset[ee]));
#endif

        if ((spcoldep == 8) && (ScreenResolution.ColorDepth > 8))
            select_palette(palette);

        spriteset.set(ee, gfxDriver->ConvertBitmapToSupportedColourDepth(spriteset[ee]));

        if ((spcoldep == 8) && (ScreenResolution.ColorDepth > 8))
            unselect_palette();

        if (ScreenResolution.ColorDepth < 32) {
            game.spriteflags[ee] &= ~SPF_ALPHACHANNEL;
            // save the fact that it had one for the next time this
            // is re-loaded from disk
            game.spriteflags[ee] |= SPF_HADALPHACHANNEL;
        }

        platform->RunPluginHooks(AGSE_SPRITELOAD, ee);
        update_polled_stuff_if_runtime();

        our_eip = oldeip;
    }
}
