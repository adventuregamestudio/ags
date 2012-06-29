
#include <stdio.h>
#include <aastr.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_draw.h"
#include "acmain/ac_commonheaders.h"
#include "ac/ac_compress.h"
#include "plugin/agsplugin.h"
#include "acmain/ac_spritelistentry.h"
#include "ac/ac_object.h"
#include "mousew32.h"
#include "media/audio/audio.h"


#if defined(ANDROID_VERSION)
#include <sys/stat.h>
#include <android/log.h>

extern "C" void android_render();
extern "C" void selectLatestSavegame();
extern bool psp_load_latest_savegame;
#endif


#if defined(IOS_VERSION)
extern "C" void ios_render();
#endif


// How is this actually used??
// We need COLOR_DEPTH_24 to allow it to load the preload PCX in hi-col
BEGIN_COLOR_DEPTH_LIST
    COLOR_DEPTH_8
    COLOR_DEPTH_15
    COLOR_DEPTH_16
    COLOR_DEPTH_24
    COLOR_DEPTH_32
END_COLOR_DEPTH_LIST


color palette[256];

COLOR_MAP maincoltable;

IGraphicsDriver *gfxDriver;
IDriverDependantBitmap *mouseCursor = NULL;
IDriverDependantBitmap *blankImage = NULL;
IDriverDependantBitmap *blankSidebarImage = NULL;
IDriverDependantBitmap *debugConsole = NULL;



// actsps is used for temporary storage of the bitamp image
// of the latest version of the sprite
int actSpsCount = 0;
block *actsps;
IDriverDependantBitmap* *actspsbmp;
// temporary cache of walk-behind for this actsps image
block *actspswb;
IDriverDependantBitmap* *actspswbbmp;
CachedActSpsData* actspswbcache;

block virtual_screen;

bool current_background_is_dirty = false;

block _old_screen=NULL;
block _sub_screen=NULL;

int offsetx = 0, offsety = 0;

int trans_mode=0;

IDriverDependantBitmap* roomBackgroundBmp = NULL;


#define MAX_SPRITES_ON_SCREEN 76
SpriteListEntry sprlist[MAX_SPRITES_ON_SCREEN];
int sprlistsize=0;
#define MAX_THINGS_TO_DRAW 125
SpriteListEntry thingsToDrawList[MAX_THINGS_TO_DRAW];
int thingsToDrawSize = 0;

//GUIMain dummygui;
//GUIButton dummyguicontrol;
block *guibg = NULL;
IDriverDependantBitmap **guibgbmp = NULL;


block debugConsoleBuffer = NULL;
block blank_mouse_cursor = NULL;

// whether there are currently remnants of a DisplaySpeech
int screen_is_dirty = 0;

block raw_saved_screen = NULL;
block dotted_mouse_cursor = NULL;
block dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];


void setpal() {
    wsetpalette(0,255,palette);
}


#ifdef USE_15BIT_FIX
extern "C" {
AL_FUNC(GFX_VTABLE *, _get_vtable, (int color_depth));
}

block convert_16_to_15(block iii) {
//  int xx,yy,rpix;
  int iwid = iii->w, ihit = iii->h;
  int x,y;

  if (bitmap_color_depth(iii) > 16) {
    // we want a 32-to-24 conversion
    block tempbl = create_bitmap_ex(final_col_dep,iwid,ihit);

    if (final_col_dep < 24) {
      // 32-to-16
      blit(iii, tempbl, 0, 0, 0, 0, iwid, ihit);
      return tempbl;
    }

	  GFX_VTABLE *vtable = _get_vtable(final_col_dep);
	  if (vtable == NULL) {
      quit("unable to get 24-bit bitmap vtable");
    }

    tempbl->vtable = vtable;

    for (y=0; y < tempbl->h; y++) {
      unsigned char*p32 = (unsigned char *)iii->line[y];
      unsigned char*p24 = (unsigned char *)tempbl->line[y];

      // strip out the alpha channel bit and copy the rest across
      for (x=0; x < tempbl->w; x++) {
        memcpy(&p24[x * 3], &p32[x * 4], 3);
		  }
    }

    return tempbl;
  }

  // we want a 16-to-15 converstion

  unsigned short c,r,g,b;
  // we do this process manually - no allegro color conversion
  // because we store the RGB in a particular order in the data files
  block tempbl = create_bitmap_ex(15,iwid,ihit);

	GFX_VTABLE *vtable = _get_vtable(15);

	if (vtable == NULL) {
    quit("unable to get 15-bit bitmap vtable");
  }

  tempbl->vtable = vtable;

  for (y=0; y < tempbl->h; y++) {
    unsigned short*p16 = (unsigned short *)iii->line[y];
    unsigned short*p15 = (unsigned short *)tempbl->line[y];

    for (x=0; x < tempbl->w; x++) {
			c = p16[x];
			b = _rgb_scale_5[c & 0x1F];
			g = _rgb_scale_6[(c >> 5) & 0x3F];
			r = _rgb_scale_5[(c >> 11) & 0x1F];
			p15[x] = makecol15(r, g, b);
		}
  }
/*
  // the auto color conversion doesn't seem to work
  for (xx=0;xx<iwid;xx++) {
    for (yy=0;yy<ihit;yy++) {
      rpix = _getpixel16(iii,xx,yy);
      rpix = (rpix & 0x001f) | ((rpix >> 1) & 0x7fe0);
      // again putpixel16 because the dest is actually 16bit
      _putpixel15(tempbl,xx,yy,rpix);
    }
  }*/

  return tempbl;
}

int _places_r = 3, _places_g = 2, _places_b = 3;

// convert RGB to BGR for strange graphics cards
block convert_16_to_16bgr(block tempbl) {

  int x,y;
  unsigned short c,r,g,b;

  for (y=0; y < tempbl->h; y++) {
    unsigned short*p16 = (unsigned short *)tempbl->line[y];

    for (x=0; x < tempbl->w; x++) {
			c = p16[x];
      if (c != MASK_COLOR_16) {
        b = _rgb_scale_5[c & 0x1F];
			  g = _rgb_scale_6[(c >> 5) & 0x3F];
			  r = _rgb_scale_5[(c >> 11) & 0x1F];
        // allegro assumes 5-6-5 for 16-bit
        p16[x] = (((r >> _places_r) << _rgb_r_shift_16) |
            ((g >> _places_g) << _rgb_g_shift_16) |
            ((b >> _places_b) << _rgb_b_shift_16));

      }
		}
  }

  return tempbl;
}
#endif



// PSP: convert 32 bit RGB to BGR.
block convert_32_to_32bgr(block tempbl) {

    unsigned char* current = tempbl->line[0];

    int i = 0;
    int j = 0;
    while (i < tempbl->h)
    {
        current = tempbl->line[i];
        while (j < tempbl->w)
        {
            current[0] ^= current[2];
            current[2] ^= current[0];
            current[0] ^= current[2];
            current += 4;
            j++;
        }
        i++;
        j = 0;
    }

    return tempbl;
}





// Begin resolution system functions

// Multiplies up the number of pixels depending on the current 
// resolution, to give a relatively fixed size at any game res
AGS_INLINE int get_fixed_pixel_size(int pixels)
{
  return pixels * current_screen_resolution_multiplier;
}

AGS_INLINE int convert_to_low_res(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord;
  else
    return coord / current_screen_resolution_multiplier;
}

AGS_INLINE int convert_back_to_high_res(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord;
  else
    return coord * current_screen_resolution_multiplier;
}

AGS_INLINE int multiply_up_coordinate(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord * current_screen_resolution_multiplier;
  else
    return coord;
}

AGS_INLINE void multiply_up_coordinates(int *x, int *y)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
  {
    x[0] *= current_screen_resolution_multiplier;
    y[0] *= current_screen_resolution_multiplier;
  }
}

AGS_INLINE void multiply_up_coordinates_round_up(int *x, int *y)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
  {
    x[0] = x[0] * current_screen_resolution_multiplier + (current_screen_resolution_multiplier - 1);
    y[0] = y[0] * current_screen_resolution_multiplier + (current_screen_resolution_multiplier - 1);
  }
}

AGS_INLINE int divide_down_coordinate(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord / current_screen_resolution_multiplier;
  else
    return coord;
}

AGS_INLINE int divide_down_coordinate_round_up(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return (coord / current_screen_resolution_multiplier) + (current_screen_resolution_multiplier - 1);
  else
    return coord;
}

// End resolution system functions



// ** dirty rectangle system **

#define MAXDIRTYREGIONS 25
#define WHOLESCREENDIRTY (MAXDIRTYREGIONS + 5)
#define MAX_SPANS_PER_ROW 4
struct InvalidRect {
    int x1, y1, x2, y2;
};
struct IRSpan {
    int x1, x2;
    int mergeSpan(int tx1, int tx2);
};
struct IRRow {
    IRSpan span[MAX_SPANS_PER_ROW];
    int numSpans;
};
IRRow *dirtyRow = NULL;
int _dirtyRowSize;
InvalidRect dirtyRegions[MAXDIRTYREGIONS];
int numDirtyRegions = 0;
int numDirtyBytes = 0;

int IRSpan::mergeSpan(int tx1, int tx2) {
    if ((tx1 > x2) || (tx2 < x1))
        return 0;
    // overlapping, increase the span
    if (tx1 < x1)
        x1 = tx1;
    if (tx2 > x2)
        x2 = tx2;
    return 1;
}

void init_invalid_regions(int scrnHit) {
  numDirtyRegions = WHOLESCREENDIRTY;
  dirtyRow = (IRRow*)malloc(sizeof(IRRow) * scrnHit);

  for (int e = 0; e < scrnHit; e++)
    dirtyRow[e].numSpans = 0;
  _dirtyRowSize = scrnHit;
}

void update_invalid_region(int x, int y, block src, block dest) {
  int i;

  // convert the offsets for the destination into
  // offsets into the source
  x = -x;
  y = -y;

  if (numDirtyRegions == WHOLESCREENDIRTY) {
    blit(src, dest, x, y, 0, 0, dest->w, dest->h);
  }
  else {
    int k, tx1, tx2, srcdepth = bitmap_color_depth(src);
    if ((srcdepth == bitmap_color_depth(dest)) && (is_memory_bitmap(dest))) {
      int bypp = bmp_bpp(src);
      // do the fast copy
      for (i = 0; i < scrnhit; i++) {
        for (k = 0; k < dirtyRow[i].numSpans; k++) {
          tx1 = dirtyRow[i].span[k].x1;
          tx2 = dirtyRow[i].span[k].x2;
          memcpyfast(&dest->line[i][tx1 * bypp], &src->line[i + y][(tx1 + x) * bypp], ((tx2 - tx1) + 1) * bypp);
        }
      }
    }
    else {
      // do the fast copy
      int rowsInOne;
      for (i = 0; i < scrnhit; i++) {
        rowsInOne = 1;
        
        // if there are rows with identical masks, do them all in one go
        while ((i+rowsInOne < scrnhit) && (memcmp(&dirtyRow[i], &dirtyRow[i+rowsInOne], sizeof(IRRow)) == 0))
          rowsInOne++;

        for (k = 0; k < dirtyRow[i].numSpans; k++) {
          tx1 = dirtyRow[i].span[k].x1;
          tx2 = dirtyRow[i].span[k].x2;
          blit(src, dest, tx1 + x, i + y, tx1, i, (tx2 - tx1) + 1, rowsInOne);
        }
        
        i += (rowsInOne - 1);
      }
    }
   /* else {
      // update the dirty regions
      for (i = 0; i < numDirtyRegions; i++) {
        blit(src, dest, x + dirtyRegions[i].x1, y + dirtyRegions[i].y1,
           dirtyRegions[i].x1, dirtyRegions[i].y1,
           (dirtyRegions[i].x2 - dirtyRegions[i].x1) + 1,
           (dirtyRegions[i].y2 - dirtyRegions[i].y1) + 1);
      }
    }*/
  }
}


void update_invalid_region_and_reset(int x, int y, block src, block dest) {

  int i;

  update_invalid_region(x, y, src, dest);

  // screen has been updated, no longer dirty
  numDirtyRegions = 0;
  numDirtyBytes = 0;

  for (i = 0; i < _dirtyRowSize; i++)
    dirtyRow[i].numSpans = 0;

}

int combine_new_rect(InvalidRect *r1, InvalidRect *r2) {

  // check if new rect is within old rect X-wise
  if ((r2->x1 >= r1->x1) && (r2->x2 <= r1->x2)) {
    if ((r2->y1 >= r1->y1) && (r2->y2 <= r1->y2)) {
      // Y is also within the old one - scrap the new rect
      return 1;
    }
  }

  return 0;
}

void invalidate_rect(int x1, int y1, int x2, int y2) {
  if (numDirtyRegions >= MAXDIRTYREGIONS) {
    // too many invalid rectangles, just mark the whole thing dirty
    numDirtyRegions = WHOLESCREENDIRTY;
    return;
  }

  int a;

  if (x1 >= scrnwid) x1 = scrnwid-1;
  if (y1 >= scrnhit) y1 = scrnhit-1;
  if (x2 >= scrnwid) x2 = scrnwid-1;
  if (y2 >= scrnhit) y2 = scrnhit-1;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 < 0) x2 = 0;
  if (y2 < 0) y2 = 0;
/*
  dirtyRegions[numDirtyRegions].x1 = x1;
  dirtyRegions[numDirtyRegions].y1 = y1;
  dirtyRegions[numDirtyRegions].x2 = x2;
  dirtyRegions[numDirtyRegions].y2 = y2;

  for (a = 0; a < numDirtyRegions; a++) {
    // see if we can merge it into any other regions
    if (combine_new_rect(&dirtyRegions[a], &dirtyRegions[numDirtyRegions]))
      return;
  }

  numDirtyBytes += (x2 - x1) * (y2 - y1);

  if (numDirtyBytes > (scrnwid * scrnhit) / 2)
    numDirtyRegions = WHOLESCREENDIRTY;
  else {*/
    numDirtyRegions++;

    // ** Span code
    int s, foundOne;
    // add this rect to the list for this row
    for (a = y1; a <= y2; a++) {
      foundOne = 0;
      for (s = 0; s < dirtyRow[a].numSpans; s++) {
        if (dirtyRow[a].span[s].mergeSpan(x1, x2)) {
          foundOne = 1;
          break;
        }
      }
      if (foundOne) {
        // we were merged into a span, so we're ok
        int t;
        // check whether now two of the spans overlap each other
        // in which case merge them
        for (s = 0; s < dirtyRow[a].numSpans; s++) {
          for (t = s + 1; t < dirtyRow[a].numSpans; t++) {
            if (dirtyRow[a].span[s].mergeSpan(dirtyRow[a].span[t].x1, dirtyRow[a].span[t].x2)) {
              dirtyRow[a].numSpans--;
              for (int u = t; u < dirtyRow[a].numSpans; u++)
                dirtyRow[a].span[u] = dirtyRow[a].span[u + 1];
              break;
            }
          }
        }
      }
      else if (dirtyRow[a].numSpans < MAX_SPANS_PER_ROW) {
        dirtyRow[a].span[dirtyRow[a].numSpans].x1 = x1;
        dirtyRow[a].span[dirtyRow[a].numSpans].x2 = x2;
        dirtyRow[a].numSpans++;
      }
      else {
        // didn't fit in an existing span, and there are none spare
        int nearestDist = 99999, nearestWas = -1, extendLeft;
        int tleft, tright;
        // find the nearest span, and enlarge that to include this rect
        for (s = 0; s < dirtyRow[a].numSpans; s++) {
          tleft = dirtyRow[a].span[s].x1 - x2;
          if ((tleft > 0) && (tleft < nearestDist)) {
            nearestDist = tleft;
            nearestWas = s;
            extendLeft = 1;
          }
          tright = x1 - dirtyRow[a].span[s].x2;
          if ((tright > 0) && (tright < nearestDist)) {
            nearestDist = tright;
            nearestWas = s;
            extendLeft = 0;
          }
        }
        if (extendLeft)
          dirtyRow[a].span[nearestWas].x1 = x1;
        else
          dirtyRow[a].span[nearestWas].x2 = x2;
      }
    }
    // ** End span code
  //}
}



void invalidate_sprite(int x1, int y1, IDriverDependantBitmap *pic) {
    invalidate_rect(x1, y1, x1 + pic->GetWidth(), y1 + pic->GetHeight());
}

void draw_and_invalidate_text(int x1, int y1, int font, const char *text) {
    wouttext_outline(x1, y1, font, (char*)text);
    invalidate_rect(x1, y1, x1 + wgettextwidth_compensate(text, font), y1 + wgetfontheight(font) + get_fixed_pixel_size(1));
}

void invalidate_screen() {
    // mark the whole screen dirty
    numDirtyRegions = WHOLESCREENDIRTY;
}

// ** dirty rectangle system end **

void mark_current_background_dirty()
{
    current_background_is_dirty = true;
}


void TintScreen(int red, int grn, int blu) {
  if ((red < 0) || (grn < 0) || (blu < 0) || (red > 100) || (grn > 100) || (blu > 100))
    quit("!TintScreen: RGB values must be 0-100");

  invalidate_screen();

  if ((red == 0) && (grn == 0) && (blu == 0)) {
    play.screen_tint = -1;
    return;
  }
  red = (red * 25) / 10;
  grn = (grn * 25) / 10;
  blu = (blu * 25) / 10;
  play.screen_tint = red + (grn << 8) + (blu << 16);
}

int get_screen_y_adjustment(BITMAP *checkFor) {

  if ((screen == _sub_screen) && (checkFor->h < final_scrn_hit))
    return get_fixed_pixel_size(20);

  return 0;
}

int get_screen_x_adjustment(BITMAP *checkFor) {

  if ((screen == _sub_screen) && (checkFor->w < final_scrn_wid))
    return (final_scrn_wid - checkFor->w) / 2;

  return 0;
}

void render_black_borders(int atx, int aty)
{
  if (!gfxDriver->UsesMemoryBackBuffer())
  {
    if (aty > 0)
    {
      // letterbox borders
      blankImage->SetStretch(scrnwid, aty);
      gfxDriver->DrawSprite(0, -aty, blankImage);
      gfxDriver->DrawSprite(0, scrnhit, blankImage);
    }
    if (atx > 0)
    {
      // sidebar borders for widescreen
      blankSidebarImage->SetStretch(atx, scrnhit);
      gfxDriver->DrawSprite(-atx, 0, blankSidebarImage);
      gfxDriver->DrawSprite(scrnwid, 0, blankSidebarImage);
    }
  }
}


void render_to_screen(BITMAP *toRender, int atx, int aty) {

  atx += get_screen_x_adjustment(toRender);
  aty += get_screen_y_adjustment(toRender);
  gfxDriver->SetRenderOffset(atx, aty);

  render_black_borders(atx, aty);

  gfxDriver->DrawSprite(AGSE_FINALSCREENDRAW, 0, NULL);

  if (play.screen_is_faded_out)
  {
    if (gfxDriver->UsesMemoryBackBuffer())
      gfxDriver->RenderToBackBuffer();
    gfxDriver->ClearDrawList();
    return;
  }

  // only vsync in full screen mode, it makes things worse
  // in a window
  gfxDriver->EnableVsyncBeforeRender((scsystem.vsync > 0) && (usetup.windowed == 0));

  bool succeeded = false;
  while (!succeeded)
  {
    try
    {
      gfxDriver->Render((GlobalFlipType)play.screen_flipped);

#if defined(ANDROID_VERSION)
      if (game.color_depth == 1)
        android_render();
#elif defined(IOS_VERSION)
      if (game.color_depth == 1)
        ios_render();
#endif

      succeeded = true;
    }
    catch (Ali3DFullscreenLostException) 
    { 
      platform->Delay(500);
    }
  }
}


void clear_letterbox_borders() {

  if (multiply_up_coordinate(thisroom.height) < final_scrn_hit) {
    // blank out any traces in borders left by a full-screen room
    gfxDriver->ClearRectangle(0, 0, _old_screen->w - 1, get_fixed_pixel_size(20) - 1, NULL);
    gfxDriver->ClearRectangle(0, final_scrn_hit - get_fixed_pixel_size(20), _old_screen->w - 1, final_scrn_hit - 1, NULL);
  }

}

// writes the virtual screen to the screen, converting colours if
// necessary
void write_screen() {

    if (play.fast_forward)
        return;

    static int wasShakingScreen = 0;
    bool clearScreenBorders = false;
    int at_yp = 0;

    if (play.shakesc_length > 0) {
        wasShakingScreen = 1;
        if ( (loopcounter % play.shakesc_delay) < (play.shakesc_delay / 2) )
            at_yp = multiply_up_coordinate(play.shakesc_amount);
        invalidate_screen();
    }
    else if (wasShakingScreen) {
        wasShakingScreen = 0;

        if (!gfxDriver->RequiresFullRedrawEachFrame())
        {
            clear_letterbox_borders();
        }
    }

    if (play.screen_tint < 1)
        gfxDriver->SetScreenTint(0, 0, 0);
    else
        gfxDriver->SetScreenTint(play.screen_tint & 0xff, (play.screen_tint >> 8) & 0xff, (play.screen_tint >> 16) & 0xff);

    render_to_screen(virtual_screen, 0, at_yp);
}



void draw_screen_callback()
{
    construct_virtual_screen(false);

    render_black_borders(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));
}



void putpixel_compensate (block onto, int xx,int yy, int col) {
  if ((bitmap_color_depth(onto) == 32) && (col != 0)) {
    // ensure the alpha channel is preserved if it has one
    int alphaval = geta32(getpixel(onto, xx, yy));
    col = makeacol32(getr32(col), getg32(col), getb32(col), alphaval);
  }
  rectfill(onto, xx, yy, xx + get_fixed_pixel_size(1) - 1, yy + get_fixed_pixel_size(1) - 1, col);
}




void draw_sprite_support_alpha(int xpos, int ypos, block image, int slot) {

    if ((game.spriteflags[slot] & SPF_ALPHACHANNEL) && (trans_mode == 0)) 
    {
        set_alpha_blender();
        draw_trans_sprite(abuf, image, xpos, ypos);
    }
    else {
        put_sprite_256(xpos, ypos, image);
    }

}


// Avoid freeing and reallocating the memory if possible
IDriverDependantBitmap* recycle_ddb_bitmap(IDriverDependantBitmap *bimp, BITMAP *source, bool hasAlpha) {
    if (bimp != NULL) {
        // same colour depth, width and height -> reuse
        if (((bimp->GetColorDepth() + 1) / 8 == bmp_bpp(source)) && 
            (bimp->GetWidth() == source->w) && (bimp->GetHeight() == source->h))
        {
            gfxDriver->UpdateDDBFromBitmap(bimp, source, hasAlpha);
            return bimp;
        }

        gfxDriver->DestroyDDB(bimp);
    }
    bimp = gfxDriver->CreateDDBFromBitmap(source, hasAlpha, false);
    return bimp;
}

// sort_out_walk_behinds: modifies the supplied sprite by overwriting parts
// of it with transparent pixels where there are walk-behind areas
// Returns whether any pixels were updated
int sort_out_walk_behinds(block sprit,int xx,int yy,int basel, block copyPixelsFrom = NULL, block checkPixelsFrom = NULL, int zoom=100) {
    if (noWalkBehindsAtAll)
        return 0;

    if ((!is_memory_bitmap(thisroom.object)) ||
        (!is_memory_bitmap(sprit)))
        quit("!sort_out_walk_behinds: wb bitmap not linear");

    int rr,tmm, toheight;//,tcol;
    // precalculate this to try and shave some time off
    int maskcol = bitmap_mask_color(sprit);
    int spcoldep = bitmap_color_depth(sprit);
    int screenhit = thisroom.object->h;
    short *shptr, *shptr2;
    long *loptr, *loptr2;
    int pixelsChanged = 0;
    int ee = 0;
    if (xx < 0)
        ee = 0 - xx;

    if ((checkPixelsFrom != NULL) && (bitmap_color_depth(checkPixelsFrom) != spcoldep))
        quit("sprite colour depth does not match background colour depth");

    for ( ; ee < sprit->w; ee++) {
        if (ee + xx >= thisroom.object->w)
            break;

        if ((!walkBehindExists[ee+xx]) ||
            (walkBehindEndY[ee+xx] <= yy) ||
            (walkBehindStartY[ee+xx] > yy+sprit->h))
            continue;

        toheight = sprit->h;

        if (walkBehindStartY[ee+xx] < yy)
            rr = 0;
        else
            rr = (walkBehindStartY[ee+xx] - yy);

        // Since we will use _getpixel, ensure we only check within the screen
        if (rr + yy < 0)
            rr = 0 - yy;
        if (toheight + yy > screenhit)
            toheight = screenhit - yy;
        if (toheight + yy > walkBehindEndY[ee+xx])
            toheight = walkBehindEndY[ee+xx] - yy;
        if (rr < 0)
            rr = 0;

        for ( ; rr < toheight;rr++) {

            // we're ok with _getpixel because we've checked the screen edges
            //tmm = _getpixel(thisroom.object,ee+xx,rr+yy);
            // actually, _getpixel is well inefficient, do it ourselves
            // since we know it's 8-bit bitmap
            tmm = thisroom.object->line[rr+yy][ee+xx];
            if (tmm<1) continue;
            if (croom->walkbehind_base[tmm] <= basel) continue;

            if (copyPixelsFrom != NULL)
            {
                if (spcoldep <= 8)
                {
                    if (checkPixelsFrom->line[(rr * 100) / zoom][(ee * 100) / zoom] != maskcol) {
                        sprit->line[rr][ee] = copyPixelsFrom->line[rr + yy][ee + xx];
                        pixelsChanged = 1;
                    }
                }
                else if (spcoldep <= 16) {
                    shptr = (short*)&sprit->line[rr][0];
                    shptr2 = (short*)&checkPixelsFrom->line[(rr * 100) / zoom][0];
                    if (shptr2[(ee * 100) / zoom] != maskcol) {
                        shptr[ee] = ((short*)(&copyPixelsFrom->line[rr + yy][0]))[ee + xx];
                        pixelsChanged = 1;
                    }
                }
                else if (spcoldep == 24) {
                    char *chptr = (char*)&sprit->line[rr][0];
                    char *chptr2 = (char*)&checkPixelsFrom->line[(rr * 100) / zoom][0];
                    if (memcmp(&chptr2[((ee * 100) / zoom) * 3], &maskcol, 3) != 0) {
                        memcpy(&chptr[ee * 3], &copyPixelsFrom->line[rr + yy][(ee + xx) * 3], 3);
                        pixelsChanged = 1;
                    }
                }
                else if (spcoldep <= 32) {
                    loptr = (long*)&sprit->line[rr][0];
                    loptr2 = (long*)&checkPixelsFrom->line[(rr * 100) / zoom][0];
                    if (loptr2[(ee * 100) / zoom] != maskcol) {
                        loptr[ee] = ((long*)(&copyPixelsFrom->line[rr + yy][0]))[ee + xx];
                        pixelsChanged = 1;
                    }
                }
            }
            else
            {
                pixelsChanged = 1;
                if (spcoldep <= 8)
                    sprit->line[rr][ee] = maskcol;
                else if (spcoldep <= 16) {
                    shptr = (short*)&sprit->line[rr][0];
                    shptr[ee] = maskcol;
                }
                else if (spcoldep == 24) {
                    char *chptr = (char*)&sprit->line[rr][0];
                    memcpy(&chptr[ee * 3], &maskcol, 3);
                }
                else if (spcoldep <= 32) {
                    loptr = (long*)&sprit->line[rr][0];
                    loptr[ee] = maskcol;
                }
                else
                    quit("!Sprite colour depth >32 ??");
            }
        }
    }
    return pixelsChanged;
}

void invalidate_cached_walkbehinds() 
{
    memset(&actspswbcache[0], 0, sizeof(CachedActSpsData) * actSpsCount);
}

void sort_out_char_sprite_walk_behind(int actspsIndex, int xx, int yy, int basel, int zoom, int width, int height)
{
    if (noWalkBehindsAtAll)
        return;

    if ((!actspswbcache[actspsIndex].valid) ||
        (actspswbcache[actspsIndex].xWas != xx) ||
        (actspswbcache[actspsIndex].yWas != yy) ||
        (actspswbcache[actspsIndex].baselineWas != basel))
    {
        actspswb[actspsIndex] = recycle_bitmap(actspswb[actspsIndex], bitmap_color_depth(thisroom.ebscene[play.bg_frame]), width, height);

        block wbSprite = actspswb[actspsIndex];
        clear_to_color(wbSprite, bitmap_mask_color(wbSprite));

        actspswbcache[actspsIndex].isWalkBehindHere = sort_out_walk_behinds(wbSprite, xx, yy, basel, thisroom.ebscene[play.bg_frame], actsps[actspsIndex], zoom);
        actspswbcache[actspsIndex].xWas = xx;
        actspswbcache[actspsIndex].yWas = yy;
        actspswbcache[actspsIndex].baselineWas = basel;
        actspswbcache[actspsIndex].valid = 1;

        if (actspswbcache[actspsIndex].isWalkBehindHere)
        {
            actspswbbmp[actspsIndex] = recycle_ddb_bitmap(actspswbbmp[actspsIndex], actspswb[actspsIndex], false);
        }
    }

    if (actspswbcache[actspsIndex].isWalkBehindHere)
    {
        add_to_sprite_list(actspswbbmp[actspsIndex], xx - offsetx, yy - offsety, basel, 0, -1, true);
    }
}

void clear_draw_list() {
    thingsToDrawSize = 0;
}
void add_thing_to_draw(IDriverDependantBitmap* bmp, int x, int y, int trans, bool alphaChannel) {
    thingsToDrawList[thingsToDrawSize].pic = NULL;
    thingsToDrawList[thingsToDrawSize].bmp = bmp;
    thingsToDrawList[thingsToDrawSize].x = x;
    thingsToDrawList[thingsToDrawSize].y = y;
    thingsToDrawList[thingsToDrawSize].transparent = trans;
    thingsToDrawList[thingsToDrawSize].hasAlphaChannel = alphaChannel;
    thingsToDrawSize++;
    if (thingsToDrawSize >= MAX_THINGS_TO_DRAW - 1)
        quit("add_thing_to_draw: too many things added");
}

// the sprite list is an intermediate list used to order 
// objects and characters by their baselines before everything
// is added to the Thing To Draw List
void clear_sprite_list() {
    sprlistsize=0;
}
void add_to_sprite_list(IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind) {

    // completely invisible, so don't draw it at all
    if (trans == 255)
        return;

    if ((sprNum >= 0) && ((game.spriteflags[sprNum] & SPF_ALPHACHANNEL) != 0))
        sprlist[sprlistsize].hasAlphaChannel = true;
    else
        sprlist[sprlistsize].hasAlphaChannel = false;

    sprlist[sprlistsize].bmp = spp;
    sprlist[sprlistsize].baseline = baseline;
    sprlist[sprlistsize].x=xx;
    sprlist[sprlistsize].y=yy;
    sprlist[sprlistsize].transparent=trans;

    if (walkBehindMethod == DrawAsSeparateSprite)
        sprlist[sprlistsize].takesPriorityIfEqual = !isWalkBehind;
    else
        sprlist[sprlistsize].takesPriorityIfEqual = isWalkBehind;

    sprlistsize++;

    if (sprlistsize >= MAX_SPRITES_ON_SCREEN)
        quit("Too many sprites have been added to the sprite list. There is a limit of 75 objects and characters being visible at the same time. You may want to reconsider your design since you have over 75 objects/characters visible at once.");

    if (spp == NULL)
        quit("add_to_sprite_list: attempted to draw NULL sprite");
}

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
extern int psp_gfx_smoothing;
extern int psp_gfx_scaling;
extern int psp_gfx_renderer;
extern int psp_gfx_super_sampling;
#endif

void put_sprite_256(int xxx,int yyy,block piccy) {

    if (trans_mode >= 255) {
        // fully transparent, don't draw it at all
        trans_mode = 0;
        return;
    }

    int screen_depth = bitmap_color_depth(abuf);

#ifdef USE_15BIT_FIX
    if ((bitmap_color_depth(piccy) < screen_depth) 
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
        || ((bmp_bpp(abuf) < screen_depth) && (psp_gfx_renderer > 0)) // Fix for corrupted speechbox outlines with the OGL driver
#endif
        ) {
            if ((bitmap_color_depth(piccy) == 8) && (screen_depth >= 24)) {
                // 256-col sprite -> truecolor background
                // this is automatically supported by allegro, no twiddling needed
                draw_sprite(abuf, piccy, xxx, yyy);
                return;
            }
            // 256-col spirte -> hi-color background, or
            // 16-bit sprite -> 32-bit background
            block hctemp=create_bitmap_ex(screen_depth, piccy->w, piccy->h);
            blit(piccy,hctemp,0,0,0,0,hctemp->w,hctemp->h);
            int bb,cc,mask_col = bitmap_mask_color(abuf);
            if (bitmap_color_depth(piccy) == 8) {
                // only do this for 256-col, cos the blit call converts
                // transparency for 16->32 bit
                for (bb=0;bb<hctemp->w;bb++) {
                    for (cc=0;cc<hctemp->h;cc++)
                        if (_getpixel(piccy,bb,cc)==0) putpixel(hctemp,bb,cc,mask_col);
                }
            }
            wputblock(xxx,yyy,hctemp,1);
            wfreeblock(hctemp);
    }
    else
#endif
    {
        if ((trans_mode!=0) && (game.color_depth > 1) && (bmp_bpp(piccy) > 1) && (bmp_bpp(abuf) > 1)) {
            set_trans_blender(0,0,0,trans_mode);
            draw_trans_sprite(abuf,piccy,xxx,yyy);
        }
        /*    else if ((lit_mode < 0) && (game.color_depth == 1) && (bmp_bpp(piccy) == 1)) {
        draw_lit_sprite(abuf,piccy,xxx,yyy,250 - ((-lit_mode) * 5)/2);
        }*/
        else
            wputblock(xxx,yyy,piccy,1);
    }
    trans_mode=0;
}

void repair_alpha_channel(block dest, block bgpic)
{
    // Repair the alpha channel, because sprites may have been drawn
    // over it by the buttons, etc
    int theWid = (dest->w < bgpic->w) ? dest->w : bgpic->w;
    int theHit = (dest->h < bgpic->h) ? dest->h : bgpic->h;
    for (int y = 0; y < theHit; y++) 
    {
        unsigned long *destination = ((unsigned long*)dest->line[y]);
        unsigned long *source = ((unsigned long*)bgpic->line[y]);
        for (int x = 0; x < theWid; x++) 
        {
            destination[x] |= (source[x] & 0xff000000);
        }
    }
}


// used by GUI renderer to draw images
void draw_sprite_compensate(int picc,int xx,int yy,int useAlpha) 
{
    if ((useAlpha) && 
        (game.options[OPT_NEWGUIALPHA] > 0) &&
        (bitmap_color_depth(abuf) == 32))
    {
        if (game.spriteflags[picc] & SPF_ALPHACHANNEL)
            set_additive_alpha_blender();
        else
            set_opaque_alpha_blender();

        draw_trans_sprite(abuf, spriteset[picc], xx, yy);
    }
    else
    {
        put_sprite_256(xx, yy, spriteset[picc]);
    }
}

// function to sort the sprites into baseline order
extern "C" int compare_listentries(const void *elem1, const void *elem2) {
    SpriteListEntry *e1, *e2;
    e1 = (SpriteListEntry*)elem1;
    e2 = (SpriteListEntry*)elem2;

    if (e1->baseline == e2->baseline) 
    { 
        if (e1->takesPriorityIfEqual)
            return 1;
        if (e2->takesPriorityIfEqual)
            return -1;

        // Trying to make the order of equal elements reproducible across
        // different libc implementations here
#if defined WINDOWS_VERSION
        return -1;
#else
        return 1;
#endif
    }

    // returns >0 if e1 is lower down, <0 if higher, =0 if the same
    return e1->baseline - e2->baseline;
}




void draw_sprite_list() {

    if (walkBehindMethod == DrawAsSeparateSprite)
    {
        for (int ee = 1; ee < MAX_OBJ; ee++)
        {
            if (walkBehindBitmap[ee] != NULL)
            {
                add_to_sprite_list(walkBehindBitmap[ee], walkBehindLeft[ee] - offsetx, walkBehindTop[ee] - offsety, 
                    croom->walkbehind_base[ee], 0, -1, true);
            }
        }
    }

    // 2.60.672 - convert horrid bubble sort to use qsort instead
    qsort(sprlist, sprlistsize, sizeof(SpriteListEntry), compare_listentries);

    clear_draw_list();

    add_thing_to_draw(NULL, AGSE_PRESCREENDRAW, 0, TRANS_RUN_PLUGIN, false);

    // copy the sorted sprites into the Things To Draw list
    thingsToDrawSize += sprlistsize;
    memcpy(&thingsToDrawList[1], sprlist, sizeof(SpriteListEntry) * sprlistsize);
}

// Avoid freeing and reallocating the memory if possible
block recycle_bitmap(block bimp, int coldep, int wid, int hit) {
    if (bimp != NULL) {
        // same colour depth, width and height -> reuse
        if ((bitmap_color_depth(bimp) == coldep) && (bimp->w == wid)
            && (bimp->h == hit))
            return bimp;

        destroy_bitmap(bimp);
    }
    bimp = create_bitmap_ex(coldep, wid, hit);
    return bimp;
}


// Get the local tint at the specified X & Y co-ordinates, based on
// room regions and SetAmbientTint
// tint_amnt will be set to 0 if there is no tint enabled
// if this is the case, then light_lev holds the light level (0=none)
void get_local_tint(int xpp, int ypp, int nolight,
                    int *tint_amnt, int *tint_r, int *tint_g,
                    int *tint_b, int *tint_lit,
                    int *light_lev) {

  int tint_level = 0, light_level = 0;
  int tint_amount = 0;
  int tint_red = 0;
  int tint_green = 0;
  int tint_blue = 0;
  int tint_light = 255;

  if (nolight == 0) {

    int onRegion = 0;

    if ((play.ground_level_areas_disabled & GLED_EFFECTS) == 0) {
      // check if the player is on a region, to find its
      // light/tint level
      onRegion = GetRegionAt (xpp, ypp);
      if (onRegion == 0) {
        // when walking, he might just be off a walkable area
        onRegion = GetRegionAt (xpp - 3, ypp);
        if (onRegion == 0)
          onRegion = GetRegionAt (xpp + 3, ypp);
        if (onRegion == 0)
          onRegion = GetRegionAt (xpp, ypp - 3);
        if (onRegion == 0)
          onRegion = GetRegionAt (xpp, ypp + 3);
      }
    }

    if ((onRegion > 0) && (onRegion <= MAX_REGIONS)) {
      light_level = thisroom.regionLightLevel[onRegion];
      tint_level = thisroom.regionTintLevel[onRegion];
    }
    else if (onRegion <= 0) {
      light_level = thisroom.regionLightLevel[0];
      tint_level = thisroom.regionTintLevel[0];
    }
    if ((game.color_depth == 1) || ((tint_level & 0x00ffffff) == 0) ||
        ((tint_level & TINT_IS_ENABLED) == 0))
      tint_level = 0;

    if (tint_level) {
      tint_red = (unsigned char)(tint_level & 0x000ff);
      tint_green = (unsigned char)((tint_level >> 8) & 0x000ff);
      tint_blue = (unsigned char)((tint_level >> 16) & 0x000ff);
      tint_amount = light_level;
      // older versions of the editor had a bug - work around it
      if (tint_amount < 0)
        tint_amount = 50;
      /*red = ((red + 100) * 25) / 20;
      grn = ((grn + 100) * 25) / 20;
      blu = ((blu + 100) * 25) / 20;*/
    }

    if (play.rtint_level > 0) {
      // override with room tint
      tint_level = 1;
      tint_red = play.rtint_red;
      tint_green = play.rtint_green;
      tint_blue = play.rtint_blue;
      tint_amount = play.rtint_level;
      tint_light = play.rtint_light;
    }
  }

  // copy to output parameters
  *tint_amnt = tint_amount;
  *tint_r = tint_red;
  *tint_g = tint_green;
  *tint_b = tint_blue;
  *tint_lit = tint_light;
  if (light_lev)
    *light_lev = light_level;
}




// Applies the specified RGB Tint or Light Level to the actsps
// sprite indexed with actspsindex
void apply_tint_or_light(int actspsindex, int light_level,
                         int tint_amount, int tint_red, int tint_green,
                         int tint_blue, int tint_light, int coldept,
                         block blitFrom) {

  // In a 256-colour game, we cannot do tinting or lightening
  // (but we can do darkening, if light_level < 0)
  if (game.color_depth == 1) {
    if ((light_level > 0) || (tint_amount != 0))
      return;
  }

  // we can only do tint/light if the colour depths match
  if (final_col_dep == bitmap_color_depth(actsps[actspsindex])) {
    block oldwas;
    // if the caller supplied a source bitmap, blit from it
    // (used as a speed optimisation where possible)
    if (blitFrom) 
      oldwas = blitFrom;
    // otherwise, make a new target bmp
    else {
      oldwas = actsps[actspsindex];
      actsps[actspsindex] = create_bitmap_ex(coldept, oldwas->w, oldwas->h);
    }

    if (tint_amount) {
      // It is an RGB tint

      tint_image (oldwas, actsps[actspsindex], tint_red, tint_green, tint_blue, tint_amount, tint_light);
    }
    else {
      // the RGB values passed to set_trans_blender decide whether it will darken
      // or lighten sprites ( <128=darken, >128=lighten). The parameter passed
      // to draw_lit_sprite defines how much it will be darkened/lightened by.
      int lit_amnt;
      clear_to_color(actsps[actspsindex], bitmap_mask_color(actsps[actspsindex]));
      // It's a light level, not a tint
      if (game.color_depth == 1) {
        // 256-col
        lit_amnt = (250 - ((-light_level) * 5)/2);
      }
      else {
        // hi-color
        if (light_level < 0)
          set_my_trans_blender(8,8,8,0);
        else
          set_my_trans_blender(248,248,248,0);
        lit_amnt = abs(light_level) * 2;
      }

      draw_lit_sprite(actsps[actspsindex], oldwas, 0, 0, lit_amnt);
    }

    if (oldwas != blitFrom)
      wfreeblock(oldwas);

  }
  else if (blitFrom) {
    // sprite colour depth != game colour depth, so don't try and tint
    // but we do need to do something, so copy the source
    blit(blitFrom, actsps[actspsindex], 0, 0, 0, 0, actsps[actspsindex]->w, actsps[actspsindex]->h);
  }

}

// Draws the specified 'sppic' sprite onto actsps[useindx] at the
// specified width and height, and flips the sprite if necessary.
// Returns 1 if something was drawn to actsps; returns 0 if no
// scaling or stretching was required, in which case nothing was done
int scale_and_flip_sprite(int useindx, int coldept, int zoom_level,
                          int sppic, int newwidth, int newheight,
                          int isMirrored) {

  int actsps_used = 1;

  // create and blank out the new sprite
  actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, newwidth, newheight);
  clear_to_color(actsps[useindx],bitmap_mask_color(actsps[useindx]));

  if (zoom_level != 100) {
    // Scaled character

    our_eip = 334;

    // Ensure that anti-aliasing routines have a palette to
    // use for mapping while faded out
    if (in_new_room)
      select_palette (palette);

    
    if (isMirrored) {
      block tempspr = create_bitmap_ex (coldept, newwidth, newheight);
      clear_to_color (tempspr, bitmap_mask_color(actsps[useindx]));
      if ((IS_ANTIALIAS_SPRITES) && ((game.spriteflags[sppic] & SPF_ALPHACHANNEL) == 0))
        aa_stretch_sprite (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      else
        stretch_sprite (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      draw_sprite_h_flip (actsps[useindx], tempspr, 0, 0);
      wfreeblock (tempspr);
    }
    else if ((IS_ANTIALIAS_SPRITES) && ((game.spriteflags[sppic] & SPF_ALPHACHANNEL) == 0))
      aa_stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
    else
      stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);

/*  AASTR2 version of code (doesn't work properly, gives black borders)
    if (IS_ANTIALIAS_SPRITES) {
      int aa_mode = AA_MASKED; 
      if (game.spriteflags[sppic] & SPF_ALPHACHANNEL)
        aa_mode |= AA_ALPHA | AA_RAW_ALPHA;
      if (isMirrored)
        aa_mode |= AA_HFLIP;

      aa_set_mode(aa_mode);
      aa_stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
    }
    else if (isMirrored) {
      block tempspr = create_bitmap_ex (coldept, newwidth, newheight);
      clear_to_color (tempspr, bitmap_mask_color(actsps[useindx]));
      stretch_sprite (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      draw_sprite_h_flip (actsps[useindx], tempspr, 0, 0);
      wfreeblock (tempspr);
    }
    else
      stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
*/
    if (in_new_room)
      unselect_palette();

  } 
  else {
    // Not a scaled character, draw at normal size

    our_eip = 339;

    if (isMirrored)
      draw_sprite_h_flip (actsps[useindx], spriteset[sppic], 0, 0);
    else
      actsps_used = 0;
      //blit (spriteset[sppic], actsps[useindx], 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);
  }

  return actsps_used;
}



// create the actsps[aa] image with the object drawn correctly
// returns 1 if nothing at all has changed and actsps is still
// intact from last time; 0 otherwise
int construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware) {
    int useindx = aa;
    bool hardwareAccelerated = gfxDriver->HasAcceleratedStretchAndFlip();

    if (alwaysUseSoftware)
        hardwareAccelerated = false;

    if (spriteset[objs[aa].num] == NULL)
        quitprintf("There was an error drawing object %d. Its current sprite, %d, is invalid.", aa, objs[aa].num);

    int coldept = bitmap_color_depth(spriteset[objs[aa].num]);
    int sprwidth = spritewidth[objs[aa].num];
    int sprheight = spriteheight[objs[aa].num];

    int tint_red, tint_green, tint_blue;
    int tint_level, tint_light, light_level;
    int zoom_level = 100;

    // calculate the zoom level
    if (objs[aa].flags & OBJF_USEROOMSCALING) {
        int onarea = get_walkable_area_at_location(objs[aa].x, objs[aa].y);

        if ((onarea <= 0) && (thisroom.walk_area_zoom[0] == 0)) {
            // just off the edge of an area -- use the scaling we had
            // while on the area
            zoom_level = objs[aa].last_zoom;
        }
        else
            zoom_level = get_area_scaling(onarea, objs[aa].x, objs[aa].y);

        if (zoom_level != 100)
            scale_sprite_size(objs[aa].num, zoom_level, &sprwidth, &sprheight);

    }
    // save the zoom level for next time
    objs[aa].last_zoom = zoom_level;

    // save width/height into parameters if requested
    if (drawnWidth)
        *drawnWidth = sprwidth;
    if (drawnHeight)
        *drawnHeight = sprheight;

    objs[aa].last_width = sprwidth;
    objs[aa].last_height = sprheight;

    if (objs[aa].flags & OBJF_HASTINT) {
        // object specific tint, use it
        tint_red = objs[aa].tint_r;
        tint_green = objs[aa].tint_g;
        tint_blue = objs[aa].tint_b;
        tint_level = objs[aa].tint_level;
        tint_light = objs[aa].tint_light;
        light_level = 0;
    }
    else {
        // get the ambient or region tint
        int ignoreRegionTints = 1;
        if (objs[aa].flags & OBJF_USEREGIONTINTS)
            ignoreRegionTints = 0;

        get_local_tint(objs[aa].x, objs[aa].y, ignoreRegionTints,
            &tint_level, &tint_red, &tint_green, &tint_blue,
            &tint_light, &light_level);
    }

    // check whether the image should be flipped
    int isMirrored = 0;
    if ( (objs[aa].view >= 0) &&
        (views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].pic == objs[aa].num) &&
        ((views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].flags & VFLG_FLIPSPRITE) != 0)) {
            isMirrored = 1;
    }

    if ((objcache[aa].image != NULL) &&
        (objcache[aa].sppic == objs[aa].num) &&
        (walkBehindMethod != DrawOverCharSprite) &&
        (actsps[useindx] != NULL) &&
        (hardwareAccelerated))
    {
        // HW acceleration
        objcache[aa].tintamntwas = tint_level;
        objcache[aa].tintredwas = tint_red;
        objcache[aa].tintgrnwas = tint_green;
        objcache[aa].tintbluwas = tint_blue;
        objcache[aa].tintlightwas = tint_light;
        objcache[aa].lightlevwas = light_level;
        objcache[aa].zoomWas = zoom_level;
        objcache[aa].mirroredWas = isMirrored;

        return 1;
    }

    if ((!hardwareAccelerated) && (gfxDriver->HasAcceleratedStretchAndFlip()))
    {
        // They want to draw it in software mode with the D3D driver,
        // so force a redraw
        objcache[aa].sppic = -389538;
    }

    // If we have the image cached, use it
    if ((objcache[aa].image != NULL) &&
        (objcache[aa].sppic == objs[aa].num) &&
        (objcache[aa].tintamntwas == tint_level) &&
        (objcache[aa].tintlightwas == tint_light) &&
        (objcache[aa].tintredwas == tint_red) &&
        (objcache[aa].tintgrnwas == tint_green) &&
        (objcache[aa].tintbluwas == tint_blue) &&
        (objcache[aa].lightlevwas == light_level) &&
        (objcache[aa].zoomWas == zoom_level) &&
        (objcache[aa].mirroredWas == isMirrored)) {
            // the image is the same, we can use it cached!
            if ((walkBehindMethod != DrawOverCharSprite) &&
                (actsps[useindx] != NULL))
                return 1;
            // Check if the X & Y co-ords are the same, too -- if so, there
            // is scope for further optimisations
            if ((objcache[aa].xwas == objs[aa].x) &&
                (objcache[aa].ywas == objs[aa].y) &&
                (actsps[useindx] != NULL) &&
                (walk_behind_baselines_changed == 0))
                return 1;
            actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, sprwidth, sprheight);
            blit(objcache[aa].image, actsps[useindx], 0, 0, 0, 0, objcache[aa].image->w, objcache[aa].image->h);
            return 0;
    }

    // Not cached, so draw the image

    int actspsUsed = 0;
    if (!hardwareAccelerated)
    {
        // draw the base sprite, scaled and flipped as appropriate
        actspsUsed = scale_and_flip_sprite(useindx, coldept, zoom_level,
            objs[aa].num, sprwidth, sprheight, isMirrored);
    }
    else
    {
        // ensure actsps exists
        actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, spritewidth[objs[aa].num], spriteheight[objs[aa].num]);
    }

    // direct read from source bitmap, where possible
    block comeFrom = NULL;
    if (!actspsUsed)
        comeFrom = spriteset[objs[aa].num];

    // apply tints or lightenings where appropriate, else just copy
    // the source bitmap
    if (((tint_level > 0) || (light_level != 0)) &&
        (!hardwareAccelerated))
    {
        apply_tint_or_light(useindx, light_level, tint_level, tint_red,
            tint_green, tint_blue, tint_light, coldept,
            comeFrom);
    }
    else if (!actspsUsed)
        blit(spriteset[objs[aa].num],actsps[useindx],0,0,0,0,spritewidth[objs[aa].num],spriteheight[objs[aa].num]);

    // Re-use the bitmap if it's the same size
    objcache[aa].image = recycle_bitmap(objcache[aa].image, coldept, sprwidth, sprheight);

    // Create the cached image and store it
    blit(actsps[useindx], objcache[aa].image, 0, 0, 0, 0, sprwidth, sprheight);

    objcache[aa].sppic = objs[aa].num;
    objcache[aa].tintamntwas = tint_level;
    objcache[aa].tintredwas = tint_red;
    objcache[aa].tintgrnwas = tint_green;
    objcache[aa].tintbluwas = tint_blue;
    objcache[aa].tintlightwas = tint_light;
    objcache[aa].lightlevwas = light_level;
    objcache[aa].zoomWas = zoom_level;
    objcache[aa].mirroredWas = isMirrored;
    return 0;
}




// This is only called from draw_screen_background, but it's seperated
// to help with profiling the program
void prepare_objects_for_drawing() {
    int aa,atxp,atyp,useindx;
    our_eip=32;

    for (aa=0;aa<croom->numobj;aa++) {
        if (objs[aa].on != 1) continue;
        // offscreen, don't draw
        if ((objs[aa].x >= thisroom.width) || (objs[aa].y < 1))
            continue;

        useindx = aa;
        int tehHeight;

        int actspsIntact = construct_object_gfx(aa, NULL, &tehHeight, false);

        // update the cache for next time
        objcache[aa].xwas = objs[aa].x;
        objcache[aa].ywas = objs[aa].y;

        atxp = multiply_up_coordinate(objs[aa].x) - offsetx;
        atyp = (multiply_up_coordinate(objs[aa].y) - tehHeight) - offsety;

        int usebasel = objs[aa].get_baseline();

        if (objs[aa].flags & OBJF_NOWALKBEHINDS) {
            // ignore walk-behinds, do nothing
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                usebasel += thisroom.height;
            }
        }
        else if (walkBehindMethod == DrawAsSeparateCharSprite) 
        {
            sort_out_char_sprite_walk_behind(useindx, atxp+offsetx, atyp+offsety, usebasel, objs[aa].last_zoom, objs[aa].last_width, objs[aa].last_height);
        }
        else if ((!actspsIntact) && (walkBehindMethod == DrawOverCharSprite))
        {
            sort_out_walk_behinds(actsps[useindx],atxp+offsetx,atyp+offsety,usebasel);
        }

        if ((!actspsIntact) || (actspsbmp[useindx] == NULL))
        {
            bool hasAlpha = (game.spriteflags[objs[aa].num] & SPF_ALPHACHANNEL) != 0;

            if (actspsbmp[useindx] != NULL)
                gfxDriver->DestroyDDB(actspsbmp[useindx]);
            actspsbmp[useindx] = gfxDriver->CreateDDBFromBitmap(actsps[useindx], hasAlpha);
        }

        if (gfxDriver->HasAcceleratedStretchAndFlip())
        {
            actspsbmp[useindx]->SetFlippedLeftRight(objcache[aa].mirroredWas != 0);
            actspsbmp[useindx]->SetStretch(objs[aa].last_width, objs[aa].last_height);
            actspsbmp[useindx]->SetTint(objcache[aa].tintredwas, objcache[aa].tintgrnwas, objcache[aa].tintbluwas, (objcache[aa].tintamntwas * 256) / 100);

            if (objcache[aa].tintamntwas > 0)
            {
                if (objcache[aa].tintlightwas == 0)  // luminance of 0 -- pass 1 to enable
                    actspsbmp[useindx]->SetLightLevel(1);
                else if (objcache[aa].tintlightwas < 250)
                    actspsbmp[useindx]->SetLightLevel(objcache[aa].tintlightwas);
                else
                    actspsbmp[useindx]->SetLightLevel(0);
            }
            else if (objcache[aa].lightlevwas != 0)
                actspsbmp[useindx]->SetLightLevel((objcache[aa].lightlevwas * 25) / 10 + 256);
            else
                actspsbmp[useindx]->SetLightLevel(0);
        }

        add_to_sprite_list(actspsbmp[useindx],atxp,atyp,usebasel,objs[aa].transparent,objs[aa].num);
    }

}



// Draws srcimg onto destimg, tinting to the specified level
// Totally overwrites the contents of the destination image
void tint_image (block srcimg, block destimg, int red, int grn, int blu, int light_level, int luminance) {

    if ((bitmap_color_depth(srcimg) != bitmap_color_depth(destimg)) ||
        (bitmap_color_depth(srcimg) <= 8)) {
            debug_log("Image tint failed - images must both be hi-color");
            // the caller expects something to have been copied
            blit(srcimg, destimg, 0, 0, 0, 0, srcimg->w, srcimg->h);
            return;
    }

    // For performance reasons, we have a seperate blender for
    // when light is being adjusted and when it is not.
    // If luminance >= 250, then normal brightness, otherwise darken
    if (luminance >= 250)
        set_blender_mode (_myblender_color15, _myblender_color16, _myblender_color32, red, grn, blu, 0);
    else
        set_blender_mode (_myblender_color15_light, _myblender_color16_light, _myblender_color32_light, red, grn, blu, 0);

    if (light_level >= 100) {
        // fully colourised
        clear_to_color(destimg, bitmap_mask_color(destimg));
        draw_lit_sprite(destimg, srcimg, 0, 0, luminance);
    }
    else {
        // light_level is between -100 and 100 normally; 0-100 in
        // this case when it's a RGB tint
        light_level = (light_level * 25) / 10;

        // Copy the image to the new bitmap
        blit(srcimg, destimg, 0, 0, 0, 0, srcimg->w, srcimg->h);
        // Render the colourised image to a temporary bitmap,
        // then transparently draw it over the original image
        block finaltarget = create_bitmap_ex(bitmap_color_depth(srcimg), srcimg->w, srcimg->h);
        clear_to_color(finaltarget, bitmap_mask_color(finaltarget));
        draw_lit_sprite(finaltarget, srcimg, 0, 0, luminance);

        // customized trans blender to preserve alpha channel
        set_my_trans_blender (0, 0, 0, light_level);
        draw_trans_sprite (destimg, finaltarget, 0, 0);
        destroy_bitmap (finaltarget);
    }
}




void prepare_characters_for_drawing() {
    int zoom_level,newwidth,newheight,onarea,sppic,atxp,atyp,useindx;
    int light_level,coldept,aa;
    int tint_red, tint_green, tint_blue, tint_amount, tint_light = 255;

    our_eip=33;
    // draw characters
    for (aa=0;aa<game.numcharacters;aa++) {
        if (game.chars[aa].on==0) continue;
        if (game.chars[aa].room!=displayed_room) continue;
        eip_guinum = aa;
        useindx = aa + MAX_INIT_SPR;

        CharacterInfo*chin=&game.chars[aa];
        our_eip = 330;
        // if it's on but set to view -1, they're being silly
        if (chin->view < 0) {
            quitprintf("!The character '%s' was turned on in the current room (room %d) but has not been assigned a view number.",
                chin->name, displayed_room);
        }

        if (chin->frame >= views[chin->view].loops[chin->loop].numFrames)
            chin->frame = 0;

        if ((chin->loop >= views[chin->view].numLoops) ||
            (views[chin->view].loops[chin->loop].numFrames < 1)) {
                quitprintf("!The character '%s' could not be displayed because there were no frames in loop %d of view %d.",
                    chin->name, chin->loop, chin->view + 1);
        }

        sppic=views[chin->view].loops[chin->loop].frames[chin->frame].pic;
        if ((sppic < 0) || (sppic >= MAX_SPRITES))
            sppic = 0;  // in case it's screwed up somehow
        our_eip = 331;
        // sort out the stretching if required
        onarea = get_walkable_area_at_character (aa);
        our_eip = 332;
        light_level = 0;
        tint_amount = 0;

        if (chin->flags & CHF_MANUALSCALING)  // character ignores scaling
            zoom_level = charextra[aa].zoom;
        else if ((onarea <= 0) && (thisroom.walk_area_zoom[0] == 0)) {
            zoom_level = charextra[aa].zoom;
            if (zoom_level == 0)
                zoom_level = 100;
        }
        else
            zoom_level = get_area_scaling (onarea, chin->x, chin->y);

        charextra[aa].zoom = zoom_level;

        if (chin->flags & CHF_HASTINT) {
            // object specific tint, use it
            tint_red = charextra[aa].tint_r;
            tint_green = charextra[aa].tint_g;
            tint_blue = charextra[aa].tint_b;
            tint_amount = charextra[aa].tint_level;
            tint_light = charextra[aa].tint_light;
            light_level = 0;
        }
        else {
            get_local_tint(chin->x, chin->y, chin->flags & CHF_NOLIGHTING,
                &tint_amount, &tint_red, &tint_green, &tint_blue,
                &tint_light, &light_level);
        }

        /*if (actsps[useindx]!=NULL) {
        wfreeblock(actsps[useindx]);
        actsps[useindx] = NULL;
        }*/

        our_eip = 3330;
        int isMirrored = 0, specialpic = sppic;
        bool usingCachedImage = false;

        coldept = bitmap_color_depth(spriteset[sppic]);

        // adjust the sppic if mirrored, so it doesn't accidentally
        // cache the mirrored frame as the real one
        if (views[chin->view].loops[chin->loop].frames[chin->frame].flags & VFLG_FLIPSPRITE) {
            isMirrored = 1;
            specialpic = -sppic;
        }

        our_eip = 3331;

        // if the character was the same sprite and scaling last time,
        // just use the cached image
        if ((charcache[aa].inUse) &&
            (charcache[aa].sppic == specialpic) &&
            (charcache[aa].scaling == zoom_level) &&
            (charcache[aa].tintredwas == tint_red) &&
            (charcache[aa].tintgrnwas == tint_green) &&
            (charcache[aa].tintbluwas == tint_blue) &&
            (charcache[aa].tintamntwas == tint_amount) &&
            (charcache[aa].tintlightwas == tint_light) &&
            (charcache[aa].lightlevwas == light_level)) 
        {
            if (walkBehindMethod == DrawOverCharSprite)
            {
                actsps[useindx] = recycle_bitmap(actsps[useindx], bitmap_color_depth(charcache[aa].image), charcache[aa].image->w, charcache[aa].image->h);
                blit (charcache[aa].image, actsps[useindx], 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);
            }
            else 
            {
                usingCachedImage = true;
            }
        }
        else if ((charcache[aa].inUse) && 
            (charcache[aa].sppic == specialpic) &&
            (gfxDriver->HasAcceleratedStretchAndFlip()))
        {
            usingCachedImage = true;
        }
        else if (charcache[aa].inUse) {
            //destroy_bitmap (charcache[aa].image);
            charcache[aa].inUse = 0;
        }

        our_eip = 3332;

        if (zoom_level != 100) {
            // it needs to be stretched, so calculate the new dimensions

            scale_sprite_size(sppic, zoom_level, &newwidth, &newheight);
            charextra[aa].width=newwidth;
            charextra[aa].height=newheight;
        }
        else {
            // draw at original size, so just use the sprite width and height
            charextra[aa].width=0;
            charextra[aa].height=0;
            newwidth = spritewidth[sppic];
            newheight = spriteheight[sppic];
        }

        our_eip = 3336;

        // Calculate the X & Y co-ordinates of where the sprite will be
        atxp=(multiply_up_coordinate(chin->x) - offsetx) - newwidth/2;
        atyp=(multiply_up_coordinate(chin->y) - newheight) - offsety;

        charcache[aa].scaling = zoom_level;
        charcache[aa].sppic = specialpic;
        charcache[aa].tintredwas = tint_red;
        charcache[aa].tintgrnwas = tint_green;
        charcache[aa].tintbluwas = tint_blue;
        charcache[aa].tintamntwas = tint_amount;
        charcache[aa].tintlightwas = tint_light;
        charcache[aa].lightlevwas = light_level;

        // If cache needs to be re-drawn
        if (!charcache[aa].inUse) {

            // create the base sprite in actsps[useindx], which will
            // be scaled and/or flipped, as appropriate
            int actspsUsed = 0;
            if (!gfxDriver->HasAcceleratedStretchAndFlip())
            {
                actspsUsed = scale_and_flip_sprite(
                    useindx, coldept, zoom_level, sppic,
                    newwidth, newheight, isMirrored);
            }
            else 
            {
                // ensure actsps exists
                actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, spritewidth[sppic], spriteheight[sppic]);
            }

            our_eip = 335;

            if (((light_level != 0) || (tint_amount != 0)) &&
                (!gfxDriver->HasAcceleratedStretchAndFlip())) {
                    // apply the lightening or tinting
                    block comeFrom = NULL;
                    // if possible, direct read from the source image
                    if (!actspsUsed)
                        comeFrom = spriteset[sppic];

                    apply_tint_or_light(useindx, light_level, tint_amount, tint_red,
                        tint_green, tint_blue, tint_light, coldept,
                        comeFrom);
            }
            else if (!actspsUsed)
                // no scaling, flipping or tinting was done, so just blit it normally
                blit (spriteset[sppic], actsps[useindx], 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);

            // update the character cache with the new image
            charcache[aa].inUse = 1;
            //charcache[aa].image = create_bitmap_ex (coldept, actsps[useindx]->w, actsps[useindx]->h);
            charcache[aa].image = recycle_bitmap(charcache[aa].image, coldept, actsps[useindx]->w, actsps[useindx]->h);
            blit (actsps[useindx], charcache[aa].image, 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);

        } // end if !cache.inUse

        int usebasel = chin->get_baseline();

        // adjust the Y positioning for the character's Z co-ord
        atyp -= multiply_up_coordinate(chin->z);

        our_eip = 336;

        int bgX = atxp + offsetx + chin->pic_xoffs;
        int bgY = atyp + offsety + chin->pic_yoffs;

        if (chin->flags & CHF_NOWALKBEHINDS) {
            // ignore walk-behinds, do nothing
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                usebasel += thisroom.height;
            }
        }
        else if (walkBehindMethod == DrawAsSeparateCharSprite) 
        {
            sort_out_char_sprite_walk_behind(useindx, bgX, bgY, usebasel, charextra[aa].zoom, newwidth, newheight);
        }
        else if (walkBehindMethod == DrawOverCharSprite)
        {
            sort_out_walk_behinds(actsps[useindx], bgX, bgY, usebasel);
        }

        if ((!usingCachedImage) || (actspsbmp[useindx] == NULL))
        {
            bool hasAlpha = (game.spriteflags[sppic] & SPF_ALPHACHANNEL) != 0;

            actspsbmp[useindx] = recycle_ddb_bitmap(actspsbmp[useindx], actsps[useindx], hasAlpha);
        }

        if (gfxDriver->HasAcceleratedStretchAndFlip()) 
        {
            actspsbmp[useindx]->SetStretch(newwidth, newheight);
            actspsbmp[useindx]->SetFlippedLeftRight(isMirrored != 0);
            actspsbmp[useindx]->SetTint(tint_red, tint_green, tint_blue, (tint_amount * 256) / 100);

            if (tint_amount != 0)
            {
                if (tint_light == 0) // tint with 0 luminance, pass as 1 instead
                    actspsbmp[useindx]->SetLightLevel(1);
                else if (tint_light < 250)
                    actspsbmp[useindx]->SetLightLevel(tint_light);
                else
                    actspsbmp[useindx]->SetLightLevel(0);
            }
            else if (light_level != 0)
                actspsbmp[useindx]->SetLightLevel((light_level * 25) / 10 + 256);
            else
                actspsbmp[useindx]->SetLightLevel(0);

        }

        our_eip = 337;
        // disable alpha blending with tinted sprites (because the
        // alpha channel was lost in the tinting process)
        //if (((tint_level) && (tint_amount < 100)) || (light_level))
        //sppic = -1;
        add_to_sprite_list(actspsbmp[useindx], atxp + chin->pic_xoffs, atyp + chin->pic_yoffs, usebasel, chin->transparency, sppic);

        chin->actx=atxp+offsetx;
        chin->acty=atyp+offsety;
    }
}



// draw_screen_background: draws the background scene, all the interfaces
// and objects; basically, the entire screen
void draw_screen_background() {

    static int offsetxWas = -100, offsetyWas = -100;

    screen_reset = 1;

    if (is_complete_overlay) {
        // this is normally called as part of drawing sprites - clear it
        // here instead
        clear_draw_list();
        return;
    }

    // don't draw it before the room fades in
    /*  if ((in_new_room > 0) & (game.color_depth > 1)) {
    clear(abuf);
    return;
    }*/
    our_eip=30;
    update_viewport();

    our_eip=31;

    if ((offsetx != offsetxWas) || (offsety != offsetyWas)) {
        invalidate_screen();

        offsetxWas = offsetx;
        offsetyWas = offsety;
    }

    if (play.screen_tint >= 0)
        invalidate_screen();

    if (gfxDriver->RequiresFullRedrawEachFrame())
    {
        if (roomBackgroundBmp == NULL) 
        {
            update_polled_stuff_if_runtime();
            roomBackgroundBmp = gfxDriver->CreateDDBFromBitmap(thisroom.ebscene[play.bg_frame], false, true);

            if ((walkBehindMethod == DrawAsSeparateSprite) && (walkBehindsCachedForBgNum != play.bg_frame))
            {
                update_walk_behind_images();
            }
        }
        else if (current_background_is_dirty)
        {
            update_polled_stuff_if_runtime();
            gfxDriver->UpdateDDBFromBitmap(roomBackgroundBmp, thisroom.ebscene[play.bg_frame], false);
            current_background_is_dirty = false;
            if (walkBehindMethod == DrawAsSeparateSprite)
            {
                update_walk_behind_images();
            }
        }
        gfxDriver->DrawSprite(-offsetx, -offsety, roomBackgroundBmp);
    }
    else
    {
        // the following line takes up to 50% of the game CPU time at
        // high resolutions and colour depths - if we can optimise it
        // somehow, significant performance gains to be had
        update_invalid_region_and_reset(-offsetx, -offsety, thisroom.ebscene[play.bg_frame], abuf);
    }

    clear_sprite_list();

    if ((debug_flags & DBG_NOOBJECTS)==0) {

        prepare_objects_for_drawing();

        prepare_characters_for_drawing ();

        if ((debug_flags & DBG_NODRAWSPRITES)==0) {
            our_eip=34;
            draw_sprite_list();
        }
    }
    our_eip=36;
}


void draw_fps()
{
    static IDriverDependantBitmap* ddb = NULL;
    static block fpsDisplay = NULL;

    if (fpsDisplay == NULL)
    {
        fpsDisplay = create_bitmap_ex(final_col_dep, get_fixed_pixel_size(100), (wgetfontheight(FONT_SPEECH) + get_fixed_pixel_size(5)));
        fpsDisplay = gfxDriver->ConvertBitmapToSupportedColourDepth(fpsDisplay);
    }
    clear_to_color(fpsDisplay, bitmap_mask_color(fpsDisplay));
    block oldAbuf = abuf;
    abuf = fpsDisplay;
    char tbuffer[60];
    sprintf(tbuffer,"FPS: %d",fps);
    wtextcolor(14);
    wouttext_outline(1, 1, FONT_SPEECH, tbuffer);
    abuf = oldAbuf;

    if (ddb == NULL)
        ddb = gfxDriver->CreateDDBFromBitmap(fpsDisplay, false);
    else
        gfxDriver->UpdateDDBFromBitmap(ddb, fpsDisplay, false);

    int yp = scrnhit - fpsDisplay->h;

    gfxDriver->DrawSprite(1, yp, ddb);
    invalidate_sprite(1, yp, ddb);

    sprintf(tbuffer,"Loop %ld", loopcounter);
    draw_and_invalidate_text(get_fixed_pixel_size(250), yp, FONT_SPEECH,tbuffer);
}

// draw_screen_overlay: draws any stuff currently on top of the background,
// like a message box or popup interface
void draw_screen_overlay() {
    int gg;

    add_thing_to_draw(NULL, AGSE_PREGUIDRAW, 0, TRANS_RUN_PLUGIN, false);

    // draw overlays, except text boxes
    for (gg=0;gg<numscreenover;gg++) {
        // complete overlay draw in non-transparent mode
        if (screenover[gg].type == OVER_COMPLETE)
            add_thing_to_draw(screenover[gg].bmp, screenover[gg].x, screenover[gg].y, TRANS_OPAQUE, false);
        else if (screenover[gg].type != OVER_TEXTMSG) {
            int tdxp, tdyp;
            get_overlay_position(gg, &tdxp, &tdyp);
            add_thing_to_draw(screenover[gg].bmp, tdxp, tdyp, 0, screenover[gg].hasAlphaChannel);
        }
    }

    // Draw GUIs - they should always be on top of overlays like
    // speech background text
    our_eip=35;
    mouse_on_iface_button=-1;
    if (((debug_flags & DBG_NOIFACE)==0) && (displayed_room >= 0)) {
        int aa;

        if (playerchar->activeinv >= MAX_INV) {
            quit("!The player.activeinv variable has been corrupted, probably as a result\n"
                "of an incorrect assignment in the game script.");
        }
        if (playerchar->activeinv < 1) gui_inv_pic=-1;
        else gui_inv_pic=game.invinfo[playerchar->activeinv].pic;
        /*    for (aa=0;aa<game.numgui;aa++) {
        if (guis[aa].on<1) continue;
        guis[aa].draw();
        guis[aa].poll();
        }*/
        our_eip = 37;
        if (guis_need_update) {
            block abufwas = abuf;
            guis_need_update = 0;
            for (aa=0;aa<game.numgui;aa++) {
                if (guis[aa].on<1) continue;
                eip_guinum = aa;
                our_eip = 370;
                clear_to_color (guibg[aa], bitmap_mask_color(guibg[aa]));
                abuf = guibg[aa];
                our_eip = 372;
                guis[aa].draw_at(0,0);
                our_eip = 373;

                bool isAlpha = false;
                if (guis[aa].is_alpha()) 
                {
                    isAlpha = true;

                    if ((game.options[OPT_NEWGUIALPHA] == 0) && (guis[aa].bgpic > 0))
                    {
                        // old-style (pre-3.0.2) GUI alpha rendering
                        repair_alpha_channel(guibg[aa], spriteset[guis[aa].bgpic]);
                    }
                }

                if (guibgbmp[aa] != NULL) 
                {
                    gfxDriver->UpdateDDBFromBitmap(guibgbmp[aa], guibg[aa], isAlpha);
                }
                else
                {
                    guibgbmp[aa] = gfxDriver->CreateDDBFromBitmap(guibg[aa], isAlpha);
                }
                our_eip = 374;
            }
            abuf = abufwas;
        }
        our_eip = 38;
        // Draw the GUIs
        for (gg = 0; gg < game.numgui; gg++) {
            aa = play.gui_draw_order[gg];
            if (guis[aa].on < 1) continue;

            // Don't draw GUI if "GUIs Turn Off When Disabled"
            if ((game.options[OPT_DISABLEOFF] == 3) &&
                (all_buttons_disabled > 0) &&
                (guis[aa].popup != POPUP_NOAUTOREM))
                continue;

            add_thing_to_draw(guibgbmp[aa], guis[aa].x, guis[aa].y, guis[aa].transparency, guis[aa].is_alpha());

            // only poll if the interface is enabled (mouseovers should not
            // work while in Wait state)
            if (IsInterfaceEnabled())
                guis[aa].poll();
        }
    }

    // draw text boxes (so that they appear over GUIs)
    for (gg=0;gg<numscreenover;gg++) 
    {
        if (screenover[gg].type == OVER_TEXTMSG) 
        {
            int tdxp, tdyp;
            get_overlay_position(gg, &tdxp, &tdyp);
            add_thing_to_draw(screenover[gg].bmp, tdxp, tdyp, 0, false);
        }
    }

    our_eip = 1099;

    // *** Draw the Things To Draw List ***

    SpriteListEntry *thisThing;

    for (gg = 0; gg < thingsToDrawSize; gg++) {
        thisThing = &thingsToDrawList[gg];

        if (thisThing->bmp != NULL) {
            // mark the image's region as dirty
            invalidate_sprite(thisThing->x, thisThing->y, thisThing->bmp);
        }
        else if ((thisThing->transparent != TRANS_RUN_PLUGIN) &&
            (thisThing->bmp == NULL)) 
        {
            quit("Null pointer added to draw list");
        }

        if (thisThing->bmp != NULL)
        {
            if (thisThing->transparent <= 255)
            {
                thisThing->bmp->SetTransparency(thisThing->transparent);
            }

            gfxDriver->DrawSprite(thisThing->x, thisThing->y, thisThing->bmp);
        }
        else if (thisThing->transparent == TRANS_RUN_PLUGIN) 
        {
            // meta entry to run the plugin hook
            gfxDriver->DrawSprite(thisThing->x, thisThing->y, NULL);
        }
        else
            quit("Unknown entry in draw list");
    }

    clear_draw_list();

    our_eip = 1100;


    if (display_fps) 
    {
        draw_fps();
    }
    /*
    if (channels[SCHAN_SPEECH] != NULL) {

    char tbuffer[60];
    sprintf(tbuffer,"mpos: %d", channels[SCHAN_SPEECH]->get_pos_ms());
    write_log(tbuffer);
    int yp = scrnhit - (wgetfontheight(FONT_SPEECH) + 25 * symult);
    wtextcolor(14);
    draw_and_invalidate_text(1, yp, FONT_SPEECH,tbuffer);
    }*/

    if (play.recording) {
        // Flash "REC" while recording
        wtextcolor (12);
        //if ((loopcounter % (frames_per_second * 2)) > frames_per_second/2) {
        char tformat[30];
        sprintf (tformat, "REC %02d:%02d:%02d", replay_time / 3600, (replay_time % 3600) / 60, replay_time % 60);
        draw_and_invalidate_text(get_fixed_pixel_size(5), get_fixed_pixel_size(10), FONT_SPEECH, tformat);
        //}
    }
    else if (play.playback) {
        wtextcolor (10);
        char tformat[30];
        sprintf (tformat, "PLAY %02d:%02d:%02d", replay_time / 3600, (replay_time % 3600) / 60, replay_time % 60);

        draw_and_invalidate_text(get_fixed_pixel_size(5), get_fixed_pixel_size(10), FONT_SPEECH, tformat);
    }

    our_eip = 1101;
}

bool GfxDriverNullSpriteCallback(int x, int y)
{
    if (displayed_room < 0)
    {
        // if no room loaded, various stuff won't be initialized yet
        return 1;
    }
    return (platform->RunPluginHooks(x, y) != 0);
}

void GfxDriverOnInitCallback(void *data)
{
    platform->RunPluginInitGfxHooks(gfxDriver->GetDriverID(), data);
}





int numOnStack = 0;
block screenstack[10];
void push_screen () {
    if (numOnStack >= 10)
        quit("!Too many push screen calls");

    screenstack[numOnStack] = abuf;
    numOnStack++;
}
void pop_screen() {
    if (numOnStack <= 0)
        quit("!Too many pop screen calls");
    numOnStack--;
    wsetscreen(screenstack[numOnStack]);
}


// update_screen: copies the contents of the virtual screen to the actual
// screen, and draws the mouse cursor on.
void update_screen() {
    // cos hi-color doesn't fade in, don't draw it the first time
    if ((in_new_room > 0) & (game.color_depth > 1))
        return;
    gfxDriver->DrawSprite(AGSE_POSTSCREENDRAW, 0, NULL);

    // update animating mouse cursor
    if (game.mcurs[cur_cursor].view>=0) {
        domouse (DOMOUSE_NOCURSOR);
        // only on mousemove, and it's not moving
        if (((game.mcurs[cur_cursor].flags & MCF_ANIMMOVE)!=0) &&
            (mousex==lastmx) && (mousey==lastmy)) ;
        // only on hotspot, and it's not on one
        else if (((game.mcurs[cur_cursor].flags & MCF_HOTSPOT)!=0) &&
            (GetLocationType(divide_down_coordinate(mousex), divide_down_coordinate(mousey)) == 0))
            set_new_cursor_graphic(game.mcurs[cur_cursor].pic);
        else if (mouse_delay>0) mouse_delay--;
        else {
            int viewnum=game.mcurs[cur_cursor].view;
            int loopnum=0;
            if (loopnum >= views[viewnum].numLoops)
                quitprintf("An animating mouse cursor is using view %d which has no loops", viewnum + 1);
            if (views[viewnum].loops[loopnum].numFrames < 1)
                quitprintf("An animating mouse cursor is using view %d which has no frames in loop %d", viewnum + 1, loopnum);

            mouse_frame++;
            if (mouse_frame >= views[viewnum].loops[loopnum].numFrames)
                mouse_frame=0;
            set_new_cursor_graphic(views[viewnum].loops[loopnum].frames[mouse_frame].pic);
            mouse_delay = views[viewnum].loops[loopnum].frames[mouse_frame].speed + 5;
            CheckViewFrame (viewnum, loopnum, mouse_frame);
        }
        lastmx=mousex; lastmy=mousey;
    }

    // draw the debug console, if appropriate
    if ((play.debug_mode > 0) && (display_console != 0)) 
    {
        int otextc = textcol, ypp = 1;
        int txtheight = wgetfontheight(0);
        int barheight = (DEBUG_CONSOLE_NUMLINES - 1) * txtheight + 4;

        if (debugConsoleBuffer == NULL)
            debugConsoleBuffer = create_bitmap_ex(final_col_dep, scrnwid, barheight);

        push_screen();
        abuf = debugConsoleBuffer;
        wsetcolor(15);
        wbar (0, 0, scrnwid - 1, barheight);
        wtextcolor(16);
        for (int jj = first_debug_line; jj != last_debug_line; jj = (jj + 1) % DEBUG_CONSOLE_NUMLINES) {
            wouttextxy(1, ypp, 0, debug_line[jj].text);
            wouttextxy(scrnwid - get_fixed_pixel_size(40), ypp, 0, debug_line[jj].script);
            ypp += txtheight;
        }
        textcol = otextc;
        pop_screen();

        if (debugConsole == NULL)
            debugConsole = gfxDriver->CreateDDBFromBitmap(debugConsoleBuffer, false, true);
        else
            gfxDriver->UpdateDDBFromBitmap(debugConsole, debugConsoleBuffer, false);

        gfxDriver->DrawSprite(0, 0, debugConsole);
        invalidate_sprite(0, 0, debugConsole);
    }

    domouse(DOMOUSE_NOCURSOR);

    if (!play.mouse_cursor_hidden)
    {
        gfxDriver->DrawSprite(mousex - hotx, mousey - hoty, mouseCursor);
        invalidate_sprite(mousex - hotx, mousey - hoty, mouseCursor);
    }

    /*
    domouse(1);
    // if the cursor is hidden, remove it again. However, it needs
    // to go on-off in order to update the stored mouse coordinates
    if (play.mouse_cursor_hidden)
    domouse(2);*/

    write_screen();

    wsetscreen(virtual_screen);

    if (!play.screen_is_faded_out) {
        // always update the palette, regardless of whether the plugin
        // vetos the screen update
        if (bg_just_changed) {
            setpal ();
            bg_just_changed = 0;
        }
    }

    //if (!play.mouse_cursor_hidden)
    //    domouse(2);

    screen_is_dirty = 0;
}





int Game_GetColorFromRGB(int red, int grn, int blu) {
    if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
        (blu < 0) || (blu > 255))
        quit("!GetColorFromRGB: colour values must be 0-255");

    if (game.color_depth == 1)
    {
        return makecol8(red, grn, blu);
    }

    int agscolor = ((blu >> 3) & 0x1f);
    agscolor += ((grn >> 2) & 0x3f) << 5;
    agscolor += ((red >> 3) & 0x1f) << 11;
    return agscolor;
}

// Raw screen writing routines - similar to old CapturedStuff
#define RAW_START() block oldabuf=abuf; abuf=thisroom.ebscene[play.bg_frame]; play.raw_modified[play.bg_frame] = 1
#define RAW_END() abuf = oldabuf
// RawSaveScreen: copy the current screen to a backup bitmap
void RawSaveScreen () {
    if (raw_saved_screen != NULL)
        wfreeblock(raw_saved_screen);
    block source = thisroom.ebscene[play.bg_frame];
    raw_saved_screen = wallocblock(source->w, source->h);
    blit(source, raw_saved_screen, 0, 0, 0, 0, source->w, source->h);
}
// RawRestoreScreen: copy backup bitmap back to screen; we
// deliberately don't free the block cos they can multiple restore
// and it gets freed on room exit anyway
void RawRestoreScreen() {
    if (raw_saved_screen == NULL) {
        debug_log("RawRestoreScreen: unable to restore, since the screen hasn't been saved previously.");
        return;
    }
    block deston = thisroom.ebscene[play.bg_frame];
    blit(raw_saved_screen, deston, 0, 0, 0, 0, deston->w, deston->h);
    invalidate_screen();
    mark_current_background_dirty();
}
// Restores the backup bitmap, but tints it to the specified level
void RawRestoreScreenTinted(int red, int green, int blue, int opacity) {
    if (raw_saved_screen == NULL) {
        debug_log("RawRestoreScreenTinted: unable to restore, since the screen hasn't been saved previously.");
        return;
    }
    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 1) || (opacity > 100))
        quit("!RawRestoreScreenTinted: invalid parameter. R,G,B must be 0-255, opacity 1-100");

    DEBUG_CONSOLE("RawRestoreTinted RGB(%d,%d,%d) %d%%", red, green, blue, opacity);

    block deston = thisroom.ebscene[play.bg_frame];
    tint_image(raw_saved_screen, deston, red, green, blue, opacity);
    invalidate_screen();
    mark_current_background_dirty();
}

void RawDrawFrameTransparent (int frame, int translev) {
    if ((frame < 0) || (frame >= thisroom.num_bscenes) ||
        (translev < 0) || (translev > 99))
        quit("!RawDrawFrameTransparent: invalid parameter (transparency must be 0-99, frame a valid BG frame)");

    if (bitmap_color_depth(thisroom.ebscene[frame]) <= 8)
        quit("!RawDrawFrameTransparent: 256-colour backgrounds not supported");

    if (frame == play.bg_frame)
        quit("!RawDrawFrameTransparent: cannot draw current background onto itself");

    if (translev == 0) {
        // just draw it over the top, no transparency
        blit(thisroom.ebscene[frame], thisroom.ebscene[play.bg_frame], 0, 0, 0, 0, thisroom.ebscene[frame]->w, thisroom.ebscene[frame]->h);
        play.raw_modified[play.bg_frame] = 1;
        return;
    }
    // Draw it transparently
    RAW_START();
    trans_mode = ((100-translev) * 25) / 10;
    put_sprite_256 (0, 0, thisroom.ebscene[frame]);
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}

void RawClear (int clr) {
    play.raw_modified[play.bg_frame] = 1;
    clr = get_col8_lookup(clr);
    clear_to_color (thisroom.ebscene[play.bg_frame], clr);
    invalidate_screen();
    mark_current_background_dirty();
}
void RawSetColor (int clr) {
    push_screen();
    wsetscreen(thisroom.ebscene[play.bg_frame]);
    // set the colour at the appropriate depth for the background
    play.raw_color = get_col8_lookup(clr);
    pop_screen();
}
void RawSetColorRGB(int red, int grn, int blu) {
    if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
        (blu < 0) || (blu > 255))
        quit("!RawSetColorRGB: colour values must be 0-255");

    play.raw_color = makecol_depth(bitmap_color_depth(thisroom.ebscene[play.bg_frame]), red, grn, blu);
}
void RawPrint (int xx, int yy, char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);
    // don't use wtextcolor because it will do a 16->32 conversion
    textcol = play.raw_color;
    RAW_START();
    wtexttransparent(TEXTFG);
    if ((bitmap_color_depth(abuf) <= 8) && (play.raw_color > 255)) {
        wtextcolor(1);
        debug_log ("RawPrint: Attempted to use hi-color on 256-col background");
    }
    multiply_up_coordinates(&xx, &yy);
    wouttext_outline(xx, yy, play.normal_font, displbuf);
    // we must invalidate the entire screen because these are room
    // co-ordinates, not screen co-ords which it works with
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}
void RawPrintMessageWrapped (int xx, int yy, int wid, int font, int msgm) {
    char displbuf[3000];
    int texthit = wgetfontheight(font);
    multiply_up_coordinates(&xx, &yy);
    wid = multiply_up_coordinate(wid);

    get_message_text (msgm, displbuf);
    // it's probably too late but check anyway
    if (strlen(displbuf) > 2899)
        quit("!RawPrintMessageWrapped: message too long");
    break_up_text_into_lines (wid, font, displbuf);

    textcol = play.raw_color;
    RAW_START();
    wtexttransparent(TEXTFG);
    for (int i = 0; i < numlines; i++)
        wouttext_outline(xx, yy + texthit*i, font, lines[i]);
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}

void RawDrawImageCore(int xx, int yy, int slot) {
    if ((slot < 0) || (slot >= MAX_SPRITES) || (spriteset[slot] == NULL))
        quit("!RawDrawImage: invalid sprite slot number specified");
    RAW_START();

    if (bitmap_color_depth(spriteset[slot]) != bitmap_color_depth(abuf)) {
        debug_log("RawDrawImage: Sprite %d colour depth %d-bit not same as background depth %d-bit", slot, bitmap_color_depth(spriteset[slot]), bitmap_color_depth(abuf));
    }

    draw_sprite_support_alpha(xx, yy, spriteset[slot], slot);
    invalidate_screen();
    mark_current_background_dirty();
    RAW_END();
}

void RawDrawImage(int xx, int yy, int slot) {
    multiply_up_coordinates(&xx, &yy);
    RawDrawImageCore(xx, yy, slot);
}

void RawDrawImageOffset(int xx, int yy, int slot) {

    if ((current_screen_resolution_multiplier == 1) && (game.default_resolution >= 3)) {
        // running a 640x400 game at 320x200, adjust
        xx /= 2;
        yy /= 2;
    }
    else if ((current_screen_resolution_multiplier > 1) && (game.default_resolution <= 2)) {
        // running a 320x200 game at 640x400, adjust
        xx *= 2;
        yy *= 2;
    }

    RawDrawImageCore(xx, yy, slot);
}

void RawDrawImageTransparent(int xx, int yy, int slot, int trans) {
    if ((trans < 0) || (trans > 100))
        quit("!RawDrawImageTransparent: invalid transparency setting");

    // since RawDrawImage uses putsprite256, we can just select the
    // transparency mode and call it
    trans_mode = (trans * 255) / 100;
    RawDrawImage(xx, yy, slot);

    update_polled_stuff_if_runtime();  // this operation can be slow so stop music skipping
}
void RawDrawImageResized(int xx, int yy, int gotSlot, int width, int height) {
    if ((gotSlot < 0) || (gotSlot >= MAX_SPRITES) || (spriteset[gotSlot] == NULL))
        quit("!RawDrawImageResized: invalid sprite slot number specified");
    // very small, don't draw it
    if ((width < 1) || (height < 1))
        return;

    multiply_up_coordinates(&xx, &yy);
    multiply_up_coordinates(&width, &height);

    // resize the sprite to the requested size
    block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[gotSlot]), width, height);

    stretch_blit(spriteset[gotSlot], newPic,
        0, 0, spritewidth[gotSlot], spriteheight[gotSlot],
        0, 0, width, height);

    RAW_START();
    if (bitmap_color_depth(newPic) != bitmap_color_depth(abuf))
        quit("!RawDrawImageResized: image colour depth mismatch: the background image must have the same colour depth as the sprite being drawn");

    put_sprite_256(xx, yy, newPic);
    destroy_bitmap(newPic);
    invalidate_screen();
    mark_current_background_dirty();
    update_polled_stuff_if_runtime();  // this operation can be slow so stop music skipping
    RAW_END();
}
void RawDrawLine (int fromx, int fromy, int tox, int toy) {
    multiply_up_coordinates(&fromx, &fromy);
    multiply_up_coordinates(&tox, &toy);

    play.raw_modified[play.bg_frame] = 1;
    int ii,jj;
    // draw a line thick enough to look the same at all resolutions
    for (ii = 0; ii < get_fixed_pixel_size(1); ii++) {
        for (jj = 0; jj < get_fixed_pixel_size(1); jj++)
            line (thisroom.ebscene[play.bg_frame], fromx+ii, fromy+jj, tox+ii, toy+jj, play.raw_color);
    }
    invalidate_screen();
    mark_current_background_dirty();
}
void RawDrawCircle (int xx, int yy, int rad) {
    multiply_up_coordinates(&xx, &yy);
    rad = multiply_up_coordinate(rad);

    play.raw_modified[play.bg_frame] = 1;
    circlefill (thisroom.ebscene[play.bg_frame], xx, yy, rad, play.raw_color);
    invalidate_screen();
    mark_current_background_dirty();
}
void RawDrawRectangle(int x1, int y1, int x2, int y2) {
    play.raw_modified[play.bg_frame] = 1;
    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates_round_up(&x2, &y2);

    rectfill(thisroom.ebscene[play.bg_frame], x1,y1,x2,y2, play.raw_color);
    invalidate_screen();
    mark_current_background_dirty();
}
void RawDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    play.raw_modified[play.bg_frame] = 1;
    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates(&x2, &y2);
    multiply_up_coordinates(&x3, &y3);

    triangle (thisroom.ebscene[play.bg_frame], x1,y1,x2,y2,x3,y3, play.raw_color);
    invalidate_screen();
    mark_current_background_dirty();
}




void CyclePalette(int strt,int eend) {
  // hi-color game must invalidate screen since the palette changes
  // the effect of the drawing operations
  if (game.color_depth > 1)
    invalidate_screen();

  if ((strt < 0) || (strt > 255) || (eend < 0) || (eend > 255))
    quit("!CyclePalette: start and end must be 0-255");

  if (eend > strt) {
    // forwards
    wcolrotate(strt, eend, 0, palette);
    wsetpalette(strt, eend, palette);
  }
  else {
    // backwards
    wcolrotate(eend, strt, 1, palette);
    wsetpalette(eend, strt, palette);
  }
  
}
void SetPalRGB(int inndx,int rr,int gg,int bb) {
  if (game.color_depth > 1)
    invalidate_screen();

  wsetrgb(inndx,rr,gg,bb,palette);
  wsetpalette(inndx,inndx,palette);
}
/*void scSetPal(color*pptr) {
  wsetpalette(0,255,pptr);
  }
void scGetPal(color*pptr) {
  get_palette(pptr);
  }*/

void UpdatePalette() {
  if (game.color_depth > 1)
    invalidate_screen();

  if (!play.fast_forward)  
    setpal();
}

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
extern volatile int psp_audio_multithreaded; // in ac_audio
#endif


void construct_virtual_screen(bool fullRedraw) 
{
  gfxDriver->ClearDrawList();

  if (play.fast_forward)
    return;

  our_eip=3;

  gfxDriver->UseSmoothScaling(IS_ANTIALIAS_SPRITES);

  platform->RunPluginHooks(AGSE_PRERENDER, 0);

  if (displayed_room >= 0) {
    
    if (fullRedraw)
      invalidate_screen();

    draw_screen_background();
  }
  else if (!gfxDriver->RequiresFullRedrawEachFrame()) 
  {
    // if the driver is not going to redraw the screen,
    // black it out so we don't get cursor trails
    clear(abuf);
  }

  // reset the Baselines Changed flag now that we've drawn stuff
  walk_behind_baselines_changed = 0;

  // make sure that the mp3 is always playing smoothly
  UPDATE_MP3
  our_eip=4;
  draw_screen_overlay();

  if (fullRedraw)
  {
    // ensure the virtual screen is reconstructed
    // in case we want to take any screenshots before
    // the next game loop
    if (gfxDriver->UsesMemoryBackBuffer())
      gfxDriver->RenderToBackBuffer();
  }
}

// Draw everything 
void render_graphics(IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {

  construct_virtual_screen(false);
  our_eip=5;

  if (extraBitmap != NULL)
    gfxDriver->DrawSprite(extraX, extraY, extraBitmap);

  update_screen();
}
