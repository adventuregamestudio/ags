
#include "ac/global_debug.h"
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_room.h"
#include "ac/movelist.h"
#include "ac/properties.h"
#include "ac/record.h"
#include "ac/roomstruct.h"
#include "ac/tree_map.h"
#include "ac/walkablearea.h"
#include "gfx/gfxfilter.h"
#include "gui/guidialog.h"
#include "script/cc_options.h"
#include "debug/debug.h"
#include "main/main.h"
#include "ac/spritecache.h"

extern GameSetupStruct game;
extern GameSetup usetup;
extern GameState play;
extern roomstruct thisroom;
extern CharacterInfo*playerchar;

extern int convert_16bit_bgr;
extern IGraphicsDriver *gfxDriver;
extern SpriteCache spriteset;
extern TreeMap *transtree;
extern int offsetx, offsety;
extern int displayed_room, starting_room;
extern MoveList *mls;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;
extern char transFileName[MAX_PATH];

void script_debug(int cmdd,int dataa) {
    if (play.debug_mode==0) return;
    int rr;
    if (cmdd==0) {
        for (rr=1;rr<game.numinvitems;rr++)
            playerchar->inv[rr]=1;
        update_invorder();
        //    Display("invorder decided there are %d items[display %d",play.inv_numorder,play.inv_numdisp);
    }
    else if (cmdd==1) {
        char toDisplay[STD_BUFFER_SIZE];
        const char *filterName = filter->GetVersionBoxText();
        sprintf(toDisplay,"Adventure Game Studio run-time engine[ACI version " ACI_VERSION_TEXT
            "[Running %d x %d at %d-bit %s[GFX: %s[%s" "Sprite cache size: %ld KB (limit %ld KB; %ld locked)",
            final_scrn_wid,final_scrn_hit,final_col_dep, (convert_16bit_bgr) ? "BGR" : "",
            gfxDriver->GetDriverName(), filterName,
            spriteset.cachesize / 1024, spriteset.maxCacheSize / 1024, spriteset.lockedSize / 1024);
        if (play.seperate_music_lib)
            strcat(toDisplay,"[AUDIO.VOX enabled");
        if (play.want_speech >= 1)
            strcat(toDisplay,"[SPEECH.VOX enabled");
        if (transtree != NULL) {
            strcat(toDisplay,"[Using translation ");
            strcat(toDisplay, transFileName);
        }
        if (opts.mod_player == 0)
            strcat(toDisplay,"[(mod/xm player discarded)");
        Display(toDisplay);
        //    Display("shftR: %d  shftG: %d  shftB: %d", _rgb_r_shift_16, _rgb_g_shift_16, _rgb_b_shift_16);
        //    Display("Remaining memory: %d kb",_go32_dpmi_remaining_virtual_memory()/1024);
        //Display("Play char bcd: %d",bitmap_color_depth(spriteset[views[playerchar->view].frames[playerchar->loop][playerchar->frame].pic]));
    }
    else if (cmdd==2) 
    {  // show walkable areas from here
        block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
        blit(prepare_walkable_areas(-1),tempw,0,0,0,0,tempw->w,tempw->h);
        block stretched = create_bitmap(scrnwid, scrnhit);
        stretch_sprite(stretched, tempw, -offsetx, -offsety, get_fixed_pixel_size(tempw->w), get_fixed_pixel_size(tempw->h));

        IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(stretched, false, true);
        render_graphics(ddb, 0, 0);

        destroy_bitmap(tempw);
        destroy_bitmap(stretched);
        gfxDriver->DestroyDDB(ddb);
        while (!kbhit()) ;
        getch();
        invalidate_screen();
    }
    else if (cmdd==3) 
    {
        int goToRoom = -1;
        if (game.roomCount == 0)
        {
            char inroomtex[80];
            sprintf(inroomtex, "!Enter new room: (in room %d)", displayed_room);
            setup_for_dialog();
            goToRoom = enternumberwindow(inroomtex);
            restore_after_dialog();
        }
        else
        {
            setup_for_dialog();
            goToRoom = roomSelectorWindow(displayed_room, game.roomCount, game.roomNumbers, game.roomNames);
            restore_after_dialog();
        }
        if (goToRoom >= 0) 
            NewRoom(goToRoom);
    }
    else if (cmdd == 4) {
        if (display_fps != 2)
            display_fps = dataa;
    }
    else if (cmdd == 5) {
        if (dataa == 0) dataa = game.playercharacter;
        if (game.chars[dataa].walking < 1) {
            Display("Not currently moving.");
            return;
        }
        block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
        int mlsnum = game.chars[dataa].walking;
        if (game.chars[dataa].walking >= TURNING_AROUND)
            mlsnum %= TURNING_AROUND;
        MoveList*cmls = &mls[mlsnum];
        clear_to_color(tempw, bitmap_mask_color(tempw));
        for (int i = 0; i < cmls->numstage-1; i++) {
            short srcx=short((cmls->pos[i] >> 16) & 0x00ffff);
            short srcy=short(cmls->pos[i] & 0x00ffff);
            short targetx=short((cmls->pos[i+1] >> 16) & 0x00ffff);
            short targety=short(cmls->pos[i+1] & 0x00ffff);
            line (tempw, srcx, srcy, targetx, targety, get_col8_lookup(i+1));
        }
        stretch_sprite(screen, tempw, -offsetx, -offsety, multiply_up_coordinate(tempw->w), multiply_up_coordinate(tempw->h));
        render_to_screen(screen, 0, 0);
        wfreeblock(tempw);
        while (!kbhit()) ;
        getch();
    }
    else if (cmdd == 99)
        ccSetOption(SCOPT_DEBUGRUN, dataa);
    else quit("!Debug: unknown command code");
}
