/*
  AGS Runtime header

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/
#ifndef __ACRUNTIME_H
#define __ACRUNTIME_H

#include "bigend.h"
#include "sprcache.h"
//#include "acsound.h"
//#include "acgfx.h"

#include "ali3d.h"

#include "ac/ac_gamesetupstruct.h"  // constants
#include "ac/ac_move.h"             // MoveList
#include "ac/ac_room.h"             // constants
#include "ac/ac_view.h"             // ViewStruct
#include "cs/cc_dynamicobject.h"    // ICCDynamicObject
#include "cs/cc_instance.h"         // ccInstance



extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];









// The text script's "mouse" struct
struct ScriptMouse {
  int x,y;
};

// The text script's "system" struct
struct ScriptSystem {
  int width,height;
  int coldepth;
  int os;
  int windowed;
  int vsync;
  int viewport_width, viewport_height;
  char aci_version[10];
  int reserved[5];  // so that future scripts don't overwrite data
};

struct AnimatingGUIButton {
  // index into guibuts array, GUI, button
  short buttonid, ongui, onguibut;
  // current animation status
  short view, loop, frame;
  short speed, repeat, wait;
};

struct EventHappened {
  int type;
  int data1,data2,data3;
  int player;
};

struct SpriteListEntry {
  IDriverDependantBitmap *bmp;
  block pic;
  int baseline;
  int x,y;
  int transparent;
  bool takesPriorityIfEqual;
  bool hasAlphaChannel;
};



#define BASEWIDTH usetup.base_width
#define BASEHEIGHT usetup.base_height
#define TRANS_ALPHA_CHANNEL 20000
#define TRANS_OPAQUE        20001
#define TRANS_RUN_PLUGIN    20002

struct ScreenOverlay {
  IDriverDependantBitmap *bmp;
  block pic;
  int type,x,y,timeout;
  int bgSpeechForChar;
  int associatedOverlayHandle;
  bool hasAlphaChannel;
  bool positionRelativeToScreen;
};

struct ScriptObject {
  int id;
  RoomObject *obj;
};


#include "acrun/ac_ccdynamicobject.h"


struct ScriptOverlay : AGSCCDynamicObject {
  int overlayId;
  int borderWidth;
  int borderHeight;
  int isBackgroundSpeech;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);
  void Remove();
  ScriptOverlay();
};

struct ScriptDateTime : AGSCCDynamicObject {
  int year, month, day;
  int hour, minute, second;
  int rawUnixTime;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);

  ScriptDateTime();
};

struct ScriptDrawingSurface : AGSCCDynamicObject {
  int roomBackgroundNumber;
  int dynamicSpriteNumber;
  int dynamicSurfaceNumber;
  bool isLinkedBitmapOnly;
  BITMAP *linkedBitmapOnly;
  int currentColour;
  int currentColourScript;
  int highResCoordinates;
  int modified;
  int hasAlphaChannel;
  BITMAP* abufBackup;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);
  BITMAP* GetBitmapSurface();
  void StartDrawing();
  void MultiplyThickness(int *adjustValue);
  void UnMultiplyThickness(int *adjustValue);
  void MultiplyCoordinates(int *xcoord, int *ycoord);
  void FinishedDrawing();
  void FinishedDrawingReadOnly();

  ScriptDrawingSurface();
};

struct ScriptViewFrame : AGSCCDynamicObject {
  int view, loop, frame;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);

  ScriptViewFrame(int p_view, int p_loop, int p_frame);
  ScriptViewFrame();
};

struct ScriptDynamicSprite : AGSCCDynamicObject {
  int slot;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);

  ScriptDynamicSprite(int slot);
  ScriptDynamicSprite();
};

struct ScriptString : AGSCCDynamicObject, ICCStringClass {
  char *text;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);

  virtual void* CreateString(const char *fromText);

  ScriptString();
  ScriptString(const char *fromText);
};



struct CharacterExtras {
  // UGLY UGLY UGLY!! The CharacterInfo struct size is fixed because it's
  // used in the scripts, therefore overflowing stuff has to go here
  short invorder[MAX_INVORDER];
  short invorder_count;
  short width,height;
  short zoom;
  short xwas, ywas;
  short tint_r, tint_g;
  short tint_b, tint_level;
  short tint_light;
  char  process_idle_this_time;
  char  slow_move_counter;
  short animwait;
};

#include "acaudio/ac_soundclip.h"








struct ScriptDialog {
  int id;
  int reserved;
};


// object-based File routine -- struct definition
#define scFileRead   1
#define scFileWrite  2
#define scFileAppend 3
extern const char *fopenModes[];

struct sc_File : ICCDynamicObject {
  FILE *handle;

  virtual int Dispose(const char *address, bool force) {
    Close();
    delete this;
    return 1;
  }

  virtual const char *GetType() {
    return "File";
  }

  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    // we cannot serialize an open file, so it will get closed
    return 0;
  }

  int OpenFile(const char *filename, int mode);
  void Close();

  sc_File() {
    handle = NULL;
  }
};


// stores cached info about the character
struct CharacterCache {
  block image;
  int sppic;
  int scaling;
  int inUse;
  short tintredwas, tintgrnwas, tintbluwas, tintamntwas;
  short lightlevwas, tintlightwas;
  // no mirroredWas is required, since the code inverts the sprite number
};

// stores cached object info
struct ObjectCache {
  block image;
  int   sppic;
  short tintredwas, tintgrnwas, tintbluwas, tintamntwas, tintlightwas;
  short lightlevwas, mirroredWas, zoomWas;
  // The following are used to determine if the character has moved
  int   xwas, ywas;
};

enum PostScriptAction {
  ePSANewRoom,
  ePSAInvScreen,
  ePSARestoreGame,
  ePSARestoreGameDialog,
  ePSARunAGSGame,
  ePSARunDialog,
  ePSARestartGame,
  ePSASaveGame,
  ePSASaveGameDialog
};

#define MAX_QUEUED_SCRIPTS 4
#define MAX_QUEUED_ACTIONS 5
struct ExecutingScript {
  ccInstance *inst;
  PostScriptAction postScriptActions[MAX_QUEUED_ACTIONS];
  const char *postScriptActionNames[MAX_QUEUED_ACTIONS];
  char postScriptSaveSlotDescription[MAX_QUEUED_ACTIONS][100];
  int  postScriptActionData[MAX_QUEUED_ACTIONS];
  int  numPostScriptActions;
  char script_run_another[MAX_QUEUED_SCRIPTS][30];
  int  run_another_p1[MAX_QUEUED_SCRIPTS];
  int  run_another_p2[MAX_QUEUED_SCRIPTS];
  int  numanother;
  char forked;

  int queue_action(PostScriptAction act, int data, const char *aname);
  void run_another (char *namm, int p1, int p2);
  void init();
  ExecutingScript();
};

#ifndef _AGS_PLUGIN_H
#define IAGSManagedObjectReader void
#endif

struct PluginObjectReader {
  IAGSManagedObjectReader *reader;
  const char *type;
};

#ifndef _AGS_PLUGIN_H
#undef IAGSManagedObjectReader
#endif

enum eScriptSystemOSID {
  eOS_DOS = 1,
  eOS_Win = 2,
  eOS_Linux = 3,
  eOS_Mac = 4
};

struct AGSPlatformDriver {
  virtual void AboutToQuitGame();
  virtual void Delay(int millis) = 0;
  virtual void DisplayAlert(const char*, ...) = 0;
  virtual const char *GetAllUsersDataDirectory() { return NULL; }
  virtual unsigned long GetDiskFreeSpaceMB() = 0;
  virtual const char* GetNoMouseErrorString() = 0;
  virtual eScriptSystemOSID GetSystemOSID() = 0;
  virtual void GetSystemTime(ScriptDateTime*) = 0;
  virtual void PlayVideo(const char* name, int skip, int flags) = 0;
  virtual void InitialiseAbufAtStartup();
  virtual void PostAllegroInit(bool windowed);
  virtual void PostAllegroExit() = 0;
  virtual void FinishedUsingGraphicsMode();
  virtual void ReplaceSpecialPaths(const char *sourcePath, char *destPath) = 0;
  virtual int  RunSetup() = 0;
  virtual void SetGameWindowIcon();
  virtual void WriteConsole(const char*, ...) = 0;
  virtual void WriteDebugString(const char*, ...);
  virtual void YieldCPU() = 0;
  virtual void DisplaySwitchOut();
  virtual void DisplaySwitchIn();
  virtual void RegisterGameWithGameExplorer();
  virtual void UnRegisterGameWithGameExplorer();
  virtual int  ConvertKeycodeToScanCode(int keyCode);

  virtual int  InitializeCDPlayer() = 0;  // return 0 on success
  virtual int  CDPlayerCommand(int cmdd, int datt) = 0;
  virtual void ShutdownCDPlayer() = 0;

  virtual void ReadPluginsFromDisk(FILE *);
  virtual void StartPlugins();
  virtual int  RunPluginHooks(int event, int data);
  virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
  virtual int  RunPluginDebugHooks(const char *scriptfile, int linenum);
  virtual void ShutdownPlugins();

  static AGSPlatformDriver *GetDriver();

private:
  static AGSPlatformDriver *instance;
};

extern AGSPlatformDriver *platform;


extern GFXFilter *filter;




#define NUM_DIGI_VOICES     16
#define NUM_MOD_DIGI_VOICES 12

#define DEBUG_CONSOLE_NUMLINES 6
#define TXT_SCOREBAR        29
#define MAXSCORE play.totalscore
#define CHANIM_REPEAT    2
#define CHANIM_BACKWARDS 4
#define ANIM_BACKWARDS 10
#define ANIM_ONCE      1
#define ANIM_REPEAT    2
#define ANIM_ONCERESET 3
#define FONT_STATUSBAR  0
#define FONT_NORMAL     play.normal_font
//#define FONT_SPEECHBACK 1
#define FONT_SPEECH     play.speech_font
#define MODE_WALK 0
#define MODE_LOOK 1
#define MODE_HAND 2
#define MODE_TALK 3
#define MODE_USE  4
#define MODE_PICKUP 5
#define CURS_ARROW  6
#define CURS_WAIT   7
#define MODE_CUSTOM1 8
#define MODE_CUSTOM2 9

#define OVER_TEXTMSG  1
#define OVER_COMPLETE 2
#define OVER_PICTURE  3
#define OVER_CUSTOM   100
#define OVR_AUTOPLACE 30000
#define FOR_ANIMATION 1
#define FOR_SCRIPT    2
#define FOR_EXITLOOP  3
#define opts usetup
#define CHMLSOFFS (MAX_INIT_SPR+1)    // reserve this many movelists for objects & stuff
#define MAX_SCREEN_OVERLAYS 20
#define abort_all_conditions restrict_until
#define MAX_SCRIPT_AT_ONCE 10
#define EVENT_NONE       0
#define EVENT_INPROGRESS 1
#define EVENT_CLAIMED    2

#define SKIP_AUTOTIMER  1
#define SKIP_KEYPRESS   2
#define SKIP_MOUSECLICK 4

#define UNTIL_ANIMEND   1
#define UNTIL_MOVEEND   2
#define UNTIL_CHARIS0   3
#define UNTIL_NOOVERLAY 4
#define UNTIL_NEGATIVE  5
#define UNTIL_INTIS0    6
#define UNTIL_SHORTIS0  7
#define UNTIL_INTISNEG  8
#define MANOBJNUM 99

#define STD_BUFFER_SIZE 3000

#define TURNING_AROUND     1000
#define TURNING_BACKWARDS 10000


#define MAX_PLUGIN_OBJECT_READERS 50

#define NEXT_ITERATION() play.gamestep++

#include "acgui/ac_guidefines.h"
#include "acaudio/ac_audiochannel.h"
#include "acrun/ac_inventoryitem.h"

extern GameSetupStruct game;
extern GameState play;
extern GameSetup usetup;
extern CharacterExtras *charextra;
extern MoveList *mls;
extern ViewStruct *views;
extern roomstruct thisroom;
extern RoomObject*objs;
extern CharacterInfo*playerchar;
extern int displayed_room;
extern int final_scrn_wid,final_scrn_hit,final_col_dep;
extern int turnlooporder[8];
extern int in_enters_screen, done_es_error;
extern int new_room_pos, new_room_x, new_room_y;
extern int scrnwid, scrnhit;
extern int cur_mode,cur_cursor;
extern block wallscreen;
extern AmbientSound ambient[MAX_SOUND_CHANNELS + 1];
extern int lastcx,lastcy;
extern int guis_need_update;
extern int use_cdplayer, need_to_stop_cd;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern ScriptInvItem scrInv[MAX_INV];
extern color palette[256];
extern block virtual_screen; 
extern int pluginsWantingDebugHooks;
extern int pluginsWantingAudioHooks;
extern char lines[MAXLINE][200];
extern int  numlines,longestline;
extern int offsetx,offsety;     // for scumm-type scrolling rooms
extern int game_paused;
extern int inside_script;
extern int numPluginReaders;
extern int our_eip;
extern ccInstance *gameinst, *roominst;
extern RoomStatus *croom;
extern SpriteCache spriteset;
extern SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];
extern int last_sound_played[MAX_SOUND_CHANNELS+1];
extern CharacterCache *charcache;
extern ObjectCache objcache[MAX_INIT_SPR];
extern ExecutingScript *curscript;
extern PluginObjectReader pluginReaders[MAX_PLUGIN_OBJECT_READERS];

#define DOMOUSE_NOCURSOR 5
#define NONE -1
#define LEFT  0
#define RIGHT 1
extern int  mousex,mousey;
extern void domouse(int);
extern int  mgetbutton();

extern const char *get_engine_version();
extern void quitprintf(char*texx, ...) ;
extern void RefreshMouse();
extern void PluginSimulateMouseClick(int pluginButtonID);
extern int  run_script_function_if_exist(ccInstance*sci,char*tsname,int numParam, int iparam, int iparam2, int iparam3 = 0) ;
extern int  IsChannelPlaying(int chan) ;
extern void stop_and_destroy_channel (int chid) ;
extern int  rec_kbhit();
extern int  rec_getch();
extern void update_polled_stuff(bool checkForDebugMessages);
extern void invalidate_rect(int x1, int y1, int x2, int y2);
extern int  find_word_in_dictionary (char*);
extern void break_up_text_into_lines(int wii,int fonnt,char*todis) ;
extern int  wgetfontheight(int font);
extern void draw_and_invalidate_text(int x1, int y1, int font, const char *text);
extern void scriptDebugHook (ccInstance *ccinst, int linenum);
extern void invalidate_screen();
extern int  is_valid_character(int newchar);
extern int  is_valid_object(int obtest);
extern void debug_write_console (char *msg, ...);
extern int  is_route_possible(int,int,int,int,block);
extern int  find_route(short,short,short,short,block,int,int=0,int=0);
extern void set_route_move_speed(int x, int y);
extern void init_pathfinder();
extern block prepare_walkable_areas (int sourceChar);
extern void do_main_cycle(int,int);
extern void add_inventory(int inum);
extern void lose_inventory(int inum);
extern void animate_character(CharacterInfo *,int,int,int,int = 0, int = 0);
extern void calculate_move_stage (MoveList *, int );
extern void EndSkippingUntilCharStops();
extern void MoveToWalkableArea(int charid);
extern void Display(char*, ...);
extern int  do_movelist_move(short*,int*,int*);
extern int  is_char_on_another (int sourceChar, int ww, int*fromxptr, int*cwidptr);
extern int  find_looporder_index (int curloop);
extern int  doNextCharMoveStep (int aa, CharacterInfo *chi);
extern int  useDiagonal (CharacterInfo *char1);
extern void FaceCharacter(int cha,int toface);
extern void NewRoom(int);
extern void debug_log(char*texx, ...);
extern int  GetCharacterWidth(int ww);
extern block GetObjectImage(int obj, int *isFlipped);
extern block GetCharacterImage(int charid, int *isFlipped);
extern int  my_getpixel(BITMAP *blk, int x, int y);
extern void update_invorder();
extern int  GetRegionAt (int xxx, int yyy);
extern void SetNextCursor();
extern void SetActiveInventory(int iit);
extern void _DisplaySpeechCore(int chid, char *displbuf);
extern void _DisplayThoughtCore(int chid, const char *displbuf);
extern int  DisplaySpeechBackground(int charid,char*speel);
extern void DisplaySpeechAt (int xx, int yy, int wii, int aschar, char*spch);
extern char *get_translation(const char*);
extern int  can_see_from(int,int,int,int);
extern int  GetCursorMode();
extern void disable_cursor_mode(int);
extern void enable_cursor_mode(int);
extern void set_cursor_mode(int);
extern void update_inv_cursor(int invnum);
extern void setup_player_character(int charid);
extern void my_sprintf(char *buffer, const char *fmt, va_list ap);
extern int  find_overlay_of_type(int typ);
extern void run_on_event (int evtype, int wparam);
extern int  get_character_currently_talking();
extern void shutdown_sound();
extern void update_music_volume();
extern void newmusic(int);
extern void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y);
extern void* ccGetSymbolAddress (char*);
extern int GetScalingAt (int x, int y) ;
extern int wgettextwidth_compensate(const char *tex, int font) ;
extern void add_dynamic_sprite (int gotSlot, block redin, bool hasAlpha = false);
extern void free_dynamic_sprite (int gotSlot);
const char* CreateNewScriptString(const char *fromText, bool reAllocate = true);
extern void convert_move_path_to_high_res(MoveList *ml);
extern void register_audio_script_objects();
extern void register_audio_script_functions();
extern bool unserialize_audio_script_object(int index, const char *objectType, const char *serializedData, int dataSize);
extern void audio_update_polled_stuff();
extern ScriptAudioChannel* play_audio_clip_on_channel(int channel, ScriptAudioClip *clip, int priority, int repeat, int fromOffset, SOUNDCLIP *cachedClip = NULL);
extern int get_old_style_number_for_sound(int sound_number);
extern SOUNDCLIP *load_sound_clip_from_old_style_number(bool isMusic, int indexNumber, bool repeat);
extern void play_audio_clip_by_index(int audioClipIndex);
extern ScriptAudioClip* get_audio_clip_for_old_style_number(bool isMusic, int indexNumber);
extern void Game_StopAudio(int audioType);
extern void Game_SetAudioTypeVolume(int audioType, int volume, int changeType);
extern void update_directional_sound_vol();
extern void recache_queued_clips_after_loading_save_game();

#define AMBIENCE_FULL_DIST 25

// parameters to run_on_event
#define GE_LEAVE_ROOM 1
#define GE_ENTER_ROOM 2
#define GE_MAN_DIES   3
#define GE_GOT_SCORE  4
#define GE_GUI_MOUSEDOWN 5
#define GE_GUI_MOUSEUP   6
#define GE_ADD_INV       7
#define GE_LOSE_INV      8
#define GE_RESTORE_GAME  9

// These numbers were chosen arbitrarily -- the idea is
// to make sure that the user gets the parameters the right way round
#define ANYWHERE       304
#define WALKABLE_AREAS 305
#define BLOCKING       919
#define IN_BACKGROUND  920
#define FORWARDS       1062
#define BACKWARDS      1063

#define SCR_NO_VALUE   31998
#define SCR_COLOR_TRANSPARENT -1

// Character methods
extern void Character_AddInventory(CharacterInfo *chaa, ScriptInvItem *, int addIndex);
extern void Character_AddWaypoint(CharacterInfo *chaa, int x, int y);
extern void Character_Animate(CharacterInfo *chaa, int loop, int delay, int repeat, int direction, int blocking);
extern void Character_ChangeRoom(CharacterInfo *chaa, int room, int x, int y);
extern void Character_ChangeRoomAutoPosition(CharacterInfo *chaa, int room, int newPos);
extern void Character_ChangeView(CharacterInfo *chap, int vii);
extern void Character_FaceCharacter(CharacterInfo *char1, CharacterInfo *otherChar, int blockingStyle);
extern void Character_FaceLocation(CharacterInfo *char1, int xx, int yy, int blockingStyle);
extern void Character_FaceObject(CharacterInfo *char1, ScriptObject* obj, int blockingStyle);
extern void Character_FollowCharacter(CharacterInfo *chaa, CharacterInfo *tofollow, int distaway, int eagerness);
extern int  Character_GetProperty(CharacterInfo *chaa, const char *property);
extern void Character_GetPropertyText(CharacterInfo *chaa, const char *property, char *bufer);
extern int  Character_HasInventory(CharacterInfo *chaa, ScriptInvItem *);
extern int  Character_IsCollidingWithChar(CharacterInfo *char1, CharacterInfo *cchar2) ;
extern int  Character_IsCollidingWithObject(CharacterInfo *chin, ScriptObject *objid);
extern void Character_LockView(CharacterInfo *chap, int vii);
extern void Character_LockViewAligned(CharacterInfo *chap, int vii, int loop, int align);
extern void Character_LockViewFrame(CharacterInfo *chaa, int view, int loop, int frame);
extern void Character_LockViewOffset(CharacterInfo *chap, int vii, int xoffs, int yoffs);
extern void Character_LoseInventory(CharacterInfo *chap, ScriptInvItem *);
extern void Character_Move(CharacterInfo *chaa, int x, int y, int direct, int blocking);
extern void Character_PlaceOnWalkableArea(CharacterInfo *chap);
extern void Character_RemoveTint(CharacterInfo *chaa);
extern void Character_RunInteraction(CharacterInfo *chaa, int mood);
extern void Character_Say(CharacterInfo *chaa, const char *texx, ...);
extern void Character_SayAt(CharacterInfo *chaa, int x, int y, int width, const char *texx);
extern ScriptOverlay* Character_SayBackground(CharacterInfo *chaa, const char *texx);
extern void Character_SetAsPlayer(CharacterInfo *chaa);
extern void Character_SetIdleView(CharacterInfo *chaa, int iview, int itime);
extern void Character_SetOption(CharacterInfo *chaa, int option, int yesorno);
extern void Character_SetSpeed(CharacterInfo *chaa, int xspeed, int yspeed);
extern void Character_StopMoving(CharacterInfo *charp);
extern void Character_Think(CharacterInfo *chaa, const char *texx, ...);
extern void Character_Tint(CharacterInfo *chaa, int red, int green, int blue, int opacity, int luminance);
extern void Character_UnlockView(CharacterInfo *chaa);
extern void Character_Walk(CharacterInfo *chaa, int x, int y, int direct, int blocking);
extern void Character_WalkStraight(CharacterInfo *chaa, int x, int y, int blocking);

// Character properties
extern ScriptInvItem* Character_GetActiveInventory(CharacterInfo *chaa);
extern void Character_SetActiveInventory(CharacterInfo *chaa, ScriptInvItem *iit);
extern int  Character_GetAnimating(CharacterInfo *chaa);
extern int  Character_GetAnimationSpeed(CharacterInfo *chaa) ;
extern void Character_SetAnimationSpeed(CharacterInfo *chaa, int newval);
extern int  Character_GetBaseline(CharacterInfo *chaa);
extern void Character_SetBaseline(CharacterInfo *chaa, int basel);
extern int  Character_GetBlinkInterval(CharacterInfo *chaa);
extern void Character_SetBlinkInterval(CharacterInfo *chaa, int interval);
extern int  Character_GetBlinkView(CharacterInfo *chaa);
extern void Character_SetBlinkView(CharacterInfo *chaa, int vii);
extern int  Character_GetBlinkWhileThinking(CharacterInfo *chaa);
extern void Character_SetBlinkWhileThinking(CharacterInfo *chaa, int yesOrNo);
extern int  Character_GetBlockingHeight(CharacterInfo *chaa);
extern void Character_SetBlockingHeight(CharacterInfo *chaa, int hit);
extern int  Character_GetBlockingWidth(CharacterInfo *chaa);
extern void Character_SetBlockingWidth(CharacterInfo *chaa, int wid);
extern int  Character_GetClickable(CharacterInfo *chaa);
extern void Character_SetClickable(CharacterInfo *chaa, int clik);
extern int  Character_GetDiagonalWalking(CharacterInfo *chaa);
extern void Character_SetDiagonalWalking(CharacterInfo *chaa, int yesorno);
extern int  Character_GetFrame(CharacterInfo *chaa);
extern void Character_SetFrame(CharacterInfo *chaa, int newval);
extern int  Character_GetHasExplicitTint(CharacterInfo *chaa);
extern int  Character_GetID(CharacterInfo *chaa);
extern int  Character_GetIdleView(CharacterInfo *chaa);
extern int  Character_GetIInventoryQuantity(CharacterInfo *chaa, int index);
extern void Character_SetIInventoryQuantity(CharacterInfo *chaa, int index, int quantity);
extern int  Character_GetIgnoreLighting(CharacterInfo *chaa);
extern void Character_SetIgnoreLighting(CharacterInfo *chaa, int yesorno);
extern int  Character_GetIgnoreScaling(CharacterInfo *chaa);
extern void Character_SetIgnoreScaling(CharacterInfo *chaa, int yesorno);
extern int  Character_GetIgnoreWalkbehinds(CharacterInfo *chaa);
extern void Character_SetIgnoreWalkbehinds(CharacterInfo *chaa, int yesorno);
extern int  Character_GetLoop(CharacterInfo *chaa);
extern void Character_SetLoop(CharacterInfo *chaa, int newval);
extern void Character_SetManualScaling(CharacterInfo *chaa, int yesorno);
extern int  Character_GetMovementLinkedToAnimation(CharacterInfo *chaa);
extern void Character_SetMovementLinkedToAnimation(CharacterInfo *chaa, int yesorno);
extern int  Character_GetMoving(CharacterInfo *chaa);
extern const char* Character_GetName(CharacterInfo *chaa);
extern void Character_SetName(CharacterInfo *chaa, const char *newName);
extern int  Character_GetNormalView(CharacterInfo *chaa);
extern int  Character_GetPreviousRoom(CharacterInfo *chaa);
extern int  Character_GetRoom(CharacterInfo *chaa);
extern int  Character_GetScaleMoveSpeed(CharacterInfo *chaa);
extern void Character_SetScaleMoveSpeed(CharacterInfo *chaa, int yesorno);
extern int  Character_GetScaleVolume(CharacterInfo *chaa);
extern void Character_SetScaleVolume(CharacterInfo *chaa, int yesorno);
extern int  Character_GetScaling(CharacterInfo *chaa);
extern void Character_SetScaling(CharacterInfo *chaa, int zoomlevel);
extern int  Character_GetSolid(CharacterInfo *chaa);
extern void Character_SetSolid(CharacterInfo *chaa, int yesorno);
extern int  Character_GetSpeaking(CharacterInfo *chaa);
extern int  GetCharacterSpeechAnimationDelay(CharacterInfo *cha);
extern void Character_SetSpeechAnimationDelay(CharacterInfo *chaa, int newDelay);
extern int  Character_GetSpeechColor(CharacterInfo *chaa);
extern void Character_SetSpeechColor(CharacterInfo *chaa, int ncol);
extern int  Character_GetSpeechView(CharacterInfo *chaa);
extern void Character_SetSpeechView(CharacterInfo *chaa, int vii);
extern int  Character_GetThinkView(CharacterInfo *chaa);
extern void Character_SetThinkView(CharacterInfo *chaa, int vii);
extern int  Character_GetTransparency(CharacterInfo *chaa);
extern void Character_SetTransparency(CharacterInfo *chaa, int trans);
extern int  Character_GetTurnBeforeWalking(CharacterInfo *chaa);
extern void Character_SetTurnBeforeWalking(CharacterInfo *chaa, int yesorno);
extern int  Character_GetView(CharacterInfo *chaa);
extern int  Character_GetWalkSpeedX(CharacterInfo *chaa);
extern int  Character_GetWalkSpeedY(CharacterInfo *chaa);
extern int  Character_GetX(CharacterInfo *chaa);
extern void Character_SetX(CharacterInfo *chaa, int newval);
extern int  Character_GetY(CharacterInfo *chaa);
extern void Character_SetY(CharacterInfo *chaa, int newval);
extern int  Character_GetZ(CharacterInfo *chaa);
extern void Character_SetZ(CharacterInfo *chaa, int newval);

#ifdef WINDOWS_VERSION
#define AGS_INLINE inline
#else
// the linux compiler won't allow extern inline
#define AGS_INLINE
#endif

extern AGS_INLINE int divide_down_coordinate(int coord);
extern AGS_INLINE int multiply_up_coordinate(int coord);
extern AGS_INLINE void multiply_up_coordinates(int *x, int *y);
extern AGS_INLINE int get_fixed_pixel_size(int pixels);
extern AGS_INLINE int convert_to_low_res(int coord);
extern AGS_INLINE int convert_back_to_high_res(int coord);

#endif
