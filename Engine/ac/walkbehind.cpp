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
#include "game/game_objects.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "media/audio/audio.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern IGraphicsDriver *gfxDriver;

char *walkBehindExists = NULL;  // whether a WB area is in this column
int *walkBehindStartY = NULL, *walkBehindEndY = NULL;
char noWalkBehindsAtAll = 0;
int walkBehindsCachedForBgNum = 0;
WalkBehindMethodEnum walkBehindMethod = DrawOverCharSprite;
int walk_behind_baselines_changed = 0;

AGS::Common::ObjectArray<WalkBehindPlacement> WalkBehindPlacements;

WalkBehindPlacement::WalkBehindPlacement()
    : Left(0)
    , Top(0)
    , Right(0)
    , Bottom(0)
    , Ddb(NULL)
{
}

WalkBehindPlacement::WalkBehindPlacement(const WalkBehindPlacement &wbplace)
{
    Left = wbplace.Left;
    Top = wbplace.Top;
    Right = wbplace.Right;
    Bottom = wbplace.Bottom;
    Ddb = wbplace.Ddb ? gfxDriver->CreateDDBReference(wbplace.Ddb) : NULL;
}

WalkBehindPlacement::~WalkBehindPlacement()
{
    if (Ddb)
    {
        gfxDriver->DestroyDDB(Ddb);
    }
}

void update_walk_behind_images()
{
  int ee, rr;
  int bpp = (thisroom.Backgrounds[play.RoomBkgFrameIndex].Graphic->GetColorDepth() + 7) / 8;
  Bitmap *wbbmp;
  for (ee = 1; ee < WalkBehindPlacements.GetCount(); ee++)
  {
    update_polled_stuff_if_runtime();
    
    if (WalkBehindPlacements[ee].Right > 0)
    {
      wbbmp = BitmapHelper::CreateTransparentBitmap( 
                               (WalkBehindPlacements[ee].Right - WalkBehindPlacements[ee].Left) + 1,
                               (WalkBehindPlacements[ee].Bottom - WalkBehindPlacements[ee].Top) + 1,
							   thisroom.Backgrounds[play.RoomBkgFrameIndex].Graphic->GetColorDepth());
      int yy, startX = WalkBehindPlacements[ee].Left, startY = WalkBehindPlacements[ee].Top;
      for (rr = startX; rr <= WalkBehindPlacements[ee].Right; rr++)
      {
        for (yy = startY; yy <= WalkBehindPlacements[ee].Bottom; yy++)
        {
          if (thisroom.WalkBehindMask->GetScanLine(yy)[rr] == ee)
          {
            for (int ii = 0; ii < bpp; ii++)
              wbbmp->GetScanLineForWriting(yy - startY)[(rr - startX) * bpp + ii] = thisroom.Backgrounds[play.RoomBkgFrameIndex].Graphic->GetScanLine(yy)[rr * bpp + ii];
          }
        }
      }

      update_polled_stuff_if_runtime();

      if (WalkBehindPlacements[ee].Ddb != NULL)
      {
        gfxDriver->DestroyDDB(WalkBehindPlacements[ee].Ddb);
      }
      WalkBehindPlacements[ee].Ddb = gfxDriver->CreateDDBFromBitmap(wbbmp, false);
      delete wbbmp;
    }
  }

  walkBehindsCachedForBgNum = play.RoomBkgFrameIndex;
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
  WalkBehindPlacements.SetLength(thisroom.WalkBehindCount);
  for (ee = 0; ee < WalkBehindPlacements.GetCount(); ee++)
  {
    WalkBehindPlacements[ee].Left = NO_WALK_BEHIND;
    WalkBehindPlacements[ee].Top = NO_WALK_BEHIND;
    WalkBehindPlacements[ee].Right = 0;
    WalkBehindPlacements[ee].Bottom = 0;

    if (WalkBehindPlacements[ee].Ddb != NULL)
    {
      gfxDriver->DestroyDDB(WalkBehindPlacements[ee].Ddb);
      WalkBehindPlacements[ee].Ddb = NULL;
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
      if ((tmm >= 1) && (tmm < thisroom.WalkBehindCount)) {
        if (!walkBehindExists[ee]) {
          walkBehindStartY[ee] = rr;
          walkBehindExists[ee] = tmm;
          noWalkBehindsAtAll = 0;
        }
        walkBehindEndY[ee] = rr + 1;  // +1 to allow bottom line of screen to work

        if (ee < WalkBehindPlacements[tmm].Left) WalkBehindPlacements[tmm].Left = ee;
        if (rr < WalkBehindPlacements[tmm].Top) WalkBehindPlacements[tmm].Top = rr;
        if (ee > WalkBehindPlacements[tmm].Right) WalkBehindPlacements[tmm].Right = ee;
        if (rr > WalkBehindPlacements[tmm].Bottom) WalkBehindPlacements[tmm].Bottom = rr;
      }
    }
  }

  if (walkBehindMethod == DrawAsSeparateSprite)
  {
    update_walk_behind_images();
  }
}
