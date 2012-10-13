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

#include "event.h"
#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_room.h"
#include "ac/global_screen.h"
#include "ac/gui.h"
#include "ac/roomstatus.h"
#include "ac/roomstruct.h"
#include "ac/screen.h"
#include "script/cc_error.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "script/script.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = Common::BitmapHelper;

extern GameSetupStruct game;
extern roomstruct thisroom;
extern RoomStatus*croom;
extern int displayed_room;
extern GameState play;
extern color palette[256];
extern IGraphicsDriver *gfxDriver;
extern AGSPlatformDriver *platform;
extern Bitmap *temp_virtual;
extern Bitmap *virtual_screen;
extern volatile int timerloop;
extern int scrnwid,scrnhit;
extern color old_palette[256];

int in_enters_screen=0,done_es_error = 0;
int in_leaves_screen = -1;

EventHappened event[MAXEVENTS+1];
int numevents=0;

char*evblockbasename;
int evblocknum;

int eventClaimed = EVENT_NONE;

char*tsnames[4]={NULL, REP_EXEC_NAME, "on_key_press","on_mouse_click"};


int run_claimable_event(char *tsname, bool includeRoom, int numParams, long param1, long param2, bool *eventWasClaimed) {
    *eventWasClaimed = true;
    // Run the room script function, and if it is not claimed,
    // then run the main one
    // We need to remember the eventClaimed variable's state, in case
    // this is a nested event
    int eventClaimedOldValue = eventClaimed;
    eventClaimed = EVENT_INPROGRESS;
    int toret;

    if (includeRoom) {
        toret = roominst->RunScriptFunctionIfExists(tsname, numParams, param1, param2);

        if (eventClaimed == EVENT_CLAIMED) {
            eventClaimed = eventClaimedOldValue;
            return toret;
        }
    }

    // run script modules
    for (int kk = 0; kk < numScriptModules; kk++) {
        toret = moduleInst[kk]->RunScriptFunctionIfExists(tsname, numParams, param1, param2);

        if (eventClaimed == EVENT_CLAIMED) {
            eventClaimed = eventClaimedOldValue;
            return toret;
        }
    }

    eventClaimed = eventClaimedOldValue;
    *eventWasClaimed = false;
    return 0;
}

// runs the global script on_event fnuction
void run_on_event (int evtype, int wparam) {
    if (inside_script) {
        curscript->run_another("#on_event", evtype, wparam);
    }
    else
        gameinst->RunTextScript2IParam("on_event", evtype, wparam);
}

void run_room_event(int id) {
    evblockbasename="room";

    if (thisroom.roomScripts != NULL)
    {
        run_interaction_script(thisroom.roomScripts, id);
    }
    else
    {
        run_interaction_event (&croom->intrRoom, id);
    }
}

void run_event_block_inv(int invNum, int aaa) {
    evblockbasename="inventory%d";
    if (game.invScripts != NULL)
    {
        run_interaction_script(game.invScripts[invNum], aaa);
    }
    else 
    {
        run_interaction_event(game.intrInv[invNum], aaa);
    }

}

// event list functions
void setevent(int evtyp,int ev1,int ev2,int ev3) {
    event[numevents].type=evtyp;
    event[numevents].data1=ev1;
    event[numevents].data2=ev2;
    event[numevents].data3=ev3;
    event[numevents].player=game.playercharacter;
    numevents++;
    if (numevents>=MAXEVENTS) quit("too many events posted");
}

void process_event(EventHappened*evp) {
    if (evp->type==EV_TEXTSCRIPT) {
        int resl=0; ccError=0;
        if (evp->data2 > -1000) {
            if (inside_script) {
                char nameToExec[50];
                sprintf (nameToExec, "!%s", tsnames[evp->data1]);
                curscript->run_another(nameToExec, evp->data2, 0);
            }
            else
                resl=gameinst->RunTextScriptIParam(tsnames[evp->data1],evp->data2);
        }
        else {
            if (inside_script)
                curscript->run_another (tsnames[evp->data1], 0, 0);
            else
                resl=gameinst->RunTextScript(tsnames[evp->data1]);
        }
        //    Display("relt: %d err:%d",resl,scErrorNo);
    }
    else if (evp->type==EV_NEWROOM) {
        NewRoom(evp->data1);
    }
    else if (evp->type==EV_RUNEVBLOCK) {
        NewInteraction*evpt=NULL;
        InteractionScripts *scriptPtr = NULL;
        char *oldbasename = evblockbasename;
        int   oldblocknum = evblocknum;

        if (evp->data1==EVB_HOTSPOT) {

            if (thisroom.hotspotScripts != NULL)
                scriptPtr = thisroom.hotspotScripts[evp->data2];
            else
                evpt=&croom->intrHotspot[evp->data2];

            evblockbasename="hotspot%d";
            evblocknum=evp->data2;
            //platform->WriteDebugString("Running hotspot interaction for hotspot %d, event %d", evp->data2, evp->data3);
        }
        else if (evp->data1==EVB_ROOM) {

            if (thisroom.roomScripts != NULL)
                scriptPtr = thisroom.roomScripts;
            else
                evpt=&croom->intrRoom;

            evblockbasename="room";
            if (evp->data3 == 5) {
                in_enters_screen ++;
                run_on_event (GE_ENTER_ROOM, displayed_room);

            }
            //platform->WriteDebugString("Running room interaction, event %d", evp->data3);
        }

        if (scriptPtr != NULL)
        {
            run_interaction_script(scriptPtr, evp->data3);
        }
        else if (evpt != NULL)
        {
            run_interaction_event(evpt,evp->data3);
        }
        else
            quit("process_event: RunEvBlock: unknown evb type");

        evblockbasename = oldbasename;
        evblocknum = oldblocknum;

        if ((evp->data3 == 5) && (evp->data1 == EVB_ROOM))
            in_enters_screen --;
    }
    else if (evp->type==EV_FADEIN) {
        // if they change the transition type before the fadein, make
        // sure the screen doesn't freeze up
        play.screen_is_faded_out = 0;

        // determine the transition style
        int theTransition = play.fade_effect;

        if (play.next_screen_transition >= 0) {
            // a one-off transition was selected, so use it
            theTransition = play.next_screen_transition;
            play.next_screen_transition = -1;
        }

        if (platform->RunPluginHooks(AGSE_TRANSITIONIN, 0))
            return;

        if (play.fast_forward)
            return;

        if (((theTransition == FADE_CROSSFADE) || (theTransition == FADE_DISSOLVE)) &&
            (temp_virtual == NULL)) 
        {
            // transition type was not crossfade/dissolve when the screen faded out,
            // but it is now when the screen fades in (Eg. a save game was restored
            // with a different setting). Therefore just fade normally.
            my_fade_out(5);
            theTransition = FADE_NORMAL;
        }

		Bitmap *screen_bmp = BitmapHelper::GetScreenBitmap();

        if ((theTransition == FADE_INSTANT) || (play.screen_tint >= 0))
            wsetpalette(0,255,palette);
        else if (theTransition == FADE_NORMAL)
        {
            if (gfxDriver->UsesMemoryBackBuffer())
                gfxDriver->RenderToBackBuffer();

            my_fade_in(palette,5);
        }
        else if (theTransition == FADE_BOXOUT) 
        {
            if (!gfxDriver->UsesMemoryBackBuffer())
            {
                gfxDriver->BoxOutEffect(false, get_fixed_pixel_size(16), 1000 / GetGameSpeed());
            }
            else
            {
                wsetpalette(0,255,palette);
                gfxDriver->RenderToBackBuffer();
				gfxDriver->SetMemoryBackBuffer(screen_bmp);
                screen_bmp->Clear();
                render_to_screen(screen_bmp, 0, 0);

                int boxwid = get_fixed_pixel_size(16);
                int boxhit = multiply_up_coordinate(GetMaxScreenHeight() / 20);
                while (boxwid < screen_bmp->GetWidth()) {
                    timerloop = 0;
                    boxwid += get_fixed_pixel_size(16);
                    boxhit += multiply_up_coordinate(GetMaxScreenHeight() / 20);
                    int lxp = scrnwid / 2 - boxwid / 2, lyp = scrnhit / 2 - boxhit / 2;
                    gfxDriver->Vsync();
                    screen_bmp->Blit(virtual_screen, lxp, lyp, lxp, lyp,
                        boxwid, boxhit);
                    render_to_screen(screen_bmp, 0, 0);
                    UPDATE_MP3
                        while (timerloop == 0) ;
                }
                gfxDriver->SetMemoryBackBuffer(virtual_screen);
            }
            play.screen_is_faded_out = 0;
        }
        else if (theTransition == FADE_CROSSFADE) 
        {
            if (game.color_depth == 1)
                quit("!Cannot use crossfade screen transition in 256-colour games");

            IDriverDependantBitmap *ddb = prepare_screen_for_transition_in();

            int transparency = 254;

            while (transparency > 0) {
                timerloop=0;
                // do the crossfade
                ddb->SetTransparency(transparency);
                invalidate_screen();
                draw_screen_callback();

                if (transparency > 16)
                {
                    // on last frame of fade (where transparency < 16), don't
                    // draw the old screen on top
                    gfxDriver->DrawSprite(0, -(temp_virtual->GetHeight() - virtual_screen->GetHeight()), ddb);
                }
				render_to_screen(screen_bmp, 0, 0);
                update_polled_stuff_if_runtime();
                while (timerloop == 0) ;
                transparency -= 16;
            }
            temp_virtual->Release();

            delete temp_virtual;
            temp_virtual = NULL;
            wsetpalette(0,255,palette);
            gfxDriver->DestroyDDB(ddb);
        }
        else if (theTransition == FADE_DISSOLVE) {
            int pattern[16]={0,4,14,9,5,11,2,8,10,3,12,7,15,6,13,1};
            int aa,bb,cc,thcol=0;
            color interpal[256];

            IDriverDependantBitmap *ddb = prepare_screen_for_transition_in();

            for (aa=0;aa<16;aa++) {
                timerloop=0;
                // merge the palette while dithering
                if (game.color_depth == 1) 
                {
                    fade_interpolate(old_palette,palette,interpal,aa*4,0,255);
                    wsetpalette(0,255,interpal);
                }
                // do the dissolving
                int maskCol = temp_virtual->GetMaskColor();
                for (bb=0;bb<scrnwid;bb+=4) {
                    for (cc=0;cc<scrnhit;cc+=4) {
                        temp_virtual->PutPixel(bb+pattern[aa]/4, cc+pattern[aa]%4, maskCol);
                    }
                }
                gfxDriver->UpdateDDBFromBitmap(ddb, temp_virtual, false);
                invalidate_screen();
                draw_screen_callback();
                gfxDriver->DrawSprite(0, -(temp_virtual->GetHeight() - virtual_screen->GetHeight()), ddb);
				render_to_screen(screen_bmp, 0, 0);
                update_polled_stuff_if_runtime();
                while (timerloop == 0) ;
            }
            temp_virtual->Release();

            delete temp_virtual;
            temp_virtual = NULL;
            wsetpalette(0,255,palette);
            gfxDriver->DestroyDDB(ddb);
        }

    }
    else if (evp->type==EV_IFACECLICK)
        process_interface_click(evp->data1, evp->data2, evp->data3);
    else quit("process_event: unknown event to process");
}


void runevent_now (int evtyp, int ev1, int ev2, int ev3) {
    EventHappened evh;
    evh.type = evtyp;
    evh.data1 = ev1;
    evh.data2 = ev2;
    evh.data3 = ev3;
    evh.player = game.playercharacter;
    process_event(&evh);
}

int inside_processevent=0;
void processallevents(int numev,EventHappened*evlist) {
    int dd;

    if (inside_processevent)
        return;

    // make a copy of the events - if processing an event includes
    // a blocking function it will continue to the next game loop
    // and wipe out the event pointer we were passed
    EventHappened copyOfList[MAXEVENTS];
    memcpy(&copyOfList[0], &evlist[0], sizeof(EventHappened) * numev);

    int room_was = play.room_changes;

    inside_processevent++;

    for (dd=0;dd<numev;dd++) {

        process_event(&copyOfList[dd]);

        if (room_was != play.room_changes)
            break;  // changed room, so discard other events
    }

    inside_processevent--;
}

void update_events() {
    processallevents(numevents,&event[0]);
    numevents=0;
}
// end event list functions


void ClaimEvent() {
    if (eventClaimed == EVENT_NONE)
        quit("!ClaimEvent: no event to claim");

    eventClaimed = EVENT_CLAIMED;
}
