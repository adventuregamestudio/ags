//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <vector>
#include "core/platform.h"
#include <allegro.h>
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif

#include "ac/common.h" // quit
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/file.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_audio.h"
#include "ac/global_walkablearea.h"
#include "ac/mouse.h"
#include "ac/parser.h"
#include "ac/path_helper.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/sys_events.h"
#include "ac/dynobj/cc_pluginobject.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/dynobj/scriptsystem.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "device/mousew32.h"
#include "font/fonts.h"
#include "game/roomstruct.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "gui/guimain.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/main.h"
#include "media/audio/audio_system.h"
#include "plugin/plugin_builtin.h"
#include "plugin/plugin_engine.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "util/library.h"
#include "util/wgt2allg.h"

// hide internal constants conflicting with plugin API
#undef OBJF_NOINTERACT
#undef OBJF_NOWALKBEHINDS

#include "plugin/agsplugin.h"


using namespace AGS::Common;
using namespace AGS::Engine;


#if defined(BUILTIN_PLUGINS)
#include "../Plugins/AGSflashlight/agsflashlight.h"
#include "../Plugins/agsblend/agsblend.h"
#include "../Plugins/ags_snowrain/ags_snowrain.h"
#include "../Plugins/ags_parallax/ags_parallax.h"
#include "../Plugins/agspalrender/agspalrender.h"
#include "../Plugins/AGSSpriteFont/AGSSpriteFont/AGSSpriteFont.h"
#if AGS_PLATFORM_OS_IOS
#include "../Plugins/agstouch/agstouch.h"
#endif // AGS_PLATFORM_OS_IOS
#endif // BUILTIN_PLUGINS


extern IGraphicsDriver *gfxDriver;
extern ScriptSystem scsystem;
extern int mousex, mousey;
extern GameSetupStruct game;
extern std::vector<ViewStruct> views;
extern RGB palette[256];
extern int displayed_room;
extern RoomStruct thisroom;
extern RoomStatus *croom;
extern ScriptObject scrObj[MAX_ROOM_OBJECTS];
extern RuntimeScriptValue GlobalReturnValue;

// **************** PLUGIN IMPLEMENTATION ****************


const int PLUGIN_API_VERSION = 26;
struct EnginePlugin
{
    EnginePlugin() {
        eiface.version = 0;
        eiface.pluginId = 0;
    }
    EnginePlugin(const EnginePlugin&) = delete;
    EnginePlugin(EnginePlugin&&) = default;

    AGS::Common::String  filename;
    AGS::Engine::Library library;
    bool        available = false;
    std::vector<uint8_t> savedata;
    int         wantHook = 0;
    int         invalidatedRegion = 0;
    void      (*engineStartup) (IAGSEngine *) = nullptr;
    void      (*engineShutdown) () = nullptr;
    int       (*onEvent) (int, int) = nullptr;
    void      (*initGfxHook) (const char *driverName, void *data) = nullptr;
    int       (*debugHook) (const char * whichscript, int lineNumber, int reserved) = nullptr;
    IAGSEngine  eiface; // CHECKME: why do we have a separate object per plugin?
    bool        builtin = false;
};

std::vector<EnginePlugin> plugins;
int pluginsWantingDebugHooks = 0;

//
// Managed object unserializers
//
std::vector<PluginObjectReader> pluginReaders;

std::vector<InbuiltPluginDetails> _registered_builtin_plugins;

void IAGSEngine::AbortGame (const char *reason) {
    quit(reason);
}
const char* IAGSEngine::GetEngineVersion () {
    return get_engine_version();
}
void IAGSEngine::RegisterScriptFunction (const char*name, void*addy) {
    ccAddExternalPluginFunction (name, addy);
}
const char* IAGSEngine::GetGraphicsDriverID()
{
    if (gfxDriver == nullptr)
        return nullptr;

    return gfxDriver->GetDriverID();
}

BITMAP *IAGSEngine::GetScreen () 
{
    // TODO: we could actually return stage buffer here, will that make a difference?
    if (!gfxDriver->UsesMemoryBackBuffer())
        quit("!This plugin requires software graphics driver.");

    Bitmap *buffer = gfxDriver->GetMemoryBackBuffer();
    return buffer ? (BITMAP*)buffer->GetAllegroBitmap() : nullptr;
}

BITMAP *IAGSEngine::GetVirtualScreen () 
{
    Bitmap *stage = gfxDriver->GetStageBackBuffer(true);
    return stage ? (BITMAP*)stage->GetAllegroBitmap() : nullptr;
}

void IAGSEngine::RequestEventHook (int32 event) {
    if (event >= AGSE_TOOHIGH) 
        quit("!IAGSEngine::RequestEventHook: invalid event requested");

    if (plugins[this->pluginId].onEvent == nullptr)
        quit("!IAGSEngine::RequestEventHook: no callback AGS_EngineOnEvent function exported from plugin");

    if ((event & AGSE_SCRIPTDEBUG) &&
        ((plugins[this->pluginId].wantHook & AGSE_SCRIPTDEBUG) == 0)) {
            pluginsWantingDebugHooks++;
            ccSetDebugHook(scriptDebugHook);
    }

    if (event & AGSE_AUDIODECODE) {
        quit("Plugin requested AUDIODECODE, which is no longer supported");
    }


    plugins[this->pluginId].wantHook |= event;
}

void IAGSEngine::UnrequestEventHook(int32 event) {
    if (event >= AGSE_TOOHIGH) 
        quit("!IAGSEngine::UnrequestEventHook: invalid event requested");

    if ((event & AGSE_SCRIPTDEBUG) &&
        (plugins[this->pluginId].wantHook & AGSE_SCRIPTDEBUG)) {
            pluginsWantingDebugHooks--;
            if (pluginsWantingDebugHooks < 1)
                ccSetDebugHook(nullptr);
    }

    plugins[this->pluginId].wantHook &= ~event;
}

int IAGSEngine::GetSavedData (char *buffer, int32 bufsize) {
    int savedatasize = plugins[this->pluginId].savedata.size();

    if (bufsize < savedatasize)
        quit("!IAGSEngine::GetSavedData: buffer too small");

    if (savedatasize > 0)
        memcpy(buffer, &plugins[this->pluginId].savedata.front(), savedatasize);

    return savedatasize;
}

void IAGSEngine::DrawText (int32 x, int32 y, int32 font, int32 color, char *text) 
{
    Bitmap *ds = gfxDriver->GetStageBackBuffer(true);
    if (!ds)
        return;
    color_t text_color = ds->GetCompatibleColor(color);
    draw_and_invalidate_text(ds, x, y, font, text_color, text);
}

void IAGSEngine::GetScreenDimensions (int32 *width, int32 *height, int32 *coldepth) {
    if (width != nullptr)
        width[0] = play.GetMainViewport().GetWidth();
    if (height != nullptr)
        height[0] = play.GetMainViewport().GetHeight();
    if (coldepth != nullptr)
        coldepth[0] = scsystem.coldepth;
}

unsigned char ** IAGSEngine::GetRawBitmapSurface (BITMAP *bmp)
{
    Bitmap *stage = gfxDriver->GetStageBackBuffer(true);
    if (stage && bmp == stage->GetAllegroBitmap())
        plugins[this->pluginId].invalidatedRegion = 0;

    return bmp->line;
}

void IAGSEngine::ReleaseBitmapSurface (BITMAP *bmp)
{
    Bitmap *stage = gfxDriver->GetStageBackBuffer(true);
    if (stage && bmp == stage->GetAllegroBitmap())
    {
        // plugin does not manaually invalidate stuff, so
        // we must invalidate the whole screen to be safe
        if (!plugins[this->pluginId].invalidatedRegion)
            invalidate_screen();
    }
}

void IAGSEngine::GetMousePosition (int32 *x, int32 *y) {
    if (x) x[0] = mousex;
    if (y) y[0] = mousey;
}
int IAGSEngine::GetCurrentRoom () {
    return displayed_room;
}
int IAGSEngine::GetNumBackgrounds () {
    return thisroom.BgFrameCount;
}
int IAGSEngine::GetCurrentBackground () {
    return play.bg_frame;
}
BITMAP *IAGSEngine::GetBackgroundScene (int32 index) {
    return (BITMAP*)thisroom.BgFrames[index].Graphic->GetAllegroBitmap();
}
void IAGSEngine::GetBitmapDimensions (BITMAP *bmp, int32 *width, int32 *height, int32 *coldepth) {
    if (bmp == nullptr)
        return;

    if (width != nullptr)
        width[0] = bmp->w;
    if (height != nullptr)
        height[0] = bmp->h;
    if (coldepth != nullptr)
        coldepth[0] = bitmap_color_depth(bmp);
}

// On save/restore, the Engine will provide the plugin with a handle. Because we only ever save to one file at a time,
// we can reuse the same handle.

static long pl_file_handle = -1;
static Stream *pl_file_stream = nullptr;

void pl_set_file_handle(long data, Stream *stream) {
    pl_file_handle = data;
    pl_file_stream = stream;
}

void pl_clear_file_handle() {
    pl_file_handle = -1;
    pl_file_stream = nullptr;
}

int IAGSEngine::FRead (void *buffer, int32 len, int32 handle) {
    if (handle != pl_file_handle) {
        quitprintf("IAGSEngine::FRead: invalid file handle: %d", handle);
    }
    if (!pl_file_stream) {
        quit("IAGSEngine::FRead: file stream not set");
    }
    return pl_file_stream->Read(buffer, len);
}

int IAGSEngine::FWrite (void *buffer, int32 len, int32 handle) {
    if (handle != pl_file_handle) {
        quitprintf("IAGSEngine::FWrite: invalid file handle: %d", handle);
    }
    if (!pl_file_stream) {
        quit("IAGSEngine::FWrite: file stream not set");
    }
    return pl_file_stream->Write(buffer, len);
}

void IAGSEngine::DrawTextWrapped (int32 xx, int32 yy, int32 wid, int32 font, int32 color, const char*text)
{
    // TODO: use generic function from the engine instead of having copy&pasted code here
    const int linespacing = get_font_linespacing(font);

    if (break_up_text_into_lines(text, Lines, wid, font) == 0)
        return;

    Bitmap *ds = gfxDriver->GetStageBackBuffer(true);
    if (!ds)
        return;
    color_t text_color = ds->GetCompatibleColor(color);
    for (size_t i = 0; i < Lines.Count(); i++)
        draw_and_invalidate_text(ds, xx, yy + linespacing*i, font, text_color, Lines[i].GetCStr());
}

Bitmap glVirtualScreenWrap;
void IAGSEngine::SetVirtualScreen (BITMAP *bmp)
{
    if (!gfxDriver->UsesMemoryBackBuffer())
    {
        debug_script_warn("SetVirtualScreen: this plugin requires software graphics driver to work correctly.");
        return;
    }

    if (bmp)
    {
        glVirtualScreenWrap.WrapAllegroBitmap(bmp, true);
        gfxDriver->SetStageBackBuffer(&glVirtualScreenWrap);
    }
    else
    {
        glVirtualScreenWrap.Destroy();
        gfxDriver->SetStageBackBuffer(nullptr);
    }
}

int IAGSEngine::LookupParserWord (const char *word) {
    return find_word_in_dictionary(word);
}

void IAGSEngine::BlitBitmap (int32 x, int32 y, BITMAP *bmp, int32 masked)
{
    Bitmap *ds = gfxDriver->GetStageBackBuffer(true);
    if (!ds)
        return;
    wputblock_raw(ds, x, y, bmp, masked);
    invalidate_rect(x, y, x + bmp->w, y + bmp->h, false);
}

void IAGSEngine::BlitSpriteTranslucent(int32 x, int32 y, BITMAP *bmp, int32 trans)
{
    Bitmap *ds = gfxDriver->GetStageBackBuffer(true);
    if (!ds)
        return;
    Bitmap wrap(bmp, true);
    if (gfxDriver->UsesMemoryBackBuffer())
        GfxUtil::DrawSpriteWithTransparency(ds, &wrap, x, y, trans);
    else
        GfxUtil::DrawSpriteBlend(ds, Point(x,y), &wrap, kBlend_Normal, trans);
}

void IAGSEngine::BlitSpriteRotated(int32 x, int32 y, BITMAP *bmp, int32 angle)
{
    Bitmap *ds = gfxDriver->GetStageBackBuffer(true);
    if (!ds)
        return;
    // FIXME: call corresponding Graphics Blit
    rotate_sprite(ds->GetAllegroBitmap(), bmp, x, y, itofix(angle));
}

void IAGSEngine::PollSystem () {
    update_polled_stuff();
    ags_domouse();
    // Handle all the buffered input events
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        eAGSMouseButton mbut;
        KeyInput ki;
        if (type == kInputKeyboard)
        {
            if (run_service_key_controls(ki) && !play.IsIgnoringInput())
                pl_run_plugin_hooks(AGSE_KEYPRESS, ki.Key);
        }
        else if (type == kInputMouse)
        {
            if (run_service_mb_controls(mbut) && !play.IsIgnoringInput())
                pl_run_plugin_hooks(AGSE_MOUSECLICK, mbut);
        }
    }
    
}
AGSCharacter* IAGSEngine::GetCharacter (int32 charnum) {
    if (charnum >= game.numcharacters)
        quit("!AGSEngine::GetCharacter: invalid character request");
    // IMPORTANT: if we change the script functions object argument type, we must change this return value too!
    return (AGSCharacter*)&game.chars[charnum];
}
AGSGameOptions* IAGSEngine::GetGameOptions () {
    // We should not return anything, because script functions do not require passing actual instance of GameState
    quit("!AGSEngine::GetGameOptions() is no longer supported. Use related script API instead.");
    return NULL;
}
AGSColor* IAGSEngine::GetPalette () {
    return (AGSColor*)&palette[0];
}
void IAGSEngine::SetPalette (int32 start, int32 finish, AGSColor *cpl) {
    set_palette_range((RGB*)cpl, start, finish, 0);
}
int IAGSEngine::GetNumCharacters () {
    return game.numcharacters;
}
int IAGSEngine::GetPlayerCharacter () {
    return game.playercharacter;
}
void IAGSEngine::RoomToViewport (int32 *x, int32 *y) {
    Point scrp = play.RoomToScreen(x ? *x : 0, y ? *y : 0);
    if (x)
        *x = scrp.X;
    if (y)
        *y = scrp.Y;
}
void IAGSEngine::ViewportToRoom (int32 *x, int32 *y) {
    // NOTE: This is an old function that did not account for custom/multiple viewports
    // and does not expect to fail, therefore we always use primary viewport here.
    // (Not sure if it's good though)
    VpPoint vpt = play.ScreenToRoom(x ? *x : 0, y ? *y : 0);
    if (x)
        *x = vpt.first.X;
    if (y)
        *y = vpt.first.Y;
}
int IAGSEngine::GetNumObjects () {
    return croom->numobj;
}
AGSObject *IAGSEngine::GetObject (int32 num) {
    if (num < 0 || static_cast<uint32_t>(num) >= croom->numobj)
        quit("!IAGSEngine::GetObject: invalid object");
    // IMPORTANT: if we change the script functions object argument type, we must change this return value too!
    return (AGSObject*)&scrObj[num];
}
BITMAP *IAGSEngine::CreateBlankBitmap (int32 width, int32 height, int32 coldep) {
	// [IKM] We should not create Bitmap object here, because
	// a) we are returning raw allegro bitmap and therefore loosing control over it
	// b) plugin won't use Bitmap anyway
    BITMAP *tempb = create_bitmap_ex(coldep, width, height);
    clear_to_color(tempb, bitmap_mask_color(tempb));
    return tempb;
}
void IAGSEngine::FreeBitmap (BITMAP *tofree) {
    if (tofree)
        destroy_bitmap (tofree);
}
BITMAP *IAGSEngine::GetSpriteGraphic (int32 num) {
    return (BITMAP*)spriteset[num]->GetAllegroBitmap();
}
BITMAP *IAGSEngine::GetRoomMask (int32 index) {
    if (index == MASK_WALKABLE)
        return (BITMAP*)thisroom.WalkAreaMask->GetAllegroBitmap();
    else if (index == MASK_WALKBEHIND)
        return (BITMAP*)thisroom.WalkBehindMask->GetAllegroBitmap();
    else if (index == MASK_HOTSPOT)
        return (BITMAP*)thisroom.HotspotMask->GetAllegroBitmap();
    else if (index == MASK_REGIONS)
        return (BITMAP*)thisroom.RegionMask->GetAllegroBitmap();
    else
        quit("!IAGSEngine::GetRoomMask: invalid mask requested");
    return nullptr;
}
// [DEPRECATED]
AGSViewFrame *IAGSEngine::GetViewFrame (int32 view, int32 loop, int32 frame) {
    quit("!AGSEngine::GetViewFrame() is no longer supported. Use Game_GetViewFrame() of the script API instead.");
    return nullptr;
}

int IAGSEngine::GetRawPixelColor(int32 color)
{
    // Convert the standardized colour to the local gfx mode color
    // NOTE: it is unclear whether this has to be game colour depth or display color depth.
    // there was no difference in the original engine, but there is now.
    int result;
    __my_setcolor(&result, color, game.GetColorDepth());
    return result;
}

int IAGSEngine::GetWalkbehindBaseline (int32 wa) {
    if ((wa < 1) || (wa >= MAX_WALK_BEHINDS))
        quit("!IAGSEngine::GetWalkBehindBase: invalid walk-behind area specified");
    return croom->walkbehind_base[wa];
}
void* IAGSEngine::GetScriptFunctionAddress (const char *funcName) {
    return ccGetSymbolAddressForPlugin(funcName);
}
int IAGSEngine::GetBitmapTransparentColor(BITMAP *bmp) {
    return bitmap_mask_color (bmp);
}
// get the character scaling level at a particular point
int IAGSEngine::GetAreaScaling (int32 x, int32 y) {
    return GetScalingAt(x,y);
}
int IAGSEngine::IsGamePaused () {
    return game_paused;
}
int IAGSEngine::GetSpriteWidth (int32 slot) {
    return game.SpriteInfos[slot].Width;
}
int IAGSEngine::GetSpriteHeight (int32 slot) {
    return game.SpriteInfos[slot].Height;
}
void IAGSEngine::GetTextExtent (int32 font, const char *text, int32 *width, int32 *height) {
    if ((font < 0) || (font >= game.numfonts)) {
        if (width != nullptr) width[0] = 0;
        if (height != nullptr) height[0] = 0;
        return;
    }

    if (width != nullptr)
        width[0] = get_text_width_outlined (text, font);
    if (height != nullptr)
        height[0] = get_font_height_outlined(font);
}
void IAGSEngine::PrintDebugConsole (const char *text) {
    debug_script_log("[PLUGIN] %s", text);
}
int IAGSEngine::IsChannelPlaying (int32 channel) {
    return ::IsChannelPlaying (channel);
}
void IAGSEngine::PlaySoundChannel (int32 channel, int32 soundType, int32 volume, int32 loop, const char *filename) {
    stop_and_destroy_channel (channel);
    // Not sure if it's right to let it play on *any* channel, but this is plugin so let it go...
    // we must correctly stop background voice speech if it takes over speech chan
    if (channel == SCHAN_SPEECH && play.IsNonBlockingVoiceSpeech())
        stop_voice_nonblocking();

    AssetPath asset_name(filename, "audio");
    const char *ext = "";
    switch (soundType)
    {
    case PSND_WAVE:
        ext = "wav"; break;
    case PSND_MP3STREAM:
    case PSND_MP3STATIC:
        ext = "mp3"; break;
    case PSND_OGGSTREAM:
    case PSND_OGGSTATIC:
        ext = "ogg"; break;
    case PSND_MIDI:
        ext = "mid"; break;
    case PSND_MOD:
        ext = "mod"; break;
    case PSND_FLAC:
        ext = "flac"; break;
    default:
        debug_script_warn("IAGSEngine::PlaySoundChannel: unknown sound type %d", soundType);
        return;
    }

    std::unique_ptr<SOUNDCLIP> newcha(load_sound_clip(asset_name, ext, (loop != 0)));
    if (!newcha)
    {
        debug_script_warn("IAGSEngine::PlaySoundChannel: failed to load %s", filename);
        return;
    }

    newcha->set_volume255(volume);
    AudioChans::SetChannel(channel, std::move(newcha));
}
// Engine interface 12 and above are below
void IAGSEngine::MarkRegionDirty(int32 left, int32 top, int32 right, int32 bottom) {
    invalidate_rect(left, top, right, bottom, false);
    plugins[this->pluginId].invalidatedRegion++;
}
AGSMouseCursor * IAGSEngine::GetMouseCursor(int32 cursor) {
    // We should not return anything, because (at this time) script functions do not require actual cursor instance
    quit("!AGSEngine::GetMouseCursor() is no longer supported. Use related script API instead.");
    return nullptr;
}
void IAGSEngine::GetRawColorComponents(int32 coldepth, int32 color, int32 *red, int32 *green, int32 *blue, int32 *alpha) {
    if (red)
        *red = getr_depth(coldepth, color);
    if (green)
        *green = getg_depth(coldepth, color);
    if (blue)
        *blue = getb_depth(coldepth, color);
    if (alpha)
        *alpha = geta_depth(coldepth, color);
}
int IAGSEngine::MakeRawColorPixel(int32 coldepth, int32 red, int32 green, int32 blue, int32 alpha) {
    return makeacol_depth(coldepth, red, green, blue, alpha);
}
int IAGSEngine::GetFontType(int32 fontNum) {
    if ((fontNum < 0) || (fontNum >= game.numfonts))
        return FNT_INVALID;

    if (is_bitmap_font(fontNum))
        return FNT_TTF;

    return FNT_SCI;
}
int IAGSEngine::CreateDynamicSprite(int32 coldepth, int32 width, int32 height) {
    if ((width < 1) || (height < 1))
        quit("!IAGSEngine::CreateDynamicSprite: invalid width/height requested by plugin");

    if (!spriteset.HasFreeSlots())
        return 0;

    std::unique_ptr<Bitmap> image(BitmapHelper::CreateTransparentBitmap(width, height, coldepth));
    if (!image)
        return 0;

    return add_dynamic_sprite(std::move(image));
}
void IAGSEngine::DeleteDynamicSprite(int32 slot) {
    free_dynamic_sprite(slot);
}
int IAGSEngine::IsSpriteAlphaBlended(int32 slot) {
    // [DEPRECATED] TODO: may test for 32-bit? not sure if this function is necessary
    debug_script_warn("IsSpriteAlphaBlended: this function is deprecated and won't have any effect.");
    return 0;
}

// disable AGS's sound engine
void IAGSEngine::DisableSound() {
    shutdown_sound();
}

int IAGSEngine::CanRunScriptFunctionNow() {
    if (inside_script)
        return 0;
    return 1;
}

int IAGSEngine::CallGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1, long arg2, long arg3) {
    if (inside_script)
        return -300;

    ccInstance *toRun = GetScriptInstanceByType(globalScript ? kScInstGame : kScInstRoom);

    RuntimeScriptValue params[]{
        RuntimeScriptValue().SetPluginArgument(arg1),
        RuntimeScriptValue().SetPluginArgument(arg2),
        RuntimeScriptValue().SetPluginArgument(arg3),
    };
    int toret = RunScriptFunction(toRun, name, numArgs, params);
    return toret;
}

void IAGSEngine::NotifySpriteUpdated(int32 slot) {
    game_sprite_updated(slot);
}

void IAGSEngine::SetSpriteAlphaBlended(int32 /*slot*/, int32 /*isAlphaBlended*/) {
    // [DEPRECATED]
    debug_script_warn("SetSpriteAlphaBlended: this function is deprecated and won't have any effect.");
}

void IAGSEngine::QueueGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1, long arg2) {
    if (!inside_script) {
        this->CallGameScriptFunction(name, globalScript, numArgs, arg1, arg2, 0);
        return;
    }

    if (numArgs < 0 || numArgs > 2)
        quit("IAGSEngine::QueueGameScriptFunction: invalid number of arguments");
    RuntimeScriptValue params[] { RuntimeScriptValue().SetPluginArgument(arg1),
        RuntimeScriptValue().SetPluginArgument(arg2) };
    curscript->run_another(name, globalScript ? kScInstGame : kScInstRoom, numArgs, params);
}

int IAGSEngine::RegisterManagedObject(void *object, IAGSScriptManagedObject *callback) {
    // TODO: the managers may be either a separate static object, or the managed object itself.
    // we may try to optimize following by having a cache of CCPluginObjects per callback
    // address. Need to research if that's reliable, and will actually be more performant.
    auto *pl_obj = new CCPluginObject((IScriptObject*)callback);
    GlobalReturnValue.SetPluginObject((void*)object, pl_obj);
    return ccRegisterManagedObject(object, pl_obj, kScValPluginObject);
}

void IAGSEngine::AddManagedObjectReader(const char *typeName, IAGSManagedObjectReader *reader) {
    if ((typeName == nullptr) || (typeName[0] == 0))
        quit("Plugin error: IAGSEngine::AddObjectReader: invalid name for type");

    for (const auto &pr : pluginReaders) {
        if (pr.Type == typeName)
            quitprintf("Plugin error: IAGSEngine::AddObjectReader: type '%s' has been registered already", pr.Type.GetCStr());
    }

    pluginReaders.push_back(PluginObjectReader(typeName, reinterpret_cast<ICCObjectReader*>(reader)));
}

void IAGSEngine::RegisterUnserializedObject(int key, void *object, IAGSScriptManagedObject *callback) {
    auto *pl_obj = new CCPluginObject((IScriptObject*)callback);
    GlobalReturnValue.SetPluginObject((void*)object, pl_obj);
    ccRegisterUnserializedObject(key, object, pl_obj, kScValPluginObject);
}

int IAGSEngine::GetManagedObjectKeyByAddress(void *address) {
    return ccGetObjectHandleFromAddress(address);
}

void* IAGSEngine::GetManagedObjectAddressByKey(int key) {
    void *object;
    IScriptObject *manager;
    ScriptValueType obj_type = ccGetObjectAddressAndManagerFromHandle(key, object, manager);
    GlobalReturnValue.SetScriptObject(obj_type, object, manager);
    return object;
}

const char* IAGSEngine::CreateScriptString(const char *fromText) {
    const char *string = CreateNewScriptString(fromText);
    // Should be still standard dynamic object, because not managed by plugin
    GlobalReturnValue.SetScriptObject((void*)string, &myScriptStringImpl);
    return string;
}

int IAGSEngine::IncrementManagedObjectRefCount(void *address) {
    return ccAddObjectReference(GetManagedObjectKeyByAddress(address));
}

int IAGSEngine::DecrementManagedObjectRefCount(void *address) {
    return ccReleaseObjectReference(GetManagedObjectKeyByAddress(address));
}

void IAGSEngine::SetMousePosition(int32 x, int32 y) {
    Mouse::SetPosition(Point(x, y));
    RefreshMouse();
}

void IAGSEngine::SimulateMouseClick(int32 button) {
    SimulateMouseClick(button);
}

int IAGSEngine::GetMovementPathWaypointCount(int32 pathId) {
    return mls[pathId % TURNING_AROUND].numstage;
}

int IAGSEngine::GetMovementPathLastWaypoint(int32 pathId) {
    return mls[pathId % TURNING_AROUND].onstage;
}

void IAGSEngine::GetMovementPathWaypointLocation(int32 pathId, int32 waypoint, int32 *x, int32 *y) {
    *x = mls[pathId % TURNING_AROUND].pos[waypoint].X;
    *y = mls[pathId % TURNING_AROUND].pos[waypoint].Y;
}

void IAGSEngine::GetMovementPathWaypointSpeed(int32 pathId, int32 waypoint, int32 *xSpeed, int32 *ySpeed) {
    *xSpeed = mls[pathId % TURNING_AROUND].xpermove[waypoint];
    *ySpeed = mls[pathId % TURNING_AROUND].ypermove[waypoint];
}

int IAGSEngine::IsRunningUnderDebugger()
{
    return (editor_debugging_initialized != 0) ? 1 : 0;
}

void IAGSEngine::GetPathToFileInCompiledFolder(const char*fileName, char *buffer)
{
    // TODO: this is very unsafe, deprecate and make a better API function if still necessary
    strcpy(buffer, PathFromInstallDir(fileName).GetCStr());
}

void IAGSEngine::BreakIntoDebugger()
{
    break_on_next_script_step = 1;
}

IAGSFontRenderer* IAGSEngine::ReplaceFontRenderer(int fontNumber, IAGSFontRenderer *newRenderer)
{
    auto *old_render = font_replace_renderer(fontNumber, newRenderer);
    GUI::MarkForFontUpdate(fontNumber);
    return old_render;
}

IAGSFontRenderer* IAGSEngine::ReplaceFontRenderer2(int fontNumber, IAGSFontRenderer2 *newRenderer)
{
    auto *old_render = font_replace_renderer(fontNumber, newRenderer);
    GUI::MarkForFontUpdate(fontNumber);
    return old_render;
}

void IAGSEngine::NotifyFontUpdated(int fontNumber)
{
    font_recalc_metrics(fontNumber);
    GUI::MarkForFontUpdate(fontNumber);
}

const char *IAGSEngine::ResolveFilePath(const char *script_path)
{
    return File_ResolvePath(script_path);
}

void IAGSEngine::GetRenderStageDesc(AGSRenderStageDesc* desc)
{
    if (desc->Version >= 25)
    {
        gfxDriver->GetStageMatrixes((RenderMatrixes&)desc->Matrixes);
    }
}

void IAGSEngine::GetGameInfo(AGSGameInfo* ginfo)
{
    if (ginfo->Version >= 26)
    {
        snprintf(ginfo->GameName, sizeof(ginfo->GameName), "%s", game.gamename);
        snprintf(ginfo->Guid, sizeof(ginfo->Guid), "%s", game.guid);
        ginfo->UniqueId = game.uniqueid;
    }
}

// *********** General plugin implementation **********

void pl_stop_plugins() {
    ccSetDebugHook(nullptr);

    for (auto &plugin : plugins) {
        if (plugin.available) {
            if (plugin.engineShutdown != nullptr)
                plugin.engineShutdown();
            if (!plugin.builtin)
              plugin.library.Unload();
        }
    }
    plugins.clear();
}

void pl_startup_plugins() {
    for (auto &plugin : plugins) {
        if (plugin.available)
            plugin.engineStartup(&plugin.eiface);
    }
}

int pl_run_plugin_hooks (int event, int data) {
    for (auto &plugin : plugins) {
        if (plugin.wantHook & event) {
            int retval = plugin.onEvent(event, data);
            if (retval)
                return retval;
        }
    }
    return 0;
}

int pl_run_plugin_debug_hooks (const char *scriptfile, int linenum) {
    for (auto &plugin : plugins) {
        if (plugin.wantHook & AGSE_SCRIPTDEBUG) {
            int retval = plugin.debugHook(scriptfile, linenum, 0);
            if (retval)
                return retval;
        }
    }
    return 0;
}

void pl_run_plugin_init_gfx_hooks (const char *driverName, void *data) {
    for (auto &plugin : plugins)
    {
        if (plugin.initGfxHook != nullptr)
        {
            plugin.initGfxHook(driverName, data);
        }
    }
}

int pl_register_builtin_plugin(InbuiltPluginDetails const &details) {
    _registered_builtin_plugins.push_back(details);
    return 0;
}

bool pl_use_builtin_plugin(EnginePlugin* apl)
{
#if defined(BUILTIN_PLUGINS)
    if (apl->filename.CompareNoCase("agsflashlight") == 0)
    {
        apl->engineStartup = agsflashlight::AGS_EngineStartup;
        apl->engineShutdown = agsflashlight::AGS_EngineShutdown;
        apl->onEvent = agsflashlight::AGS_EngineOnEvent;
        apl->debugHook = agsflashlight::AGS_EngineDebugHook;
        apl->initGfxHook = agsflashlight::AGS_EngineInitGfx;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
    else if (apl->filename.CompareNoCase("agsblend") == 0)
    {
        apl->engineStartup = agsblend::AGS_EngineStartup;
        apl->engineShutdown = agsblend::AGS_EngineShutdown;
        apl->onEvent = agsblend::AGS_EngineOnEvent;
        apl->debugHook = agsblend::AGS_EngineDebugHook;
        apl->initGfxHook = agsblend::AGS_EngineInitGfx;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
    else if (apl->filename.CompareNoCase("ags_snowrain") == 0)
    {
        apl->engineStartup = ags_snowrain::AGS_EngineStartup;
        apl->engineShutdown = ags_snowrain::AGS_EngineShutdown;
        apl->onEvent = ags_snowrain::AGS_EngineOnEvent;
        apl->debugHook = ags_snowrain::AGS_EngineDebugHook;
        apl->initGfxHook = ags_snowrain::AGS_EngineInitGfx;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
    else if (apl->filename.CompareNoCase("ags_parallax") == 0)
    {
        apl->engineStartup = ags_parallax::AGS_EngineStartup;
        apl->engineShutdown = ags_parallax::AGS_EngineShutdown;
        apl->onEvent = ags_parallax::AGS_EngineOnEvent;
        apl->debugHook = ags_parallax::AGS_EngineDebugHook;
        apl->initGfxHook = ags_parallax::AGS_EngineInitGfx;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
    else if (apl->filename.CompareNoCase("agspalrender") == 0)
    {
        apl->engineStartup = agspalrender::AGS_EngineStartup;
        apl->engineShutdown = agspalrender::AGS_EngineShutdown;
        apl->onEvent = agspalrender::AGS_EngineOnEvent;
        apl->debugHook = agspalrender::AGS_EngineDebugHook;
        apl->initGfxHook = agspalrender::AGS_EngineInitGfx;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
    else if (apl->filename.CompareNoCase("agsspritefont") == 0 ||
             apl->filename.CompareNoCase("agsplugin.spritefont") == 0)
    {
        apl->engineStartup = agsspritefont::AGS_EngineStartup;
        apl->engineShutdown = agsspritefont::AGS_EngineShutdown;
        apl->onEvent = agsspritefont::AGS_EngineOnEvent;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
#if AGS_PLATFORM_OS_IOS
    else if (apl->filename.CompareNoCase("agstouch") == 0)
    {
        apl->engineStartup = agstouch::AGS_EngineStartup;
        apl->engineShutdown = agstouch::AGS_EngineShutdown;
        apl->onEvent = agstouch::AGS_EngineOnEvent;
        apl->debugHook = agstouch::AGS_EngineDebugHook;
        apl->initGfxHook = agstouch::AGS_EngineInitGfx;
        apl->available = true;
        apl->builtin = true;
        return true;
    }
#endif // IOS_VERSION
#endif // BUILTIN_PLUGINS

    for(std::vector<InbuiltPluginDetails>::iterator it = _registered_builtin_plugins.begin(); it != _registered_builtin_plugins.end(); ++it) {
        if (apl->filename.CompareNoCase(it->filename) == 0) {
            apl->engineStartup = it->engineStartup;
            apl->engineShutdown = it->engineShutdown;
            apl->onEvent = it->onEvent;
            apl->debugHook = it->debugHook;
            apl->initGfxHook = it->initGfxHook;
            apl->available = true;
            apl->builtin = true;
            return true;
        }
    }
    return false;
}

Engine::GameInitError pl_register_plugins(const std::vector<PluginInfo> &infos)
{
    std::vector<String> lookup_dirs;
    lookup_dirs.push_back(appDirectory);
    if (ResPaths.DataDir != appDirectory)
        lookup_dirs.push_back(ResPaths.DataDir);

    for (size_t inf_index = 0; inf_index < infos.size(); ++inf_index)
    {
        const Common::PluginInfo &info = infos[inf_index];
        String name = info.Name;
        if (name.GetLast() == '!')
            continue; // editor-only plugin, ignore it
        // AGS Editor currently saves plugin names in game data with
        // ".dll" extension appended; we need to take care of that
        const String name_ext = ".dll";
        if (name.GetLength() <= name_ext.GetLength() ||
                name.CompareRightNoCase(name_ext, name_ext.GetLength())) {
            return kGameInitErr_PluginNameInvalid;
        }
        // remove ".dll" from plugin's name
        name.ClipRight(name_ext.GetLength());

        plugins.push_back(EnginePlugin());
        EnginePlugin *apl = &plugins.back();
        // Copy plugin info
        apl->filename = name;
        apl->savedata = info.Data;

        // Compatibility with the old SnowRain module
        if (apl->filename.CompareNoCase("ags_SnowRain20") == 0) {
            apl->filename = "ags_snowrain";
        }

        if (apl->library.Load(apl->filename, lookup_dirs))
        {
          AGS::Common::Debug::Printf(kDbgMsg_Info, "Plugin '%s' loaded from '%s', resolving imports...",
              apl->filename.GetCStr(), apl->library.GetPath().GetCStr());

          if (apl->library.GetFunctionAddress("AGS_PluginV2") == nullptr) {
              quitprintf("Plugin '%s' is an old incompatible version.", apl->filename.GetCStr());
          }
          apl->engineStartup = (void(*)(IAGSEngine*))apl->library.GetFunctionAddress("AGS_EngineStartup");
          apl->engineShutdown = (void(*)())apl->library.GetFunctionAddress("AGS_EngineShutdown");

          if (apl->engineStartup == nullptr) {
              quitprintf("Plugin '%s' is not a valid AGS plugin (no engine startup entry point)", apl->filename.GetCStr());
          }
          apl->onEvent = (int(*)(int,int))apl->library.GetFunctionAddress("AGS_EngineOnEvent");
          apl->debugHook = (int(*)(const char*,int,int))apl->library.GetFunctionAddress("AGS_EngineDebugHook");
          apl->initGfxHook = (void(*)(const char*, void*))apl->library.GetFunctionAddress("AGS_EngineInitGfx");
        }
        else
        {
          String expect_filename = apl->library.GetFilenameForLib(apl->filename);
          AGS::Common::Debug::Printf(kDbgMsg_Info, "Plugin '%s' could not be loaded (expected '%s'), trying built-in plugins...",
              apl->filename.GetCStr(), expect_filename.GetCStr());
          if (pl_use_builtin_plugin(apl))
          {
            AGS::Common::Debug::Printf(kDbgMsg_Info, "Build-in plugin '%s' found and being used.", apl->filename.GetCStr());
          }
          else
          {
            // Plugin loading has failed at this point, try using built-in plugin function stubs
            if (RegisterPluginStubs(apl->filename.GetCStr()))
              AGS::Common::Debug::Printf(kDbgMsg_Info, "Placeholder functions for the plugin '%s' found.", apl->filename.GetCStr());
            else
              AGS::Common::Debug::Printf(kDbgMsg_Info, "No placeholder functions for the plugin '%s' found. The game might fail to load!", apl->filename.GetCStr());
            continue;
          }
        }

        apl->eiface.pluginId = plugins.size() - 1;
        apl->eiface.version = PLUGIN_API_VERSION;
        apl->wantHook = 0;
        apl->available = true;
    }
    return kGameInitErr_NoError;
}

bool pl_is_plugin_loaded(const char *pl_name)
{
    if (!pl_name)
        return false;

    for (auto &plugin : plugins)
    {
        if (plugin.filename.CompareNoCase(pl_name) == 0)
            return plugin.available;
    }
    return false;
}

bool pl_any_want_hook(int event)
{
    for (auto &plugin : plugins)
    {
        if(plugin.wantHook & event)
            return true;
    }
    return false;
}
