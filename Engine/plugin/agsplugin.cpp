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

#include <vector>

#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/roomstruct.h"
#include "ac/view.h"
#include "ac/charactercache.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_audio.h"
#include "ac/global_plugin.h"
#include "ac/global_walkablearea.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/movelist.h"
#include "ac/objectcache.h"
#include "ac/parser.h"
#include "ac/path_helper.h"
#include "ac/record.h"
#include "ac/roomstatus.h"
#include "ac/string.h"
#include "font/fonts.h"
#include "util/string_utils.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "device/mousew32.h"
#include "gui/guidefines.h"
#include "main/game_run.h"
#include "main/engine.h"
#include "media/audio/audio.h"
#include "media/audio/sound.h"
#include "plugin/agsplugin.h"
#include "plugin/plugin_engine.h"
#include "plugin/pluginobjectreader.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gfx/gfxfilter.h"
#include "script/runtimescriptvalue.h"
#include "debug/out.h"
#include "ac/dynobj/scriptstring.h"
#include "main/graphics_mode.h"
#include "gfx/gfx_util.h"

using namespace AGS::Common;
using namespace AGS::Engine;


#if defined(BUILTIN_PLUGINS)
#include "../Plugins/AGSflashlight/agsflashlight.h"
#include "../Plugins/agsblend/agsblend.h"
#include "../Plugins/ags_snowrain/ags_snowrain.h"
#include "../Plugins/ags_parallax/ags_parallax.h"
#if defined(IOS_VERSION)
#include "../Plugins/agstouch/agstouch.h"
#endif // IOS_VERSION
#endif // BUILTIN_PLUGINS

#if defined(MAC_VERSION)
extern "C"
{
    int osx_sys_question(AL_CONST char *msg, AL_CONST char *but1, AL_CONST char *but2);
}
#endif

#if defined(PSP_VERSION)
#include <pspsdk.h>
#include <pspkernel.h>
extern "C"
{
#include <systemctrl.h>
}
#include "../PSP/kernel/kernel.h"
#endif



extern IGraphicsDriver *gfxDriver;
extern int mousex, mousey;
extern int displayed_room;
extern roomstruct thisroom;
extern GameSetupStruct game;
extern RoomStatus*croom;
extern SpriteCache spriteset;
extern ViewStruct*views;
extern int game_paused;
extern GameSetup usetup;
extern int inside_script;
extern ccInstance *gameinst, *roominst;
extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_INIT_SPR];
extern MoveList *mls;
extern int numlines;
extern char lines[MAXLINE][200];
extern color palette[256];
extern PluginObjectReader pluginReaders[MAX_PLUGIN_OBJECT_READERS];
extern int numPluginReaders;
extern RuntimeScriptValue GlobalReturnValue;
extern ScriptString myScriptStringImpl;

// **************** PLUGIN IMPLEMENTATION ****************


#include "util/library.h"




struct EnginePlugin {
    char        filename[PLUGIN_FILENAME_MAX+1];
    AGS::Engine::Library   library;
    bool       available;
    char       *savedata;
    int         savedatasize;
    int         wantHook;
    int         invalidatedRegion;
    void      (*engineStartup) (IAGSEngine *);
    void      (*engineShutdown) ();
    int       (*onEvent) (int, int);
    void      (*initGfxHook) (const char *driverName, void *data);
    int       (*debugHook) (const char * whichscript, int lineNumber, int reserved);
    IAGSEngine  eiface;
    bool        builtin;

    EnginePlugin() {
        filename[0] = 0;
        wantHook = 0;
        invalidatedRegion = 0;
        savedata = NULL;
        builtin = false;
        available = false;
    }
};
#define MAXPLUGINS 20
EnginePlugin plugins[MAXPLUGINS];
int numPlugins = 0;
int pluginsWantingDebugHooks = 0;

std::vector<InbuiltPluginDetails> _registered_builtin_plugins;

void IAGSEngine::AbortGame (const char *reason) {
    quit ((char*)reason);
}
const char* IAGSEngine::GetEngineVersion () {
    return get_engine_version();
}
void IAGSEngine::RegisterScriptFunction (const char*name, void*addy) {
    ccAddExternalPluginFunction (name, addy);
}
const char* IAGSEngine::GetGraphicsDriverID()
{
    if (gfxDriver == NULL)
        return NULL;

    return gfxDriver->GetDriverID();
}

BITMAP *IAGSEngine::GetScreen () 
{
    if (!gfxDriver->UsesMemoryBackBuffer())
        quit("!This plugin is not compatible with the Direct3D driver.");

	return (BITMAP*)BitmapHelper::GetScreenBitmap()->GetAllegroBitmap();
}
BITMAP *IAGSEngine::GetVirtualScreen () 
{
	// [IKM] Aaahh... this is very dangerous, but what can we do?
	return (BITMAP*)gfxDriver->GetMemoryBackBuffer()->GetAllegroBitmap();
}
void IAGSEngine::RequestEventHook (int32 event) {
    if (event >= AGSE_TOOHIGH) 
        quit("!IAGSEngine::RequestEventHook: invalid event requested");

    if (plugins[this->pluginId].onEvent == NULL)
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
                ccSetDebugHook(NULL);
    }

    plugins[this->pluginId].wantHook &= ~event;
}

int IAGSEngine::GetSavedData (char *buffer, int32 bufsize) {
    int savedatasize = plugins[this->pluginId].savedatasize;

    if (bufsize < savedatasize)
        quit("!IAGSEngine::GetSavedData: buffer too small");

    if (savedatasize > 0)
        memcpy (buffer, plugins[this->pluginId].savedata, savedatasize);

    return savedatasize;
}
void IAGSEngine::DrawText (int32 x, int32 y, int32 font, int32 color, char *text) 
{
    Bitmap *ds = gfxDriver->GetMemoryBackBuffer();
    color_t text_color = ds->GetCompatibleColor(color);
    draw_and_invalidate_text(ds, x, y, font, text_color, text);
}
void IAGSEngine::GetScreenDimensions (int32 *width, int32 *height, int32 *coldepth) {
    if (width != NULL)
        width[0] = play.GetMainViewport().GetWidth();
    if (height != NULL)
        height[0] = play.GetMainViewport().GetHeight();
    if (coldepth != NULL)
        coldepth[0] = scsystem.coldepth;
}
unsigned char ** IAGSEngine::GetRawBitmapSurface (BITMAP *bmp) {
    if (!is_linear_bitmap (bmp))
        quit("!IAGSEngine::GetRawBitmapSurface: invalid bitmap for access to surface");
    acquire_bitmap (bmp);

    if (bmp == gfxDriver->GetMemoryBackBuffer()->GetAllegroBitmap())
        plugins[this->pluginId].invalidatedRegion = 0;

    return bmp->line;
}
void IAGSEngine::ReleaseBitmapSurface (BITMAP *bmp) {
    release_bitmap (bmp);

    if (bmp == gfxDriver->GetMemoryBackBuffer()->GetAllegroBitmap()) {
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
    return thisroom.num_bscenes;
}
int IAGSEngine::GetCurrentBackground () {
    return play.bg_frame;
}
BITMAP *IAGSEngine::GetBackgroundScene (int32 index) {
    return (BITMAP*)thisroom.ebscene[index]->GetAllegroBitmap();
}
void IAGSEngine::GetBitmapDimensions (BITMAP *bmp, int32 *width, int32 *height, int32 *coldepth) {
    if (bmp == NULL)
        return;

    if (width != NULL)
        width[0] = bmp->w;
    if (height != NULL)
        height[0] = bmp->h;
    if (coldepth != NULL)
        coldepth[0] = bitmap_color_depth(bmp);
}
// [IKM] Interesting, why does AGS need those two functions?
// Can it be that it was planned to change implementation in the future?
int IAGSEngine::FRead (void *buffer, int32 len, int32 handle) {
    return fread (buffer, 1, len, (FILE*)handle);
}
int IAGSEngine::FWrite (void *buffer, int32 len, int32 handle) {
    return fwrite (buffer, 1, len, (FILE*)handle);
}
void IAGSEngine::DrawTextWrapped (int32 xx, int32 yy, int32 wid, int32 font, int32 color, const char*text) {
    // TODO: use generic function from the engine instead of having copy&pasted code here
    int linespacing = getfontspacing_outlined(font);

    break_up_text_into_lines (wid, font, (char*)text);

    Bitmap *ds = gfxDriver->GetMemoryBackBuffer();
    color_t text_color = ds->GetCompatibleColor(color);
    multiply_up_coordinates((int*)&xx, (int*)&yy); // stupid! quick tweak
    for (int i = 0; i < numlines; i++)
        draw_and_invalidate_text(ds, xx, yy + linespacing*i, font, text_color, lines[i]);
}
void IAGSEngine::SetVirtualScreen (BITMAP *bmp) {
	// [IKM] Very, very dangerous :'(
    // TODO: this won't work with hardware-accelerated renderers
    SetVirtualScreenRaw (bmp);
}
int IAGSEngine::LookupParserWord (const char *word) {
    return find_word_in_dictionary ((char*)word);
}
void IAGSEngine::BlitBitmap (int32 x, int32 y, BITMAP *bmp, int32 masked) {
    wputblock_raw (gfxDriver->GetMemoryBackBuffer(), x, y, bmp, masked);
    invalidate_rect(x, y, x + bmp->w, y + bmp->h, false);
}
void IAGSEngine::BlitSpriteTranslucent(int32 x, int32 y, BITMAP *bmp, int32 trans) {
    Bitmap wrap(bmp, true);
    if (gfxDriver->UsesMemoryBackBuffer())
        GfxUtil::DrawSpriteWithTransparency(gfxDriver->GetMemoryBackBuffer(), &wrap, x, y, trans);
    else
        GfxUtil::DrawSpriteBlend(gfxDriver->GetMemoryBackBuffer(), Point(x,y), &wrap, kBlendMode_Alpha, true, false, trans);
}
void IAGSEngine::BlitSpriteRotated(int32 x, int32 y, BITMAP *bmp, int32 angle) {
    Common::Bitmap *ds = gfxDriver->GetMemoryBackBuffer();
    // FIXME: call corresponding Graphics Blit
    rotate_sprite(ds->GetAllegroBitmap(), bmp, x, y, itofix(angle));
}

extern void domouse(int);
extern int  mgetbutton();

void IAGSEngine::PollSystem () {

    NEXT_ITERATION();
    domouse(DOMOUSE_NOCURSOR);
    update_polled_stuff_if_runtime();
    int mbut = mgetbutton();
    if (mbut > NONE)
        pl_run_plugin_hooks (AGSE_MOUSECLICK, mbut);

    int kp;
    if (run_service_key_controls(kp)) {
        pl_run_plugin_hooks (AGSE_KEYPRESS, kp);
    }

}
AGSCharacter* IAGSEngine::GetCharacter (int32 charnum) {
    if (charnum >= game.numcharacters)
        quit("!AGSEngine::GetCharacter: invalid character request");

    return (AGSCharacter*)&game.chars[charnum];
}
AGSGameOptions* IAGSEngine::GetGameOptions () {
    return (AGSGameOptions*)&play;
}
AGSColor* IAGSEngine::GetPalette () {
    return (AGSColor*)&palette[0];
}
void IAGSEngine::SetPalette (int32 start, int32 finish, AGSColor *cpl) {
    set_palette_range((color*)cpl, start, finish, 0);
}
int IAGSEngine::GetNumCharacters () {
    return game.numcharacters;
}
int IAGSEngine::GetPlayerCharacter () {
    return game.playercharacter;
}
void IAGSEngine::RoomToViewport (int32 *x, int32 *y) {
    Point scrp = play.RoomToScreen(x ? multiply_up_coordinate(*x) : 0, y ? multiply_up_coordinate(*y) : 0);
    if (x)
        *x = scrp.X;
    if (y)
        *y = scrp.Y;
}
void IAGSEngine::ViewportToRoom (int32 *x, int32 *y) {
    Point scrp = play.ScreenToRoom(x ? divide_down_coordinate(*x) : 0, y ? divide_down_coordinate(*y) : 0);
    if (x)
        *x = scrp.X;
    if (y)
        *y = scrp.Y;
}
int IAGSEngine::GetNumObjects () {
    return croom->numobj;
}
AGSObject *IAGSEngine::GetObject (int32 num) {
    if (num >= croom->numobj)
        quit("!IAGSEngine::GetObject: invalid object");

    return (AGSObject*)&croom->obj[num];
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
        return (BITMAP*)thisroom.walls->GetAllegroBitmap();
    else if (index == MASK_WALKBEHIND)
        return (BITMAP*)thisroom.object->GetAllegroBitmap();
    else if (index == MASK_HOTSPOT)
        return (BITMAP*)thisroom.lookat->GetAllegroBitmap();
    else if (index == MASK_REGIONS)
        return (BITMAP*)thisroom.regions->GetAllegroBitmap();
    else
        quit("!IAGSEngine::GetRoomMask: invalid mask requested");
    return NULL;
}
AGSViewFrame *IAGSEngine::GetViewFrame (int32 view, int32 loop, int32 frame) {
    view--;
    if ((view < 0) || (view >= game.numviews))
        quit("!IAGSEngine::GetViewFrame: invalid view");
    if ((loop < 0) || (loop >= views[view].numLoops))
        quit("!IAGSEngine::GetViewFrame: invalid loop");
    if ((frame < 0) || (frame >= views[view].loops[loop].numFrames))
        return NULL;

    return (AGSViewFrame*)&views[view].loops[loop].frames[frame];
}
int IAGSEngine::GetRawPixelColor (int32 color) {
    // Convert the standardized colour to the local gfx mode color
    // TODO: this won't work with hardware-accelerated renderers
    int result;
    __my_setcolor(&result, color, ::GetVirtualScreen()->GetColorDepth());

    return result;
}
int IAGSEngine::GetWalkbehindBaseline (int32 wa) {
    if ((wa < 1) || (wa >= MAX_OBJ))
        quit("!IAGSEngine::GetWalkBehindBase: invalid walk-behind area specified");
    return croom->walkbehind_base[wa];
}
void* IAGSEngine::GetScriptFunctionAddress (const char *funcName) {
    return ccGetSymbolAddressForPlugin ((char*)funcName);
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
        if (width != NULL) width[0] = 0;
        if (height != NULL) height[0] = 0;
        return;
    }

    if (width != NULL)
        width[0] = wgettextwidth_compensate (text, font);
    if (height != NULL)
        height[0] = wgettextheight ((char*)text, font);
}
void IAGSEngine::PrintDebugConsole (const char *text) {
    debug_script_log("[PLUGIN] %s", text);
    platform->WriteStdOut("[PLUGIN] %s", text);
}
int IAGSEngine::IsChannelPlaying (int32 channel) {
    return ::IsChannelPlaying (channel);
}
void IAGSEngine::PlaySoundChannel (int32 channel, int32 soundType, int32 volume, int32 loop, const char *filename) {
    stop_and_destroy_channel (channel);
    SOUNDCLIP *newcha = NULL;

    if (((soundType == PSND_MP3STREAM) || (soundType == PSND_OGGSTREAM)) 
        && (loop != 0))
        quit("IAGSEngine::PlaySoundChannel: streamed samples cannot loop");

    // TODO: find out how engine was supposed to decide on where to load the sound from
    AssetPath asset_name("", filename);

    if (soundType == PSND_WAVE)
        newcha = my_load_wave (asset_name, volume, loop);
    else if (soundType == PSND_MP3STREAM)
        newcha = my_load_mp3 (asset_name, volume);
    else if (soundType == PSND_OGGSTREAM)
        newcha = my_load_ogg (asset_name, volume);
    else if (soundType == PSND_MP3STATIC)
        newcha = my_load_static_mp3 (asset_name, volume, (loop != 0));
    else if (soundType == PSND_OGGSTATIC)
        newcha = my_load_static_ogg (asset_name, volume, (loop != 0));
    else if (soundType == PSND_MIDI) {
        if (midi_pos >= 0)
            quit("!IAGSEngine::PlaySoundChannel: MIDI already in use");
        newcha = my_load_midi (asset_name, loop);
        newcha->set_volume (volume);
    }
#ifndef PSP_NO_MOD_PLAYBACK
    else if (soundType == PSND_MOD) {
        newcha = my_load_mod (asset_name, loop);
        newcha->set_volume (volume);
    }
#endif
    else
        quit("!IAGSEngine::PlaySoundChannel: unknown sound type");

    channels[channel] = newcha;
}
// Engine interface 12 and above are below
void IAGSEngine::MarkRegionDirty(int32 left, int32 top, int32 right, int32 bottom) {
    invalidate_rect(left, top, right, bottom, false);
    plugins[this->pluginId].invalidatedRegion++;
}
AGSMouseCursor * IAGSEngine::GetMouseCursor(int32 cursor) {
    if ((cursor < 0) || (cursor >= game.numcursors))
        return NULL;

    return (AGSMouseCursor*)&game.mcurs[cursor];
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

    if (font_supports_extended_characters(fontNum))
        return FNT_TTF;

    return FNT_SCI;
}
int IAGSEngine::CreateDynamicSprite(int32 coldepth, int32 width, int32 height) {

    // TODO: why is this implemented right here, should not an existing
    // script handling implementation be called instead?

    int gotSlot = spriteset.AddNewSprite();
    if (gotSlot <= 0)
        return 0;

    if ((width < 1) || (height < 1))
        quit("!IAGSEngine::CreateDynamicSprite: invalid width/height requested by plugin");

    // resize the sprite to the requested size
    Bitmap *newPic = BitmapHelper::CreateTransparentBitmap(width, height, coldepth);
    if (newPic == NULL)
        return 0;

    // add it into the sprite set
    add_dynamic_sprite(gotSlot, newPic);
    return gotSlot;
}
void IAGSEngine::DeleteDynamicSprite(int32 slot) {
    free_dynamic_sprite(slot);
}
int IAGSEngine::IsSpriteAlphaBlended(int32 slot) {
    if (game.SpriteInfos[slot].Flags & SPF_ALPHACHANNEL)
        return 1;
    return 0;
}

// disable AGS's sound engine
void IAGSEngine::DisableSound() {
    shutdown_sound();
    usetup.digicard = DIGI_NONE;
    usetup.midicard = MIDI_NONE;
    install_sound(usetup.digicard,usetup.midicard,NULL);
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

    RuntimeScriptValue params[3];
    params[0].SetPluginArgument(arg1);
    params[1].SetPluginArgument(arg2);
    params[2].SetPluginArgument(arg3);
    int toret = toRun->RunScriptFunctionIfExists((char*)name, numArgs, params);
    return toret;
}

void IAGSEngine::NotifySpriteUpdated(int32 slot) {
    int ff;
    // wipe the character cache when we change rooms
    for (ff = 0; ff < game.numcharacters; ff++) {
        if ((charcache[ff].inUse) && (charcache[ff].sppic == slot)) {
            delete charcache[ff].image;
            charcache[ff].image = NULL;
            charcache[ff].inUse = 0;
        }
    }

    // clear the object cache
    for (ff = 0; ff < MAX_INIT_SPR; ff++) {
        if ((objcache[ff].image != NULL) && (objcache[ff].sppic == slot)) {
            delete objcache[ff].image;
            objcache[ff].image = NULL;
        }
    }
}

void IAGSEngine::SetSpriteAlphaBlended(int32 slot, int32 isAlphaBlended) {

    game.SpriteInfos[slot].Flags &= ~SPF_ALPHACHANNEL;

    if (isAlphaBlended)
        game.SpriteInfos[slot].Flags |= SPF_ALPHACHANNEL;
}

void IAGSEngine::QueueGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1, long arg2) {
    if (!inside_script) {
        this->CallGameScriptFunction(name, globalScript, numArgs, arg1, arg2, 0);
        return;
    }

    if (numArgs < 0 || numArgs > 2)
        quit("IAGSEngine::QueueGameScriptFunction: invalid number of arguments");

    curscript->run_another(name, globalScript ? kScInstGame : kScInstRoom, numArgs,
        RuntimeScriptValue().SetPluginArgument(arg1), RuntimeScriptValue().SetPluginArgument(arg2));
}

int IAGSEngine::RegisterManagedObject(const void *object, IAGSScriptManagedObject *callback) {
    GlobalReturnValue.SetPluginObject((void*)object, (ICCDynamicObject*)callback);
    return ccRegisterManagedObject(object, (ICCDynamicObject*)callback, true);
}

void IAGSEngine::AddManagedObjectReader(const char *typeName, IAGSManagedObjectReader *reader) {
    if (numPluginReaders >= MAX_PLUGIN_OBJECT_READERS) 
        quit("Plugin error: IAGSEngine::AddObjectReader: Too many object readers added");

    if ((typeName == NULL) || (typeName[0] == 0))
        quit("Plugin error: IAGSEngine::AddObjectReader: invalid name for type");

    for (int ii = 0; ii < numPluginReaders; ii++) {
        if (strcmp(pluginReaders[ii].type, typeName) == 0)
            quitprintf("Plugin error: IAGSEngine::AddObjectReader: type '%s' has been registered already", typeName);
    }

    pluginReaders[numPluginReaders].reader = reader;
    pluginReaders[numPluginReaders].type = typeName;
    numPluginReaders++;
}

void IAGSEngine::RegisterUnserializedObject(int key, const void *object, IAGSScriptManagedObject *callback) {
    GlobalReturnValue.SetPluginObject((void*)object, (ICCDynamicObject*)callback);
    ccRegisterUnserializedObject(key, object, (ICCDynamicObject*)callback, true);
}

int IAGSEngine::GetManagedObjectKeyByAddress(const char *address) {
    return ccGetObjectHandleFromAddress(address);
}

void* IAGSEngine::GetManagedObjectAddressByKey(int key) {
    void *object;
    ICCDynamicObject *manager;
    ScriptValueType obj_type = ccGetObjectAddressAndManagerFromHandle(key, object, manager);
    if (obj_type == kScValPluginObject)
    {
        GlobalReturnValue.SetPluginObject(object, manager);
    }
    else
    {
        GlobalReturnValue.SetDynamicObject(object, manager);
    }
    return object;
}

const char* IAGSEngine::CreateScriptString(const char *fromText) {
    const char *string = CreateNewScriptString(fromText);
    // Should be still standard dynamic object, because not managed by plugin
    GlobalReturnValue.SetDynamicObject((void*)string, &myScriptStringImpl);
    return string;
}

int IAGSEngine::IncrementManagedObjectRefCount(const char *address) {
    return ccAddObjectReference(GetManagedObjectKeyByAddress(address));
}

int IAGSEngine::DecrementManagedObjectRefCount(const char *address) {
    return ccReleaseObjectReference(GetManagedObjectKeyByAddress(address));
}

void IAGSEngine::SetMousePosition(int32 x, int32 y) {
    Mouse::SetPosition(Point(x, y));
    RefreshMouse();
}

void IAGSEngine::SimulateMouseClick(int32 button) {
    PluginSimulateMouseClick(button);
}

int IAGSEngine::GetMovementPathWaypointCount(int32 pathId) {
    return mls[pathId % TURNING_AROUND].numstage;
}

int IAGSEngine::GetMovementPathLastWaypoint(int32 pathId) {
    return mls[pathId % TURNING_AROUND].onstage;
}

void IAGSEngine::GetMovementPathWaypointLocation(int32 pathId, int32 waypoint, int32 *x, int32 *y) {
    *x = (mls[pathId % TURNING_AROUND].pos[waypoint] >> 16) & 0x0000ffff;
    *y = (mls[pathId % TURNING_AROUND].pos[waypoint] & 0x0000ffff);
}

void IAGSEngine::GetMovementPathWaypointSpeed(int32 pathId, int32 waypoint, int32 *xSpeed, int32 *ySpeed) {
    *xSpeed = mls[pathId % TURNING_AROUND].xpermove[waypoint];
    *ySpeed = mls[pathId % TURNING_AROUND].ypermove[waypoint];
}

int IAGSEngine::IsRunningUnderDebugger()
{
    return (editor_debugging_enabled != 0) ? 1 : 0;
}

void IAGSEngine::GetPathToFileInCompiledFolder(const char*fileName, char *buffer)
{
    get_install_dir_path(buffer, fileName);
}

void IAGSEngine::BreakIntoDebugger()
{
    break_on_next_script_step = 1;
}

IAGSFontRenderer* IAGSEngine::ReplaceFontRenderer(int fontNumber, IAGSFontRenderer *newRenderer)
{
    return font_replace_renderer(fontNumber, newRenderer);
}


// *********** General plugin implementation **********

void pl_stop_plugins() {
    int a;
    ccSetDebugHook(NULL);

    for (a = 0; a < numPlugins; a++) {
        if (plugins[a].available) {
            if (plugins[a].engineShutdown != NULL)
                plugins[a].engineShutdown();
            plugins[a].wantHook = 0;
            if (plugins[a].savedata) {
                free(plugins[a].savedata);
                plugins[a].savedata = NULL;
            }
            if (!plugins[a].builtin) {
              plugins[a].library.Unload();
            }
        }
    }
    numPlugins = 0;
}

void pl_startup_plugins() {
    int i;
    for (i = 0; i < numPlugins; i++) {
        if (plugins[i].available)
            plugins[i].engineStartup (&plugins[i].eiface);
    }
}

int pl_run_plugin_hooks (int event, long data) {
    int i, retval = 0;
    for (i = 0; i < numPlugins; i++) {
        if (plugins[i].wantHook & event) {
            retval = plugins[i].onEvent (event, data);
            if (retval)
                return retval;
        }
    }
    return 0;
}

int pl_run_plugin_debug_hooks (const char *scriptfile, int linenum) {
    int i, retval = 0;
    for (i = 0; i < numPlugins; i++) {
        if (plugins[i].wantHook & AGSE_SCRIPTDEBUG) {
            retval = plugins[i].debugHook(scriptfile, linenum, 0);
            if (retval)
                return retval;
        }
    }
    return 0;
}

void pl_run_plugin_init_gfx_hooks (const char *driverName, void *data) {
    for (int i = 0; i < numPlugins; i++) 
    {
        if (plugins[i].initGfxHook != NULL)
        {
            plugins[i].initGfxHook(driverName, data);
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
    if (stricmp(apl->filename, "agsflashlight") == 0)
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
    else if (stricmp(apl->filename, "agsblend") == 0)
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
    else if (stricmp(apl->filename, "ags_snowrain") == 0)
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
    else if (stricmp(apl->filename, "ags_parallax") == 0)
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
#if defined(IOS_VERSION)
    else if (stricmp(apl->filename, "agstouch") == 0)
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
        if (stricmp(apl->filename, it->filename) == 0) {
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
    
    AGS::Common::Debug::Printf("No built-in plugin found. Plugin loading failed!");
    return false;
}

Engine::GameInitError pl_register_plugins(const std::vector<Common::PluginInfo> &infos)
{
    numPlugins = 0;
    for (size_t inf_index = 0; inf_index < infos.size(); ++inf_index)
    {
        const Common::PluginInfo &info = infos[inf_index];
        String name = info.Name;
        if (name.GetLast() == '!')
            continue; // editor-only plugin, ignore it
        if (numPlugins == MAXPLUGINS)
            return kGameInitErr_TooManyPlugins;
        // AGS Editor currently saves plugin names in game data with
        // ".dll" extension appended; we need to take care of that
        const String name_ext = ".dll";
        if (name.GetLength() <= name_ext.GetLength() || name.GetLength() > PLUGIN_FILENAME_MAX + name_ext.GetLength() ||
                name.CompareRightNoCase(name_ext, name_ext.GetLength())) {
            return kGameInitErr_PluginNameInvalid;
        }
        // remove ".dll" from plugin's name
        name.ClipRight(name_ext.GetLength());

        EnginePlugin *apl = &plugins[numPlugins++];
        // Copy plugin info
        snprintf(apl->filename, sizeof(apl->filename), "%s", name.GetCStr());
        if (info.DataLen)
        {
            apl->savedata = (char*)malloc(info.DataLen);
            memcpy(apl->savedata, info.Data.get(), info.DataLen);
        }
        apl->savedatasize = info.DataLen;

        // Compatibility with the old SnowRain module
        if (stricmp(apl->filename, "ags_SnowRain20") == 0) {
            strcpy(apl->filename, "ags_snowrain");
        }

        if (apl->library.Load(apl->filename))
        {
          AGS::Common::Debug::Printf(kDbgMsg_Init, "Plugin '%s' loading succeeded, resolving imports...", apl->filename);

          if (apl->library.GetFunctionAddress("AGS_PluginV2") == NULL) {
              quitprintf("Plugin '%s' is an old incompatible version.", apl->filename);
          }
          apl->engineStartup = (void(*)(IAGSEngine*))apl->library.GetFunctionAddress("AGS_EngineStartup");
          apl->engineShutdown = (void(*)())apl->library.GetFunctionAddress("AGS_EngineShutdown");

          if (apl->engineStartup == NULL) {
              quitprintf("Plugin '%s' is not a valid AGS plugin (no engine startup entry point)", apl->filename);
          }
          apl->onEvent = (int(*)(int,int))apl->library.GetFunctionAddress("AGS_EngineOnEvent");
          apl->debugHook = (int(*)(const char*,int,int))apl->library.GetFunctionAddress("AGS_EngineDebugHook");
          apl->initGfxHook = (void(*)(const char*, void*))apl->library.GetFunctionAddress("AGS_EngineInitGfx");
        }
        else
        {
          AGS::Common::Debug::Printf("Plugin loading failed, trying built-in plugins...");
          if (!pl_use_builtin_plugin(apl))
          {
            // Plugin loading has failed at this point, try using built-in plugin function stubs
            if (RegisterPluginStubs((const char*)apl->filename))
              AGS::Common::Debug::Printf(kDbgMsg_Init, "Placeholder functions for the plugin '%s' found.", apl->filename);
            else
              AGS::Common::Debug::Printf(kDbgMsg_Init, "No placeholder functions for the plugin '%s' found. The game might fail to load.", apl->filename);

            continue;
          }
        }

        apl->eiface.pluginId = numPlugins - 1;
        apl->eiface.version = 24;
        apl->wantHook = 0;
        apl->available = true;
    }
    return kGameInitErr_NoError;
}

bool pl_is_plugin_loaded(const char *pl_name)
{
    if (!pl_name)
        return false;

    for (int i = 0; i < numPlugins; ++i)
    {
        if (stricmp(pl_name, plugins[i].filename) == 0)
            return plugins[i].available;
    }
    return false;
}
