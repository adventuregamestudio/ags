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

#include "ac/character.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/gamesetup.h"
#include "ac/global_audio.h"
#include "ac/global_character.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/region.h"
#include "ac/richgamemedia.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/system.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "game/savegame.h"
#include "game/savegame_internal.h"
#include "main/main.h"
#include "main/version.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "script/script.h"
#include "script/cc_error.h"
#include "util/alignedstream.h"
#include "util/file.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace Common;
using namespace Engine;

// function is currently implemented in game.cpp
SavegameError restore_game_data(Stream *in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data);
extern GameSetupStruct game;
extern Bitmap **guibg;
extern AGS::Engine::IDriverDependantBitmap **guibgbmp;
extern AGS::Engine::IGraphicsDriver *gfxDriver;
extern Bitmap *dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern Bitmap *raw_saved_screen;
extern RoomStatus troom;


namespace AGS
{
namespace Engine
{

const String SavegameSource::Signature = "Adventure Game Studio saved game";


PreservedParams::PreservedParams()
    : SpeechVOX(0)
    , MusicVOX(0)
{
}

RestoredData::ScriptData::ScriptData()
    : Len(0)
{
}

RestoredData::RestoredData()
    : FPS(0)
    , RoomVolume(0)
    , CursorID(0)
    , CursorMode(0)
{
    memset(RoomBkgScene, 0, sizeof(RoomBkgScene));
    memset(RoomLightLevels, 0, sizeof(RoomLightLevels));
    memset(RoomTintLevels, 0, sizeof(RoomTintLevels));
    memset(RoomZoomLevels1, 0, sizeof(RoomZoomLevels1));
    memset(RoomZoomLevels2, 0, sizeof(RoomZoomLevels2));
    memset(DoAmbient, 0, sizeof(DoAmbient));
}

String GetSavegameErrorText(SavegameError err)
{
    switch (err)
    {
    case kSvgErr_NoError:
        return "No error";
    case kSvgErr_FileNotFound:
        return "File not found";
    case kSvgErr_SignatureFailed:
        return "Not an AGS saved game or unsupported format";
    case kSvgErr_FormatVersionNotSupported:
        return "Save format version not supported";
    case kSvgErr_IncompatibleEngine:
        return "Save was written by incompatible engine, or file is corrupted";
    case kSvgErr_InconsistentFormat:
        return "Inconsistent format, or file is corrupted";
    case kSvgErr_GameContentAssertion:
        return "Saved content does not match current game";
    case kSvgErr_InconsistentPlugin:
        return "One of the game plugins did not restore its game data correctly.";
    case kSvgErr_DifferentColorDepth:
        return "Saved with different colour depth";
    case kSvgErr_GameObjectInitFailed:
        return "Game object initialization failed after save restoration";
    }
    return "Unknown error";
}

Bitmap *RestoreSaveImage(Stream *in)
{
    if (in->ReadInt32())
        return read_serialized_bitmap(in);
    return NULL;
}

void SkipSaveImage(Stream *in)
{
    if (in->ReadInt32())
        skip_serialized_bitmap(in);
}

SavegameError OpenSavegameBase(const String &filename, SavegameSource *src, SavegameDescription *desc, SavegameDescElem elems)
{
    AStream in(File::OpenFileRead(filename));
    if (!in.get())
        return kSvgErr_FileNotFound;

    // Skip MS Windows Vista rich media header
    RICH_GAME_MEDIA_HEADER rich_media_header;
    rich_media_header.ReadFromFile(in.get());

    // Check saved game signature
    String svg_sig = String::FromStreamCount(in.get(), SavegameSource::Signature.GetLength());
    if (svg_sig.Compare(SavegameSource::Signature))
        return kSvgErr_SignatureFailed;

    String desc_text;
    if (desc && elems == kSvgDesc_UserText)
        desc_text.Read(in.get());
    else
        for (; in->ReadByte(); ); // skip until null terminator
    SavegameVersion svg_ver = (SavegameVersion)in->ReadInt32();

    // Check saved game format version
    if (svg_ver < kSvgVersion_LowestSupported ||
        svg_ver > kSvgVersion_Current)
    {
        return kSvgErr_FormatVersionNotSupported;
    }

    ABitmap image;
    if (desc && elems == kSvgDesc_UserImage)
        image.reset(RestoreSaveImage(in.get()));
    else
        SkipSaveImage(in.get());

    String version_str = String::FromStream(in.get());
    Version eng_version(version_str);
    if (eng_version > EngineVersion ||
        eng_version < SavedgameLowestBackwardCompatVersion)
    {
        // Engine version is either non-forward or non-backward compatible
        return kSvgErr_IncompatibleEngine;
    }
    String main_file;
    if (desc && elems == kSvgDesc_EnvInfo)
        main_file.Read(in.get());
    else
        for (; in->ReadByte(); ); // skip until null terminator

    if (src)
    {
        src->Filename = filename;
        src->Version = svg_ver;
        src->InputStream.reset(in.release());
    }
    if (desc)
    {
        if (elems == kSvgDesc_EnvInfo)
        {
            desc->EngineVersion = eng_version;
            desc->MainDataFilename = main_file;
        }
        if (elems == kSvgDesc_UserText)
            desc->UserText = desc_text;
        if (elems == kSvgDesc_UserImage)
            desc->UserImage.reset(image.release());
    }
    return kSvgErr_NoError;
}

SavegameError OpenSavegame(const String &filename, SavegameSource &src, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, &src, &desc, elems);
}

SavegameError OpenSavegame(const String &filename, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, NULL, &desc, elems);
}

// Prepares engine for actual save restore (stops processes, cleans up memory)
void DoBeforeRestore(PreservedParams &pp)
{
    pp.SpeechVOX = play.want_speech;
    pp.MusicVOX = play.separate_music_lib;

    unload_old_room();
    delete raw_saved_screen;
    raw_saved_screen = NULL;
    remove_screen_overlay(-1);
    is_complete_overlay = 0;
    is_text_overlay = 0;

    // cleanup dynamic sprites
    for (int i = 1; i < spriteset.elements; ++i)
    {
        if (game.spriteflags[i] & SPF_DYNAMICALLOC)
        {
            // do this early, so that it changing guibuts doesn't
            // affect the restored data
            free_dynamic_sprite(i);
        }
    }

    // cleanup GUI backgrounds
    for (int i = 0; i < game.numgui; ++i)
    {
        delete guibg[i];
        guibg[i] = NULL;

        if (guibgbmp[i])
            gfxDriver->DestroyDDB(guibgbmp[i]);
        guibgbmp[i] = NULL;
    }

    // preserve script data sizes and cleanup scripts
    pp.GlScDataSize = gameinst->globaldatasize;
    delete gameinstFork;
    delete gameinst;
    gameinstFork = NULL;
    gameinst = NULL;
    pp.ScMdDataSize.resize(numScriptModules);
    for (int i = 0; i < numScriptModules; ++i)
    {
        pp.ScMdDataSize[i] = moduleInst[i]->globaldatasize;
        delete moduleInstFork[i];
        delete moduleInst[i];
        moduleInst[i] = NULL;
    }

    play.FreeProperties();

    delete roominstFork;
    delete roominst;
    roominstFork = NULL;
    roominst = NULL;

    delete dialogScriptsInst;
    dialogScriptsInst = NULL;

    resetRoomStatuses();
    troom.FreeScriptData();
    troom.FreeProperties();
    free_do_once_tokens();

    // unregister gui controls from API exports
    for (int i = 0; i < game.numgui; ++i)
    {
        unexport_gui_controls(i);
    }

    for (int i = 0; i <= MAX_SOUND_CHANNELS; ++i)
    {
        stop_and_destroy_channel_ex(i, false);
    }

    clear_music_cache();
}

// Final processing after successfully restoring from save
SavegameError DoAfterRestore(const PreservedParams &pp, const RestoredData &r_data)
{
    // Use a yellow dialog highlight for older game versions
    // CHECKME: it is dubious that this should be right here
    if(loaded_game_file_version < kGameVersion_331)
        play.dialog_options_highlight_color = DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT;

    // Preserve whether the music vox is available
    play.separate_music_lib = pp.MusicVOX;
    // If they had the vox when they saved it, but they don't now
    if ((pp.SpeechVOX < 0) && (play.want_speech >= 0))
        play.want_speech = (-play.want_speech) - 1;
    // If they didn't have the vox before, but now they do
    else if ((pp.SpeechVOX >= 0) && (play.want_speech < 0))
        play.want_speech = (-play.want_speech) - 1;

    // recache queued clips
    for (int i = 0; i < play.new_music_queue_size; ++i)
    {
        play.new_music_queue[i].cachedClip = NULL;
    }

    // restore these to the ones retrieved from the save game
    const size_t dynsurf_num = Math::Min((size_t)MAX_DYNAMIC_SURFACES, r_data.DynamicSurfaces.size());
    for (size_t i = 0; i < dynsurf_num; ++i)
    {
        dynamicallyCreatedSurfaces[i] = r_data.DynamicSurfaces[i];
    }

    for (int i = 0; i < game.numgui; ++i)
        export_gui_controls(i);
    update_gui_zorder();

    if (create_global_script())
    {
        Out::FPrint("Restore game error: unable to recreate global script: %s", ccErrorString);
        return kSvgErr_GameObjectInitFailed;
    }

    // read the global data into the newly created script
    if (r_data.GlobalScript.Data.get())
        memcpy(gameinst->globaldata, r_data.GlobalScript.Data.get(),
                Math::Min((size_t)gameinst->globaldatasize, r_data.GlobalScript.Len));

    // restore the script module data
    for (int i = 0; i < numScriptModules; ++i)
    {
        if (r_data.ScriptModules[i].Data.get())
            memcpy(moduleInst[i]->globaldata, r_data.ScriptModules[i].Data.get(),
                    Math::Min((size_t)moduleInst[i]->globaldatasize, r_data.ScriptModules[i].Len));
    }

    setup_player_character(game.playercharacter);

    // Save some parameters to restore them after room load
    int gstimer=play.gscript_timer;
    int oldx1 = play.mboundx1, oldx2 = play.mboundx2;
    int oldy1 = play.mboundy1, oldy2 = play.mboundy2;

    // disable the queue momentarily
    int queuedMusicSize = play.music_queue_size;
    play.music_queue_size = 0;

    update_polled_stuff_if_runtime();

    // load the room the game was saved in
    if (displayed_room >= 0)
        load_new_room(displayed_room, NULL);

    update_polled_stuff_if_runtime();

    play.gscript_timer=gstimer;
    // restore the correct room volume (they might have modified
    // it with SetMusicVolume)
    thisroom.options[ST_VOLUME] = r_data.RoomVolume;

    Mouse::SetMoveLimit(Rect(oldx1, oldy1, oldx2, oldy2));

    set_cursor_mode(r_data.CursorMode);
    set_mouse_cursor(r_data.CursorID);
    if (r_data.CursorMode == MODE_USE)
        SetActiveInventory(playerchar->activeinv);
    // ensure that the current cursor is locked
    spriteset.precache(game.mcurs[r_data.CursorID].pic);

#if (ALLEGRO_DATE > 19990103)
    set_window_title(play.game_name);
#endif

    update_polled_stuff_if_runtime();

    if (displayed_room >= 0)
    {
        for (int i = 0; i < MAX_BSCENE; ++i)
        {
            if (r_data.RoomBkgScene[i])
            {
                delete thisroom.ebscene[i];
                thisroom.ebscene[i] = r_data.RoomBkgScene[i];
            }
        }

        in_new_room=3;  // don't run "enters screen" events
        // now that room has loaded, copy saved light levels in
        memcpy(thisroom.regionLightLevel, r_data.RoomLightLevels, sizeof(short) * MAX_REGIONS);
        memcpy(thisroom.regionTintLevel, r_data.RoomTintLevels, sizeof(int) * MAX_REGIONS);
        generate_light_table();

        memcpy(thisroom.walk_area_zoom, r_data.RoomZoomLevels1, sizeof(short) * (MAX_WALK_AREAS + 1));
        memcpy(thisroom.walk_area_zoom2, r_data.RoomZoomLevels2, sizeof(short) * (MAX_WALK_AREAS + 1));

        on_background_frame_change();
    }

    gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);

    // restore the queue now that the music is playing
    play.music_queue_size = queuedMusicSize;

    if (play.digital_master_volume >= 0)
        System_SetVolume(play.digital_master_volume);

    // Run audio clips on channels
    // these two crossfading parameters have to be temporarily reset
    const int cf_in_chan = play.crossfading_in_channel;
    const int cf_out_chan = play.crossfading_out_channel;
    play.crossfading_in_channel = 0;
    play.crossfading_out_channel = 0;
    for (int i = 0; i <= MAX_SOUND_CHANNELS; ++i)
    {
        const RestoredData::ChannelInfo &chan_info = r_data.AudioChans[i];
        if (chan_info.ClipID < 0)
            continue;
        if (chan_info.ClipID >= game.audioClipCount)
        {
            Out::FPrint("Restore game error: invalid audio clip index: %d (clip count: %d)", chan_info.ClipID, game.audioClipCount);
            return kSvgErr_GameObjectInitFailed;
        }
        play_audio_clip_on_channel(i, &game.audioClips[chan_info.ClipID],
            chan_info.Priority, chan_info.Repeat, chan_info.Pos);
        if (channels[i] != NULL)
        {
            channels[i]->set_volume_alternate(chan_info.VolAsPercent, chan_info.Vol);
            channels[i]->set_speed(chan_info.Speed);
            channels[i]->set_panning(chan_info.Pan);
            channels[i]->panningAsPercentage = chan_info.PanAsPercent;
        }
    }
    if ((cf_in_chan > 0) && (channels[cf_in_chan] != NULL))
        play.crossfading_in_channel = cf_in_chan;
    if ((cf_out_chan > 0) && (channels[cf_out_chan] != NULL))
        play.crossfading_out_channel = cf_out_chan;

    // If there were synced audio tracks, the time taken to load in the
    // different channels will have thrown them out of sync, so re-time it
    for (int i = 0; i <= MAX_SOUND_CHANNELS; ++i)
    {
        int pos = r_data.AudioChans[i].Pos;
        if ((pos > 0) && (channels[i] != NULL) && (channels[i]->done == 0))
        {
            channels[i]->seek(pos);
        }
    }

    for (int i = 1; i < MAX_SOUND_CHANNELS; ++i)
    {
        if (r_data.DoAmbient[i])
            PlayAmbientSound(i, r_data.DoAmbient[i], ambient[i].vol, ambient[i].x, ambient[i].y);
    }

    for (int i = 0; i < game.numgui; ++i)
    {
        guibg[i] = BitmapHelper::CreateBitmap(guis[i].Width, guis[i].Height, ScreenResolution.ColorDepth);
        guibg[i] = ReplaceBitmapWithSupportedFormat(guibg[i]);
    }

    recreate_overlay_ddbs();

    if (gfxDriver->SupportsGammaControl())
        gfxDriver->SetGamma(play.gamma_adjustment);

    guis_need_update = 1;

    play.ignore_user_input_until_time = 0;
    update_polled_stuff_if_runtime();

    platform->RunPluginHooks(AGSE_POSTRESTOREGAME, 0);

    if (displayed_room < 0)
    {
        // the restart point, no room was loaded
        load_new_room(playerchar->room, playerchar);
        playerchar->prevroom = -1;

        first_room_initialization();
    }

    if ((play.music_queue_size > 0) && (cachedQueuedMusic == NULL))
    {
        cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
    }

    // test if the playing music was properly loaded
    if (current_music_type > 0)
    {
        if (crossFading > 0 && !channels[crossFading] ||
            crossFading <= 0 && !channels[SCHAN_MUSIC])
        {
            current_music_type = 0;
        }
    }

    set_game_speed(r_data.FPS);

    return kSvgErr_NoError;
}

SavegameError RestoreGameState(Stream *in, SavegameVersion svg_version)
{
    PreservedParams pp;
    RestoredData r_data;
    DoBeforeRestore(pp);
    SavegameError err = restore_game_data(in, svg_version, pp, r_data);
    if (err != kSvgErr_NoError)
        return err;
    return DoAfterRestore(pp, r_data);
}

void WriteSaveImage(Stream *out, const Bitmap *screenshot)
{
    // store the screenshot at the start to make it easily accesible
    out->WriteInt32((screenshot == NULL) ? 0 : 1);

    if (screenshot)
        serialize_bitmap(screenshot, out);
}

Stream *StartSavegame(const String &filename, const String &desc, const Bitmap *image)
{
    Stream *out = Common::File::CreateFile(filename);
    if (!out)
        return NULL;

    // Initialize and write Vista header
    RICH_GAME_MEDIA_HEADER vistaHeader;
    memset(&vistaHeader, 0, sizeof(RICH_GAME_MEDIA_HEADER));
    memcpy(&vistaHeader.dwMagicNumber, RM_MAGICNUMBER, sizeof(int));
    vistaHeader.dwHeaderVersion = 1;
    vistaHeader.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
    vistaHeader.dwThumbnailOffsetHigherDword = 0;
    vistaHeader.dwThumbnailOffsetLowerDword = 0;
    vistaHeader.dwThumbnailSize = 0;
    convert_guid_from_text_to_binary(game.guid, &vistaHeader.guidGameId[0]);
    uconvert(game.gamename, U_ASCII, (char*)&vistaHeader.szGameName[0], U_UNICODE, RM_MAXLENGTH);
    uconvert(desc, U_ASCII, (char*)&vistaHeader.szSaveName[0], U_UNICODE, RM_MAXLENGTH);
    vistaHeader.szLevelName[0] = 0;
    vistaHeader.szComments[0] = 0;

    // MS Windows Vista rich media header
    vistaHeader.WriteToFile(out);

    // Savegame signature
    out->Write(SavegameSource::Signature, SavegameSource::Signature.GetLength());
    // Description
    StrUtil::WriteCStr(desc, out);

    platform->RunPluginHooks(AGSE_PRESAVEGAME, 0);
    out->WriteInt32(kSvgVersion_Current);
    WriteSaveImage(out, image);

    // Write lowest forward-compatible version string, so that
    // earlier versions could load savedgames made by current engine
    String compat_version;
    if (SavedgameLowestForwardCompatVersion <= Version::LastOldFormatVersion)
        compat_version = SavedgameLowestForwardCompatVersion.BackwardCompatibleString;
    else
        compat_version = SavedgameLowestForwardCompatVersion.LongString;
    StrUtil::WriteCStr(compat_version, out);
    StrUtil::WriteCStr(usetup.main_data_filename, out);
    return out;
}

} // namespace Engine
} // namespace AGS
