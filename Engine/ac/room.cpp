//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include <ctype.h> // for toupper

#include "core/platform.h"
#include "util/string_utils.h" //strlwr()
#include "ac/common.h"
#include "ac/character.h"
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
#include "ac/movelist.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/properties.h"
#include "ac/region.h"
#include "ac/sys_events.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/screen.h"
#include "ac/string.h"
#include "ac/system.h"
#include "ac/walkablearea.h"
#include "ac/walkbehind.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scripthotspot.h"
#include "ac/dynobj/dynobj_manager.h"
#include "gui/guimain.h"
#include "script/cc_instance.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "game/room_version.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin_evts.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/graphicsdriver.h"
#include "core/assetmanager.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "gfx/bitmap.h"
#include "gfx/gfxfilter.h"
#include "media/audio/audio_system.h"
#include "main/game_run.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern RoomStatus*croom;
extern RoomStatus troom;    // used for non-saveable rooms, eg. intro
extern int displayed_room;
extern RoomObject*objs;
extern AGSPlatformDriver *platform;
extern int done_es_error;
extern Bitmap *walkareabackup, *walkable_areas_temp;
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern SpriteCache spriteset;
extern int in_new_room, new_room_was;  // 1 in new room, 2 first time in new room, 3 loading saved game
extern ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
extern int in_leaves_screen;
extern CharacterInfo*playerchar;
extern int starting_room;
extern unsigned int loopcounter;
extern IDriverDependantBitmap* roomBackgroundBmp;
extern IGraphicsDriver *gfxDriver;
extern RGB palette[256];

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

    if ((backgroundNumber < 0) || ((size_t)backgroundNumber >= thisroom.BgFrameCount))
        quit("!Room.GetDrawingSurfaceForBackground: invalid background number specified");


    ScriptDrawingSurface *surface = new ScriptDrawingSurface();
    surface->roomBackgroundNumber = backgroundNumber;
    ccRegisterManagedObject(surface, surface);
    return surface;
}

ScriptDrawingSurface* Room_GetDrawingSurfaceForMask(RoomAreaMask mask)
{
    if (displayed_room < 0)
        quit("!Room_GetDrawingSurfaceForMask: no room is currently loaded");
    ScriptDrawingSurface *surface = new ScriptDrawingSurface();
    surface->roomMaskType = mask;
    ccRegisterManagedObject(surface, surface);
    return surface;
}

int Room_GetObjectCount() {
    return croom->numobj;
}

int Room_GetWidth() {
    return thisroom.Width;
}

int Room_GetHeight() {
    return thisroom.Height;
}

int Room_GetColorDepth() {
    return thisroom.BgFrames[0].Graphic->GetColorDepth();
}

int Room_GetLeftEdge() {
    return thisroom.Edges.Left;
}

int Room_GetRightEdge() {
    return thisroom.Edges.Right;
}

int Room_GetTopEdge() {
    return thisroom.Edges.Top;
}

int Room_GetBottomEdge() {
    return thisroom.Edges.Bottom;
}

int Room_GetMusicOnLoad() {
    return thisroom.Options.StartupMusic;
}

int Room_GetProperty(const char *property)
{
    return get_int_property(thisroom.Properties, croom->roomProps, property);
}

const char* Room_GetTextProperty(const char *property)
{
    return get_text_property_dynamic_string(thisroom.Properties, croom->roomProps, property);
}

bool Room_SetProperty(const char *property, int value)
{
    return set_int_property(croom->roomProps, property, value);
}

bool Room_SetTextProperty(const char *property, const char *value)
{
    return set_text_property(croom->roomProps, property, value);
}

const char* Room_GetMessages(int index) {
    if ((index < 0) || ((size_t)index >= thisroom.MessageCount)) {
        return nullptr;
    }
    char buffer[STD_BUFFER_SIZE];
    buffer[0]=0;
    replace_tokens(get_translation(thisroom.Messages[index].GetCStr()), buffer, STD_BUFFER_SIZE);
    return CreateNewScriptString(buffer);
}

bool Room_Exists(int room)
{
    String room_filename;
    room_filename.Format("room%d.crm", room);
    return AssetMgr->DoesAssetExist(room_filename);
}

ScriptDrawingSurface *GetDrawingSurfaceForWalkableArea()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaWalkable);
}

ScriptDrawingSurface *GetDrawingSurfaceForWalkbehind()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaWalkBehind);
}

ScriptDrawingSurface *Hotspot_GetDrawingSurface()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaHotspot);
}

ScriptDrawingSurface *Region_GetDrawingSurface()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaRegion);
}

//=============================================================================

// Makes sure that room background and walk-behind mask are matching room size
// in game resolution coordinates; in other words makes graphics appropriate
// for display in the game.
void convert_room_background_to_game_res()
{
    if (!game.AllowRelativeRes() || !thisroom.IsRelativeRes())
        return;

    const int bkg_width = data_to_game_coord(thisroom.Width);
    const int bkg_height = data_to_game_coord(thisroom.Height);

    for (size_t i = 0; i < thisroom.BgFrameCount; ++i)
        thisroom.BgFrames[i].Graphic = FixBitmap(thisroom.BgFrames[i].Graphic, bkg_width, bkg_height);

    // Fix masks to match resized room background
    // Walk-behind is always 1:1 with room background size
    thisroom.WalkBehindMask = FixBitmap(thisroom.WalkBehindMask, bkg_width, bkg_height);
    // For the rest we keep the masks at original res, but update the MaskResolution,
    // as it must correspond to the runtime data->game coordinate conversion
    thisroom.MaskResolution = data_to_game_coord(thisroom.MaskResolution);
}


void save_room_data_segment () {
    croom->FreeScriptData();
    
    croom->tsdatasize = roominst->globaldatasize;
    if (croom->tsdatasize > 0) {
        croom->tsdata.resize(croom->tsdatasize);
        memcpy(croom->tsdata.data(),&roominst->globaldata[0],croom->tsdatasize);
    }

}

void unload_old_room() {
    // if switching games on restore, don't do this
    if (displayed_room < 0)
        return;

    current_fade_out_effect();

    // room unloaded callback
    run_room_event(EVROM_AFTERFADEOUT);
    // global room unloaded event
    run_on_event(GE_LEAVE_ROOM_AFTERFADE, RuntimeScriptValue().SetInt32(displayed_room));

    debug_script_log("Unloading room %d", displayed_room);

    dispose_room_drawdata();

    for (uint32_t ff=0;ff<croom->numobj;ff++)
        objs[ff].moving = 0;

    if (!play.ambient_sounds_persist) {
        for (int ff = NUM_SPEECH_CHANS; ff < game.numGameChannels; ff++)
            StopAmbientSound(ff);
    }

    cancel_all_scripts();
    events.clear();  // cancel any pending room events

    if (roomBackgroundBmp != nullptr)
    {
        gfxDriver->DestroyDDB(roomBackgroundBmp);
        roomBackgroundBmp = nullptr;
    }

    if (croom==nullptr) ;
    else if (roominst!=nullptr) {
        save_room_data_segment();
        FreeRoomScriptInstance();
    }
    else croom->tsdatasize=0;
    memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS);
    play.bg_frame = 0;
    play.bg_frame_locked = 0;
    remove_all_overlays();
    raw_saved_screen = nullptr;
    for (int ff = 0; ff < MAX_ROOM_BGFRAMES; ff++)
        play.raw_modified[ff] = 0;
    for (size_t i = 0; i < thisroom.LocalVariables.size() && i < MAX_GLOBAL_VARIABLES; ++i)
        croom->interactionVariableValues[i] = thisroom.LocalVariables[i].Value;

    // ensure that any half-moves (eg. with scaled movement) are stopped
    for (int ff = 0; ff < game.numcharacters; ff++) {
        charextra[ff].xwas = INVALID_X;
    }

    play.swap_portrait_lastchar = -1;
    play.swap_portrait_lastlastchar = -1;

    for (uint32_t ff = 0; ff < croom->numobj; ff++) {
        // un-export the object's script object
        if (thisroom.Objects[ff].ScriptName.IsEmpty())
            continue;

        ccRemoveExternalSymbol(thisroom.Objects[ff].ScriptName);
    }

    for (int ff = 0; ff < MAX_ROOM_HOTSPOTS; ff++) {
        if (thisroom.Hotspots[ff].ScriptName.IsEmpty())
            continue;

        ccRemoveExternalSymbol(thisroom.Hotspots[ff].ScriptName);
    }

    croom_ptr_clear();

    // clear the draw caches to save memory, since many of the the involved
    // objects probably aren't on the new screen
    clear_drawobj_cache();

    // if Hide Player Character was ticked, restore it to visible
    if (play.temporarily_turned_off_character >= 0) {
        game.chars[play.temporarily_turned_off_character].on = 1;
        play.temporarily_turned_off_character = -1;
    }

}

// Convert all room objects to the data resolution (only if it's different from game resolution).
// TODO: merge this into UpdateRoomData? or this is required for engine only?
void convert_room_coordinates_to_data_res(RoomStruct *rstruc)
{
    if (game.GetDataUpscaleMult() == 1)
        return;

    const int mul = game.GetDataUpscaleMult();
    for (auto &obj : rstruc->Objects)
    {
        obj.X /= mul;
        obj.Y /= mul;
        if (obj.Baseline > 0)
        {
            obj.Baseline /= mul;
        }
    }

    for (size_t i = 0; i < rstruc->HotspotCount; ++i)
    {
        rstruc->Hotspots[i].WalkTo.X /= mul;
        rstruc->Hotspots[i].WalkTo.Y /= mul;
    }

    for (size_t i = 0; i < rstruc->WalkBehindCount; ++i)
    {
        rstruc->WalkBehinds[i].Baseline /= mul;
    }

    rstruc->Edges.Left /= mul;
    rstruc->Edges.Top /= mul;
    rstruc->Edges.Bottom /= mul;
    rstruc->Edges.Right /= mul;
    rstruc->Width /= mul;
    rstruc->Height /= mul;
}

void update_letterbox_mode()
{
    const Size real_room_sz = Size(data_to_game_coord(thisroom.Width), data_to_game_coord(thisroom.Height));
    const Rect game_frame = RectWH(game.GetGameRes());
    Rect new_main_view = game_frame;
    // In the original engine the letterbox feature only allowed viewports of
    // either 200 or 240 (400 and 480) pixels, if the room height was equal or greater than 200 (400).
    // Also, the UI viewport should be matching room viewport in that case.
    // NOTE: if "OPT_LETTERBOX" is false, altsize.Height = size.Height always.
    const int viewport_height =
        real_room_sz.Height < game.GetLetterboxSize().Height ? real_room_sz.Height :
        (real_room_sz.Height >= game.GetLetterboxSize().Height && real_room_sz.Height < game.GetGameRes().Height) ? game.GetLetterboxSize().Height :
        game.GetGameRes().Height;
    new_main_view.SetHeight(viewport_height);

    play.SetMainViewport(CenterInRect(game_frame, new_main_view));
    play.SetUIViewport(new_main_view);
    on_mainviewport_changed();
}

// Automatically reset primary room viewport and camera to match the new room size
static void adjust_viewport_to_room()
{
    const Size real_room_sz = Size(data_to_game_coord(thisroom.Width), data_to_game_coord(thisroom.Height));
    const Rect main_view = play.GetMainViewport();
    Rect new_room_view = RectWH(Size::Clamp(real_room_sz, Size(1, 1), main_view.GetSize()));

    auto view = play.GetRoomViewport(0);
    view->SetRect(new_room_view);
    auto cam = view->GetCamera();
    if (cam)
    {
        cam->SetSize(new_room_view.GetSize());
        cam->SetAt(0, 0);
        cam->Release();
    }
}

// Run through all viewports and cameras to make sure they can work in new room's bounds
static void update_all_viewcams_with_newroom()
{
    for (int i = 0; i < play.GetRoomCameraCount(); ++i)
    {
        auto cam = play.GetRoomCamera(i);
        const Rect old_pos = cam->GetRect();
        cam->SetSize(old_pos.GetSize());
        cam->SetAt(old_pos.Left, old_pos.Top);
    }
}

// Looks up for the room script available as a separate asset.
// This is optional, so no error is raised if one is not found.
// If found however, it will replace room script if one had been loaded
// from the room file itself.
HError LoadRoomScript(RoomStruct *room, int newnum)
{
    String filename = String::FromFormat("room%d.o", newnum);
    std::unique_ptr<Stream> in(AssetMgr->OpenAsset(filename));
    if (in)
    {
        PScript script(ccScript::CreateFromStream(in.get()));
        if (!script)
            return new Error(String::FromFormat(
                "Failed to load a script module: %s", filename.GetCStr()),
                cc_get_error().ErrorString);
        room->CompiledScript = script;
    }
    return HError::None();
}

static void reset_temp_room()
{
    troom = RoomStatus();
}

// forchar = playerchar on NewRoom, or NULL if restore saved game
void load_new_room(int newnum, CharacterInfo*forchar) {

    debug_script_log("Loading room %d", newnum);

    String room_filename;
    done_es_error = 0;
    play.room_changes ++;
    // TODO: find out why do we need to temporarily lower color depth to 8-bit.
    // Or do we? There's a serious usability problem in this: if any bitmap is
    // created meanwhile it will have this color depth by default, which may
    // lead to unexpected errors.
    set_color_depth(8);
    displayed_room=newnum;

    room_filename.Format("room%d.crm", newnum);
    if (newnum == 0) {
        // support both room0.crm and intro.crm
        // 2.70: Renamed intro.crm to room0.crm, to stop it causing confusion
        if ((loaded_game_file_version < kGameVersion_270 && AssetMgr->DoesAssetExist("intro.crm")) ||
            (loaded_game_file_version >= kGameVersion_270 && !AssetMgr->DoesAssetExist(room_filename)))
        {
            room_filename = "intro.crm";
        }
    }

    // load the room from disk
    set_our_eip(200);
    thisroom.GameID = NO_GAME_ID_IN_ROOM_FILE;
    load_room(room_filename, &thisroom, game.IsLegacyHiRes(), game.SpriteInfos);

    if ((thisroom.GameID != NO_GAME_ID_IN_ROOM_FILE) &&
        (thisroom.GameID != game.uniqueid)) {
            quitprintf("!Unable to load '%s'. This room file is assigned to a different game.", room_filename.GetCStr());
    }

    HError err = LoadRoomScript(&thisroom, newnum);
    if (!err)
        quitprintf("!Unable to load '%s'. Error: %s", room_filename.GetCStr(),
            err->FullMessage().GetCStr());

    convert_room_coordinates_to_data_res(&thisroom);

    set_our_eip(201);

    play.room_width = thisroom.Width;
    play.room_height = thisroom.Height;
    play.anim_background_speed = thisroom.BgAnimSpeed;
    play.bg_anim_delay = play.anim_background_speed;

    // Fixup the frame index, in case the new room does not have enough background frames
    if (play.bg_frame < 0 || static_cast<size_t>(play.bg_frame) >= thisroom.BgFrameCount)
        play.bg_frame = 0;

    // do the palette
    for (size_t cc=0;cc<256;cc++) {
        if (game.paluses[cc]==PAL_BACKGROUND)
            palette[cc]=thisroom.Palette[cc];
        else {
            // copy the gamewide colours into the room palette
            for (size_t i = 0; i < thisroom.BgFrameCount; ++i)
                thisroom.BgFrames[i].Palette[cc] = palette[cc];
        }
    }

    for (size_t i = 0; i < thisroom.BgFrameCount; ++i) {
        thisroom.BgFrames[i].Graphic = PrepareSpriteForUse(thisroom.BgFrames[i].Graphic, false);
    }

    set_our_eip(202);
    // Update game viewports
    if (game.IsLegacyLetterbox())
        update_letterbox_mode();
    SetMouseBounds(0, 0, 0, 0);

    set_our_eip(203);
    in_new_room=1;

    set_color_depth(game.GetColorDepth());
    // Make sure the room gfx and masks are matching game's native res
    convert_room_background_to_game_res();

    // walkable_areas_temp is used by the pathfinder to generate a
    // copy of the walkable areas - allocate it here to save time later
    delete walkable_areas_temp;
    walkable_areas_temp = BitmapHelper::CreateBitmap(thisroom.WalkAreaMask->GetWidth(), thisroom.WalkAreaMask->GetHeight(), 8);

    // Make a backup copy of the walkable areas prior to
    // any RemoveWalkableArea commands
    delete walkareabackup;
    // copy the walls screen
    walkareabackup=BitmapHelper::CreateBitmapCopy(thisroom.WalkAreaMask.get());

    set_our_eip(204);
    redo_walkable_areas();
    walkbehinds_recalc();

    set_our_eip(205);
    // setup objects
    if (forchar != nullptr) {
        // if not restoring a game, always reset this room
        reset_temp_room();
    }
    if ((newnum>=0) & (newnum<MAX_ROOMS))
        croom = getRoomStatus(newnum);
    else croom=&troom;

    // Decide what to do if we have been or not in this room before
    if (croom->beenhere > 0)
    {
        // if we've been here before, save the Times Run information
        // since we will overwrite the actual NewInteraction structs
        // (cos they have pointers and this might have been loaded from
        // a save game)
        if (thisroom.EventHandlers == nullptr)
        {// legacy interactions
            thisroom.Interaction->CopyTimesRun(croom->intrRoom);
            for (int cc=0;cc < MAX_ROOM_HOTSPOTS;cc++)
                thisroom.Hotspots[cc].Interaction->CopyTimesRun(croom->intrHotspot[cc]);
            for (size_t cc=0;cc < thisroom.Objects.size();cc++)
                thisroom.Objects[cc].Interaction->CopyTimesRun(croom->intrObject[cc]);
            for (int cc=0;cc < MAX_ROOM_REGIONS;cc++)
                thisroom.Regions[cc].Interaction->CopyTimesRun(croom->intrRegion[cc]);
        }
        for (size_t i = 0; i < thisroom.LocalVariables.size() && i < (size_t)MAX_GLOBAL_VARIABLES; ++i)
            thisroom.LocalVariables[i].Value = croom->interactionVariableValues[i];

        // Always copy object and hotspot names for < 3.6.0 games, because they were not settable
        if ((loaded_game_file_version < kGameVersion_360_16) ||
            (croom->contentFormat < kRoomStatSvgVersion_36025))
        {
            for (size_t cc = 0; cc < thisroom.Objects.size(); ++cc)
                croom->obj[cc].name = thisroom.Objects[cc].Name;
            for (int cc = 0; cc < MAX_ROOM_HOTSPOTS; cc++) 
                croom->hotspot[cc].Name = thisroom.Hotspots[cc].Name;
        }
    }
    else
    {
        // If we have not been in this room before, then copy necessary fields from thisroom
        croom->numobj=thisroom.Objects.size();
        croom->tsdatasize=0;
        croom->obj.resize(croom->numobj);
        croom->objProps.resize(croom->numobj);
        croom->intrObject.resize(croom->numobj);
        for (uint32_t cc=0;cc<croom->numobj;cc++) {
            const auto &trobj = thisroom.Objects[cc];
            auto &crobj = croom->obj[cc];
            crobj.x=trobj.X;
            crobj.y=trobj.Y;
            crobj.num = Math::InRangeOrDef<uint16_t>(trobj.Sprite, 0);
            crobj.on=trobj.IsOn;
            crobj.view=RoomObject::NoView;
            crobj.loop=0;
            crobj.frame=0;
            crobj.wait=0;
            crobj.transparent=0;
            crobj.moving=-1;
            crobj.flags = trobj.Flags;
            crobj.baseline=-1;
            crobj.zoom = 100;
            crobj.last_width = 0;
            crobj.last_height = 0;
            crobj.blocking_width = 0;
            crobj.blocking_height = 0;
            crobj.name = trobj.Name;
            if (trobj.Baseline>=0)
                crobj.baseline=trobj.Baseline;
            if (trobj.Sprite > UINT16_MAX)
                debug_script_warn("Warning: object's (id %d) sprite %d outside of internal range (%d), reset to 0",
                    cc, trobj.Sprite, UINT16_MAX);
        }
        for (size_t i = 0; i < (size_t)MAX_WALK_BEHINDS; ++i)
            croom->walkbehind_base[i] = thisroom.WalkBehinds[i].Baseline;

        for (int cc=0;cc < MAX_ROOM_HOTSPOTS;cc++) {
            croom->hotspot[cc].Enabled = true;
            croom->hotspot[cc].Name = thisroom.Hotspots[cc].Name;
        }
        for (int cc = 0; cc < MAX_ROOM_REGIONS; cc++) {
            croom->region_enabled[cc] = 1;
        }

#if defined (OBSOLETE)
        for (cc = 0; cc<MAX_LEGACY_ROOM_FLAGS; cc++) croom->flagstates[cc] = 0;
        // we copy these structs for the Score column to work
        croom->misccond = thisroom.misccond;
        for (cc = 0; cc<MAX_ROOM_HOTSPOTS; cc++)
            croom->hscond[cc] = thisroom.hscond[cc];
        for (cc = 0; cc<MAX_ROOM_OBJECTS; cc++)
            croom->objcond[cc] = thisroom.objcond[cc];
#endif

        croom->beenhere=1;
        in_new_room=2;
    }
    // Reset contentFormat hint to avoid doing fixups later
    croom->contentFormat = kRoomStatSvgVersion_Current;

    if (thisroom.EventHandlers == nullptr)
    {// legacy interactions
        // copy interactions from room file into our temporary struct
        croom->intrRoom = *thisroom.Interaction;
        for (int cc=0;cc<MAX_ROOM_HOTSPOTS;cc++)
            croom->intrHotspot[cc] = *thisroom.Hotspots[cc].Interaction;
        for (size_t cc=0;cc<thisroom.Objects.size();cc++)
            croom->intrObject[cc] = *thisroom.Objects[cc].Interaction;
        for (int cc=0;cc<MAX_ROOM_REGIONS;cc++)
            croom->intrRegion[cc] = *thisroom.Regions[cc].Interaction;
    }

    objs = croom->obj.size() > 0 ? &croom->obj[0] : nullptr;

    for (uint32_t cc = 0; cc < croom->numobj; cc++) {
        // export the object's script object
        if (thisroom.Objects[cc].ScriptName.IsEmpty())
            continue;
        ccAddExternalScriptObject(thisroom.Objects[cc].ScriptName, &scrObj[cc], &ccDynamicObject);
    }

    for (int cc = 0; cc < MAX_ROOM_HOTSPOTS; cc++) {
        if (thisroom.Hotspots[cc].ScriptName.IsEmpty())
            continue;

        ccAddExternalScriptObject(thisroom.Hotspots[cc].ScriptName, &scrHotspot[cc], &ccDynamicHotspot);
    }

    set_our_eip(210);
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
        create_rgb_table (&rgb_table, palette, nullptr);
        rgb_map = &rgb_table;
    }
    set_our_eip(211);
    if (forchar!=nullptr) {
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

        forchar->prevroom=forchar->room;
        forchar->room=newnum;
        // only stop moving if it's a new room, not a restore game
        for (int cc=0;cc<game.numcharacters;cc++)
            StopMoving(cc);
    }

    roominst=nullptr;
    if (debug_flags & DBG_NOSCRIPT) ;
    else if (thisroom.CompiledScript!=nullptr) {
        compile_room_script();
        if (croom->tsdatasize>0) {
            if (croom->tsdatasize != roominst->globaldatasize)
                quit("room script data segment size has changed");
            memcpy(&roominst->globaldata[0],croom->tsdata.data(),croom->tsdatasize);
        }
    }
    set_our_eip(207);
    play.entered_edge = -1;

    if ((new_room_x != SCR_NO_VALUE) && (forchar != nullptr))
    {
        forchar->x = new_room_x;
        forchar->y = new_room_y;
        if (new_room_placeonwalkable)
            Character_PlaceOnWalkableArea(forchar);

		if (new_room_loop != SCR_NO_VALUE)
			forchar->loop = new_room_loop;
    }
    // reset new_room instructions
    new_room_x = new_room_y = SCR_NO_VALUE;
    new_room_loop = SCR_NO_VALUE;
    new_room_placeonwalkable = false;

    if ((new_room_pos>0) & (forchar!=nullptr)) {
        if (new_room_pos>=4000) {
            play.entered_edge = 3;
            forchar->y = thisroom.Edges.Top + get_fixed_pixel_size(1);
            forchar->x=new_room_pos%1000;
            if (forchar->x==0) forchar->x=thisroom.Width/2;
            if (forchar->x <= thisroom.Edges.Left)
                forchar->x = thisroom.Edges.Left + 3;
            if (forchar->x >= thisroom.Edges.Right)
                forchar->x = thisroom.Edges.Right - 3;
            forchar->loop=0;
        }
        else if (new_room_pos>=3000) {
            play.entered_edge = 2;
            forchar->y = thisroom.Edges.Bottom - get_fixed_pixel_size(1);
            forchar->x=new_room_pos%1000;
            if (forchar->x==0) forchar->x=thisroom.Width/2;
            if (forchar->x <= thisroom.Edges.Left)
                forchar->x = thisroom.Edges.Left + 3;
            if (forchar->x >= thisroom.Edges.Right)
                forchar->x = thisroom.Edges.Right - 3;
            forchar->loop=3;
        }
        else if (new_room_pos>=2000) {
            play.entered_edge = 1;
            forchar->x = thisroom.Edges.Right - get_fixed_pixel_size(1);
            forchar->y=new_room_pos%1000;
            if (forchar->y==0) forchar->y=thisroom.Height/2;
            if (forchar->y <= thisroom.Edges.Top)
                forchar->y = thisroom.Edges.Top + 3;
            if (forchar->y >= thisroom.Edges.Bottom)
                forchar->y = thisroom.Edges.Bottom - 3;
            forchar->loop=1;
        }
        else if (new_room_pos>=1000) {
            play.entered_edge = 0;
            forchar->x = thisroom.Edges.Left + get_fixed_pixel_size(1);
            forchar->y=new_room_pos%1000;
            if (forchar->y==0) forchar->y=thisroom.Height/2;
            if (forchar->y <= thisroom.Edges.Top)
                forchar->y = thisroom.Edges.Top + 3;
            if (forchar->y >= thisroom.Edges.Bottom)
                forchar->y = thisroom.Edges.Bottom - 3;
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
                    if (tryleft>thisroom.Edges.Left) { tryleft--; nowhere++; }
                    if (tryright<thisroom.Edges.Right) { tryright++; nowhere++; }
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
                    if (tryleft>thisroom.Edges.Top) { tryleft--; nowhere++; }
                    if (tryright<thisroom.Edges.Bottom) { tryright++; nowhere++; }
                    if (nowhere==0) break;  // no place to go, so leave him
                }
            }
        }
        new_room_pos=0;
    }
    if (forchar!=nullptr) {
        play.entered_at_x=forchar->x;
        play.entered_at_y=forchar->y;
        if (forchar->x >= thisroom.Edges.Right)
            play.entered_edge = 1;
        else if (forchar->x <= thisroom.Edges.Left)
            play.entered_edge = 0;
        else if (forchar->y >= thisroom.Edges.Bottom)
            play.entered_edge = 2;
        else if (forchar->y <= thisroom.Edges.Top)
            play.entered_edge = 3;
    }
    if (thisroom.Options.StartupMusic>0)
        PlayMusicResetQueue(thisroom.Options.StartupMusic);

    set_our_eip(208);
    if (forchar!=nullptr) {
        if (thisroom.Options.PlayerCharOff==0) { forchar->on=1;
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
        else if (thisroom.Options.PlayerView==0) forchar->view=forchar->defview;
        else forchar->view=thisroom.Options.PlayerView-1;
        forchar->frame=0;   // make him standing
    }
    color_map = nullptr;

    set_our_eip(209);
    generate_light_table();
    update_music_volume();

    // If we are not restoring a save, update cameras to accomodate for this
    // new room; otherwise this is done later when cameras are recreated.
    if (forchar != nullptr)
    {
        if (play.IsAutoRoomViewport())
            adjust_viewport_to_room();
        update_all_viewcams_with_newroom();
        play.UpdateRoomCameras(); // update auto tracking
    }
    init_room_drawdata();

    set_our_eip(212);
    invalidate_screen();
    for (uint32_t cc=0;cc<croom->numobj;cc++) {
        if (objs[cc].on == 2)
            MergeObject(cc);
    }
    new_room_flags=0;
    play.gscript_timer=-1;  // avoid screw-ups with changing screens
    play.player_on_region = 0;

    // trash any input which they might have done while it was loading
    ags_clear_input_buffer();
    // no fade in, so set the palette immediately in case of 256-col sprites
    if (game.color_depth > 1)
        setpal();

    set_our_eip(220);
    update_polled_stuff();
    debug_script_log("Now in room %d", displayed_room);
    GUI::MarkAllGUIForUpdate(true, true);
    pl_run_plugin_hooks(AGSE_ENTERROOM, displayed_room);
}

// new_room: changes the current room number, and loads the new room from disk
void new_room(int newnum,CharacterInfo*forchar) {
    EndSkippingUntilCharStops();

    debug_script_log("Room change requested to room %d", newnum);

    // we are currently running Leaves Screen scripts
    in_leaves_screen = newnum;

    // player leaves screen event
    run_room_event(EVROM_LEAVE);
    // Run the global OnRoomLeave event
    run_on_event (GE_LEAVE_ROOM, RuntimeScriptValue().SetInt32(displayed_room));

    pl_run_plugin_hooks(AGSE_LEAVEROOM, displayed_room);

    // update the new room number if it has been altered by OnLeave scripts
    newnum = in_leaves_screen;
    in_leaves_screen = -1;

    if ((playerchar->following >= 0) &&
        (game.chars[playerchar->following].room != newnum)) {
            // the player character is following another character,
            // who is not in the new room. therefore, abort the follow
            playerchar->following = -1;
    }

    // change rooms
    unload_old_room();

    if (usetup.clear_cache_on_room_change)
    {
        // Delete all cached resources
        spriteset.DisposeAllCached();
        soundcache_clear();
        texturecache_clear();
    }

    load_new_room(newnum,forchar);

    // Update background frame state (it's not a part of the RoomStatus currently)
    play.bg_frame = 0;
    play.bg_frame_locked = (thisroom.Options.Flags & kRoomFlag_BkgFrameLocked) != 0;
}

void set_room_placeholder()
{
    thisroom.InitDefaults();
    std::shared_ptr<Bitmap> dummy_bg(new Bitmap(1, 1, 8));
    thisroom.BgFrames[0].Graphic = dummy_bg;
    thisroom.HotspotMask = dummy_bg;
    thisroom.RegionMask = dummy_bg;
    thisroom.WalkAreaMask = dummy_bg;
    thisroom.WalkBehindMask = dummy_bg;

    reset_temp_room();
    croom = &troom;
}

int find_highest_room_entered() {
    int qq,fndas=-1;
    for (qq=0;qq<MAX_ROOMS;qq++) {
        if (isRoomStatusValid(qq) && (getRoomStatus(qq)->beenhere != 0))
            fndas = qq;
    }
    return fndas;
}

void first_room_initialization() {
    starting_room = displayed_room;
    playerchar->prevroom = -1;
    set_loop_counter(0);
    // Reset background frame state
    play.bg_frame = 0;
    play.bg_frame_locked = (thisroom.Options.Flags & kRoomFlag_BkgFrameLocked) != 0;
}

void check_new_room() {
    // if they're in a new room, run Player Enters Screen and on_event(ENTER_ROOM)
    if ((in_new_room>0) & (in_new_room!=3)) {
        EventHappened evh(EV_RUNEVBLOCK, EVB_ROOM, 0, EVROM_BEFOREFADEIN, game.playercharacter);
        // make sure that any script calls don't re-call enters screen
        int newroom_was = in_new_room;
        in_new_room = 0;
        play.disabled_user_interface ++;
        process_event(&evh);
        play.disabled_user_interface --;
        in_new_room = newroom_was;
    }
}

void compile_room_script() {
    cc_clear_error();

    roominst.reset(ccInstance::CreateFromScript(thisroom.CompiledScript));
    if ((cc_has_error()) || (roominst==nullptr)) {
        quitprintf("Unable to create local script:\n%s", cc_get_error().ErrorString.GetCStr());
    }

    if (!roominst->ResolveScriptImports(roominst->instanceof.get()))
        quitprintf("Unable to resolve imports in room script:\n%s", cc_get_error().ErrorString.GetCStr());

    if (!roominst->ResolveImportFixups(roominst->instanceof.get()))
        quitprintf("Unable to resolve import fixups in room script:\n%s", cc_get_error().ErrorString.GetCStr());

    roominstFork.reset(roominst->Fork());
    if (roominstFork == nullptr)
        quitprintf("Unable to create forked room instance:\n%s", cc_get_error().ErrorString.GetCStr());

    repExecAlways.roomHasFunction = true;
    lateRepExecAlways.roomHasFunction = true;
    getDialogOptionsDimensionsFunc.roomHasFunction = true;
}

int bg_just_changed = 0;

void on_background_frame_change () {

    invalidate_screen();
    mark_current_background_dirty();

    // get the new frame's palette
    memcpy (palette, thisroom.BgFrames[play.bg_frame].Palette, sizeof(RGB) * 256);

    // hi-colour, update the palette. It won't have an immediate effect
    // but will be drawn properly when the screen fades in
    if (game.color_depth > 1)
        setpal();

    if (in_enters_screen)
        return;

    // Don't update the palette if it hasn't changed
    if (thisroom.BgFrames[play.bg_frame].IsPaletteShared)
        return;

    // 256-colours, tell it to update the palette (will actually be done as
    // close as possible to the screen update to prevent flicker problem)
    if (game.color_depth == 1)
        bg_just_changed = 1;
}

void croom_ptr_clear()
{
    croom = nullptr;
    objs = nullptr;
}


// coordinate conversion (data) ---> game ---> (room mask)
int room_to_mask_coord(int coord)
{
    return coord * game.GetDataUpscaleMult() / thisroom.MaskResolution;
}

// coordinate conversion (room mask) ---> game ---> (data)
int mask_to_room_coord(int coord)
{
    return coord * thisroom.MaskResolution / game.GetDataUpscaleMult();
}

void convert_move_path_to_room_resolution(MoveList *ml, int from_step, int to_step)
{
    if (to_step < 0)
        to_step = ml->numstage;
    to_step = Math::Clamp(to_step, 0, ml->numstage - 1);
    from_step = Math::Clamp(from_step, 0, to_step);

    // If speed is independent from MaskResolution...
    if ((game.options[OPT_WALKSPEEDABSOLUTE] != 0) && game.GetDataUpscaleMult() > 1)
    {
        for (int i = from_step; i <= to_step; i++)
        { // ...we still need to convert from game to data coords
            ml->xpermove[i] = game_to_data_coord(ml->xpermove[i]);
            ml->ypermove[i] = game_to_data_coord(ml->ypermove[i]);
        }
    }

    // Skip the conversion if these are equal, as they are multiplier and divisor
    if (thisroom.MaskResolution == game.GetDataUpscaleMult())
        return;

    if (from_step == 0)
    {
        ml->from = { mask_to_room_coord(ml->from.X), mask_to_room_coord(ml->from.Y) };
    }

    for (int i = from_step; i <= to_step; i++)
    {
        ml->pos[i] = { mask_to_room_coord(ml->pos[i].X), mask_to_room_coord(ml->pos[i].Y) };
    }

    // If speed is scaling with MaskResolution...
    if (game.options[OPT_WALKSPEEDABSOLUTE] == 0)
    {
        for (int i = from_step; i <= to_step; i++)
        {
            ml->xpermove[i] = mask_to_room_coord(ml->xpermove[i]);
            ml->ypermove[i] = mask_to_room_coord(ml->ypermove[i]);
        }
    }
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

// ScriptDrawingSurface* (int backgroundNumber)
RuntimeScriptValue Sc_Room_GetDrawingSurfaceForBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDrawingSurface, Room_GetDrawingSurfaceForBackground);
}

// int (const char *property)
RuntimeScriptValue Sc_Room_GetProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Room_GetProperty, const char);
}

// const char* (const char *property)
RuntimeScriptValue Sc_Room_GetTextProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, Room_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Room_SetProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ_PINT(Room_SetProperty, const char);
}

// const char* (const char *property)
RuntimeScriptValue Sc_Room_SetTextProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ2(Room_SetTextProperty, const char, const char);
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

// void (int xx,int yy,int mood)
RuntimeScriptValue Sc_RoomProcessClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(RoomProcessClick);
}

RuntimeScriptValue Sc_Room_Exists(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_PINT(Room_Exists);
}

void RegisterRoomAPI()
{
    ScFnRegister room_api[] = {
        { "Room::GetDrawingSurfaceForBackground^1",   API_FN_PAIR(Room_GetDrawingSurfaceForBackground) },
        { "Room::GetProperty^1",                      API_FN_PAIR(Room_GetProperty) },
        { "Room::GetTextProperty^1",                  API_FN_PAIR(Room_GetTextProperty) },
        { "Room::SetProperty^2",                      API_FN_PAIR(Room_SetProperty) },
        { "Room::SetTextProperty^2",                  API_FN_PAIR(Room_SetTextProperty) },
        { "Room::ProcessClick^3",                     API_FN_PAIR(RoomProcessClick) },
        { "ProcessClick",                             API_FN_PAIR(RoomProcessClick) },
        { "Room::get_BottomEdge",                     API_FN_PAIR(Room_GetBottomEdge) },
        { "Room::get_ColorDepth",                     API_FN_PAIR(Room_GetColorDepth) },
        { "Room::get_Height",                         API_FN_PAIR(Room_GetHeight) },
        { "Room::get_LeftEdge",                       API_FN_PAIR(Room_GetLeftEdge) },
        { "Room::geti_Messages",                      API_FN_PAIR(Room_GetMessages) },
        { "Room::get_MusicOnLoad",                    API_FN_PAIR(Room_GetMusicOnLoad) },
        { "Room::get_ObjectCount",                    API_FN_PAIR(Room_GetObjectCount) },
        { "Room::get_RightEdge",                      API_FN_PAIR(Room_GetRightEdge) },
        { "Room::get_TopEdge",                        API_FN_PAIR(Room_GetTopEdge) },
        { "Room::get_Width",                          API_FN_PAIR(Room_GetWidth) },
        { "Room::Exists",                             API_FN_PAIR(Room_Exists) },
    };

    ccAddExternalFunctions(room_api);
}
