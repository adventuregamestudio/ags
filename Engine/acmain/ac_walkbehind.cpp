
#include <stdio.h>
#include "wgt2allg.h"
#include "ali3d.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_walkbehind.h"
#include "acmain/ac_commonheaders.h"

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
  int bpp = (bitmap_color_depth(thisroom.ebscene[play.bg_frame]) + 7) / 8;
  BITMAP *wbbmp;
  for (ee = 1; ee < MAX_OBJ; ee++)
  {
    update_polled_stuff_if_runtime();
    
    if (walkBehindRight[ee] > 0)
    {
      wbbmp = create_bitmap_ex(bitmap_color_depth(thisroom.ebscene[play.bg_frame]), 
                               (walkBehindRight[ee] - walkBehindLeft[ee]) + 1,
                               (walkBehindBottom[ee] - walkBehindTop[ee]) + 1);
      clear_to_color(wbbmp, bitmap_mask_color(wbbmp));
      int yy, startX = walkBehindLeft[ee], startY = walkBehindTop[ee];
      for (rr = startX; rr <= walkBehindRight[ee]; rr++)
      {
        for (yy = startY; yy <= walkBehindBottom[ee]; yy++)
        {
          if (thisroom.object->line[yy][rr] == ee)
          {
            for (int ii = 0; ii < bpp; ii++)
              wbbmp->line[yy - startY][(rr - startX) * bpp + ii] = thisroom.ebscene[play.bg_frame]->line[yy][rr * bpp + ii];
          }
        }
      }

      update_polled_stuff_if_runtime();

      if (walkBehindBitmap[ee] != NULL)
      {
        gfxDriver->DestroyDDB(walkBehindBitmap[ee]);
      }
      walkBehindBitmap[ee] = gfxDriver->CreateDDBFromBitmap(wbbmp, false);
      destroy_bitmap(wbbmp);
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

  walkBehindExists = (char*)malloc (thisroom.object->w);
  walkBehindStartY = (int*)malloc (thisroom.object->w * sizeof(int));
  walkBehindEndY = (int*)malloc (thisroom.object->w * sizeof(int));
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
  if ((!is_linear_bitmap(thisroom.object)) || (bitmap_color_depth(thisroom.object) != 8))
    quit("Walk behinds bitmap not linear");

  for (ee=0;ee<thisroom.object->w;ee++) {
    walkBehindExists[ee] = 0;
    for (rr=0;rr<thisroom.object->h;rr++) {
      tmm = thisroom.object->line[rr][ee];
      //tmm = _getpixel(thisroom.object,ee,rr);
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


void SetWalkBehindBase(int wa,int bl) {
  if ((wa < 1) || (wa >= MAX_OBJ))
    quit("!SetWalkBehindBase: invalid walk-behind area specified");

  if (bl != croom->walkbehind_base[wa]) {
    walk_behind_baselines_changed = 1;
    invalidate_cached_walkbehinds();
    croom->walkbehind_base[wa] = bl;
    DEBUG_CONSOLE("Walk-behind %d baseline changed to %d", wa, bl);
  }
}
