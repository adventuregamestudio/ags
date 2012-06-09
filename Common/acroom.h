/*
** ACROOM - AGS main header file
** Copyright (C) 1995-2003, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
** INTERNAL WORKING COPY: This file should NEVER leave my computer - if
** you have this file then you are in breach of the license agreement
** and must delete it at once.
*/

#ifndef __CROOM_H
#define __CROOM_H

//=============================================================================
// [IKM] 2012-06-08: a quick way to know if acroom.h included in a module
// with all the function definitions; usage: put following line at the
// very start of *.cpp file in question:
// #define TEST__CROOM_NOFUNCTIONS
//
#if defined TEST__CROOM_NOFUNCTIONS
  #ifdef CROOM_NOFUNCTIONS
    #error CROOM_NOFUNCTIONS IS defined!
  #else
    #error CROOM_NOFUNCTIONS NOT defined!
  #endif
#endif
// Same for NO_SAVE_FUNCTIONS
#if defined TEST__CROOM_NOSAVEFUNCTIONS
  #ifdef NO_SAVE_FUNCTIONS
    #error NO_SAVE_FUNCTIONS IS defined!
  #else
    #error NO_SAVE_FUNCTIONS NOT defined!
  #endif
#endif
//=============================================================================

#if !defined __CROOM_FUNC_H && !defined __CROOM_NOFUNC_H
#error Do not include acroom.h directly, include either acroom_func.h or acroom_nofunc.h instead.
#endif

#if defined CROOM_NOFUNCTIONS
#error CROOM_NOFUNCTIONS macro is obsolete and should not be defined anymore.
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cscomp.h"

// MACPORT FIX: endian support
#include "bigend.h"

#define EXIT_NORMAL 91
#define EXIT_CRASH  92

#define ROOM_FILE_VERSION 29
/* room file versions history
  8:  final v1.14 release
  9:  intermediate v2 alpha releases
 10:  v2 alpha-7 release
 11:  final v2.00 release
 12:  v2.08, to add colour depth byte
 13:  v2.14, add walkarea light levels
 14:  v2.4, fixed so it saves walkable area 15
 15:  v2.41, supports NewInteraction
 16:  v2.5
 17:  v2.5 - just version change to force room re-compile for new charctr struct
 18:  v2.51 - vector scaling
 19:  v2.53 - interaction variables
 20:  v2.55 - shared palette backgrounds
 21:  v2.55 - regions
 22:  v2.61 - encrypt room messages
 23:  v2.62 - object flags
 24:  v2.7  - hotspot script names
 25:  v2.72 - game id embedded
 26:  v3.0 - new interaction format, and no script source
 27:  v3.0 - store Y of bottom of object, not top
 28:  v3.0.3 - remove hotspot name length limit
 29:  v3.0.3 - high-res coords for object x/y, edges and hotspot walk-to point
 */
#define MAX_INIT_SPR  40
#define MAX_OBJ       16  // max walk-behinds
#define NUM_MISC      20
#define MAXMESS       100
#define NUMOTCON      7                 // number of conditions before standing on
#define NUM_CONDIT    (120 + NUMOTCON)
#define MAX_HOTSPOTS  50   // v2.62 increased from 20 to 30; v2.8 to 50
#define MAX_REGIONS   16

#define MAX_SCRIPT_NAME_LEN 20

//const int MISC_COND = MAX_OBJ * 4 + NUMOTCON + MAX_INIT_SPR * 4;

// NUMCONDIT : whataction[0]:  Char walks off left
//                       [1]:  Char walks off right
//                       [2]:  Char walks off bottom
//                       [3]:  Char walks off top
//			 [4]:  First enters screen
//                       [5]:  Every time enters screen
//                       [6]:  execute every loop
//                [5]...[19]:  Char stands on lookat type
//               [20]...[35]:  Look at type
//               [36]...[49]:  Action on type
//               [50]...[65]:  Use inv on type
//               [66]...[75]:  Look at object
//               [76]...[85]:  Action on object
//               [86]...[95]:  Speak to object
//		[96]...[105]:  Use inv on object
//             [106]...[124]:  Misc conditions 1-20

// game ver     whataction[]=
// v1.00              0  :  Go to screen
//                    1  :  Don't do anything
//                    2  :  Can't walk
//                    3  :  Man dies
//                    4  :  Run animation
//                    5  :  Display message
//                    6  :  Remove an object (set object.on=0)
//		                7  :  Remove object & add Val2 to inventory
//                    8  :  Add Val1 to inventory (Val2=num times)
//                    9  :  Run a script
// v1.00 SR-1        10  :  Run graphical script
// v1.1              11  :  Play sound effect SOUND%d.WAV
// v1.12             12  :  Play FLI/FLC animation FLIC%d.FLC or FLIC%d.FLI
//                   13  :  Turn object on
// v2.00             14  :  Run conversation
#define NUMRESPONSE   14
#define NUMCOMMANDS   15
#define GO_TO_SCREEN  0
#define NO_ACTION     1
#define NO_WALK       2
#define MAN_DIES      3
#define RUN_ANIMATE   4
#define SHOW_MESSAGE  5
#define OBJECT_OFF    6
#define OBJECT_INV    7
#define ADD_INV       8
#define RUNSCRIPT     9
#define GRAPHSCRIPT   10
#define PLAY_SOUND    11
#define PLAY_FLI      12
#define OBJECT_ON     13
#define RUN_DIALOG    14

#ifdef DJGPP
#include <unistd.h>
#endif

#ifdef _MSC_VER
#undef VTA_LEFT
#undef VTA_RIGHT
#endif


// thisroom.options[0] = startup music
// thisroom.options[1] = can save/load on screen (0=yes, 1=no)
// thisroom.options[2] = player character disabled? (0=no, 1=yes)
// thisroom.options[3] = player special view (0=normal)
//                 [4] = music volume (0=normal, <0 quiter, >0 louder)

const int ST_TUNE = 0, ST_SAVELOAD = 1, ST_MANDISABLED = 2, ST_MANVIEW = 3, ST_VOLUME = 4;

void quit(char *);

#ifdef DJGPP
#define PCKD __attribute__((packed))
#else
#define PCKD
#endif

#pragma pack(1)
struct sprstruc {
  short sprnum  PCKD;   // number from array
  short x,y     PCKD;   // x,y co-ords
  short room    PCKD;   // room number
  short on      PCKD;
  sprstruc() { on = 0; }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    sprnum = __getshort__bigendian(fp);
    x = __getshort__bigendian(fp);
    y = __getshort__bigendian(fp);
    room = __getshort__bigendian(fp);
    on = __getshort__bigendian(fp);
  }
#endif
};

#define MSG_DISPLAYNEXT 1 // supercedes using alt-200 at end of message
#define MSG_TIMELIMIT   2
struct MessageInfo {
  char  displayas  PCKD; // 0 = normal window, 1 = as speech
  char  flags      PCKD; // combination of MSG_xxx flags
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    displayas = getc(fp);
    flags = getc(fp);
  }
#endif
};
#pragma pack()

#define AE_WAITFLAG   0x80000000
#define MAXANIMSTAGES 10
struct AnimationStruct {
  int   x, y;
  int   data;
  int   object;
  int   speed;
  char  action;
  char  wait;
  AnimationStruct() { action = 0; object = 0; wait = 1; speed = 5; }
};

struct FullAnimation {
  AnimationStruct stage[MAXANIMSTAGES];
  int             numstages;
  FullAnimation() { numstages = 0; }
};

struct _Point {
  short x, y;
};

#define MAXCOMMANDS 8
struct EventBlock {
  int   list[MAXCOMMANDS];
  int   respond[MAXCOMMANDS];
  int   respondval[MAXCOMMANDS];
  int   data[MAXCOMMANDS];
  int   numcmd;
  short score[MAXCOMMANDS];
};

// careful with this - the shadinginfo[] array needs to be
// MAX_WALK_AREAS + 1 if this gets changed
#define MAX_WALK_AREAS 15
#define MAXPOINTS 30
struct PolyPoints {
  int x[MAXPOINTS];
  int y[MAXPOINTS];
  int numpoints;
  void add_point(int xxx,int yyy) {
    x[numpoints] = xxx;
    y[numpoints] = yyy;
    numpoints++;

    if (numpoints >= MAXPOINTS)
      quit("too many poly points added");
  }
  PolyPoints() { numpoints = 0; }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    fread(x, sizeof(int), MAXPOINTS, fp);
    fread(y, sizeof(int), MAXPOINTS, fp);
    numpoints = getw(fp);
  }
#endif
};

#define POPUP_NONE      0
#define POPUP_MOUSEY    1
#define POPUP_SCRIPT    2
#define POPUP_NOAUTOREM 3  // don't remove automatically during cutscene
#define POPUP_NONEINITIALLYOFF 4   // normal GUI, initially off
#define VTA_LEFT        0
#define VTA_RIGHT       1
#define VTA_CENTRE      2
#define IFLG_TEXTWINDOW 1

#define MAXBUTTON       20
#define IBACT_SETMODE   1
#define IBACT_SCRIPT    2
#define IBFLG_ENABLED   1
#define IBFLG_INVBOX    2
struct InterfaceButton {
  int x, y, pic, overpic, pushpic, leftclick;
  int rightclick; // if inv, then leftclick = wid, rightclick = hit
  int reserved_for_future;
  char flags;
  void set(int xx, int yy, int picc, int overpicc, int actionn) {
    x = xx; y = yy; pic = picc; overpic = overpicc; leftclick = actionn; pushpic = 0;
    rightclick = 0; flags = IBFLG_ENABLED;
    reserved_for_future = 0;
  }
};

// this struct should go in a Game struct, not the room structure.
struct InterfaceElement {
  int             x, y, x2, y2;
  int             bgcol, fgcol, bordercol;
  int             vtextxp, vtextyp, vtextalign;  // X & Y relative to topleft of interface
  char            vtext[40];
  int             numbuttons;
  InterfaceButton button[MAXBUTTON];
  int             flags;
  int             reserved_for_future;
  int             popupyp;   // pops up when mousey < this
  char            popup;     // does it pop up? (like sierra icon bar)
  char            on;
  InterfaceElement() {
    vtextxp = 0; vtextyp = 1; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
    numbuttons = 0; bgcol = 8; fgcol = 15; bordercol = 0; on = 1; flags = 0;
  }
};

/*struct InterfaceStyle {
  int  playareax1,playareay1,playareax2,playareay2; // where the game takes place
  int  vtextxp,vtextyp;
  char vtext[40];
  int  numbuttons,popupbuttons;
  InterfaceButton button[MAXBUTTON];
  int  invx1,invy1,invx2,invy2;  // set invx1=-1 for Sierra-style inventory
  InterfaceStyle() {  // sierra interface
    playareax1=0; playareay1=13; playareax2=319; playareay2=199;
    vtextxp=160; vtextyp=2; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
    invx1=-1; numbuttons=2; popupbuttons=1;
    button[0].set(0,13,3,-1,0);
    }
  };*/

#define MAX_PARSER_WORD_LENGTH 30
#define ANYWORD     29999
#define RESTOFLINE  30000

struct WordsDictionary {
  int   num_words;
  char**word;
  short*wordnum;

  void allocate_memory(int wordCount)
  {
    num_words = wordCount;
    if (num_words > 0)
    {
      word = (char**)malloc(wordCount * sizeof(char*));
      word[0] = (char*)malloc(wordCount * MAX_PARSER_WORD_LENGTH);
      wordnum = (short*)malloc(wordCount * sizeof(short));
      for (int i = 1; i < wordCount; i++)
      {
        word[i] = word[0] + MAX_PARSER_WORD_LENGTH * i;
      }
    }
  }
  void free_memory()
  {
    if (num_words > 0)
    {
      free(word[0]);
      free(word);
      free(wordnum);
      word = NULL;
      wordnum = NULL;
      num_words = 0;
    }
  }
  void  sort();
  int   find_index (const char *);
};

#define MAX_CUSTOM_PROPERTIES 30
#define MAX_CUSTOM_PROPERTY_VALUE_LENGTH 500
#define PROP_TYPE_BOOL   1
#define PROP_TYPE_INT    2
#define PROP_TYPE_STRING 3
struct CustomPropertySchema {
  char  propName[MAX_CUSTOM_PROPERTIES][20];
  char  propDesc[MAX_CUSTOM_PROPERTIES][100];
  char *defaultValue[MAX_CUSTOM_PROPERTIES];
  int   propType[MAX_CUSTOM_PROPERTIES];
  int   numProps;

  // Find the index of the specified property
  int findProperty (const char *pname) {
    for (int ii = 0; ii < numProps; ii++) {
      if (stricmp (pname, propName[ii]) == 0)
        return ii;
    }
    return -1;
  }
  
  void deleteProperty (int idx) {
    if ((idx < 0) || (idx >= numProps))
      return;

    if (defaultValue[idx])
      delete defaultValue[idx];

    numProps--;
    for (int qq = idx; qq < numProps; qq++) {
      strcpy (propName[qq], propName[qq+1]);
      strcpy (propDesc[qq], propDesc[qq+1]);
      propType[qq] = propType[qq+1];
      defaultValue[qq] = defaultValue[qq+1];
    }
    defaultValue[numProps] = NULL;
  }

  void resetProperty (int idx) {
    propName[idx][0] = 0;
    propDesc[idx][0] = 0;
    if (defaultValue[idx])
      delete defaultValue[idx];
    propType[idx] = PROP_TYPE_BOOL;
    defaultValue[idx] = new char[MAX_CUSTOM_PROPERTY_VALUE_LENGTH];
    defaultValue[idx][0] = 0;
  }

  CustomPropertySchema () {
    numProps = 0;
    for (int kk = 0; kk < MAX_CUSTOM_PROPERTIES; kk++) {
      defaultValue[kk] = NULL;
    }
  }

  void Serialize (FILE *outto);
  int UnSerialize (FILE *infrom);

};


struct CustomProperties {
  char *propName[MAX_CUSTOM_PROPERTIES];
  char *propVal[MAX_CUSTOM_PROPERTIES];
  int   numProps;
  
  CustomProperties() {
    numProps = 0;
  }

  const char *getPropertyValue (const char *pname) {
    int idxx = findProperty(pname);
    if (idxx < 0)
      return NULL;

    return propVal[idxx];
  }

  // Find the index of the specified property
  int findProperty (const char *pname) {
    for (int ii = 0; ii < numProps; ii++) {
      if (stricmp (pname, propName[ii]) == 0)
        return ii;
    }
    return -1;
  }

  void reset () {
    for (int ii = 0; ii < numProps; ii++) {
      free (propName[ii]);
      free (propVal[ii]);
    }
    numProps = 0;
  }

  void addProperty (const char *newname, const char *newval) {
    if (numProps >= MAX_CUSTOM_PROPERTIES) {
      return;
    }
    propName[numProps] = (char*)malloc(200);
    propVal[numProps] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    strcpy (propName[numProps], newname);
    strcpy (propVal[numProps], newval);
    numProps++;
  }

  void Serialize (FILE *outto);
  int  UnSerialize (FILE *infrom);
};



/* THE WAY THIS WORKS:
 *
 * NewInteraction (Hotspot 1)
 *  |
 *  +-- eventTypes [NUM_EVENTS]
 *  +-- NewInteractionCommandList [NUM_EVENTS]   (Look at hotspot)
 *        |
 *        +-- NewInteractionCommand [NUM_COMMANDS]   (Display message)
 *             |
 *             +-- NewInteractionValue [NUM_ARGUMENTS]   (5)
 */

#define LOCAL_VARIABLE_OFFSET 10000
#define MAX_GLOBAL_VARIABLES 100
#define MAX_ACTION_ARGS 5
#define MAX_NEWINTERACTION_EVENTS 30
#define MAX_COMMANDS_PER_LIST 40
#define int32 int
#define VALTYPE_LITERALINT 1
#define VALTYPE_VARIABLE   2
#define VALTYPE_BOOLEAN    3
#define VALTYPE_CHARNUM    4

struct NewInteractionValue {
  unsigned char valType;
  int  val;
  int  extra;

  NewInteractionValue() {
    valType = VALTYPE_LITERALINT;
    val = 0;
    extra = 0;
  }
  

#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    fread(&valType, sizeof(char), 1, fp);
    char pad[3]; fread(pad, sizeof(char), 3, fp);
    val = getw(fp);
    extra = getw(fp);
  }
  void WriteToFile(FILE *fp)
  {
    fwrite(&valType, sizeof(char), 1, fp);
    char pad[3]; fwrite(pad, sizeof(char), 3, fp);
    putw(val, fp);
    putw(extra, fp);
  }
#endif
};

struct NewInteractionAction {
  virtual void reset() = 0;
};
struct NewInteractionCommandList;

struct NewInteractionCommand: public NewInteractionAction {
  int32 type;
  NewInteractionValue data[MAX_ACTION_ARGS];
  NewInteractionAction * children;
  NewInteractionCommandList *parent;

  NewInteractionCommand() {
    type = 0;
    children = NULL;
    parent = NULL;
  }
  NewInteractionCommandList *get_child_list() {
    return (NewInteractionCommandList*)children;
  }
  void remove();

  void reset() { remove(); }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    getw(fp); // skip the vtbl ptr
    type = getw(fp);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
      data[i].ReadFromFile(fp);
    }
    // all that matters is whether or not these are null...
    children = (NewInteractionAction *) getw(fp);
    parent = (NewInteractionCommandList *) getw(fp);
  }
  void WriteToFile(FILE *fp)
  {
    putw(0, fp); // write dummy vtbl ptr 
    putw(type, fp);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
      data[i].WriteToFile(fp);
    }
    putw((int)children, fp);
    putw((int)parent, fp);
  }
#endif
};

struct NewInteractionCommandList : public NewInteractionAction {
  int32 numCommands;
  NewInteractionCommand command[MAX_COMMANDS_PER_LIST];
  int32 timesRun;   // used by engine to track score changes

  NewInteractionCommandList () {
    numCommands = 0;
    timesRun = 0;
  }
  void reset();
};

struct NewInteraction {
  int numEvents;
  // the first few event types depend on the item - ID's of 100+ are
  // custom events (used for subroutines)
  int eventTypes[MAX_NEWINTERACTION_EVENTS];
  int timesRun[MAX_NEWINTERACTION_EVENTS];
  NewInteractionCommandList *response[MAX_NEWINTERACTION_EVENTS];


  NewInteraction() { 
    numEvents = 0;
    // NULL all the pointers
    memset (response, 0, sizeof(NewInteractionCommandList*) * MAX_NEWINTERACTION_EVENTS);
    memset (&timesRun[0], 0, sizeof(int) * MAX_NEWINTERACTION_EVENTS);
  }


  void copy_timesrun_from (NewInteraction *nifrom) {
    memcpy (&timesRun[0], &nifrom->timesRun[0], sizeof(int) * MAX_NEWINTERACTION_EVENTS);
  }
  void reset() {
    for (int i = 0; i < numEvents; i++) {
      if (response[i] != NULL) {
        response[i]->reset();
        delete response[i];
        response[i] = NULL;
      }
    }
    numEvents = 0;
  }
  ~NewInteraction() {
    reset();
  }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    // it's all ints!
    fread(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
  }
  void WriteToFile(FILE *fp)
  {
    fwrite(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
  }
#endif
};

#define NUM_ACTION_TYPES 48
#define ARG_TYPE_INT 1
#define ARG_TYPE_INV 2
#define ARG_TYPE_MSG 3
#define ARG_TYPE_CHAR 4
#define ARG_TYPE_BOOL 5
#define ARG_TYPE_VARONLY 6  // must be variable, no literal values
#define AFLG_COND      1
#define AFLG_RUNSCRIPT 2
#define AFLG_INVCHECK  4
#define AFLG_MESSAGE   8

struct ActionTypes {
  char  name[80];
  short flags;
  char  numArgs;
  char  argTypes[MAX_ACTION_ARGS];
  char  argNames[MAX_ACTION_ARGS][25];
  char  description[200];
  char  textscript[80];
};
extern ActionTypes actions[NUM_ACTION_TYPES];

struct InteractionVariable {
  char name[23];
  char type;
  int  value;
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    fread(name, sizeof(char), 23, fp);
    type = getc(fp);
    value = getw(fp);
  }
#endif
};
extern InteractionVariable globalvars[];
extern int numGlobalVars;

struct InteractionScripts {
  int numEvents;
  char *scriptFuncNames[MAX_NEWINTERACTION_EVENTS];

  InteractionScripts() {
    numEvents = 0;
  }

  ~InteractionScripts() {
    for (int i = 0; i < numEvents; i++)
      delete scriptFuncNames[i];
  }
};

#define AUCL_BUNDLE_EXE 1
#define AUCL_BUNDLE_VOX 2
enum AudioFileType {
  eAudioFileOGG = 1,
  eAudioFileMP3 = 2,
  eAudioFileWAV = 3,
  eAudioFileVOC = 4,
  eAudioFileMIDI = 5,
  eAudioFileMOD = 6
};
struct ScriptAudioClip {
  int id;  // not used by editor, set in engine only
  char scriptName[30];
  char fileName[15];
  char bundlingType;
  char type;
  char fileType;
  char defaultRepeat;
  short defaultPriority;
  short defaultVolume;
  int  reserved;
};
#define AUDIO_CLIP_TYPE_SOUND 1
struct AudioClipType {
  int id;
  int reservedChannels;
  int volume_reduction_while_speech_playing;
  int crossfadeSpeed;
  int reservedForFuture;
};

#define MAX_ROOMS 300

// object flags (currently only a char)
#define OBJF_NOINTERACT        1  // not clickable
#define OBJF_NOWALKBEHINDS     2  // ignore walk-behinds
#define OBJF_HASTINT           4  // the tint_* members are valid
#define OBJF_USEREGIONTINTS    8  // obey region tints/light areas
#define OBJF_USEROOMSCALING 0x10  // obey room scaling areas
#define OBJF_SOLID          0x20  // blocks characters from moving
#define OBJF_DELETED        0x40  // object has been deleted

#define MAXANIMS      10
#define MAX_FLAGS     15
#define MAXOBJNAMELEN 30
#define MAX_BSCENE    5   // max number of frames in animating bg scene
#define NOT_VECTOR_SCALED -10000
#define TINT_IS_ENABLED 0x80000000
struct roomstruct {
  block         walls, object, lookat;          // 'object' is the walk-behind
  block         regions;
  color         pal[256];
  short         numobj;                         // num hotspots, not sprites
  short         objyval[MAX_OBJ];               // baselines of walkbehind areas
  // obsolete v2.00 action editor stuff below
  short         whataction[NUM_CONDIT+3];       // what to do if condition appears
  short         val1[NUM_CONDIT+3];             // variable, eg. screen number to go to
  short         val2[NUM_CONDIT+3];             // 2nd var, optional, eg. which side of screen to come on
  short         otcond[NUM_CONDIT+3];           // do extra misc condition
  char          points[NUM_CONDIT+3];           // extra points for doing it
  // end obsolete v2.00 action editor
  short         left,right,top,bottom;          // to walk off screen
  short         numsprs,nummes;                 // number of initial sprites and messages
  sprstruc      sprs[MAX_INIT_SPR];             // structures for each sprite
  NewInteraction *intrObject[MAX_INIT_SPR];
  InteractionScripts **objectScripts;
  int           objbaseline[MAX_INIT_SPR];                // or -1 (use bottom of object graphic)
  short         objectFlags[MAX_INIT_SPR];
  char          objectnames[MAX_INIT_SPR][MAXOBJNAMELEN];
  char          objectscriptnames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN];
  CustomProperties objProps[MAX_INIT_SPR];
  char          password[11];
  char          options[10];                    // [0]=startup music
  char          *message[MAXMESS];
  MessageInfo   msgi[MAXMESS];
  short         wasversion;                     // when loaded from file
  short         flagstates[MAX_FLAGS];
  FullAnimation anims[MAXANIMS];
  short         numanims;
  short         shadinginfo[16];    // walkable area-specific view number
  // new version 2 roommake stuff below
  int           numwalkareas;
  PolyPoints    wallpoints[MAX_WALK_AREAS];
  int           numhotspots;
  _Point        hswalkto[MAX_HOTSPOTS];
  char*         hotspotnames[MAX_HOTSPOTS];
  char          hotspotScriptNames[MAX_HOTSPOTS][MAX_SCRIPT_NAME_LEN];
  NewInteraction *intrHotspot[MAX_HOTSPOTS];
  NewInteraction *intrRoom;
  NewInteraction *intrRegion[MAX_REGIONS];
  InteractionScripts **hotspotScripts;
  InteractionScripts **regionScripts;
  InteractionScripts *roomScripts;
  int           numRegions;
  short         regionLightLevel[MAX_REGIONS];
  int           regionTintLevel[MAX_REGIONS];
  short         width,height;                             // in 320x200 terms (scrolling room size)
  short         resolution;                               // 1 = 320x200, 2 = 640x400
  short         walk_area_zoom[MAX_WALK_AREAS + 1];       // 0 = 100%, 1 = 101%, -1 = 99%
  short         walk_area_zoom2[MAX_WALK_AREAS + 1];      // for vector scaled areas
  short         walk_area_light[MAX_WALK_AREAS + 1];      // 0 = normal, + lighter, - darker
  short         walk_area_top[MAX_WALK_AREAS + 1];     // top YP of area
  short         walk_area_bottom[MAX_WALK_AREAS + 1];  // bottom YP of area
  char          *scripts;
  ccScript      *compiled_script;
  int           cscriptsize;
  int           num_bscenes, bscene_anim_speed;
  int           bytes_per_pixel;
  block         ebscene[MAX_BSCENE];
  color         bpalettes[MAX_BSCENE][256];
  InteractionVariable *localvars;
  int           numLocalVars;
  char          ebpalShared[MAX_BSCENE];  // used internally by engine atm
  CustomProperties roomProps;
  CustomProperties hsProps[MAX_HOTSPOTS];
  int           gameId;
  int           lastLoadNumHotspots;
  int           lastLoadNumObjects;
  int           lastLoadNumRegions;

  roomstruct() {
    ebscene[0] = NULL; walls = NULL; object = NULL; lookat = NULL; nummes = 0;
    left = 0; right = 317; top = 40; bottom = 199; numobj = MAX_OBJ; numsprs = 0; password[0] = 0;
    wasversion = ROOM_FILE_VERSION; numanims = 0; regions = NULL; numwalkareas = 0;
    numhotspots = 0;
    memset(&objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
    memset(&objectFlags[0], 0, sizeof(short) * MAX_INIT_SPR);
    width = 320; height = 200; scripts = NULL; compiled_script = NULL;
    cscriptsize = 0;
    memset(&walk_area_zoom[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
    memset(&walk_area_light[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
    resolution = 1; num_bscenes = 1; ebscene[0] = NULL;
    bscene_anim_speed = 5; bytes_per_pixel = 1;
    numLocalVars = 0;
    localvars = NULL;
    lastLoadNumHotspots = 0;
    lastLoadNumRegions = 0;
    lastLoadNumObjects = 0;
    int i;
    for (i = 0; i <= MAX_WALK_AREAS; i++) {
      walk_area_zoom2[i] = NOT_VECTOR_SCALED;
      walk_area_top[i] = -1;
      walk_area_bottom[i] = -1;
    }
    for (i = 0; i < MAX_HOTSPOTS; i++) {
      intrHotspot[i] = new NewInteraction();
	  hotspotnames[i] = NULL;
      hotspotScriptNames[i][0] = 0;
    }
    for (i = 0; i < MAX_INIT_SPR; i++)
      intrObject[i] = new NewInteraction();
    for (i = 0; i < MAX_REGIONS; i++)
      intrRegion[i] = new NewInteraction();
    intrRoom = new NewInteraction();
    gameId = 0;
	numRegions = 0;
	hotspotScripts = NULL;
	regionScripts = NULL;
	objectScripts = NULL;
	roomScripts = NULL;
  }
  //void allocall();
  //void freeall();
  void freemessage();
};

extern int in_interaction_editor;  // whether to remove script functions/etc

#pragma pack(1)
struct ScriptEvent {
  long type     PCKD;   // eg. display message, or if is less
  char sort     PCKD;
  long _using   PCKD;   // ^var1
  long with     PCKD;   // number 3 than 9
  long data     PCKD;
  long branchto PCKD;
  long screeny  PCKD;
  void settype(long);
};

#define MAXINBLOCK 10
struct ScriptBlock {
  long        numevents           PCKD;
  ScriptEvent events[MAXINBLOCK]  PCKD;
};
#pragma pack()

#define VFLG_FLIPSPRITE 1

struct ViewFrame {
  int   pic;
  short xoffs, yoffs;
  short speed;
  int   flags;
  int   sound;  // play sound when this frame comes round
  int   reserved_for_future[2];
  ViewFrame() { pic = 0; xoffs = 0; yoffs = 0; speed = 0; }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    pic = getw(fp);
    xoffs = __getshort__bigendian(fp);
    yoffs = __getshort__bigendian(fp);
    speed = __getshort__bigendian(fp);
    fseek(fp, 2, SEEK_CUR);
    flags = getw(fp);
    sound = getw(fp);
    reserved_for_future[0] = getw(fp);
    reserved_for_future[1] = getw(fp);
  }
#else
  void ReadFromFile(FILE *fp)
  {
    pic = getw(fp);
    fread(&xoffs, 2, 1, fp);
    fread(&yoffs, 2, 1, fp);
    fread(&speed, 2, 1, fp);
    fseek(fp, 2, SEEK_CUR);
    flags = getw(fp);
    sound = getw(fp);
    reserved_for_future[0] = getw(fp);
    reserved_for_future[1] = getw(fp);
  }
#endif
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopNew
{
  short numFrames;
  int   flags;
  ViewFrame *frames;

  bool RunNextLoop() 
  {
    return (flags & LOOPFLAG_RUNNEXTLOOP);
  }

  void Initialize(int frameCount);
  void Dispose();
  void WriteToFile(FILE *ooo);
  void ReadFromFile(FILE *iii);
};

struct ViewStruct
{
  short numLoops;
  ViewLoopNew *loops;

  void Initialize(int loopCount);
  void Dispose();
  void WriteToFile(FILE *ooo);
  void ReadFromFile(FILE *iii);
};

struct ViewStruct272 {
  short     numloops;
  short     numframes[16];
  int       loopflags[16];
  ViewFrame frames[16][20];
  ViewStruct272() { numloops = 0; numframes[0] = 0; }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    numloops = __getshort__bigendian(fp);
    for (int i = 0; i < 16; ++i)
    {
      numframes[i] = __getshort__bigendian(fp);
    }
    // skip padding if there is any
    fseek(fp, 2*(2 - ((16+1)%2)), SEEK_CUR);
    fread(loopflags, sizeof(int), 16, fp);
    for (int j = 0; j < 16; ++j)
    {
      for (int i = 0; i < 20; ++i)
      {
        frames[j][i].ReadFromFile(fp);
      }
    }
  }
#else
  void ReadFromFile(FILE *fp)
  {
    fread(&numloops, 2, 1, fp);
    for (int i = 0; i < 16; ++i)
    {
      fread(&numframes[i], 2, 1, fp);
    }
    fseek(fp, 2*(2 - ((16+1)%2)), SEEK_CUR);
    fread(loopflags, sizeof(int), 16, fp);
    for (int j = 0; j < 16; ++j)
    {
      for (int i = 0; i < 20; ++i)
      {
        frames[j][i].ReadFromFile(fp);
      }
    }
  }
#endif
};

#define MCF_ANIMMOVE 1
#define MCF_DISABLED 2
#define MCF_STANDARD 4
#define MCF_HOTSPOT  8  // only animate when over hotspot
// this struct is also in the plugin header file
struct MouseCursor {
  int   pic;
  short hotx, hoty;
  short view;
  char  name[10];
  char  flags;
  MouseCursor() { pic = 2054; hotx = 0; hoty = 0; name[0] = 0; flags = 0; view = -1; }
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    pic = getw(fp);
    hotx = __getshort__bigendian(fp);
    hoty = __getshort__bigendian(fp);
    view = __getshort__bigendian(fp);
    // may need to read padding?
    fread(name, sizeof(char), 10, fp);
    flags = getc(fp);
    fseek(fp, 3, SEEK_CUR);
  }
#endif
};

#define MAX_INV             301
#define CHF_MANUALSCALING   1
#define CHF_FIXVIEW         2     // between SetCharView and ReleaseCharView
#define CHF_NOINTERACT      4
#define CHF_NODIAGONAL      8
#define CHF_ALWAYSIDLE      0x10
#define CHF_NOLIGHTING      0x20
#define CHF_NOTURNING       0x40
#define CHF_NOWALKBEHINDS   0x80
#define CHF_FLIPSPRITE      0x100  // ?? Is this used??
#define CHF_NOBLOCKING      0x200
#define CHF_SCALEMOVESPEED  0x400
#define CHF_NOBLINKANDTHINK 0x800
#define CHF_SCALEVOLUME     0x1000
#define CHF_HASTINT         0x2000   // engine only
#define CHF_BEHINDSHEPHERD  0x4000   // engine only
#define CHF_AWAITINGMOVE    0x8000   // engine only
#define CHF_MOVENOTWALK     0x10000   // engine only - do not do walk anim
#define CHF_ANTIGLIDE       0x20000
// Speechcol is no longer part of the flags as of v2.5
#define OCHF_SPEECHCOL      0xff000000
#define OCHF_SPEECHCOLSHIFT 24
#define UNIFORM_WALK_SPEED  0
#define FOLLOW_ALWAYSONTOP  0x7ffe
// remember - if change this struct, also change AGSDEFNS.SH and
// plugin header file struct
struct CharacterInfo {
  int   defview;
  int   talkview;
  int   view;
  int   room, prevroom;
  int   x, y, wait;
  int   flags;
  short following;
  short followinfo;
  int   idleview;           // the loop will be randomly picked
  short idletime, idleleft; // num seconds idle before playing anim
  short transparency;       // if character is transparent
  short baseline;
  int   activeinv;
  int   talkcolor;
  int   thinkview;
  short blinkview, blinkinterval; // design time
  short blinktimer, blinkframe;   // run time
  short walkspeed_y, pic_yoffs;
  int   z;    // z-location, for flying etc
  int   walkwait;
  short speech_anim_speed, reserved1;  // only 1 reserved left!!
  short blocking_width, blocking_height;
  int   index_id;  // used for object functions to know the id
  short pic_xoffs, walkwaitcounter;
  short loop, frame;
  short walking, animating;
  short walkspeed, animspeed;
  short inv[MAX_INV];
  short actx, acty;
  char  name[40];
  char  scrname[MAX_SCRIPT_NAME_LEN];
  char  on;

  int get_effective_y();   // return Y - Z
  int get_baseline();      // return baseline, or Y if not set
  int get_blocking_top();    // return Y - BlockingHeight/2
  int get_blocking_bottom(); // return Y + BlockingHeight/2
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    defview = getw(fp);
    talkview = getw(fp);
    view = getw(fp);
    room = getw(fp);
    prevroom = getw(fp);
    x = getw(fp);
    y = getw(fp);
    wait = getw(fp);
    flags = getw(fp);
    following = __getshort__bigendian(fp);
    followinfo = __getshort__bigendian(fp);
    idleview = getw(fp);
    idletime = __getshort__bigendian(fp);
    idleleft = __getshort__bigendian(fp);
    transparency = __getshort__bigendian(fp);
    baseline = __getshort__bigendian(fp);
    activeinv = getw(fp);
    talkcolor = getw(fp);
    thinkview = getw(fp);
    blinkview = __getshort__bigendian(fp);
    blinkinterval = __getshort__bigendian(fp);
    blinktimer = __getshort__bigendian(fp);
    blinkframe = __getshort__bigendian(fp);
    walkspeed_y = __getshort__bigendian(fp);
    pic_yoffs = __getshort__bigendian(fp);
    z = getw(fp);
    reserved[0] = getw(fp);
    reserved[1] = getw(fp);
    blocking_width = __getshort__bigendian(fp);
    blocking_height = __getshort__bigendian(fp);;
    index_id = getw(fp);
    pic_xoffs = __getshort__bigendian(fp);
    walkwaitcounter = __getshort__bigendian(fp);
    loop = __getshort__bigendian(fp);
    frame = __getshort__bigendian(fp);
    walking = __getshort__bigendian(fp);
    animating = __getshort__bigendian(fp);
    walkspeed = __getshort__bigendian(fp);
    animspeed = __getshort__bigendian(fp);
    fread(inv, sizeof(short), MAX_INV, fp);
    actx = __getshort__bigendian(fp);
    acty = __getshort__bigendian(fp);
    fread(name, sizeof(char), 40, fp);
    fread(scrname, sizeof(char), MAX_SCRIPT_NAME_LEN, fp);
    on = getc(fp);
    // MAX_INV is odd, so need to sweep up padding
    // skip over padding that makes struct a multiple of 4 bytes long
    fseek(fp, 4 - (((MAX_INV+2)*sizeof(short)+40+MAX_SCRIPT_NAME_LEN+1)%4), SEEK_CUR);
  }
#endif
};


struct OldCharacterInfo {
  int   defview;
  int   talkview;
  int   view;
  int   room, prevroom;
  int   x, y, wait;
  int   flags;
  short following;
  short followinfo;
  int   idleview;           // the loop will be randomly picked
  short idletime, idleleft; // num seconds idle before playing anim
  short transparency;       // if character is transparent
  short baseline;
  int   activeinv;          // this is an INT to support SeeR (no signed shorts)
  short loop, frame;
  short walking, animating;
  short walkspeed, animspeed;
  short inv[100];
  short actx, acty;
  char  name[30];
  char  scrname[16];
  char  on;
};

#define IFLG_STARTWITH 1
struct InventoryItemInfo {
  char name[25];
  int  pic;
  int  cursorPic, hotx, hoty;
  int  reserved[5];
  char flags;
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    fread(name, sizeof(char), 25, fp);
    fseek(fp, 3, SEEK_CUR);
    pic = getw(fp);
    cursor = getw(fp);
    hotx = getw(fp);
    hoty = getw(fp);
    fread(reserved, sizeof(int), 5, fp);
    flags = getc(fp);
    fseek(fp, 3, SEEK_CUR);
  }
#endif
};

#define MAXTOPICOPTIONS     30
#define DFLG_ON             1  // currently enabled
#define DFLG_OFFPERM        2  // off forever (can't be trurned on)
#define DFLG_NOREPEAT       4  // character doesn't repeat it when clicked
#define DFLG_HASBEENCHOSEN  8  // dialog option is 'read'
#define DTFLG_SHOWPARSER    1  // show parser in this topic
#define DCMD_SAY            1
#define DCMD_OPTOFF         2
#define DCMD_OPTON          3
#define DCMD_RETURN         4
#define DCMD_STOPDIALOG     5
#define DCMD_OPTOFFFOREVER  6
#define DCMD_RUNTEXTSCRIPT  7
#define DCMD_GOTODIALOG     8
#define DCMD_PLAYSOUND      9
#define DCMD_ADDINV         10
#define DCMD_SETSPCHVIEW    11
#define DCMD_NEWROOM        12
#define DCMD_SETGLOBALINT   13
#define DCMD_GIVESCORE      14
#define DCMD_GOTOPREVIOUS   15
#define DCMD_LOSEINV        16
#define DCMD_ENDSCRIPT      0xff
#define DCHAR_NARRATOR  999
#define DCHAR_PLAYER    998
#define MAX_DIALOG          500
struct DialogTopic {
  char          optionnames[MAXTOPICOPTIONS][150];
  int           optionflags[MAXTOPICOPTIONS];
  unsigned char *optionscripts;
  short         entrypoints[MAXTOPICOPTIONS];
  short         startupentrypoint;
  short         codesize;
  int           numoptions;
  int           topicFlags;
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    fread(optionnames, 150*sizeof(char), MAXTOPICOPTIONS, fp);
    fread(optionflags, sizeof(int), MAXTOPICOPTIONS, fp);
    optionscripts = (unsigned char *) getw(fp);
    fread(entrypoints, sizeof(short), MAXTOPICOPTIONS, fp);
    startupentrypoint = __getshort__bigendian(fp);
    codesize = __getshort__bigendian(fp);
    numoptions = getw(fp);
    topicFlags = getw(fp);
  }
#endif
};

#define MAX_SPRITES         30000
#define MAX_CURSOR          20
#define PAL_GAMEWIDE        0
#define PAL_LOCKED          1
#define PAL_BACKGROUND      2
#define MAXGLOBALMES        500
#define MAXLANGUAGE         5
#define MAX_FONTS           15
#define OPT_DEBUGMODE       0
#define OPT_SCORESOUND      1
#define OPT_WALKONLOOK      2
#define OPT_DIALOGIFACE     3
#define OPT_ANTIGLIDE       4
#define OPT_TWCUSTOM        5
#define OPT_DIALOGGAP       6
#define OPT_NOSKIPTEXT      7
#define OPT_DISABLEOFF      8
#define OPT_ALWAYSSPCH      9
#define OPT_SPEECHTYPE      10
#define OPT_PIXPERFECT      11
#define OPT_NOWALKMODE      12
#define OPT_LETTERBOX       13
#define OPT_FIXEDINVCURSOR  14
#define OPT_NOLOSEINV       15
#define OPT_NOSCALEFNT      16
#define OPT_SPLITRESOURCES  17
#define OPT_ROTATECHARS     18
#define OPT_FADETYPE        19
#define OPT_HANDLEINVCLICKS 20
#define OPT_MOUSEWHEEL      21
#define OPT_DIALOGNUMBERED  22
#define OPT_DIALOGUPWARDS   23
#define OPT_CROSSFADEMUSIC  24
#define OPT_ANTIALIASFONTS  25
#define OPT_THOUGHTGUI      26
#define OPT_TURNTOFACELOC   27
#define OPT_RIGHTLEFTWRITE  28  // right-to-left text writing
#define OPT_DUPLICATEINV    29  // if they have 2 of the item, draw it twice
#define OPT_SAVESCREENSHOT  30
#define OPT_PORTRAITSIDE    31
#define OPT_STRICTSCRIPTING 32  // don't allow MoveCharacter-style commands
#define OPT_LEFTTORIGHTEVAL 33  // left-to-right operator evaluation
#define OPT_COMPRESSSPRITES 34
#define OPT_STRICTSTRINGS   35  // don't allow old-style strings
#define OPT_NEWGUIALPHA     36
#define OPT_RUNGAMEDLGOPTS  37
#define OPT_NATIVECOORDINATES 38
#define OPT_OLDTALKANIMSPD  39
#define OPT_HIGHESTOPTION   39
#define OPT_NOMODMUSIC      98
#define OPT_LIPSYNCTEXT     99
#define PORTRAIT_LEFT       0
#define PORTRAIT_RIGHT      1
#define PORTRAIT_ALTERNATE  2
#define PORTRAIT_XPOSITION  3
#define FADE_NORMAL         0
#define FADE_INSTANT        1
#define FADE_DISSOLVE       2
#define FADE_BOXOUT         3
#define FADE_CROSSFADE      4
#define FADE_LAST           4   // this should equal the last one
#define SPF_640x400         1
#define SPF_HICOLOR         2
#define SPF_DYNAMICALLOC    4
#define SPF_TRUECOLOR       8
#define SPF_ALPHACHANNEL 0x10
#define SPF_HADALPHACHANNEL 0x80  // the saved sprite on disk has one
//#define FFLG_NOSCALE        1
#define FFLG_SIZEMASK 0x003f
#define FONT_OUTLINE_AUTO -10
#define MAX_FONT_SIZE 63
struct OriGameSetupStruct {
  char              gamename[30];
  char              options[20];
  unsigned char     paluses[256];
  color             defpal[256];
  InterfaceElement  iface[10];
  int               numiface;
  int               numviews;
  MouseCursor       mcurs[10];
  char              *globalscript;
  int               numcharacters;
  OldCharacterInfo     *chars;
  EventBlock        __charcond[50];
  EventBlock        __invcond[100];
  ccScript          *compiled_script;
  int               playercharacter;
  unsigned char     __old_spriteflags[2100];
  int               totalscore;
  short             numinvitems;
  InventoryItemInfo invinfo[100];
  int               numdialog, numdlgmessage;
  int               numfonts;
  int               color_depth;              // in bytes per pixel (ie. 1 or 2)
  int               target_win;
  int               dialog_bullet;            // 0 for none, otherwise slot num of bullet point
  short             hotdot, hotdotouter;   // inv cursor hotspot dot
  int               uniqueid;    // random key identifying the game
  int               reserved[2];
  short             numlang;
  char              langcodes[MAXLANGUAGE][3];
  char              *messages[MAXGLOBALMES];
};

struct OriGameSetupStruct2 : public OriGameSetupStruct {
  unsigned char   fontflags[10];
  char            fontoutline[10];
  int             numgui;
  WordsDictionary *dict;
  int             reserved2[8];
};

struct OldGameSetupStruct : public OriGameSetupStruct2 {
  unsigned char spriteflags[6000];
};

// This struct is written directly to the disk file
// The GameSetupStruct subclass parts are written individually
struct GameSetupStructBase {
  char              gamename[50];
  int32             options[100];
  unsigned char     paluses[256];
  color             defpal[256];
  int32             numviews;
  int32             numcharacters;
  int32             playercharacter;
  int32             totalscore;
  short             numinvitems;
  int32             numdialog, numdlgmessage;
  int32             numfonts;
  int32             color_depth;          // in bytes per pixel (ie. 1 or 2)
  int32             target_win;
  int32             dialog_bullet;        // 0 for none, otherwise slot num of bullet point
  unsigned short    hotdot, hotdotouter;  // inv cursor hotspot dot
  int32             uniqueid;    // random key identifying the game
  int32             numgui;
  int32             numcursors;
  int32             default_resolution; // 0=undefined, 1=320x200, 2=320x240, 3=640x400 etc
  int32             default_lipsync_frame; // used for unknown chars
  int32             invhotdotsprite;
  int32             reserved[17];
  char             *messages[MAXGLOBALMES];
  WordsDictionary  *dict;
  char             *globalscript;
  CharacterInfo    *chars;
  ccScript         *compiled_script;

#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    fread(&gamename[0], sizeof(char), 50, fp);
    fseek(fp, 2, SEEK_CUR);    // skip the array padding
    fread(options, sizeof(int), 100, fp);
    fread(&paluses[0], sizeof(unsigned char), 256, fp);
    // colors are an array of chars
    fread(&defpal[0], sizeof(char), sizeof(color)*256, fp);
    numviews = getw(fp);
    numcharacters = getw(fp);
    playercharacter = getw(fp);
    totalscore = getw(fp);
    numinvitems = __getshort__bigendian(fp);
    fseek(fp, 2, SEEK_CUR);    // skip the padding
    numdialog = getw(fp);
    numdlgmessage = getw(fp);
    numfonts = getw(fp);
    color_depth = getw(fp);
    target_win = getw(fp);
    dialog_bullet = getw(fp);
    hotdot = __getshort__bigendian(fp);
    hotdotouter = __getshort__bigendian(fp);
    uniqueid = getw(fp);
    numgui = getw(fp);
    numcursors = getw(fp);
    default_resolution = getw(fp);
    default_lipsync_frame = getw(fp);
    invhotdotsprite = getw(fp);
    fread(reserved, sizeof(int), 17, fp);
    // read the final ptrs so we know to load dictionary, scripts etc
    fread(messages, sizeof(int), MAXGLOBALMES, fp);
    dict = (WordsDictionary *) getw(fp);
    globalscript = (char *) getw(fp);
    chars = (CharacterInfo *) getw(fp);
    compiled_script = (ccScript *) getw(fp);
  }
#endif
};

#define MAXVIEWNAMELENGTH 15
#define MAXLIPSYNCFRAMES  20
#define MAX_GUID_LENGTH   40
#define MAX_SG_EXT_LENGTH 20
#define MAX_SG_FOLDER_LEN 50
struct GameSetupStruct: public GameSetupStructBase {
  unsigned char     fontflags[MAX_FONTS];
  char              fontoutline[MAX_FONTS];
  unsigned char     spriteflags[MAX_SPRITES];
  InventoryItemInfo invinfo[MAX_INV];
  MouseCursor       mcurs[MAX_CURSOR];
  NewInteraction   **intrChar;
  NewInteraction   *intrInv[MAX_INV];
  InteractionScripts **charScripts;
  InteractionScripts **invScripts;
  int               filever;  // just used by editor
  char              lipSyncFrameLetters[MAXLIPSYNCFRAMES][50];
  CustomPropertySchema propSchema;
  CustomProperties  *charProps, invProps[MAX_INV];
  char              **viewNames;
  char              invScriptNames[MAX_INV][MAX_SCRIPT_NAME_LEN];
  char              dialogScriptNames[MAX_DIALOG][MAX_SCRIPT_NAME_LEN];
  char              guid[MAX_GUID_LENGTH];
  char              saveGameFileExtension[MAX_SG_EXT_LENGTH];
  char              saveGameFolderName[MAX_SG_FOLDER_LEN];
  int               roomCount;
  int              *roomNumbers;
  char            **roomNames;
  int               audioClipCount;
  ScriptAudioClip  *audioClips;
  int               audioClipTypeCount;
  AudioClipType    *audioClipTypes;
};

struct SpeechLipSyncLine {
  char  filename[14];
  int  *endtimeoffs;
  short*frame;
  short numPhenomes;
};

// permission flags
#define SMP_NOEDITINFO    1
#define SMP_NOEDITSCRIPTS 2
struct ScriptModule {
  char *name;
  char *author;
  char *version;
  char *description;
  char *scriptHeader;
  char *script;
  int  uniqueKey;
  int  permissions;
  int  weAreOwner;
  ccScript *compiled;

  void init() { 
    name = NULL;
    author = NULL;
    version = NULL;
    description = NULL;
    script = NULL;
    scriptHeader = NULL;
    uniqueKey = 0;
    permissions = 0;
    weAreOwner = 1;
    compiled = NULL;
  }

  ScriptModule() { init(); }
};


#ifndef ROOMEDIT
#define MAXNEEDSTAGES 40
struct MoveList {
  int   pos[MAXNEEDSTAGES];
  int   numstage;
  fixed xpermove[MAXNEEDSTAGES], ypermove[MAXNEEDSTAGES];
  int   fromx, fromy;
  int   onstage, onpart;
  int   lastx, lasty;
  char  doneflag;
  char  direct;  // MoveCharDirect was used or not
  };
#endif

#endif  // __CROOM_H
