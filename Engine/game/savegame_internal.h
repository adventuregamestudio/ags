//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_GAME__SAVEGAMEINTERNAL_H
#define __AGS_EE_GAME__SAVEGAMEINTERNAL_H

#include <memory>
#include <vector>
#include <unordered_map>
#include "ac/common_defines.h"
#include "game/roomstruct.h"
#include "gfx/bitmap.h"
#include "media/audio/audiodefines.h"


namespace AGS
{
namespace Engine
{

using AGS::Common::Bitmap;

typedef std::shared_ptr<Bitmap> PBitmap;

// PreservedParams keeps old values of particular gameplay
// parameters that are saved before the save restoration
// and either applied or compared to new values after
// loading save data
struct PreservedParams
{
    SavegameDescription Desc;

    // Whether speech and audio packages available
    bool SpeechVOX = false;
    bool MusicVOX = false;
    // Game options, to preserve ones that must not change at runtime
    int GameOptions[GameSetupStructBase::MAX_OPTIONS]{};
    // Script global data sizes
    size_t GlScDataSize = 0u;
    std::vector<String> ScriptModuleNames;
    std::vector<size_t> ScMdDataSize;

    PreservedParams() = default;
    PreservedParams(const SavegameDescription &desc)
        : Desc(desc) {}
};

// Flags for the image saved in header (usually a game screenshot)
enum SaveImageFlags
{
    kSvgImage_Present = 0x0001,
    kSvgImage_Deflate = 0x0002, // apply compression
};

// Audio playback state flags, used only in serialization
enum AudioSvgPlaybackFlags
{
    kSvgAudioPaused = 0x01
};

enum GameViewCamFlags
{
    kSvgGameAutoRoomView = 0x01
};

enum CameraSaveFlags
{
    kSvgCamPosLocked = 0x01
};

enum ViewportSaveFlags
{
    kSvgViewportVisible = 0x01
};

// SaveRestorationFlags mark the specifics of a restored save
enum SaveRestorationFlags
{
    kSaveRestore_Default           = 0,
    // Game data has been cleared to initial state prior to reading a save.
    // This indicates that if any game entities do not have a match in the
    // restored save, then they will be presented in their default state
    // (which they have when the game starts).
    kSaveRestore_ClearData          = 0x0001,
    // Allow save entries mismatching game contents:
    // - more entries, less entries, etc
    kSaveRestore_AllowMismatchExtra = 0x0002,
    kSaveRestore_AllowMismatchLess  = 0x0004,
    // We detected that the save file has less data of certain type
    // than the game requires.
    kSaveRestore_MissingDataInSave  = 0x0008,
    // We detected that the save file has more data of certain type
    // than the game requires.
    kSaveRestore_ExtraDataInSave    = 0x0010,
    // Run a save scan only, gather data counts and assert matches
    kSaveRestore_Prescan            = 0x0020,
    // Mask for finding out if save has any mismatches
    kSaveRestore_MismatchMask       = kSaveRestore_MissingDataInSave
                                    | kSaveRestore_ExtraDataInSave,
    // Mask for the restoration result flags
    kSaveRestore_ResultMask         = kSaveRestore_ClearData
                                    | kSaveRestore_MissingDataInSave
                                    | kSaveRestore_ExtraDataInSave
                                    | kSaveRestore_Prescan
};

// SaveRestoreResult records allowances for the save restoration
// and the general result of the restoration process
struct SaveRestoreResult
{
    SaveRestorationFlags RestoreFlags = kSaveRestore_Default;
    // Recorded first data mismatch error, this will be reported if
    // save validation fails after restoring full save
    // NOTE: may expand this to a vector if desired to record ALL mismatches.
    String FirstMismatchError;
    SaveRestoreFeedback Feedback;
};

// SaveRestoredDataCounts contains numbers of different types of data
// found in the save file. This information may be passed to the game script,
// letting it detect whether save is applicable to the current game version.
struct SaveRestoredDataCounts
{
    uint32_t Dummy = 0u; // a dummy integer, used to record something that we are not interested in
    uint32_t AudioClipTypes = 0u;
    uint32_t Characters = 0u;
    uint32_t Dialogs = 0u;
    uint32_t GUIs = 0u;
    std::vector<uint32_t> GUIControls;
    uint32_t InventoryItems = 0u;
    uint32_t Cursors = 0u;
    uint32_t Views = 0u;
    std::vector<uint32_t> ViewLoops;
    std::vector<uint32_t> ViewFrames;
    uint32_t GlobalScriptDataSz = 0u;
    uint32_t ScriptModules = 0u;
    std::vector<String> ScriptModuleNames;
    std::vector<uint32_t> ScriptModuleDataSz;
    int Room = -1; // the room this save was made in
    AGS::Common::StringMap GameInfo; // game info from the game that made this save
};

// RestoredData keeps certain temporary data to help with
// the restoration process
struct RestoredData
{
    SaveRestoreResult       Result;
    SaveRestoredDataCounts  DataCounts;

    int                     FPS;
    // Unserialized bitmaps for dynamic surfaces
    std::vector<std::unique_ptr<Bitmap>> DynamicSurfaces;
    // Unserialized bitmaps for overlays (old-style saves)
    std::unordered_map<int, std::unique_ptr<Bitmap>> OverlayImages;
    // Scripts global data
    struct ScriptData
    {
        std::vector<uint8_t> Data;
    };
    ScriptData              GlobalScript;
    std::unordered_map<String, ScriptData> ScriptModules;
    // Game state data (loaded ahead)
    uint32_t                DoOnceCount;
    // Room data (has to be be preserved until room is loaded)
    int                     Room;
    std::unique_ptr<Bitmap> RoomBkgScene[MAX_ROOM_BGFRAMES];
    std::unique_ptr<Bitmap> RoomMask[kNumRoomAreaTypes];
    short                   RoomLightLevels[MAX_ROOM_REGIONS];
    int                     RoomTintLevels[MAX_ROOM_REGIONS];
    short                   RoomZoomLevels1[MAX_WALK_AREAS];
    short                   RoomZoomLevels2[MAX_WALK_AREAS];
    RoomVolumeMod           RoomVolume;
    // Mouse cursor parameters
    int                     CursorID;
    int                     CursorMode;
    // General audio
    struct ChannelInfo
    {
        int ClipID = -1;
        int Flags = 0;
        int Pos = 0;
        int Priority = 0;
        int Repeat = 0;
        int Vol = 0;
        int VolAsPercent = 0;
        int Pan = 0;
        int Speed = 0;
        // since version 1
        int XSource = -1;
        int YSource = -1;
        int MaxDist = 0;
    };
    ChannelInfo             AudioChans[TOTAL_AUDIO_CHANNELS];
    // Ambient sounds
    int                     DoAmbient[MAX_GAME_CHANNELS];
    // Viewport and camera data, has to be preserved and applied only after
    // room gets loaded, because we must clamp these to room parameters.
    struct ViewportData
    {
        int ID = -1;
        int Flags = 0;
        int Left = 0;
        int Top = 0;
        int Width = 0;
        int Height = 0;
        int ZOrder = 0;
        int CamID = -1;
    };
    struct CameraData
    {
        int ID = -1;
        int Flags = 0;
        int Left = 0;
        int Top = 0;
        int Width = 0;
        int Height = 0;
    };
    std::vector<ViewportData> Viewports;
    std::vector<CameraData> Cameras;
    bool LegacyViewCamera = false;
    int32_t Camera0_Flags = 0; // flags for primary camera, when data is read in legacy order

    RestoredData();
};


enum PluginSvgVersion
{
    kPluginSvgVersion_Initial = 0,
    kPluginSvgVersion_36115   = 1,
};

Bitmap *ReadBitmap(Stream *in, bool compressed = false);
void SkipBitmap(Stream *in, bool compressed = false);
void WriteBitmap(const Bitmap *thispic, Stream *out, bool compressed = false);
// Runs plugin events, requesting to read save data from the given stream.
// NOTE: there's no error check in this function, because plugin API currently
// does not let plugins report any errors when restoring their saved data.
HSaveError ReadPluginSaveData(Stream *in, PluginSvgVersion svg_ver, soff_t max_size);
// Runs plugin events, requesting to write save data into the given stream.
HSaveError WritePluginSaveData(Stream *out);

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAMEINTERNAL_H
