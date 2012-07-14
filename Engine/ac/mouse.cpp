
#include "ac/mouse.h"
#include "util/wgt2allg.h"
#include "ali3d.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/dynobj/scriptmouse.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_mouse.h"
#include "ac/global_screen.h"
#include "ac/viewframe.h"
#include "debug/debug.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "mousew32.h"
#include "ac/spritecache.h"

extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;
extern block mousecurs[MAXCURSORS];
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int guis_need_update;
extern CharacterInfo*playerchar;
extern GUIMain*guis;
extern IGraphicsDriver *gfxDriver;

ScriptMouse scmouse;
int cur_mode,cur_cursor;
int mouse_frame=0,mouse_delay=0;
int lastmx=-1,lastmy=-1;
char alpha_blend_cursor = 0;
block dotted_mouse_cursor = NULL;
IDriverDependantBitmap *mouseCursor = NULL;
block blank_mouse_cursor = NULL;

// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetVisible(int isOn) {
    if (isOn)
        ShowMouseCursor();
    else
        HideMouseCursor();
}

int Mouse_GetVisible() {
    if (play.mouse_cursor_hidden)
        return 0;
    return 1;
}

void SetMouseBounds (int x1, int y1, int x2, int y2) {
    if ((x1 == 0) && (y1 == 0) && (x2 == 0) && (y2 == 0)) {
        x2 = BASEWIDTH-1;
        y2 = MOUSE_MAX_Y - 1;
    }
    if (x2 == BASEWIDTH) x2 = BASEWIDTH-1;
    if (y2 == MOUSE_MAX_Y) y2 = MOUSE_MAX_Y - 1;
    if ((x1 > x2) || (y1 > y2) || (x1 < 0) || (x2 >= BASEWIDTH) ||
        (y1 < 0) || (y2 >= MOUSE_MAX_Y))
        quit("!SetMouseBounds: invalid co-ordinates, must be within (0,0) - (320,200)");
    DEBUG_CONSOLE("Mouse bounds constrained to (%d,%d)-(%d,%d)", x1, y1, x2, y2);
    multiply_up_coordinates(&x1, &y1);
    multiply_up_coordinates_round_up(&x2, &y2);

    play.mboundx1 = x1;
    play.mboundx2 = x2;
    play.mboundy1 = y1;
    play.mboundy2 = y2;
    filter->SetMouseLimit(x1,y1,x2,y2);
}

// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs) {
    int hotspotx = game.mcurs[newcurs].hotx, hotspoty = game.mcurs[newcurs].hoty;

    set_new_cursor_graphic(game.mcurs[newcurs].pic);
    if (dotted_mouse_cursor) {
        wfreeblock (dotted_mouse_cursor);
        dotted_mouse_cursor = NULL;
    }

    if ((newcurs == MODE_USE) && (game.mcurs[newcurs].pic > 0) &&
        ((game.hotdot > 0) || (game.invhotdotsprite > 0)) ) {
            // If necessary, create a copy of the cursor and put the hotspot
            // dot onto it
            dotted_mouse_cursor = create_bitmap_ex (bitmap_color_depth(mousecurs[0]), mousecurs[0]->w,mousecurs[0]->h);
            blit (mousecurs[0], dotted_mouse_cursor, 0, 0, 0, 0, mousecurs[0]->w, mousecurs[0]->h);

            if (game.invhotdotsprite > 0) {
                block abufWas = abuf;
                abuf = dotted_mouse_cursor;

                draw_sprite_support_alpha(
                    hotspotx - spritewidth[game.invhotdotsprite] / 2,
                    hotspoty - spriteheight[game.invhotdotsprite] / 2,
                    spriteset[game.invhotdotsprite],
                    game.invhotdotsprite);

                abuf = abufWas;
            }
            else {
                putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty,
                    (bitmap_color_depth(dotted_mouse_cursor) > 8) ? get_col8_lookup (game.hotdot) : game.hotdot);

                if (game.hotdotouter > 0) {
                    int outercol = game.hotdotouter;
                    if (bitmap_color_depth (dotted_mouse_cursor) > 8)
                        outercol = get_col8_lookup(game.hotdotouter);

                    putpixel_compensate (dotted_mouse_cursor, hotspotx + get_fixed_pixel_size(1), hotspoty, outercol);
                    putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty + get_fixed_pixel_size(1), outercol);
                    putpixel_compensate (dotted_mouse_cursor, hotspotx - get_fixed_pixel_size(1), hotspoty, outercol);
                    putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty - get_fixed_pixel_size(1), outercol);
                }
            }
            mousecurs[0] = dotted_mouse_cursor;
            update_cached_mouse_cursor();
    }
    msethotspot(hotspotx, hotspoty);
    if (newcurs != cur_cursor)
    {
        cur_cursor = newcurs;
        mouse_frame=0;
        mouse_delay=0;
    }
}

// set_default_cursor: resets visual appearance to current mode (walk, look, etc)
void set_default_cursor() {
    set_mouse_cursor(cur_mode);
}

// permanently change cursor graphic
void ChangeCursorGraphic (int curs, int newslot) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!ChangeCursorGraphic: invalid mouse cursor");

    if ((curs == MODE_USE) && (game.options[OPT_FIXEDINVCURSOR] == 0))
        debug_log("Mouse.ChangeModeGraphic should not be used on the Inventory cursor when the cursor is linked to the active inventory item");

    game.mcurs[curs].pic = newslot;
    spriteset.precache (newslot);
    if (curs == cur_mode)
        set_mouse_cursor (curs);
}

int Mouse_GetModeGraphic(int curs) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!Mouse.GetModeGraphic: invalid mouse cursor");

    return game.mcurs[curs].pic;
}

void ChangeCursorHotspot (int curs, int x, int y) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!ChangeCursorHotspot: invalid mouse cursor");
    game.mcurs[curs].hotx = multiply_up_coordinate(x);
    game.mcurs[curs].hoty = multiply_up_coordinate(y);
    if (curs == cur_cursor)
        set_mouse_cursor (cur_cursor);
}

void Mouse_ChangeModeView(int curs, int newview) {
    if ((curs < 0) || (curs >= game.numcursors))
        quit("!Mouse.ChangeModeView: invalid mouse cursor");

    newview--;

    game.mcurs[curs].view = newview;

    if (newview >= 0)
    {
        precache_view(newview);
    }

    if (curs == cur_cursor)
        mouse_delay = 0;  // force update
}

void SetNextCursor () {
    set_cursor_mode (find_next_enabled_cursor(cur_mode + 1));
}

// set_cursor_mode: changes mode and appearance
void set_cursor_mode(int newmode) {
    if ((newmode < 0) || (newmode >= game.numcursors))
        quit("!SetCursorMode: invalid cursor mode specified");

    guis_need_update = 1;
    if (game.mcurs[newmode].flags & MCF_DISABLED) {
        find_next_enabled_cursor(newmode);
        return; }
    if (newmode == MODE_USE) {
        if (playerchar->activeinv == -1) {
            find_next_enabled_cursor(0);
            return;
        }
        update_inv_cursor(playerchar->activeinv);
    }
    cur_mode=newmode;
    set_default_cursor();

    DEBUG_CONSOLE("Cursor mode set to %d", newmode);
}

void enable_cursor_mode(int modd) {
    game.mcurs[modd].flags&=~MCF_DISABLED;
    // now search the interfaces for related buttons to re-enable
    int uu,ww;

    for (uu=0;uu<game.numgui;uu++) {
        for (ww=0;ww<guis[uu].numobjs;ww++) {
            if ((guis[uu].objrefptr[ww] >> 16)!=GOBJ_BUTTON) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].objs[ww];
            if (gbpt->leftclick!=IBACT_SETMODE) continue;
            if (gbpt->lclickdata!=modd) continue;
            gbpt->Enable();
        }
    }
    guis_need_update = 1;
}

void disable_cursor_mode(int modd) {
    game.mcurs[modd].flags|=MCF_DISABLED;
    // now search the interfaces for related buttons to kill
    int uu,ww;

    for (uu=0;uu<game.numgui;uu++) {
        for (ww=0;ww<guis[uu].numobjs;ww++) {
            if ((guis[uu].objrefptr[ww] >> 16)!=GOBJ_BUTTON) continue;
            GUIButton*gbpt=(GUIButton*)guis[uu].objs[ww];
            if (gbpt->leftclick!=IBACT_SETMODE) continue;
            if (gbpt->lclickdata!=modd) continue;
            gbpt->Disable();
        }
    }
    if (cur_mode==modd) find_next_enabled_cursor(modd);
    guis_need_update = 1;
}

void RefreshMouse() {
    domouse(DOMOUSE_NOCURSOR);
    scmouse.x = divide_down_coordinate(mousex);
    scmouse.y = divide_down_coordinate(mousey);
}

void SetMousePosition (int newx, int newy) {
    if (newx < 0)
        newx = 0;
    if (newy < 0)
        newy = 0;
    if (newx >= BASEWIDTH)
        newx = BASEWIDTH - 1;
    if (newy >= GetMaxScreenHeight())
        newy = GetMaxScreenHeight() - 1;

    multiply_up_coordinates(&newx, &newy);
    filter->SetMousePosition(newx, newy);
    RefreshMouse();
}

int GetCursorMode() {
    return cur_mode;
}

int IsButtonDown(int which) {
    if ((which < 1) || (which > 3))
        quit("!IsButtonDown: only works with eMouseLeft, eMouseRight, eMouseMiddle");
    if (misbuttondown(which-1))
        return 1;
    return 0;
}

//=============================================================================

int GetMouseCursor() {
    return cur_cursor;
}

void update_script_mouse_coords() {
    scmouse.x = divide_down_coordinate(mousex);
    scmouse.y = divide_down_coordinate(mousey);
}

void update_inv_cursor(int invnum) {

    if ((game.options[OPT_FIXEDINVCURSOR]==0) && (invnum > 0)) {
        int cursorSprite = game.invinfo[invnum].cursorPic;

        // Fall back to the inventory pic if no cursor pic is defined.
        if (cursorSprite == 0)
            cursorSprite = game.invinfo[invnum].pic;

        game.mcurs[MODE_USE].pic = cursorSprite;
        // all cursor images must be pre-cached
        spriteset.precache(cursorSprite);

        if ((game.invinfo[invnum].hotx > 0) || (game.invinfo[invnum].hoty > 0)) {
            // if the hotspot was set (unfortunately 0,0 isn't a valid co-ord)
            game.mcurs[MODE_USE].hotx=game.invinfo[invnum].hotx;
            game.mcurs[MODE_USE].hoty=game.invinfo[invnum].hoty;
        }
        else {
            game.mcurs[MODE_USE].hotx = spritewidth[cursorSprite] / 2;
            game.mcurs[MODE_USE].hoty = spriteheight[cursorSprite] / 2;
        }
    }
}

void update_cached_mouse_cursor() 
{
    if (mouseCursor != NULL)
        gfxDriver->DestroyDDB(mouseCursor);
    mouseCursor = gfxDriver->CreateDDBFromBitmap(mousecurs[0], alpha_blend_cursor != 0);
}

void set_new_cursor_graphic (int spriteslot) {
    mousecurs[0] = spriteset[spriteslot];

    if ((spriteslot < 1) || (mousecurs[0] == NULL))
    {
        if (blank_mouse_cursor == NULL)
        {
            blank_mouse_cursor = create_bitmap_ex(final_col_dep, 1, 1);
            clear_to_color(blank_mouse_cursor, bitmap_mask_color(blank_mouse_cursor));
        }
        mousecurs[0] = blank_mouse_cursor;
    }

    if (game.spriteflags[spriteslot] & SPF_ALPHACHANNEL)
        alpha_blend_cursor = 1;
    else
        alpha_blend_cursor = 0;

    update_cached_mouse_cursor();
}

int find_next_enabled_cursor(int startwith) {
    if (startwith >= game.numcursors)
        startwith = 0;
    int testing=startwith;
    do {
        if ((game.mcurs[testing].flags & MCF_DISABLED)==0) {
            // inventory cursor, and they have an active item
            if (testing == MODE_USE) 
            {
                if (playerchar->activeinv > 0)
                    break;
            }
            // standard cursor that's not disabled, go with it
            else if (game.mcurs[testing].flags & MCF_STANDARD)
                break;
        }

        testing++;
        if (testing >= game.numcursors) testing=0;
    } while (testing!=startwith);

    if (testing!=startwith)
        set_cursor_mode(testing);

    return testing;
}
