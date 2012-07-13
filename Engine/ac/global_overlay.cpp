
#include "ac/global_overlay.h"
#include "wgt2allg.h"
#include "ac/ac_common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/overlay.h"
#include "ac/rundefines.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "sprcache.h"


extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;
extern SpriteCache spriteset;
extern GameSetupStruct game;
extern block virtual_screen;

extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern int crovr_id;  // whether using SetTextOverlay or CreateTextOvelay


void RemoveOverlay(int ovrid) {
    if (find_overlay_of_type(ovrid) < 0) quit("!RemoveOverlay: invalid overlay id passed");
    remove_screen_overlay(ovrid);
}

int CreateGraphicOverlay(int xx,int yy,int slott,int trans) {
    multiply_up_coordinates(&xx, &yy);

    block screeno=create_bitmap_ex(final_col_dep, spritewidth[slott],spriteheight[slott]);
    wsetscreen(screeno);
    clear_to_color(screeno,bitmap_mask_color(screeno));
    wputblock(0,0,spriteset[slott],trans);

    bool hasAlpha = (game.spriteflags[slott] & SPF_ALPHACHANNEL) != 0;
    int nse = add_screen_overlay(xx, yy, OVER_CUSTOM, screeno, hasAlpha);

    wsetscreen(virtual_screen);
    return screenover[nse].type;
}

int CreateTextOverlayCore(int xx, int yy, int wii, int fontid, int clr, const char *tex, int allowShrink) {
    if (wii<8) wii=scrnwid/2;
    if (xx<0) xx=scrnwid/2-wii/2;
    if (clr==0) clr=16;
    int blcode = crovr_id;
    crovr_id = 2;
    return _display_main(xx,yy,wii, (char*)tex, blcode,fontid,-clr, 0, allowShrink, false);
}

int CreateTextOverlay(int xx,int yy,int wii,int fontid,int clr,char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);

    int allowShrink = 0;

    if (xx != OVR_AUTOPLACE) {
        multiply_up_coordinates(&xx,&yy);
        wii = multiply_up_coordinate(wii);
    }
    else  // allow DisplaySpeechBackground to be shrunk
        allowShrink = 1;

    return CreateTextOverlayCore(xx, yy, wii, fontid, clr, displbuf, allowShrink);
}

void SetTextOverlay(int ovrid,int xx,int yy,int wii,int fontid,int clr,char*texx,...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf,get_translation(texx),ap);
    va_end(ap);
    RemoveOverlay(ovrid);
    crovr_id=ovrid;
    if (CreateTextOverlay(xx,yy,wii,fontid,clr,displbuf)!=ovrid)
        quit("SetTextOverlay internal error: inconsistent type ids");
}

void MoveOverlay(int ovrid, int newx,int newy) {
    multiply_up_coordinates(&newx, &newy);

    int ovri=find_overlay_of_type(ovrid);
    if (ovri<0) quit("!MoveOverlay: invalid overlay ID specified");
    screenover[ovri].x=newx;
    screenover[ovri].y=newy;
}

int IsOverlayValid(int ovrid) {
    if (find_overlay_of_type(ovrid) < 0)
        return 0;

    return 1;
}
