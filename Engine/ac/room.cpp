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

#define USE_CLIB
#include "util/string_utils.h" //strlwr()
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "media/audio/audiodefines.h"
#include "ac/charactercache.h"
#include "ac/characterextras.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/global_game.h"
#include "ac/global_object.h"
#include "ac/global_translation.h"
#include "ac/mouse.h"
#include "ac/objectcache.h"
#include "ac/overlay.h"
#include "ac/properties.h"
#include "ac/region.h"
#include "ac/record.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/screen.h"
#include "ac/string.h"
#include "ac/viewport.h"
#include "ac/walkablearea.h"
#include "ac/walkbehind.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scripthotspot.h"
#include "script/cc_instance.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/graphicsdriver.h"
#include "core/assetmanager.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "gfx/bitmap.h"
#include "util/math.h"
#include "main/graphics_mode.h"

using AGS::Common::Bitmap;
using AGS::Common::Stream;
using AGS::Common::String;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace Math = AGS::Common::Math;
namespace Out = AGS::Common::Out;

#if !defined (WINDOWS_VERSION)
// for toupper
#include <ctype.h>
#endif

extern GameSetup usetup;
extern GameSetupStruct game;
extern GameState play;
extern RoomStatus*croom;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro
extern int displayed_room;
extern RoomObject*objs;
extern roomstruct thisroom;
extern ccInstance *roominst;
extern AGSPlatformDriver *platform;
extern int numevents;
extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_INIT_SPR];
extern CharacterExtras *charextra;
extern int done_es_error;
extern int our_eip;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int scrnwid,scrnhit;
extern Bitmap *walkareabackup, *walkable_areas_temp;
extern ScriptObject scrObj[MAX_INIT_SPR];
extern SpriteCache spriteset;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern int in_new_room, new_room_was;  // 1 in new room, 2 first time in new room, 3 loading saved game
extern ScriptHotspot scrHotspot[MAX_HOTSPOTS];
extern int guis_need_update;
extern int in_leaves_screen;
extern CharacterInfo*playerchar;
extern int starting_room;
extern unsigned int loopcounter,lastcounter;
extern int ccError;
extern char ccErrorString[400];
extern IDriverDependantBitmap* roomBackgroundBmp;
extern IGraphicsDriver *gfxDriver;
extern Bitmap *raw_saved_screen;
extern int actSpsCount;
extern Bitmap **actsps;
extern IDriverDependantBitmap* *actspsbmp;
extern Bitmap **actspswb;
extern IDriverDependantBitmap* *actspswbbmp;
extern CachedActSpsData* actspswbcache;
extern color palette[256];
extern Bitmap *virtual_screen;
extern Bitmap *_old_screen;
extern Bitmap *_sub_screen;
extern int offsetx, offsety;
extern int mouse_z_was;

extern Bitmap **guibg;
extern IDriverDependantBitmap **guibgbmp;

extern CCHotspot ccDynamicHotspot;
extern CCObject ccDynamicObject;

RGB_MAP rgb_table;  // for 256-col antialiasing
int new_room_flags=0;
int gs_to_newroom=-1;

ScriptDrawingSurface* Room_GetDrawingSurfaceForBackground(int backgroundNumber)
{
    if (displayed_room < 0)
        quit("!Room.GetDrawingSurfaceForBackground: no room is currently loaded");

    if (backgroundNumber == SCR_NO_VALUE)
    {
        backgroundNumber = play.bg_frame;
    }

    if ((backgroundNumber < 0) || (backgroundNumber >= thisroom.num_bscenes))
        quit("!Room.GetDrawingSurfaceForBackground: invalid background number specified");


    ScriptDrawingSurface *surface = new ScriptDrawingSurface();
    surface->roomBackgroundNumber = backgroundNumber;
    ccRegisterManagedObject(surface, surface);
    return surface;
}


int Room_GetObjectCount() {
    return croom->numobj;
}

int Room_GetWidth() {
    return thisroom.width;
}

int Room_GetHeight() {
    return thisroom.height;
}

int Room_GetColorDepth() {
    return thisroom.ebscene[0]->GetColorDepth();
}

int Room_GetLeftEdge() {
    return thisroom.left;
}

int Room_GetRightEdge() {
    return thisroom.right;
}

int Room_GetTopEdge() {
    return thisroom.top;
}

int Room_GetBottomEdge() {
    return thisroom.bottom;
}

int Room_GetMusicOnLoad() {
    return thisroom.options[ST_TUNE];
}

const char* Room_GetTextProperty(const char *property) {
    return get_text_property_dynamic_string(&thisroom.roomProps, property);
}

const char* Room_GetMessages(int index) {
    if ((index < 0) || (index >= thisroom.nummes)) {
        return NULL;
    }
    char buffer[STD_BUFFER_SIZE];
    buffer[0]=0;
    replace_tokens(get_translation(thisroom.message[index]), buffer, STD_BUFFER_SIZE);
    return CreateNewScriptString(buffer);
}


//=============================================================================

Bitmap *fix_bitmap_size(Bitmap *todubl) {
    int oldw=todubl->GetWidth(), oldh=todubl->GetHeight();
    int newWidth = multiply_up_coordinate(thisroom.width);
    int newHeight = multiply_up_coordinate(thisroom.height);

    if ((oldw == newWidth) && (oldh == newHeight))
        return todubl;

    //  Bitmap *tempb=BitmapHelper::CreateBitmap(scrnwid,scrnhit);
    //todubl->SetClip(Rect(0,0,oldw-1,oldh-1)); // CHECKME! [IKM] Not sure this is needed here
    Bitmap *tempb=BitmapHelper::CreateBitmap(newWidth, newHeight, todubl->GetColorDepth());
    tempb->SetClip(Rect(0,0,tempb->GetWidth()-1,tempb->GetHeight()-1));
    tempb->Fill(0);
    tempb->StretchBlt(todubl, RectWH(0,0,oldw,oldh), RectWH(0,0,tempb->GetWidth(),tempb->GetHeight()));
    delete todubl;
    return tempb;
}




void save_room_data_segment () {
    if (croom->tsdatasize > 0)
        free(croom->tsdata);
    croom->tsdata = NULL;
    croom->tsdatasize = roominst->globaldatasize;
    if (croom->tsdatasize > 0) {
        croom->tsdata=(char*)malloc(croom->tsdatasize+10);
        memcpy(croom->tsdata,&roominst->globaldata[0],croom->tsdatasize);
    }

}

void unload_old_room() {
    int ff;

    // if switching games on restore, don't do this
    if (displayed_room < 0)
        return;

    Out::FPrint("Unloading room %d", displayed_room);

    current_fade_out_effect();

    Bitmap *ds = GetVirtualScreen();
    ds->Fill(0);
    for (ff=0;ff<croom->numobj;ff++)
        objs[ff].moving = 0;

    if (!play.ambient_sounds_persist) {
        for (ff = 1; ff < MAX_SOUND_CHANNELS; ff++)
            StopAmbientSound(ff);
    }

    cancel_all_scripts();
    numevents = 0;  // cancel any pending room events

    if (roomBackgroundBmp != NULL)
    {
        gfxDriver->DestroyDDB(roomBackgroundBmp);
        roomBackgroundBmp = NULL;
    }

    if (croom==NULL) ;
    else if (roominst!=NULL) {
        save_room_data_segment();
        delete roominstFork;
        delete roominst;
        roominstFork = NULL;
        roominst=NULL;
    }
    else croom->tsdatasize=0;
    memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
    play.bg_frame=0;
    play.bg_frame_locked=0;
    play.offsets_locked=0;
    remove_screen_overlay(-1);
    delete raw_saved_screen;
    raw_saved_screen = NULL;
    for (ff = 0; ff < MAX_BSCENE; ff++)
        play.raw_modified[ff] = 0;
    for (ff = 0; ff < thisroom.numLocalVars; ff++)
        croom->interactionVariableValues[ff] = thisroom.localvars[ff].value;

    // wipe the character cache when we change rooms
    for (ff = 0; ff < game.numcharacters; ff++) {
        if (charcache[ff].inUse) {
            delete charcache[ff].image;
            charcache[ff].image = NULL;
            charcache[ff].inUse = 0;
        }
        // ensure that any half-moves (eg. with scaled movement) are stopped
        charextra[ff].xwas = INVALID_X;
    }

    play.swap_portrait_lastchar = -1;
    play.swap_portrait_lastlastchar = -1;

    for (ff = 0; ff < croom->numobj; ff++) {
        // un-export the object's script object
        if (objectScriptObjNames[ff][0] == 0)
            continue;

        ccRemoveExternalSymbol(objectScriptObjNames[ff]);
    }

    for (ff = 0; ff < MAX_HOTSPOTS; ff++) {
        if (thisroom.hotspotScriptNames[ff][0] == 0)
            continue;

        ccRemoveExternalSymbol(thisroom.hotspotScriptNames[ff]);
    }

    // clear the object cache
    for (ff = 0; ff < MAX_INIT_SPR; ff++) {
        delete objcache[ff].image;
        objcache[ff].image = NULL;
    }
    // clear the actsps buffers to save memory, since the
    // objects/characters involved probably aren't on the
    // new screen. this also ensures all cached data is flushed
    for (ff = 0; ff < MAX_INIT_SPR + game.numcharacters; ff++) {
        delete actsps[ff];
        actsps[ff] = NULL;

        if (actspsbmp[ff] != NULL)
            gfxDriver->DestroyDDB(actspsbmp[ff]);
        actspsbmp[ff] = NULL;

        delete actspswb[ff];
        actspswb[ff] = NULL;

        if (actspswbbmp[ff] != NULL)
            gfxDriver->DestroyDDB(actspswbbmp[ff]);
        actspswbbmp[ff] = NULL;

        actspswbcache[ff].valid = 0;
    }

    // if Hide Player Character was ticked, restore it to visible
    if (play.temporarily_turned_off_character >= 0) {
        game.chars[play.temporarily_turned_off_character].on = 1;
        play.temporarily_turned_off_character = -1;
    }

}



void convert_room_coordinates_to_low_res(roomstruct *rstruc)
{
    int f;
    for (f = 0; f < rstruc->numsprs; f++)
    {
        rstruc->sprs[f].x /= 2;
        rstruc->sprs[f].y /= 2;
        if (rstruc->objbaseline[f] > 0)
        {
            rstruc->objbaseline[f] /= 2;
        }
    }

    for (f = 0; f < rstruc->numhotspots; f++)
    {
        rstruc->hswalkto[f].x /= 2;
        rstruc->hswalkto[f].y /= 2;
    }

    for (f = 0; f < rstruc->numobj; f++)
    {
        rstruc->objyval[f] /= 2;
    }

    rstruc->left /= 2;
    rstruc->top /= 2;
    rstruc->bottom /= 2;
    rstruc->right /= 2;
    rstruc->width /= 2;
    rstruc->height /= 2;
}

extern int convert_16bit_bgr;

#define NO_GAME_ID_IN_ROOM_FILE 16325
// forchar = playerchar on NewRoom, or NULL if restore saved game
void load_new_room(int newnum, CharacterInfo*forchar) {

    Out::FPrint("Loading room %d", newnum);

    String room_filename;
    int cc;
    done_es_error = 0;
    play.room_changes ++;
    set_color_depth(8);
    displayed_room=newnum;

    room_filename.Format("room%d.crm", newnum);
    if (newnum == 0) {
        // support both room0.crm and intro.crm
        // 2.70: Renamed intro.crm to room0.crm, to stop it causing confusion
        if (loaded_game_file_version < kGameVersion_270 && Common::AssetManager::DoesAssetExist("intro.crm") ||
            loaded_game_file_version >= kGameVersion_270 && !Common::AssetManager::DoesAssetExist(room_filename))
        {
            room_filename = "intro.crm";
        }
    }
    // reset these back, because they might have been changed.
    delete thisroom.object;
    thisroom.object=BitmapHelper::CreateBitmap(320,200);

    delete thisroom.ebscene[0];
    thisroom.ebscene[0] = BitmapHelper::CreateBitmap(320,200);

    update_polled_stuff_if_runtime();

    // load the room from disk
    our_eip=200;
    thisroom.gameId = NO_GAME_ID_IN_ROOM_FILE;
    load_room(room_filename, &thisroom, game.IsHiRes());

    if ((thisroom.gameId != NO_GAME_ID_IN_ROOM_FILE) &&
        (thisroom.gameId != game.uniqueid)) {
            quitprintf("!Unable to load '%s'. This room file is assigned to a different game.", room_filename.GetCStr());
    }

    if (game.IsHiRes() && (game.options[OPT_NATIVECOORDINATES] == 0))
    {
        convert_room_coordinates_to_low_res(&thisroom);
    }

    update_polled_stuff_if_runtime();
    our_eip=201;
    /*  // apparently, doing this stops volume spiking between tracks
    if (thisroom.options[ST_TUNE]>0) {
    stopmusic();
    delay(100);
    }*/

    play.room_width = thisroom.width;
    play.room_height = thisroom.height;
    play.anim_background_speed = thisroom.bscene_anim_speed;
    play.bg_anim_delay = play.anim_background_speed;

    int dd;
    // do the palette
    for (cc=0;cc<256;cc++) {
        if (game.paluses[cc]==PAL_BACKGROUND)
            palette[cc]=thisroom.pal[cc];
        else {
            // copy the gamewide colours into the room palette
            for (dd = 0; dd < thisroom.num_bscenes; dd++)
                thisroom.bpalettes[dd][cc] = palette[cc];
        }
    }

    if ((thisroom.ebscene[0]->GetColorDepth() == 8) &&
        (final_col_dep > 8))
        select_palette(palette);

    for (cc=0;cc<thisroom.num_bscenes;cc++) {
        update_polled_stuff_if_runtime();
#ifdef USE_15BIT_FIX
        // convert down scenes from 16 to 15-bit if necessary
        if ((final_col_dep != game.color_depth*8) &&
            (thisroom.ebscene[cc]->GetColorDepth() == game.color_depth * 8)) {
                Bitmap *oldblock = thisroom.ebscene[cc];
                thisroom.ebscene[cc] = convert_16_to_15(oldblock);
                delete oldblock;
        }
        else if ((thisroom.ebscene[cc]->GetColorDepth () == 16) && (convert_16bit_bgr == 1))
            thisroom.ebscene[cc] = convert_16_to_16bgr (thisroom.ebscene[cc]);
#endif

#if defined (AGS_INVERTED_COLOR_ORDER)
        // PSP: Convert 32 bit backgrounds.
        if (thisroom.ebscene[cc]->GetColorDepth() == 32)
            thisroom.ebscene[cc] = convert_32_to_32bgr(thisroom.ebscene[cc]);
#endif

        thisroom.ebscene[cc] = gfxDriver->ConvertBitmapToSupportedColourDepth(thisroom.ebscene[cc]);
    }

    if ((thisroom.ebscene[0]->GetColorDepth() == 8) &&
        (final_col_dep > 8))
        unselect_palette();

    update_polled_stuff_if_runtime();

    our_eip=202;
    const int real_room_height = multiply_up_coordinate(thisroom.height);
    // Frame size is updated when letterbox mode is on, or when room's size is smaller than game's size.
    if (usetup.want_letterbox ||
        real_room_height < GameSize.Height || scrnhit < GameSize.Height) {
            int abscreen=0;

            Bitmap *ds = GetVirtualScreen();
            if (ds==BitmapHelper::GetScreenBitmap()) abscreen=1;
            else if (ds==virtual_screen) abscreen=2;
            int newScreenHeight = final_scrn_hit;
            const int viewport_height = Math::Min(real_room_height, GameSize.Height);
            if (viewport_height < final_scrn_hit) {
                clear_letterbox_borders();
                newScreenHeight = viewport_height;
            }

            // If the game is run not in letterbox mode, but there's a random room smaller than the game size,
            // then the sub_screen does not exist at this point; so we create it here.
            if (!_sub_screen)
                _sub_screen = BitmapHelper::CreateSubBitmap(_old_screen, RectWH(final_scrn_wid / 2 - scrnwid / 2, final_scrn_hit / 2-newScreenHeight/2, scrnwid, newScreenHeight));

            if (newScreenHeight == _sub_screen->GetHeight())
            {
                BitmapHelper::SetScreenBitmap( _sub_screen );
            }
            else if (_sub_screen->GetWidth() != final_scrn_wid)
            {
                int subBitmapWidth = _sub_screen->GetWidth();
                delete _sub_screen;
                _sub_screen = BitmapHelper::CreateSubBitmap(_old_screen, RectWH(_old_screen->GetWidth() / 2 - subBitmapWidth / 2, _old_screen->GetHeight() / 2 - newScreenHeight / 2, subBitmapWidth, newScreenHeight));
                BitmapHelper::SetScreenBitmap( _sub_screen );
            }
            else
            {
                BitmapHelper::SetScreenBitmap( _old_screen );
            }

            scrnhit = BitmapHelper::GetScreenBitmap()->GetHeight();
            vesa_yres = scrnhit;
            game_frame_y_offset = (final_scrn_hit - scrnhit) / 2;

            filter->SetMouseArea(0,0, scrnwid-1, vesa_yres-1);

            if (virtual_screen->GetHeight() != scrnhit) {
                int cdepth=virtual_screen->GetColorDepth();
                delete virtual_screen;
                virtual_screen=BitmapHelper::CreateBitmap(scrnwid,scrnhit,cdepth);
                virtual_screen->Clear();
                gfxDriver->SetMemoryBackBuffer(virtual_screen);
                //      ignore_mouseoff_bitmap = virtual_screen;
            }

            gfxDriver->SetRenderOffset(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));

            if (abscreen==1) //abuf=BitmapHelper::GetScreenBitmap();
                SetVirtualScreen( BitmapHelper::GetScreenBitmap() );
            else if (abscreen==2) //abuf=virtual_screen;
                SetVirtualScreen( virtual_screen );

            update_polled_stuff_if_runtime();
    }
    // update the script viewport height
    scsystem.viewport_height = divide_down_coordinate(scrnhit);

    SetMouseBounds (0,0,0,0);

    our_eip=203;
    in_new_room=1;

    // walkable_areas_temp is used by the pathfinder to generate a
    // copy of the walkable areas - allocate it here to save time later
    delete walkable_areas_temp;
    walkable_areas_temp = BitmapHelper::CreateBitmap(thisroom.walls->GetWidth(), thisroom.walls->GetHeight(), 8);

    // Make a backup copy of the walkable areas prior to
    // any RemoveWalkableArea commands
    delete walkareabackup;
    // copy the walls screen
    walkareabackup=BitmapHelper::CreateBitmapCopy(thisroom.walls);

    our_eip=204;
    update_polled_stuff_if_runtime();
    redo_walkable_areas();
    // fix walk-behinds to current screen resolution
    thisroom.object = fix_bitmap_size(thisroom.object);
    update_polled_stuff_if_runtime();

    set_color_depth(final_col_dep);
    // convert backgrounds to current res
    if (thisroom.resolution != get_fixed_pixel_size(1)) {
        for (cc=0;cc<thisroom.num_bscenes;cc++)
            thisroom.ebscene[cc] = fix_bitmap_size(thisroom.ebscene[cc]);
    }

    if ((thisroom.ebscene[0]->GetWidth() < scrnwid) ||
        (thisroom.ebscene[0]->GetHeight() < scrnhit))
    {
        quitprintf("!The background scene for this room is smaller than the game resolution. If you have recently changed " 
            "the game resolution, you will need to re-import the background for this room. (Room: %d, BG Size: %d x %d)",
            newnum, thisroom.ebscene[0]->GetWidth(), thisroom.ebscene[0]->GetHeight());
    }

    recache_walk_behinds();

    our_eip=205;
    // setup objects
    if (forchar != NULL) {
        // if not restoring a game, always reset this room
        troom.beenhere=0;  
        troom.tsdatasize=0;
        memset(&troom.hotspot_enabled[0],1,MAX_HOTSPOTS);
        memset(&troom.region_enabled[0], 1, MAX_REGIONS);
    }
    if ((newnum>=0) & (newnum<MAX_ROOMS))
        croom = getRoomStatus(newnum);
    else croom=&troom;

    if (croom->beenhere > 0) {
        // if we've been here before, save the Times Run information
        // since we will overwrite the actual NewInteraction structs
        // (cos they have pointers and this might have been loaded from
        // a save game)
        if (thisroom.roomScripts == NULL)
        {
            thisroom.intrRoom->copy_timesrun_from (&croom->intrRoom);
            for (cc=0;cc < MAX_HOTSPOTS;cc++)
                thisroom.intrHotspot[cc]->copy_timesrun_from (&croom->intrHotspot[cc]);
            for (cc=0;cc < MAX_INIT_SPR;cc++)
                thisroom.intrObject[cc]->copy_timesrun_from (&croom->intrObject[cc]);
            for (cc=0;cc < MAX_REGIONS;cc++)
                thisroom.intrRegion[cc]->copy_timesrun_from (&croom->intrRegion[cc]);
        }
    }
    if (croom->beenhere==0) {
        croom->numobj=thisroom.numsprs;
        croom->tsdatasize=0;
        for (cc=0;cc<croom->numobj;cc++) {
            croom->obj[cc].x=thisroom.sprs[cc].x;
            croom->obj[cc].y=thisroom.sprs[cc].y;

            if (thisroom.wasversion <= kRoomVersion_300a)
                croom->obj[cc].y += divide_down_coordinate(spriteheight[thisroom.sprs[cc].sprnum]);

            croom->obj[cc].num=thisroom.sprs[cc].sprnum;
            croom->obj[cc].on=thisroom.sprs[cc].on;
            croom->obj[cc].view=-1;
            croom->obj[cc].loop=0;
            croom->obj[cc].frame=0;
            croom->obj[cc].wait=0;
            croom->obj[cc].transparent=0;
            croom->obj[cc].moving=-1;
            croom->obj[cc].flags = thisroom.objectFlags[cc];
            croom->obj[cc].baseline=-1;
            croom->obj[cc].last_zoom = 100;
            croom->obj[cc].last_width = 0;
            croom->obj[cc].last_height = 0;
            croom->obj[cc].blocking_width = 0;
            croom->obj[cc].blocking_height = 0;
            if (thisroom.objbaseline[cc]>=0)
                //        croom->obj[cc].baseoffs=thisroom.objbaseline[cc]-thisroom.sprs[cc].y;
                croom->obj[cc].baseline=thisroom.objbaseline[cc];
        }
        memcpy(&croom->walkbehind_base[0],&thisroom.objyval[0],sizeof(short)*MAX_OBJ);
        for (cc=0;cc<MAX_FLAGS;cc++) croom->flagstates[cc]=0;

        /*    // we copy these structs for the Score column to work
        croom->misccond=thisroom.misccond;
        for (cc=0;cc<MAX_HOTSPOTS;cc++)
        croom->hscond[cc]=thisroom.hscond[cc];
        for (cc=0;cc<MAX_INIT_SPR;cc++)
        croom->objcond[cc]=thisroom.objcond[cc];*/

        for (cc=0;cc < MAX_HOTSPOTS;cc++) {
            croom->hotspot_enabled[cc] = 1;
        }
        for (cc = 0; cc < MAX_REGIONS; cc++) {
            croom->region_enabled[cc] = 1;
        }
        croom->beenhere=1;
        in_new_room=2;
    }
    else {
        // We have been here before
        for (int ff = 0; ff < thisroom.numLocalVars; ff++)
            thisroom.localvars[ff].value = croom->interactionVariableValues[ff];
    }

    update_polled_stuff_if_runtime();

    if (thisroom.roomScripts == NULL)
    {
        // copy interactions from room file into our temporary struct
        croom->intrRoom = thisroom.intrRoom[0];
        for (cc=0;cc<MAX_HOTSPOTS;cc++)
            croom->intrHotspot[cc] = thisroom.intrHotspot[cc][0];
        for (cc=0;cc<MAX_INIT_SPR;cc++)
            croom->intrObject[cc] = thisroom.intrObject[cc][0];
        for (cc=0;cc<MAX_REGIONS;cc++)
            croom->intrRegion[cc] = thisroom.intrRegion[cc][0];
    }

    objs=&croom->obj[0];

    for (cc = 0; cc < MAX_INIT_SPR; cc++) {
        // 64 bit: Using the id instead
        // scrObj[cc].obj = &croom->obj[cc];
        objectScriptObjNames[cc][0] = 0;
    }

    for (cc = 0; cc < croom->numobj; cc++) {
        // export the object's script object
        if (thisroom.objectscriptnames[cc][0] == 0)
            continue;

        if (thisroom.wasversion >= kRoomVersion_300a) 
        {
            strcpy(objectScriptObjNames[cc], thisroom.objectscriptnames[cc]);
        }
        else
        {
            sprintf(objectScriptObjNames[cc], "o%s", thisroom.objectscriptnames[cc]);
            strlwr(objectScriptObjNames[cc]);
            if (objectScriptObjNames[cc][1] != 0)
                objectScriptObjNames[cc][1] = toupper(objectScriptObjNames[cc][1]);
        }

        ccAddExternalDynamicObject(objectScriptObjNames[cc], &scrObj[cc], &ccDynamicObject);
    }

    for (cc = 0; cc < MAX_HOTSPOTS; cc++) {
        if (thisroom.hotspotScriptNames[cc][0] == 0)
            continue;

        ccAddExternalDynamicObject(thisroom.hotspotScriptNames[cc], &scrHotspot[cc], &ccDynamicHotspot);
    }

    our_eip=206;
    /*  THIS IS DONE IN THE EDITOR NOW
    thisroom.ebpalShared[0] = 1;
    for (dd = 1; dd < thisroom.num_bscenes; dd++) {
    if (memcmp (&thisroom.bpalettes[dd][0], &palette[0], sizeof(color) * 256) == 0)
    thisroom.ebpalShared[dd] = 1;
    else
    thisroom.ebpalShared[dd] = 0;
    }
    // only make the first frame shared if the last is
    if (thisroom.ebpalShared[thisroom.num_bscenes - 1] == 0)
    thisroom.ebpalShared[0] = 0;*/

    update_polled_stuff_if_runtime();

    our_eip = 210;
    if (IS_ANTIALIAS_SPRITES) {
        // sometimes the palette has corrupt entries, which crash
        // the create_rgb_table call
        // so, fix them
        for (int ff = 0; ff < 256; ff++) {
            if (palette[ff].r > 63)
                palette[ff].r = 63;
            if (palette[ff].g > 63)
                palette[ff].g = 63;
            if (palette[ff].b > 63)
                palette[ff].b = 63;
        }
        create_rgb_table (&rgb_table, palette, NULL);
        rgb_map = &rgb_table;
    }
    our_eip = 211;
    if (forchar!=NULL) {
        // if it's not a Restore Game

        // if a following character is still waiting to come into the
        // previous room, force it out so that the timer resets
        for (int ff = 0; ff < game.numcharacters; ff++) {
            if ((game.chars[ff].following >= 0) && (game.chars[ff].room < 0)) {
                if ((game.chars[ff].following == game.playercharacter) &&
                    (forchar->prevroom == newnum))
                    // the player went back to the previous room, so make sure
                    // the following character is still there
                    game.chars[ff].room = newnum;
                else
                    game.chars[ff].room = game.chars[game.chars[ff].following].room;
            }
        }

        offsetx=0;
        offsety=0;
        forchar->prevroom=forchar->room;
        forchar->room=newnum;
        // only stop moving if it's a new room, not a restore game
        for (cc=0;cc<game.numcharacters;cc++)
            StopMoving(cc);

    }

    update_polled_stuff_if_runtime();

    roominst=NULL;
    if (debug_flags & DBG_NOSCRIPT) ;
    else if (thisroom.compiled_script!=NULL) {
        compile_room_script();
        if (croom->tsdatasize>0) {
            if (croom->tsdatasize != roominst->globaldatasize)
                quit("room script data segment size has changed");
            memcpy(&roominst->globaldata[0],croom->tsdata,croom->tsdatasize);
        }
    }
    our_eip=207;
    play.entered_edge = -1;

    if ((new_room_x != SCR_NO_VALUE) && (forchar != NULL))
    {
        forchar->x = new_room_x;
        forchar->y = new_room_y;

        if (new_room_loop != SCR_NO_VALUE)
            forchar->loop = new_room_loop;
    }
    new_room_x = SCR_NO_VALUE;
    new_room_loop = SCR_NO_VALUE;

    if ((new_room_pos>0) & (forchar!=NULL)) {
        if (new_room_pos>=4000) {
            play.entered_edge = 3;
            forchar->y = thisroom.top + get_fixed_pixel_size(1);
            forchar->x=new_room_pos%1000;
            if (forchar->x==0) forchar->x=thisroom.width/2;
            if (forchar->x <= thisroom.left)
                forchar->x = thisroom.left + 3;
            if (forchar->x >= thisroom.right)
                forchar->x = thisroom.right - 3;
            forchar->loop=0;
        }
        else if (new_room_pos>=3000) {
            play.entered_edge = 2;
            forchar->y = thisroom.bottom - get_fixed_pixel_size(1);
            forchar->x=new_room_pos%1000;
            if (forchar->x==0) forchar->x=thisroom.width/2;
            if (forchar->x <= thisroom.left)
                forchar->x = thisroom.left + 3;
            if (forchar->x >= thisroom.right)
                forchar->x = thisroom.right - 3;
            forchar->loop=3;
        }
        else if (new_room_pos>=2000) {
            play.entered_edge = 1;
            forchar->x = thisroom.right - get_fixed_pixel_size(1);
            forchar->y=new_room_pos%1000;
            if (forchar->y==0) forchar->y=thisroom.height/2;
            if (forchar->y <= thisroom.top)
                forchar->y = thisroom.top + 3;
            if (forchar->y >= thisroom.bottom)
                forchar->y = thisroom.bottom - 3;
            forchar->loop=1;
        }
        else if (new_room_pos>=1000) {
            play.entered_edge = 0;
            forchar->x = thisroom.left + get_fixed_pixel_size(1);
            forchar->y=new_room_pos%1000;
            if (forchar->y==0) forchar->y=thisroom.height/2;
            if (forchar->y <= thisroom.top)
                forchar->y = thisroom.top + 3;
            if (forchar->y >= thisroom.bottom)
                forchar->y = thisroom.bottom - 3;
            forchar->loop=2;
        }
        // if starts on un-walkable area
        if (get_walkable_area_pixel(forchar->x, forchar->y) == 0) {
            if (new_room_pos>=3000) { // bottom or top of screen
                int tryleft=forchar->x - 1,tryright=forchar->x + 1;
                while (1) {
                    if (get_walkable_area_pixel(tryleft, forchar->y) > 0) {
                        forchar->x=tryleft; break; }
                    if (get_walkable_area_pixel(tryright, forchar->y) > 0) {
                        forchar->x=tryright; break; }
                    int nowhere=0;
                    if (tryleft>thisroom.left) { tryleft--; nowhere++; }
                    if (tryright<thisroom.right) { tryright++; nowhere++; }
                    if (nowhere==0) break;  // no place to go, so leave him
                }
            }
            else if (new_room_pos>=1000) { // left or right
                int tryleft=forchar->y - 1,tryright=forchar->y + 1;
                while (1) {
                    if (get_walkable_area_pixel(forchar->x, tryleft) > 0) {
                        forchar->y=tryleft; break; }
                    if (get_walkable_area_pixel(forchar->x, tryright) > 0) {
                        forchar->y=tryright; break; }
                    int nowhere=0;
                    if (tryleft>thisroom.top) { tryleft--; nowhere++; }
                    if (tryright<thisroom.bottom) { tryright++; nowhere++; }
                    if (nowhere==0) break;  // no place to go, so leave him
                }
            }
        }
        new_room_pos=0;
    }
    if (forchar!=NULL) {
        play.entered_at_x=forchar->x;
        play.entered_at_y=forchar->y;
        if (forchar->x >= thisroom.right)
            play.entered_edge = 1;
        else if (forchar->x <= thisroom.left)
            play.entered_edge = 0;
        else if (forchar->y >= thisroom.bottom)
            play.entered_edge = 2;
        else if (forchar->y <= thisroom.top)
            play.entered_edge = 3;
    }
    /*  if ((playerchar->x > thisroom.width) | (playerchar->y > thisroom.height))
    quit("!NewRoomEx: x/y co-ordinates are invalid");*/
    if (thisroom.options[ST_TUNE]>0)
        PlayMusicResetQueue(thisroom.options[ST_TUNE]);

    our_eip=208;
    if (forchar!=NULL) {
        if (thisroom.options[ST_MANDISABLED]==0) { forchar->on=1;
        enable_cursor_mode(0); }
        else {
            forchar->on=0;
            disable_cursor_mode(0);
            // remember which character we turned off, in case they
            // use SetPlyaerChracter within this room (so we re-enable
            // the correct character when leaving the room)
            play.temporarily_turned_off_character = game.playercharacter;
        }
        if (forchar->flags & CHF_FIXVIEW) ;
        else if (thisroom.options[ST_MANVIEW]==0) forchar->view=forchar->defview;
        else forchar->view=thisroom.options[ST_MANVIEW]-1;
        forchar->frame=0;   // make him standing
    }
    color_map = NULL;

    our_eip = 209;
    update_polled_stuff_if_runtime();
    generate_light_table();
    update_music_volume();
    update_viewport();
    our_eip = 212;
    invalidate_screen();
    for (cc=0;cc<croom->numobj;cc++) {
        if (objs[cc].on == 2)
            MergeObject(cc);
    }
    new_room_flags=0;
    play.gscript_timer=-1;  // avoid screw-ups with changing screens
    play.player_on_region = 0;
    // trash any input which they might have done while it was loading
    while (kbhit()) { if (getch()==0) getch(); }
    while (mgetbutton()!=NONE) ;
    // no fade in, so set the palette immediately in case of 256-col sprites
    if (game.color_depth > 1)
        setpal();

    our_eip=220;
    update_polled_stuff_if_runtime();
    DEBUG_CONSOLE("Now in room %d", displayed_room);
    guis_need_update = 1;
    platform->RunPluginHooks(AGSE_ENTERROOM, displayed_room);
    //  MoveToWalkableArea(game.playercharacter);
    //  MSS_CHECK_ALL_BLOCKS;
}

extern int psp_clear_cache_on_room_change;

// new_room: changes the current room number, and loads the new room from disk
void new_room(int newnum,CharacterInfo*forchar) {
    EndSkippingUntilCharStops();

    Out::FPrint("Room change requested to room %d", newnum);

    update_polled_stuff_if_runtime();

    // we are currently running Leaves Screen scripts
    in_leaves_screen = newnum;

    // player leaves screen event
    run_room_event(8);
    // Run the global OnRoomLeave event
    run_on_event (GE_LEAVE_ROOM, RuntimeScriptValue().SetInt32(displayed_room));

    platform->RunPluginHooks(AGSE_LEAVEROOM, displayed_room);

    // update the new room number if it has been altered by OnLeave scripts
    newnum = in_leaves_screen;
    in_leaves_screen = -1;

    if ((playerchar->following >= 0) &&
        (game.chars[playerchar->following].room != newnum)) {
            // the player character is following another character,
            // who is not in the new room. therefore, abort the follow
            playerchar->following = -1;
    }
    update_polled_stuff_if_runtime();

    // change rooms
    unload_old_room();

    if (psp_clear_cache_on_room_change)
    {
        // Delete all cached sprites
        spriteset.removeAll();

        // Delete all gui background images
        for (int i = 0; i < game.numgui; i++)
        {
            delete guibg[i];
            guibg[i] = NULL;

            if (guibgbmp[i])
                gfxDriver->DestroyDDB(guibgbmp[i]);
            guibgbmp[i] = NULL;
        }
        guis_need_update = 1;
    }

    update_polled_stuff_if_runtime();

    load_new_room(newnum,forchar);
}

int find_highest_room_entered() {
    int qq,fndas=-1;
    for (qq=0;qq<MAX_ROOMS;qq++) {
        if (isRoomStatusValid(qq) && (getRoomStatus(qq)->beenhere != 0))
            fndas = qq;
    }
    // This is actually legal - they might start in room 400 and save
    //if (fndas<0) quit("find_highest_room: been in no rooms?");
    return fndas;
}

extern long t1;  // defined in ac_main

void first_room_initialization() {
    starting_room = displayed_room;
    t1 = time(NULL);
    lastcounter=0;
    loopcounter=0;
    mouse_z_was = mouse_z;
}

void check_new_room() {
    // if they're in a new room, run Player Enters Screen and on_event(ENTER_ROOM)
    if ((in_new_room>0) & (in_new_room!=3)) {
        EventHappened evh;
        evh.type = EV_RUNEVBLOCK;
        evh.data1 = EVB_ROOM;
        evh.data2 = 0;
        evh.data3 = 5;
        evh.player=game.playercharacter;
        // make sure that any script calls don't re-call enters screen
        int newroom_was = in_new_room;
        in_new_room = 0;
        play.disabled_user_interface ++;
        process_event(&evh);
        play.disabled_user_interface --;
        in_new_room = newroom_was;
        //    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,5);
    }
}

void compile_room_script() {
    ccError = 0;

    roominst = ccInstance::CreateFromScript(thisroom.compiled_script);

    if ((ccError!=0) || (roominst==NULL)) {
        char thiserror[400];
        sprintf(thiserror, "Unable to create local script: %s", ccErrorString);
        quit(thiserror);
    }

    roominstFork = roominst->Fork();
    if (roominstFork == NULL)
        quitprintf("Unable to create forked room instance: %s", ccErrorString);

    repExecAlways.roomHasFunction = true;
    getDialogOptionsDimensionsFunc.roomHasFunction = true;
}

int bg_just_changed = 0;

void on_background_frame_change () {

    invalidate_screen();
    mark_current_background_dirty();
    invalidate_cached_walkbehinds();

    // get the new frame's palette
    memcpy (palette, thisroom.bpalettes[play.bg_frame], sizeof(color) * 256);

    // hi-colour, update the palette. It won't have an immediate effect
    // but will be drawn properly when the screen fades in
    if (game.color_depth > 1)
        setpal();

    if (in_enters_screen)
        return;

    // Don't update the palette if it hasn't changed
    if (thisroom.ebpalShared[play.bg_frame])
        return;

    // 256-colours, tell it to update the palette (will actually be done as
    // close as possible to the screen update to prevent flicker problem)
    if (game.color_depth == 1)
        bg_just_changed = 1;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"

extern ScriptString myScriptStringImpl;

// ScriptDrawingSurface* (int backgroundNumber)
RuntimeScriptValue Sc_Room_GetDrawingSurfaceForBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDrawingSurface, Room_GetDrawingSurfaceForBackground);
}

// const char* (const char *property)
RuntimeScriptValue Sc_Room_GetTextProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, Room_GetTextProperty, const char);
}

// int ()
RuntimeScriptValue Sc_Room_GetBottomEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetBottomEdge);
}

// int ()
RuntimeScriptValue Sc_Room_GetColorDepth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetColorDepth);
}

// int ()
RuntimeScriptValue Sc_Room_GetHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetHeight);
}

// int ()
RuntimeScriptValue Sc_Room_GetLeftEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetLeftEdge);
}

// const char* (int index)
RuntimeScriptValue Sc_Room_GetMessages(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Room_GetMessages);
}

// int ()
RuntimeScriptValue Sc_Room_GetMusicOnLoad(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetMusicOnLoad);
}

// int ()
RuntimeScriptValue Sc_Room_GetObjectCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetObjectCount);
}

// int ()
RuntimeScriptValue Sc_Room_GetRightEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetRightEdge);
}

// int ()
RuntimeScriptValue Sc_Room_GetTopEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetTopEdge);
}

// int ()
RuntimeScriptValue Sc_Room_GetWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetWidth);
}


void RegisterRoomAPI()
{
    ccAddExternalStaticFunction("Room::GetDrawingSurfaceForBackground^1",   Sc_Room_GetDrawingSurfaceForBackground);
    ccAddExternalStaticFunction("Room::GetTextProperty^1",                  Sc_Room_GetTextProperty);
    ccAddExternalStaticFunction("Room::get_BottomEdge",                     Sc_Room_GetBottomEdge);
    ccAddExternalStaticFunction("Room::get_ColorDepth",                     Sc_Room_GetColorDepth);
    ccAddExternalStaticFunction("Room::get_Height",                         Sc_Room_GetHeight);
    ccAddExternalStaticFunction("Room::get_LeftEdge",                       Sc_Room_GetLeftEdge);
    ccAddExternalStaticFunction("Room::geti_Messages",                      Sc_Room_GetMessages);
    ccAddExternalStaticFunction("Room::get_MusicOnLoad",                    Sc_Room_GetMusicOnLoad);
    ccAddExternalStaticFunction("Room::get_ObjectCount",                    Sc_Room_GetObjectCount);
    ccAddExternalStaticFunction("Room::get_RightEdge",                      Sc_Room_GetRightEdge);
    ccAddExternalStaticFunction("Room::get_TopEdge",                        Sc_Room_GetTopEdge);
    ccAddExternalStaticFunction("Room::get_Width",                          Sc_Room_GetWidth);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Room::GetDrawingSurfaceForBackground^1",   (void*)Room_GetDrawingSurfaceForBackground);
    ccAddExternalFunctionForPlugin("Room::GetTextProperty^1",                  (void*)Room_GetTextProperty);
    ccAddExternalFunctionForPlugin("Room::get_BottomEdge",                     (void*)Room_GetBottomEdge);
    ccAddExternalFunctionForPlugin("Room::get_ColorDepth",                     (void*)Room_GetColorDepth);
    ccAddExternalFunctionForPlugin("Room::get_Height",                         (void*)Room_GetHeight);
    ccAddExternalFunctionForPlugin("Room::get_LeftEdge",                       (void*)Room_GetLeftEdge);
    ccAddExternalFunctionForPlugin("Room::geti_Messages",                      (void*)Room_GetMessages);
    ccAddExternalFunctionForPlugin("Room::get_MusicOnLoad",                    (void*)Room_GetMusicOnLoad);
    ccAddExternalFunctionForPlugin("Room::get_ObjectCount",                    (void*)Room_GetObjectCount);
    ccAddExternalFunctionForPlugin("Room::get_RightEdge",                      (void*)Room_GetRightEdge);
    ccAddExternalFunctionForPlugin("Room::get_TopEdge",                        (void*)Room_GetTopEdge);
    ccAddExternalFunctionForPlugin("Room::get_Width",                          (void*)Room_GetWidth);
}
