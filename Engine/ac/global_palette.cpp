
#include "wgt2allg.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_palette.h"

extern GameSetupStruct game;
extern GameState play;
extern color palette[256];


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
