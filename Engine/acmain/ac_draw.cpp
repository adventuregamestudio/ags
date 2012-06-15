
#include "acmain/ac_maindefines.h"



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
