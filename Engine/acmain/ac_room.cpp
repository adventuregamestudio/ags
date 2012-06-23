#define USE_CLIB
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_room.h"
#include "acmain/ac_commonheaders.h"
#include "script/script_runtime.h"
#include "acmain/ac_transition.h"
#include "acgfx/ac_gfxfilters.h"
#include "agsplugin.h"
#include "cs/cc_error.h"
#include "media/audio/audio.h"
#include "ac/global_audio.h"

#if defined(MAC_VERSION) || defined(LINUX_VERSION)
// for toupper
#include <ctype.h>
#endif

roomstruct thisroom;
RGB_MAP rgb_table;  // for 256-col antialiasing
int new_room_flags=0;
int gs_to_newroom=-1;

block fix_bitmap_size(block todubl) {
    int oldw=todubl->w, oldh=todubl->h;
    int newWidth = multiply_up_coordinate(thisroom.width);
    int newHeight = multiply_up_coordinate(thisroom.height);

    if ((oldw == newWidth) && (oldh == newHeight))
        return todubl;

    //  block tempb=create_bitmap(scrnwid,scrnhit);
    block tempb=create_bitmap_ex(bitmap_color_depth(todubl), newWidth, newHeight);
    set_clip(tempb,0,0,tempb->w-1,tempb->h-1);
    set_clip(todubl,0,0,oldw-1,oldh-1);
    clear(tempb);
    stretch_blit(todubl,tempb,0,0,oldw,oldh,0,0,tempb->w,tempb->h);
    destroy_bitmap(todubl); todubl=tempb;
    return todubl;
}


void SetAmbientTint (int red, int green, int blue, int opacity, int luminance) {
  if ((red < 0) || (green < 0) || (blue < 0) ||
      (red > 255) || (green > 255) || (blue > 255) ||
      (opacity < 0) || (opacity > 100) ||
      (luminance < 0) || (luminance > 100))
    quit("!SetTint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-100");

  DEBUG_CONSOLE("Set ambient tint RGB(%d,%d,%d) %d%%", red, green, blue, opacity);

  play.rtint_red = red;
  play.rtint_green = green;
  play.rtint_blue = blue;
  play.rtint_level = opacity;
  play.rtint_light = (luminance * 25) / 10;
}

void save_room_data_segment () {
  if (croom->tsdatasize > 0)
    free(croom->tsdata);
  croom->tsdata = NULL;
  croom->tsdatasize = roominst->globaldatasize;
  if (croom->tsdatasize > 0) {
    croom->tsdata=(char*)malloc(croom->tsdatasize+10);
    ccFlattenGlobalData (roominst);
    memcpy(croom->tsdata,&roominst->globaldata[0],croom->tsdatasize);
    ccUnFlattenGlobalData (roominst);
  }

}

void unload_old_room() {
  int ff;

  // if switching games on restore, don't do this
  if (displayed_room < 0)
    return;

  platform->WriteDebugString("Unloading room %d", displayed_room);

  current_fade_out_effect();

  clear(abuf);
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
    ccFreeInstance(roominstFork);
    ccFreeInstance(roominst);
    roominstFork = NULL;
    roominst=NULL;
  }
  else croom->tsdatasize=0;
  memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
  play.bg_frame=0;
  play.bg_frame_locked=0;
  play.offsets_locked=0;
  remove_screen_overlay(-1);
  if (raw_saved_screen != NULL) {
    wfreeblock(raw_saved_screen);
    raw_saved_screen = NULL;
  }
  for (ff = 0; ff < MAX_BSCENE; ff++)
    play.raw_modified[ff] = 0;
  for (ff = 0; ff < thisroom.numLocalVars; ff++)
    croom->interactionVariableValues[ff] = thisroom.localvars[ff].value;

  // wipe the character cache when we change rooms
  for (ff = 0; ff < game.numcharacters; ff++) {
    if (charcache[ff].inUse) {
      destroy_bitmap (charcache[ff].image);
      charcache[ff].image = NULL;
      charcache[ff].inUse = 0;
    }
    // ensure that any half-moves (eg. with scaled movement) are stopped
    charextra[ff].xwas = INVALID_X;
  }

  play.swap_portrait_lastchar = -1;

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
    if (objcache[ff].image != NULL) {
      destroy_bitmap (objcache[ff].image);
      objcache[ff].image = NULL;
    }
  }
  // clear the actsps buffers to save memory, since the
  // objects/characters involved probably aren't on the
  // new screen. this also ensures all cached data is flushed
  for (ff = 0; ff < MAX_INIT_SPR + game.numcharacters; ff++) {
    if (actsps[ff] != NULL)
      destroy_bitmap(actsps[ff]);
    actsps[ff] = NULL;

    if (actspsbmp[ff] != NULL)
      gfxDriver->DestroyDDB(actspsbmp[ff]);
    actspsbmp[ff] = NULL;

    if (actspswb[ff] != NULL)
      destroy_bitmap(actspswb[ff]);
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



#define NO_GAME_ID_IN_ROOM_FILE 16325
// forchar = playerchar on NewRoom, or NULL if restore saved game
void load_new_room(int newnum,CharacterInfo*forchar) {

    platform->WriteDebugString("Loading room %d", newnum);

    char rmfile[20];
    int cc;
    done_es_error = 0;
    play.room_changes ++;
    set_color_depth(8);
    displayed_room=newnum;

    sprintf(rmfile,"room%d.crm",newnum);
    if (newnum == 0) {
        // support both room0.crm and intro.crm
        FILE *inpu = clibfopen(rmfile, "rb");
        if (inpu == NULL)
            strcpy(rmfile, "intro.crm");
        else
            fclose(inpu);
    }
    // reset these back, because they might have been changed.
    if (thisroom.object!=NULL)
        destroy_bitmap(thisroom.object);
    thisroom.object=create_bitmap(320,200);

    if (thisroom.ebscene[0]!=NULL)
        destroy_bitmap(thisroom.ebscene[0]);
    thisroom.ebscene[0] = create_bitmap(320,200);

    update_polled_stuff_if_runtime();

    // load the room from disk
    our_eip=200;
    thisroom.gameId = NO_GAME_ID_IN_ROOM_FILE;
    load_room(rmfile, &thisroom, (game.default_resolution > 2));

    if ((thisroom.gameId != NO_GAME_ID_IN_ROOM_FILE) &&
        (thisroom.gameId != game.uniqueid)) {
            quitprintf("!Unable to load '%s'. This room file is assigned to a different game.", rmfile);
    }

    if ((game.default_resolution > 2) && (game.options[OPT_NATIVECOORDINATES] == 0))
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

    if ((bitmap_color_depth(thisroom.ebscene[0]) == 8) &&
        (final_col_dep > 8))
        select_palette(palette);

    for (cc=0;cc<thisroom.num_bscenes;cc++) {
        update_polled_stuff_if_runtime();
#ifdef USE_15BIT_FIX
        // convert down scenes from 16 to 15-bit if necessary
        if ((final_col_dep != game.color_depth*8) &&
            (bitmap_color_depth(thisroom.ebscene[cc]) == game.color_depth * 8)) {
                block oldblock = thisroom.ebscene[cc];
                thisroom.ebscene[cc] = convert_16_to_15(oldblock);
                wfreeblock(oldblock);
        }
        else if ((bitmap_color_depth (thisroom.ebscene[cc]) == 16) && (convert_16bit_bgr == 1))
            thisroom.ebscene[cc] = convert_16_to_16bgr (thisroom.ebscene[cc]);
#endif

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
        // PSP: Convert 32 bit backgrounds.
        if (bitmap_color_depth(thisroom.ebscene[cc]) == 32)
            thisroom.ebscene[cc] = convert_32_to_32bgr(thisroom.ebscene[cc]);
#endif

        thisroom.ebscene[cc] = gfxDriver->ConvertBitmapToSupportedColourDepth(thisroom.ebscene[cc]);
    }

    if ((bitmap_color_depth(thisroom.ebscene[0]) == 8) &&
        (final_col_dep > 8))
        unselect_palette();

    update_polled_stuff_if_runtime();

    our_eip=202;
    if (usetup.want_letterbox) {
        int abscreen=0;
        if (abuf==screen) abscreen=1;
        else if (abuf==virtual_screen) abscreen=2;
        // if this is a 640x480 room and we're in letterbox mode, full-screen it
        int newScreenHeight = final_scrn_hit;
        if (multiply_up_coordinate(thisroom.height) < final_scrn_hit) {
            clear_letterbox_borders();
            newScreenHeight = get_fixed_pixel_size(200);
        }

        if (newScreenHeight == _sub_screen->h)
        {
            screen = _sub_screen;
        }
        else if (_sub_screen->w != final_scrn_wid)
        {
            int subBitmapWidth = _sub_screen->w;
            destroy_bitmap(_sub_screen);
            _sub_screen = create_sub_bitmap(_old_screen, _old_screen->w / 2 - subBitmapWidth / 2, _old_screen->h / 2 - newScreenHeight / 2, subBitmapWidth, newScreenHeight);
            screen = _sub_screen;
        }
        else
        {
            screen = _old_screen;
        }

        scrnhit = screen->h;
        vesa_yres = scrnhit;

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
        filter->SetMouseArea(0,0, scrnwid-1, vesa_yres-1);
#endif

        if (virtual_screen->h != scrnhit) {
            int cdepth=bitmap_color_depth(virtual_screen);
            wfreeblock(virtual_screen);
            virtual_screen=create_bitmap_ex(cdepth,scrnwid,scrnhit);
            clear(virtual_screen);
            gfxDriver->SetMemoryBackBuffer(virtual_screen);
            //      ignore_mouseoff_bitmap = virtual_screen;
        }

        gfxDriver->SetRenderOffset(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));

        if (abscreen==1) abuf=screen;
        else if (abscreen==2) abuf=virtual_screen;

        update_polled_stuff_if_runtime();
    }
    // update the script viewport height
    scsystem.viewport_height = divide_down_coordinate(scrnhit);

    SetMouseBounds (0,0,0,0);

    our_eip=203;
    in_new_room=1;

    // walkable_areas_temp is used by the pathfinder to generate a
    // copy of the walkable areas - allocate it here to save time later
    if (walkable_areas_temp != NULL)
        wfreeblock (walkable_areas_temp);
    walkable_areas_temp = create_bitmap_ex (8, thisroom.walls->w, thisroom.walls->h);

    // Make a backup copy of the walkable areas prior to
    // any RemoveWalkableArea commands
    if (walkareabackup!=NULL) wfreeblock(walkareabackup);
    walkareabackup=create_bitmap(thisroom.walls->w,thisroom.walls->h);

    our_eip=204;
    // copy the walls screen
    blit(thisroom.walls,walkareabackup,0,0,0,0,thisroom.walls->w,thisroom.walls->h);
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

    if ((thisroom.ebscene[0]->w < scrnwid) ||
        (thisroom.ebscene[0]->h < scrnhit))
    {
        quitprintf("!The background scene for this room is smaller than the game resolution. If you have recently changed " 
            "the game resolution, you will need to re-import the background for this room. (Room: %d, BG Size: %d x %d)",
            newnum, thisroom.ebscene[0]->w, thisroom.ebscene[0]->h);
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
        croom=&roomstats[newnum];
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

            if (thisroom.wasversion <= 26)
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
        for (ff = 0; ff < thisroom.numLocalVars; ff++)
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
        scrObj[cc].obj = &croom->obj[cc];
        objectScriptObjNames[cc][0] = 0;
    }

    for (cc = 0; cc < croom->numobj; cc++) {
        // export the object's script object
        if (thisroom.objectscriptnames[cc][0] == 0)
            continue;

        if (thisroom.wasversion >= 26) 
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

        ccAddExternalSymbol(objectScriptObjNames[cc], &scrObj[cc]);
    }

    for (cc = 0; cc < MAX_HOTSPOTS; cc++) {
        if (thisroom.hotspotScriptNames[cc][0] == 0)
            continue;

        ccAddExternalSymbol(thisroom.hotspotScriptNames[cc], &scrHotspot[cc]);
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
        for (ff = 0; ff < 256; ff++) {
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
        for (ff = 0; ff < game.numcharacters; ff++) {
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
            ccUnFlattenGlobalData (roominst);
        }
    }
    our_eip=207;
    play.entered_edge = -1;

    if ((new_room_x != SCR_NO_VALUE) && (forchar != NULL))
    {
        forchar->x = new_room_x;
        forchar->y = new_room_y;
    }
    new_room_x = SCR_NO_VALUE;

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

    platform->WriteDebugString("Room change requested to room %d", newnum);

    update_polled_stuff_if_runtime();

    // we are currently running Leaves Screen scripts
    in_leaves_screen = newnum;

    // player leaves screen event
    run_room_event(8);
    // Run the global OnRoomLeave event
    run_on_event (GE_LEAVE_ROOM, displayed_room);

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
        spriteset.removeAll();

    update_polled_stuff_if_runtime();

    load_new_room(newnum,forchar);
}




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
  return bitmap_color_depth(thisroom.ebscene[0]);
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



void NewRoom(int nrnum) {
  if (nrnum < 0)
    quitprintf("!NewRoom: room change requested to invalid room number %d.", nrnum);

  if (displayed_room < 0) {
    // called from game_start; change the room where the game will start
    playerchar->room = nrnum;
    return;
  }

  
  DEBUG_CONSOLE("Room change requested to room %d", nrnum);
  EndSkippingUntilCharStops();

  can_run_delayed_command();

  if (play.stop_dialog_at_end != DIALOG_NONE) {
    if (play.stop_dialog_at_end == DIALOG_RUNNING)
      play.stop_dialog_at_end = DIALOG_NEWROOM + nrnum;
    else
      quit("!NewRoom: two NewRoom/RunDialog/StopDialog requests within dialog");
    return;
  }

  if (in_leaves_screen >= 0) {
    // NewRoom called from the Player Leaves Screen event -- just
    // change which room it will go to
    in_leaves_screen = nrnum;
  }
  else if (in_enters_screen) {
    setevent(EV_NEWROOM,nrnum);
    return;
  }
  else if (in_inv_screen) {
    inv_screen_newroom = nrnum;
    return;
  }
  else if ((inside_script==0) & (in_graph_script==0)) {
    new_room(nrnum,playerchar);
    return;
  }
  else if (inside_script) {
    curscript->queue_action(ePSANewRoom, nrnum, "NewRoom");
    // we might be within a MoveCharacterBlocking -- the room
    // change should abort it
    if ((playerchar->walking > 0) && (playerchar->walking < TURNING_AROUND)) {
      // nasty hack - make sure it doesn't move the character
      // to a walkable area
      mls[playerchar->walking].direct = 1;
      StopMoving(game.playercharacter);
    }
  }
  else if (in_graph_script)
    gs_to_newroom = nrnum;
}


void NewRoomEx(int nrnum,int newx,int newy) {

  Character_ChangeRoom(playerchar, nrnum, newx, newy);

}

void NewRoomNPC(int charid, int nrnum, int newx, int newy) {
  if (!is_valid_character(charid))
    quit("!NewRoomNPC: invalid character");
  if (charid == game.playercharacter)
    quit("!NewRoomNPC: use NewRoomEx with the player character");

  Character_ChangeRoom(&game.chars[charid], nrnum, newx, newy);
}

void ResetRoom(int nrnum) {
  if (nrnum == displayed_room)
    quit("!ResetRoom: cannot reset current room");
  if ((nrnum<0) | (nrnum>=MAX_ROOMS))
    quit("!ResetRoom: invalid room number");
  if (roomstats[nrnum].beenhere) {
    if (roomstats[nrnum].tsdata!=NULL)
      free(roomstats[nrnum].tsdata);
    roomstats[nrnum].tsdata=NULL;
    roomstats[nrnum].tsdatasize=0;
    }
  roomstats[nrnum].beenhere=0;
  DEBUG_CONSOLE("Room %d reset to original state", nrnum);
}

int HasPlayerBeenInRoom(int roomnum) {
  if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
    return 0;
  return roomstats[roomnum].beenhere;
}



void CallRoomScript (int value) {
  can_run_delayed_command();

  if (!inside_script)
    quit("!CallRoomScript: not inside a script???");

  play.roomscript_finished = 0;
  curscript->run_another("$on_call", value, 0);
}


int HasBeenToRoom (int roomnum) {
  if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
    quit("!HasBeenToRoom: invalid room number specified");

  if (roomstats[roomnum].beenhere)
    return 1;
  return 0;
}

int find_highest_room_entered() {
  int qq,fndas=-1;
  for (qq=0;qq<MAX_ROOMS;qq++) {
    if (roomstats[qq].beenhere!=0) fndas=qq;
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

  roominst = ccCreateInstance(thisroom.compiled_script);

  if ((ccError!=0) || (roominst==NULL)) {
   char thiserror[400];
   sprintf(thiserror, "Unable to create local script: %s", ccErrorString);
   quit(thiserror);
  }

  roominstFork = ccForkInstance(roominst);
  if (roominstFork == NULL)
    quitprintf("Unable to create forked room instance: %s", ccErrorString);

  repExecAlways.roomHasFunction = true;
  getDialogOptionsDimensionsFunc.roomHasFunction = true;
}
