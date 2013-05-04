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

#include "util/wgt2allg.h"
#include "plugin/agsplugin.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "ac/view.h"
#include "ac/charactercache.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/file.h"
#include "ac/gamestructdefines.h"
#include "ac/global_audio.h"
#include "ac/global_plugin.h"
#include "ac/global_walkablearea.h"
#include "ac/mouse.h"
#include "ac/movelist.h"
#include "ac/objectcache.h"
#include "ac/parser.h"
#include "ac/record.h"
#include "ac/string.h"
#include "font/fonts.h"
#include "game/game_objects.h"
#include "util/string_utils.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "gui/guidefines.h"
#include "main/engine.h"
#include "media/audio/audio.h"
#include "media/audio/sound.h"
#include "plugin/pluginobjectreader.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "ac/spritecache.h"
#include "util/stream.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "debug/out.h"
#include "ac/dynobj/scriptstring.h"

using AGS::Common::Stream;

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;


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
extern char dataDirectory[512];
extern char appDirectory[512];
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
extern int scrnwid,scrnhit;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int mousex, mousey;
extern int displayed_room;
extern SpriteCache spriteset;
extern ViewStruct*views;
extern int game_paused;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern int inside_script;
extern ccInstance *gameinst, *roominst;
extern Bitmap *virtual_screen;
extern int numlines;
extern char lines[MAXLINE][200];
extern color palette[256];
extern int offsetx, offsety;
extern PluginObjectReader pluginReaders[MAX_PLUGIN_OBJECT_READERS];
extern int numPluginReaders;
extern IAGSFontRenderer* fontRenderers[MAX_FONTS];
extern RuntimeScriptValue GlobalReturnValue;
extern ScriptString myScriptStringImpl;

// **************** PLUGIN IMPLEMENTATION ****************


#include "util/library.h"




struct EnginePlugin {
    char        filename[50];
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

void IAGSEngine::AbortGame (const char *reason) {
    quit ((char*)reason);
}
const char* IAGSEngine::GetEngineVersion () {
    return get_engine_version();
}
void IAGSEngine::RegisterScriptFunction (const char*name, void*addy) {
    ccAddExternalPluginFunction ((char*)name, addy);
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
    if (!gfxDriver->UsesMemoryBackBuffer())
        quit("!This plugin is not compatible with the Direct3D driver.");

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
    Common::Graphics *g = GetVirtualScreenGraphics();
    g->SetTextColor(color);
    draw_and_invalidate_text(g, x, y, font, text);
}
void IAGSEngine::GetScreenDimensions (int32 *width, int32 *height, int32 *coldepth) {
    if (width != NULL)
        width[0] = scrnwid;
    if (height != NULL)
        height[0] = scrnhit;
    if (coldepth != NULL)
        coldepth[0] = final_col_dep;
}
unsigned char ** IAGSEngine::GetRawBitmapSurface (BITMAP *bmp) {
    if (!is_linear_bitmap (bmp))
        quit("!IAGSEngine::GetRawBitmapSurface: invalid bitmap for access to surface");
    acquire_bitmap (bmp);

	if (bmp == virtual_screen->GetAllegroBitmap())
        plugins[this->pluginId].invalidatedRegion = 0;

    return bmp->line;
}
void IAGSEngine::ReleaseBitmapSurface (BITMAP *bmp) {
    release_bitmap (bmp);

	if (bmp == virtual_screen->GetAllegroBitmap()) {
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
    return thisroom.BkgSceneCount;
}
int IAGSEngine::GetCurrentBackground () {
    return play.RoomBkgFrameIndex;
}
BITMAP *IAGSEngine::GetBackgroundScene (int32 index) {
    return (BITMAP*)thisroom.Backgrounds[index].Graphic->GetAllegroBitmap();
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
    int texthit = wgetfontheight(font);

    break_up_text_into_lines (wid, font, (char*)text);

    Common::Graphics *g = GetVirtualScreenGraphics();
    g->SetTextColor(color);
    multiply_up_coordinates((int*)&xx, (int*)&yy); // stupid! quick tweak
    for (int i = 0; i < numlines; i++)
        draw_and_invalidate_text(g, xx, yy + texthit*i, font, lines[i]);
}
void IAGSEngine::SetVirtualScreen (BITMAP *bmp) {
	// [IKM] Very, very dangerous :'(
    SetVirtualScreenRaw (bmp);
}
int IAGSEngine::LookupParserWord (const char *word) {
    return find_word_in_dictionary ((char*)word);
}
void IAGSEngine::BlitBitmap (int32 x, int32 y, BITMAP *bmp, int32 masked) {
    wputblock_raw (GetVirtualScreenGraphics(), x, y, bmp, masked);
    invalidate_rect(x, y, x + bmp->w, y + bmp->h);
}
void IAGSEngine::BlitSpriteTranslucent(int32 x, int32 y, BITMAP *bmp, int32 trans) {
    set_trans_blender(0, 0, 0, trans);
    Common::Graphics *g = GetVirtualScreenGraphics();
    // FIXME: call corresponding Graphics Blit
	draw_trans_sprite((BITMAP*)g->GetBitmap()->GetAllegroBitmap(), bmp, x, y);
}
void IAGSEngine::BlitSpriteRotated(int32 x, int32 y, BITMAP *bmp, int32 angle) {
    Common::Graphics *g = GetVirtualScreenGraphics();
    // FIXME: call corresponding Graphics Blit
    rotate_sprite((BITMAP*)g->GetBitmap()->GetAllegroBitmap(), bmp, x, y, itofix(angle));
}

extern void domouse(int);
extern int  mgetbutton();

void IAGSEngine::PollSystem () {

    NEXT_ITERATION();
    domouse(DOMOUSE_NOCURSOR);
    if (!psp_audio_multithreaded)
      update_polled_stuff(true);
    int mbut = mgetbutton();
    if (mbut > NONE)
        pl_run_plugin_hooks (AGSE_MOUSECLICK, mbut);

    if (rec_kbhit()) {
        int kp = rec_getch();
        if (kp == 0) kp = rec_getch()+300;
        pl_run_plugin_hooks (AGSE_KEYPRESS, kp);
    }

}
AGSCharacter* IAGSEngine::GetCharacter (int32 charnum) {
    if (charnum >= game.CharacterCount)
        quit("!AGSEngine::GetCharacter: invalid character request");

    return (AGSCharacter*)&game.Characters[charnum];
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
    return game.CharacterCount;
}
int IAGSEngine::GetPlayerCharacter () {
    return game.PlayerCharacterIndex;
}
void IAGSEngine::RoomToViewport (int32 *x, int32 *y) {
    if (x)
        x[0] = multiply_up_coordinate(x[0]) - offsetx;
    if (y)
        y[0] = multiply_up_coordinate(y[0]) - offsety;
}
void IAGSEngine::ViewportToRoom (int32 *x, int32 *y) {
    if (x)
        x[0] = divide_down_coordinate(x[0] + offsetx);
    if (y)
        y[0] = divide_down_coordinate(y[0] + offsety);
}
int IAGSEngine::GetNumObjects () {
    return croom->ObjectCount;
}
AGSObject *IAGSEngine::GetObject (int32 num) {
    if (num >= croom->ObjectCount)
        quit("!IAGSEngine::GetObject: invalid object");

    return (AGSObject*)&croom->Objects[num];
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
    return NULL;
}
AGSViewFrame *IAGSEngine::GetViewFrame (int32 view, int32 loop, int32 frame) {
    view--;
    if ((view < 0) || (view >= game.ViewCount))
        quit("!IAGSEngine::GetViewFrame: invalid view");
    if ((loop < 0) || (loop >= views[view].numLoops))
        quit("!IAGSEngine::GetViewFrame: invalid loop");
    if ((frame < 0) || (frame >= views[view].loops[loop].numFrames))
        return NULL;

    return (AGSViewFrame*)&views[view].loops[loop].frames[frame];
}
int IAGSEngine::GetRawPixelColor (int32 color) {
    // Convert the standardized colour to the local gfx mode color
    int result;
    __my_setcolor(&result, color, GetVirtualScreenBitmap()->GetColorDepth());

    return result;
}
int IAGSEngine::GetWalkbehindBaseline (int32 wa) {
    if ((wa < 1) || (wa >= MAX_OBJ))
        quit("!IAGSEngine::GetWalkBehindBase: invalid walk-behind area specified");
    return croom->WalkBehinds[wa].Baseline;
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
    return spritewidth[slot];
}
int IAGSEngine::GetSpriteHeight (int32 slot) {
    return spriteheight[slot];
}
void IAGSEngine::GetTextExtent (int32 font, const char *text, int32 *width, int32 *height) {
    if ((font < 0) || (font >= game.FontCount)) {
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
    DEBUG_CONSOLE("[PLUGIN] %s", text);
    printf("[PLUGIN] %s", text);
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

    if (soundType == PSND_WAVE)
        newcha = my_load_wave (filename, volume, loop);
    else if (soundType == PSND_MP3STREAM)
        newcha = my_load_mp3 (filename, volume);
    else if (soundType == PSND_OGGSTREAM)
        newcha = my_load_ogg (filename, volume);
    else if (soundType == PSND_MP3STATIC)
        newcha = my_load_static_mp3 (filename, volume, (loop != 0));
    else if (soundType == PSND_OGGSTATIC)
        newcha = my_load_static_ogg (filename, volume, (loop != 0));
    else if (soundType == PSND_MIDI) {
        if (midi_pos >= 0)
            quit("!IAGSEngine::PlaySoundChannel: MIDI already in use");
        newcha = my_load_midi (filename, loop);
        newcha->set_volume (volume);
    }
#ifndef PSP_NO_MOD_PLAYBACK
    else if (soundType == PSND_MOD) {
        newcha = my_load_mod (filename, loop);
        newcha->set_volume (volume);
    }
#endif
    else
        quit("!IAGSEngine::PlaySoundChannel: unknown sound type");

    channels[channel] = newcha;
}
// Engine interface 12 and above are below
void IAGSEngine::MarkRegionDirty(int32 left, int32 top, int32 right, int32 bottom) {
    invalidate_rect(left, top, right, bottom);
    plugins[this->pluginId].invalidatedRegion++;
}
AGSMouseCursor * IAGSEngine::GetMouseCursor(int32 cursor) {
    if ((cursor < 0) || (cursor >= game.MouseCursorCount))
        return NULL;

    return (AGSMouseCursor*)&game.MouseCursors[cursor];
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
    if ((fontNum < 0) || (fontNum >= game.FontCount))
        return FNT_INVALID;

    if (fontRenderers[fontNum]->SupportsExtendedCharacters(fontNum))
        return FNT_TTF;

    return FNT_SCI;
}
int IAGSEngine::CreateDynamicSprite(int32 coldepth, int32 width, int32 height) {

    int gotSlot = spriteset.findFreeSlot();
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
    if (game.SpriteFlags[slot] & SPF_ALPHACHANNEL)
        return 1;
    return 0;
}

// disable AGS's sound engine
void IAGSEngine::DisableSound() {
    shutdown_sound();
    usetup.DigitalSoundCard = DIGI_NONE;
    usetup.MidiSoundCard = MIDI_NONE;
    install_sound(usetup.DigitalSoundCard,usetup.MidiSoundCard,NULL);
}
int IAGSEngine::CanRunScriptFunctionNow() {
    if (inside_script)
        return 0;
    return 1;
}
int IAGSEngine::CallGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1, long arg2, long arg3) {
    if (inside_script)
        return -300;

    ccInstance *toRun = gameinst;
    if (!globalScript)
        toRun = roominst;

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
    for (ff = 0; ff < charcache.GetCount(); ff++) {
        if ((charcache[ff].inUse) && (charcache[ff].sppic == slot)) {
            delete charcache[ff].image;
            charcache[ff].image = NULL;
            charcache[ff].inUse = 0;
        }
    }

    // clear the object cache
    for (ff = 0; ff < objcache.GetCount(); ff++) {
        if ((objcache[ff].image != NULL) && (objcache[ff].sppic == slot)) {
            delete objcache[ff].image;
            objcache[ff].image = NULL;
        }
    }
}

void IAGSEngine::SetSpriteAlphaBlended(int32 slot, int32 isAlphaBlended) {

    game.SpriteFlags[slot] &= ~SPF_ALPHACHANNEL;

    if (isAlphaBlended)
        game.SpriteFlags[slot] |= SPF_ALPHACHANNEL;
}

void IAGSEngine::QueueGameScriptFunction(const char *name, int32 globalScript, int32 numArgs, long arg1, long arg2) {
    if (!inside_script) {
        this->CallGameScriptFunction(name, globalScript, numArgs, arg1, arg2, 0);
        return;
    }

    // queue it up, baby!
    char scNameToRun[100];
    if (globalScript) {

        if (numArgs == 0) {
            strcpy(scNameToRun, "");
        }
        else if (numArgs == 1) {
            strcpy(scNameToRun, "!");
        }
        else if (numArgs == 2) {
            strcpy(scNameToRun, "#");
        }
        else
            quit("IAGSEngine::QueueGameScriptFunction: invalid number of arguments");
    }
    else {
        // room script
        if (numArgs == 0) {
            strcpy(scNameToRun, "|");
        }
        else if (numArgs == 1) {
            strcpy(scNameToRun, "$");
        }
        else if (numArgs == 2) {
            strcpy(scNameToRun, "%");
        }
        else
            quit("IAGSEngine::QueueGameScriptFunction: invalid number of arguments");

    }
    strcat(scNameToRun, name);

    curscript->run_another(scNameToRun, RuntimeScriptValue().SetPluginArgument(arg1), RuntimeScriptValue().SetPluginArgument(arg2));
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
    filter->SetMousePosition(x, y);
    RefreshMouse();
}

void IAGSEngine::SimulateMouseClick(int32 button) {
    PluginSimulateMouseClick(button);
}

int IAGSEngine::GetMovementPathWaypointCount(int32 pathId) {
    pathId = pathId % TURNING_AROUND;
    if (pathId >= CHMLSOFFS)
    {
        return CharMoveLists[pathId - CHMLSOFFS + 1].numstage;
    }
    return ObjMoveLists[pathId].numstage;
}

int IAGSEngine::GetMovementPathLastWaypoint(int32 pathId) {
    pathId = pathId % TURNING_AROUND;
    if (pathId >= CHMLSOFFS)
    {
        return CharMoveLists[pathId - CHMLSOFFS + 1].onstage;
    }
    return ObjMoveLists[pathId].onstage;
}

void IAGSEngine::GetMovementPathWaypointLocation(int32 pathId, int32 waypoint, int32 *x, int32 *y) {
    pathId = pathId % TURNING_AROUND;
    const MoveList &movelist = pathId >= CHMLSOFFS ? CharMoveLists[pathId - CHMLSOFFS + 1] : ObjMoveLists[pathId];
    *x = (movelist.pos[waypoint] >> 16) & 0x0000ffff;
    *y = (movelist.pos[waypoint] & 0x0000ffff);
}

void IAGSEngine::GetMovementPathWaypointSpeed(int32 pathId, int32 waypoint, int32 *xSpeed, int32 *ySpeed) {
    pathId = pathId % TURNING_AROUND;
    const MoveList &movelist = pathId >= CHMLSOFFS ? CharMoveLists[pathId - CHMLSOFFS + 1] : ObjMoveLists[pathId];
    *xSpeed = movelist.xpermove[waypoint];
    *ySpeed = movelist.ypermove[waypoint];
}

int IAGSEngine::IsRunningUnderDebugger()
{
    return (editor_debugging_enabled != 0) ? 1 : 0;
}

void IAGSEngine::GetPathToFileInCompiledFolder(const char*fileName, char *buffer)
{
    get_current_dir_path(buffer, fileName);
}

void IAGSEngine::BreakIntoDebugger()
{
    break_on_next_script_step = 1;
}

IAGSFontRenderer* IAGSEngine::ReplaceFontRenderer(int fontNumber, IAGSFontRenderer *newRenderer)
{
    IAGSFontRenderer* oldOne = fontRenderers[fontNumber];
    fontRenderers[fontNumber] = newRenderer;
    return oldOne;
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
    int i, retval = 0;
    for (i = 0; i < numPlugins; i++) 
    {
        if (plugins[i].initGfxHook != NULL)
        {
            plugins[i].initGfxHook(driverName, data);
        }
    }
}

bool pl_use_builtin_plugin(EnginePlugin* apl)
{
#if defined(BUILTIN_PLUGINS)
    if (strncmp(apl->filename, "agsflashlight", strlen("agsflashlight")) == 0)
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
    else if (strncmp(apl->filename, "agsblend", strlen("agsblend")) == 0)
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
    else if (strncmp(apl->filename, "ags_snowrain", strlen("ags_snowrain")) == 0)
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
    else if (strncmp(apl->filename, "ags_parallax", strlen("ags_parallax")) == 0)
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
    else if (strncmp(apl->filename, "agstouch", strlen("agstouch")) == 0)
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

    AGS::Common::Out::FPrint("No built-in plugin found. Plugin loading failed!");
    return false;
}

void pl_read_plugins_from_disk (Stream *in) {
    if (in->ReadInt32() != 1)
        quit("ERROR: unable to load game, invalid version of plugin data");

    int a, datasize;
    char buffer[200];
    numPlugins = in->ReadInt32();

    if (numPlugins > MAXPLUGINS)
        quit("Too many plugins used by this game");

    for (a = 0; a < numPlugins; a++) {
        // read the plugin name
        fgetstring (buffer, in);
        datasize = in->ReadInt32();

        if (buffer[strlen(buffer) - 1] == '!') {
            // editor-only plugin, ignore it
            in->Seek(Common::kSeekCurrent, datasize);
            a--;
            numPlugins--;
            continue;
        }

        // just check for silly datasizes
        if ((datasize < 0) || (datasize > 10247680))
            quit("Too much plugin save data for this engine");

        // load the actual plugin from disk
        EnginePlugin *apl = &plugins[a];
        strncpy(apl->filename, buffer, strlen(buffer) - strlen(".dll"));

        // Compatibility with the old SnowRain module
        if (stricmp(apl->filename, "ags_SnowRain20") == 0)
            strcpy(apl->filename, "ags_snowrain");

        if (apl->library.Load(apl->filename))
        {
          AGS::Common::Out::FPrint("Plugin loading succeeded, resolving imports...");

          if (apl->library.GetFunctionAddress("AGS_PluginV2") == NULL) {
              sprintf(buffer, "Plugin '%s' is an old incompatible version.", apl->filename);
              quit(buffer);
          }
          apl->engineStartup = (void(*)(IAGSEngine*))apl->library.GetFunctionAddress("AGS_EngineStartup");
          apl->engineShutdown = (void(*)())apl->library.GetFunctionAddress("AGS_EngineShutdown");

          if (apl->engineStartup == NULL) {
              sprintf(buffer, "Plugin '%s' is not a valid AGS plugin (no engine startup entry point)", apl->filename);
              quit(buffer);
          }
          apl->onEvent = (int(*)(int,int))apl->library.GetFunctionAddress("AGS_EngineOnEvent");
          apl->debugHook = (int(*)(const char*,int,int))apl->library.GetFunctionAddress("AGS_EngineDebugHook");
          apl->initGfxHook = (void(*)(const char*, void*))apl->library.GetFunctionAddress("AGS_EngineInitGfx");
        }
        else
        {
          AGS::Common::Out::FPrint("Plugin loading failed, trying built-in plugins...");
          strlwr(apl->filename);
          if (!pl_use_builtin_plugin(apl))
          {
            // Plugin loading has failed at this point, try using built-in plugin function stubs
            if (RegisterPluginStubs((const char*)apl->filename))
              AGS::Common::Out::FPrint("Placeholder functions for the plugin found.");
            else
              AGS::Common::Out::FPrint("No placeholder functions for the plugin found. The game might fail to load.");

            continue;
          }
        }

        if (datasize > 0) {
            apl->savedata = (char*)malloc(datasize);
            in->Read (apl->savedata, datasize);
        }
        apl->savedatasize = datasize;
        apl->eiface.pluginId = a;
        apl->eiface.version = 24;
        apl->wantHook = 0;

        apl->available = true;
    }

}
