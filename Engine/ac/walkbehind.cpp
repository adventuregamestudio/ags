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

#include "ac/walkbehind.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/gamestate.h"
#include "game/game_objects.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "media/audio/audio.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern GameState play;
extern IGraphicsDriver *gfxDriver;


char *walkBehindExists = NULL;  // whether a WB area is in this column
int *walkBehindStartY = NULL, *walkBehindEndY = NULL;
char noWalkBehindsAtAll = 0;
int walkBehindLeft[MAX_OBJ], walkBehindTop[MAX_OBJ];
int walkBehindRight[MAX_OBJ], walkBehindBottom[MAX_OBJ];
IDriverDependantBitmap *walkBehindBitmap[MAX_OBJ];
int walkBehindsCachedForBgNum = 0;
WalkBehindMethodEnum walkBehindMethod = DrawOverCharSprite;
int walk_behind_baselines_changed = 0;

void update_walk_behind_images()
{
  int ee, rr;
  int bpp = (thisroom.Backgrounds[play.bg_frame].Graphic->GetColorDepth() + 7) / 8;
  Bitmap *wbbmp;
  for (ee = 1; ee < MAX_OBJ; ee++)
  {
    update_polled_stuff_if_runtime();
    
    if (walkBehindRight[ee] > 0)
    {
      wbbmp = BitmapHelper::CreateTransparentBitmap( 
                               (walkBehindRight[ee] - walkBehindLeft[ee]) + 1,
                               (walkBehindBottom[ee] - walkBehindTop[ee]) + 1,
							   thisroom.Backgrounds[play.bg_frame].Graphic->GetColorDepth());
      int yy, startX = walkBehindLeft[ee], startY = walkBehindTop[ee];
      for (rr = startX; rr <= walkBehindRight[ee]; rr++)
      {
        for (yy = startY; yy <= walkBehindBottom[ee]; yy++)
        {
          if (thisroom.WalkBehindMask->GetScanLine(yy)[rr] == ee)
          {
            for (int ii = 0; ii < bpp; ii++)
              wbbmp->GetScanLineForWriting(yy - startY)[(rr - startX) * bpp + ii] = thisroom.Backgrounds[play.bg_frame].Graphic->GetScanLine(yy)[rr * bpp + ii];
          }
        }
      }

      update_polled_stuff_if_runtime();

      if (walkBehindBitmap[ee] != NULL)
      {
        gfxDriver->DestroyDDB(walkBehindBitmap[ee]);
      }
      walkBehindBitmap[ee] = gfxDriver->CreateDDBFromBitmap(wbbmp, false);
      delete wbbmp;
    }
  }

  walkBehindsCachedForBgNum = play.bg_frame;
}


void recache_walk_behinds () {
  if (walkBehindExists) {
    free (walkBehindExists);
    free (walkBehindStartY);
    free (walkBehindEndY);
  }

  walkBehindExists = (char*)malloc (thisroom.WalkBehindMask->GetWidth());
  walkBehindStartY = (int*)malloc (thisroom.WalkBehindMask->GetWidth() * sizeof(int));
  walkBehindEndY = (int*)malloc (thisroom.WalkBehindMask->GetWidth() * sizeof(int));
  noWalkBehindsAtAll = 1;

  int ee,rr,tmm;
  const int NO_WALK_BEHIND = 100000;
  for (ee = 0; ee < MAX_OBJ; ee++)
  {
    walkBehindLeft[ee] = NO_WALK_BEHIND;
    walkBehindTop[ee] = NO_WALK_BEHIND;
    walkBehindRight[ee] = 0;
    walkBehindBottom[ee] = 0;

    if (walkBehindBitmap[ee] != NULL)
    {
      gfxDriver->DestroyDDB(walkBehindBitmap[ee]);
      walkBehindBitmap[ee] = NULL;
    }
  }

  update_polled_stuff_if_runtime();

  // since this is an 8-bit memory bitmap, we can just use direct 
  // memory access
  if ((!thisroom.WalkBehindMask->IsLinearBitmap()) || (thisroom.WalkBehindMask->GetColorDepth() != 8))
    quit("Walk behinds bitmap not linear");

  for (ee=0;ee<thisroom.WalkBehindMask->GetWidth();ee++) {
    walkBehindExists[ee] = 0;
    for (rr=0;rr<thisroom.WalkBehindMask->GetHeight();rr++) {
      tmm = thisroom.WalkBehindMask->GetScanLine(rr)[ee];
      //tmm = _getpixel(thisroom.WalkBehindMask,ee,rr);
      if ((tmm >= 1) && (tmm < MAX_OBJ)) {
        if (!walkBehindExists[ee]) {
          walkBehindStartY[ee] = rr;
          walkBehindExists[ee] = tmm;
          noWalkBehindsAtAll = 0;
        }
        walkBehindEndY[ee] = rr + 1;  // +1 to allow bottom line of screen to work

        if (ee < walkBehindLeft[tmm]) walkBehindLeft[tmm] = ee;
        if (rr < walkBehindTop[tmm]) walkBehindTop[tmm] = rr;
        if (ee > walkBehindRight[tmm]) walkBehindRight[tmm] = ee;
        if (rr > walkBehindBottom[tmm]) walkBehindBottom[tmm] = rr;
      }
    }
  }

  if (walkBehindMethod == DrawAsSeparateSprite)
  {
    update_walk_behind_images();
  }
}
