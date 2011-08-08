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

#ifndef CROOM_NOFUNCTIONS
extern FILE *clibfopen(char *, char *);
#include "compress.h"
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

#ifndef CROOM_NOFUNCTIONS
char *croom_h_copyright = "ChrisRoom v2.00 - CRM reader/writer copyright (c) 1995, 1998, 1999 by Chris Jones.";
char *game_file_sig = "Adventure Creator Game File v2";
#define GAME_FILE_VERSION 42
block backups[5];


int cunpackbitl(unsigned char *, int size, FILE *infile);

/*long cloadcompfile(FILE*outpt,block tobesaved,color*pal,long poot=0) {
  fseek(outpt,poot,SEEK_SET);
  int widt,hit,hh;
  for (hh=0;hh<4;hh++) *tobesaved++=fgetc(outpt);
  tobesaved-=4;
  widt=*tobesaved++;  widt+=(*tobesaved++)*256;
  hit=*tobesaved++; hit+=(*tobesaved++)*256;
  unsigned char* ress=(unsigned char*)malloc(widt+1);
  for (int ww=0;ww<hit;ww++) {
    cunpackbitl(ress,widt,outpt);
    for (int ss=0;ss<widt;ss++)  (*tobesaved++)=ress[ss];
    }
  for (ww=0;ww<256;ww++) {
    pal[ww].r=fgetc(outpt);
    pal[ww].g=fgetc(outpt);
    pal[ww].b=fgetc(outpt);
    }
  poot=ftell(outpt); free(ress); tobesaved-=(widt*hit+4);
  return poot;
  }*/
#endif

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


#ifndef CROOM_NOFUNCTIONS

// ** SCHEMA LOAD/SAVE FUNCTIONS
void CustomPropertySchema::Serialize (FILE *outto) {
  putw (1, outto);  // version 1 at present
  putw (numProps, outto);
  for (int jj = 0; jj < numProps; jj++) {
    fputstring (propName[jj], outto);
    fputstring (propDesc[jj], outto);
    fputstring (defaultValue[jj], outto);
    putw (propType[jj], outto);
  }

}

int CustomPropertySchema::UnSerialize (FILE *infrom) {
  if (getw(infrom) != 1)
    return -1;
  numProps = getw(infrom);
  for (int kk = 0; kk < numProps; kk++) {
    this->resetProperty (kk);
    fgetstring_limit (propName[kk], infrom, 20);
    fgetstring_limit (propDesc[kk], infrom, 100);
    fgetstring_limit (defaultValue[kk], infrom, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    propType[kk] = getw(infrom);
  }

  return 0;
}


// ** OBJECT PROPERTIES LOAD/SAVE FUNCTIONS
void CustomProperties::Serialize (FILE *outto) {
  putw (1, outto);
  putw (numProps, outto);
  for (int ee = 0; ee < numProps; ee++) {
    fputstring (propName[ee], outto);
    fputstring (propVal[ee], outto);
  }
}

int CustomProperties::UnSerialize (FILE *infrom) {
  if (getw(infrom) != 1)
    return -1;
  numProps = getw(infrom);
  for (int ee = 0; ee < numProps; ee++) {
    propName[ee] = (char*)malloc(200);
    propVal[ee] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    fgetstring_limit (propName[ee], infrom, 200);
    fgetstring_limit (propVal[ee], infrom, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
  }
  return 0;
}
#endif // NOFUNCTIONS


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

#ifndef CROOM_NOFUNCTIONS

int in_interaction_editor = 0;


void WordsDictionary::sort () {
  int aa, bb;
  for (aa = 0; aa < num_words; aa++) {
    for (bb = aa + 1; bb < num_words; bb++) {
      if (((wordnum[aa] == wordnum[bb]) && (stricmp(word[aa], word[bb]) > 0))
          || (wordnum[aa] > wordnum[bb])) {
        short temp = wordnum[aa];
        char tempst[30];

        wordnum[aa] = wordnum[bb];
        wordnum[bb] = temp;
        strcpy(tempst, word[aa]);
        strcpy(word[aa], word[bb]);
        strcpy(word[bb], tempst);
        bb = aa;
      }
    }
  }
}

int WordsDictionary::find_index (const char*wrem) {
  int aa;
  for (aa = 0; aa < num_words; aa++) {
    if (stricmp (wrem, word[aa]) == 0)
      return aa;
  }
  return -1;
}

// {name, flags, numArgs, {argTypes}, {argNames}, description, textscript}
ActionTypes actions[NUM_ACTION_TYPES] = {
  {"Do nothing", 0, 0, {NULL}, {NULL}, "Does nothing.", ""},
  {"Run script", AFLG_RUNSCRIPT, 0, {NULL}, {NULL}, "Runs a text script. Click the 'Edit Script' button to modify the script.", ""},
  {"Game - Add score on first execution", 0, 1, {ARG_TYPE_INT}, {"Points to add"},
      "Gives the player $$1 extra points the first time this action is run.",
      ""},
  {"Game - Add score", 0, 1, {ARG_TYPE_INT}, {"Points to add"},
      "Gives the player $$1 extra points every time this action is run.",
      "GiveScore($$1);"},
  {"Game - Display a message", AFLG_MESSAGE, 1, {ARG_TYPE_MSG}, {"Message number"},
      "Displays message $$1 to the player.",
      "DisplayMessage($$1);"},
  {"Game - Play music", 0, 1, {ARG_TYPE_INT}, {"Music number"},
      "Changes the current background music to MUSIC$$1.MP3, WAV, MID or MOD",
      "PlayMusic($$1);"},
  {"Game - Stop music", 0, 0, {NULL}, {NULL},
      "Stops the currently playing background music.",
      "StopMusic();"},
  {"Game - Play sound effect", 0, 1, {ARG_TYPE_INT}, {"Sound number"},
      "Plays SOUND$$1.MP3 or SOUND$$1.WAV",
      "PlaySound($$1);"},
  {"Game - Play Flic animation", 0, 2, {ARG_TYPE_INT, ARG_TYPE_BOOL}, {"Flic number", "Player can skip"},
      "Plays FLIC$$1.FLC or FLIC$$1.FLI",
      "PlayFlic($$1, $$2);"},
  {"Game - Run dialog", 0, 1, {ARG_TYPE_INT}, {"Dialog topic number"},
      "Starts a conversation using dialog topic $$1.",
      "dialog[$$1].Start();"},
  {"Game - Enable dialog option", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Dialog topic number", "Option number"},
      "Enables dialog option $$2 in topic $$1 to be visible to the player.",
      "dialog[$$1].SetOptionState($$2, eOptionOn);"},
  {"Game - Disable dialog option", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Dialog topic number", "Option number"},
      "Stops dialog option $$2 in topic $$1 from being visible to the player.",
      "dialog[$$1].SetOptionState($$2, eOptionOff);"},
  {"Player - Go to a different room", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"New room number", "Edge+offset value"},
      "Takes the player to room $$1, optionally choosing position using old-style Data column value $$2",
      "player.ChangeRoom($$1);"},
  {"Player - Give the player an inventory item", 0, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Adds inventory item $$1 to the player character's inventory.",
      "player.AddInventory(inventory[$$1]);"},
  {"Object - Move object", 0, 5, {ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_BOOL},
      {"Object number", "Destination X location", "Destination Y location", "Move speed", "Wait for move to finish"},
      "Starts object $$1 moving towards ($$2, $$3), at speed $$4.",
      "object[$$1].Move($$2, $$3, $$4);"},
  {"Object - Remove an object from the room", 0, 1, {ARG_TYPE_INT}, {"Object number"},
      "Switches room object $$1 off so the player cannot see or interact with it.",
      "object[$$1].Visible = false;"},
  {"Object - Switch an object back on", 0, 1, {ARG_TYPE_INT}, {"Object number"},
      "Switches room object $$1 on, so that the player can see and interact with it.",
      "object[$$1].Visible = true;"},
  {"Object - Set object view number", 0, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Object number", "New view number"},
      "Changes object $$1's view number to $$2",
      "object[$$1].SetView($$2);"},
  {"Object - Start object animating", 0, 4, {ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_BOOL},
      {"Object number", "Loop number", "Speed", "Repeat"},
      "Starts object $$1 animating, using loop $$2 of its current view, and animating at speed $$3.",
      "object[$$1].Animate($$2, $$3);"},
  {"Character - Move character", 0, 4, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_BOOL},
      {"Character", "Destination X location", "Destination Y location", "Wait for move to finish"},
      "Starts character $$1 moving towards ($$2, $$3).",
      "character[$$1].Walk($$2, $$3);"},
  {"Conditional - If inventory item was used", AFLG_COND | AFLG_INVCHECK, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Performs child actions if the player just used inventory item $$1 on this interaction.",
      "if (player.ActiveInventory == inventory[$$1]) {"},
  {"Conditional - If the player has an inventory item", AFLG_COND, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Performs child actions if the player character has inventory item $$1 in their current inventory.",
      "if (player.InventoryQuantity[$$1] > 0) {"},
  {"Conditional - If a character is moving", AFLG_COND, 1, {ARG_TYPE_CHAR}, {"Character number"},
      "Performs child actions if character $$1 is currently moving",
      "if (character[$$1].Moving) {"},
  {"Conditional - If a variable is set to a certain value", AFLG_COND, 2, {ARG_TYPE_INT, ARG_TYPE_INT}, {"Variable", "Value"},
      "Performs child actions if $$1 == $$2",
      "if (GetGraphicalVariable(\"$$1\") == $$2) { "},
  {"Character - Stop character walking", 0, 1, {ARG_TYPE_CHAR}, {"Character"},
      "Immediately stops character $$1 from moving.",
      "character[$$1].StopMoving();"},
  {"Player - Go to a different room (at specific co-ordinates)", 0, 3, {ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT},
      {"New room number", "X co-ordinate", "Y co-ordinate"},
      "Takes the player to room $$1, and places him at ($$2, $$3)",
      "player.ChangeRoom($$1, $$2, $$3);"},
  {"Character - Move NPC to different room", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_INT},
      {"Character", "New room number"},
      "Places non-player character $$1 into room $$2.",
      "character[$$1].ChangeRoom($$2);"},
  {"Character - Set character view", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_INT},
      {"Character", "View number"},
      "Locks character $$1's view to view $$2, in preparation for doing an animation. Use 'Release Character View' afterwards to release it.",
      "character[$$1].LockView($$2);"},
  {"Character - Release character view", 0, 1, {ARG_TYPE_CHAR}, {"Character"},
      "Reverts character $$1's view back to its normal view and enables standard engine processing.",
      "character[$$1].UnlockView();"},
  {"Character - Follow another character", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_CHAR},
      {"Character", "Follow Character"},
      "Makes character $$1 follow $$2 around the screen.",
      "character[$$1].FollowCharacter($$2);"},
  {"Character - Stop following", 0, 1, {ARG_TYPE_CHAR}, {"Character"},
      "Stops character $$1 following any other characters.",
      "character[$$1].FollowCharacter(null);"},
  {"Room - Disable hotspot", 0, 1, {ARG_TYPE_INT}, {"Hotspot number"},
      "Disables hotspot $$1 in the current room.",
      "hotspot[$$1].Enabled = false;"},
  {"Room - Enable hotspot", 0, 1, {ARG_TYPE_INT}, {"Hotspot number"},
      "Re-enables hotspot $$1 in the current room.",
      "hotspot[$$1].Enabled = true;"},
  {"Game - Set variable value", 0, 2, {ARG_TYPE_VARONLY, ARG_TYPE_INT}, {"Variable", "New value"},
      "Sets variable $$1 to have the value $$2",
      "SetGraphicalVariable(\"$$1\", $$2);"},
  {"Character - Run animation loop", 0, 3, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "Loop number", "Speed"},
      "Runs character $$1 through loop $$2 of its current view, animating at speed $$3. Waits for animation to finish before continuing.",
      ""},  
  {"Character - Quick animation", 0, 4, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "View number", "Loop number", "Speed"},
      "Does SetCharacterView($$1, $$2), AnimateCharacter($$1, $$3, $$4), ReleaseCharacterView($$1) in order.",
      ""},
  {"Character - Set idle animation", 0, 3, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "View number", "Delay"},
      "Sets character $$1 to use view $$2 as its idle animation, with a timeout of $$3 seconds of inactivity before the animation is played.",
      "character[$$1].SetIdleView($$2, $$3);"},
  {"Character - Disable idle animation", 0, 1, {ARG_TYPE_CHAR},
      {"Character"},
      "Disables character $$1's idle animation, so it will no longer be played.",
      "character[$$1].SetIdleView(-1, -1);"},
  {"Player - Remove an item from the inventory", 0, 1, {ARG_TYPE_INV}, {"Inventory item number"},
      "Removes inventory item $$1 from the player character's inventory.",
      "player.LoseInventory(inventory[$$1]);"},
  {"Game - Show GUI", 0, 1, {ARG_TYPE_INT}, {"GUI number"},
      "Switches on GUI number $$1 so the player can see it.",
      "gui[$$1].Visible = true;"},
  {"Game - Hide GUI", 0, 1, {ARG_TYPE_INT}, {"GUI number"},
      "Switches off GUI number $$1 so the player can no longer see it.",
      "gui[$$1].Visible = false;"},
  {"Stop running more commands", 0, 0, {NULL}, {NULL},
      "Stops running the interaction list at this point. Useful at the end of a block of actions inside a conditional.",
      "return;"},
  {"Character - Face location", 0, 3, {ARG_TYPE_CHAR, ARG_TYPE_INT, ARG_TYPE_INT},
      {"Character", "X co-ordinate", "Y co-ordinate"},
      "Turns character $$1 so that they are facing the room co-ordinates ($$2, $$3).",
      "character[$$1].FaceLocation($$2, $$3);"},
  {"Game - Pause command processor for a set time", 0, 1, {ARG_TYPE_INT},
      {"Loops to wait"},
      "Stops processing actions here and lets the game continue execution for $$1 game loops (default 40 per second) before continuing with the next command.",
      "Wait($$1);"},
  {"Character - Change character view", 0, 2, {ARG_TYPE_CHAR, ARG_TYPE_INT},
      {"Character", "New view number"},
      "Changes character $$1's normal walking view to View $$2 permanently, until you call this command again.",
      "character[$$1].ChangeView($$2);"},
  {"Conditional - If the player character is", AFLG_COND, 1, {ARG_TYPE_CHAR},
      {"Character"},
      "Performs child actions if the player character is currently $$1. Useful in games where the player can control multiple characters.",
      "if (player.ID == $$1) {"},
  {"Conditional - If mouse cursor mode is", AFLG_COND, 1, {ARG_TYPE_INT},
      {"Mouse cursor"},
      "Performs child actions if the current cursor mode is mode $$1 (from the Cursors pane).",
      "if (mouse.Mode == $$1) {"},
  {"Conditional - If the player has been in room", AFLG_COND, 1, {ARG_TYPE_INT},
      {"Room number"},
      "Performs child actions if the player has been to room $$1 during the game.",
	  "if (HasPlayerBeenInRoom($$1)) {"}
};
InteractionVariable globalvars[MAX_GLOBAL_VARIABLES] = {{"Global 1", 0, 0}};
int numGlobalVars = 1;

void serialize_command_list (NewInteractionCommandList *nicl, FILE*ooo) {
  if (nicl == NULL)
    return;
  putw (nicl->numCommands, ooo);
  putw (nicl->timesRun, ooo);
#ifndef ALLEGRO_BIG_ENDIAN
  fwrite (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands, ooo);
#else
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
    nicl->command[iteratorCount].WriteToFile(ooo);
  }
#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL)
      serialize_command_list (nicl->command[k].get_child_list(), ooo);
  }
}

void serialize_new_interaction (NewInteraction *nint, FILE*ooo) {
  int a;

  putw (1, ooo);  // Version
  putw (nint->numEvents, ooo);
  fwrite (&nint->eventTypes[0], sizeof(int), nint->numEvents, ooo);
  for (a = 0; a < nint->numEvents; a++)
    putw ((int)nint->response[a], ooo);

  for (a = 0; a < nint->numEvents; a++) {
    if (nint->response[a] != NULL)
      serialize_command_list (nint->response[a], ooo);
  }
}

NewInteractionCommandList *deserialize_command_list (FILE *ooo) {
  NewInteractionCommandList *nicl = new NewInteractionCommandList;
  nicl->numCommands = getw(ooo);
  nicl->timesRun = getw(ooo);
#ifndef ALLEGRO_BIG_ENDIAN
  fread (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands, ooo);
#else
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
    nicl->command[iteratorCount].ReadFromFile(ooo);
  }
#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL) {
      nicl->command[k].children = deserialize_command_list (ooo);
    }
    nicl->command[k].parent = nicl;
  }
  return nicl;
}

NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (FILE *ooo) {
  int a;

  if (getw(ooo) != 1)
    return NULL;
  nitemp = new NewInteraction;
  nitemp->numEvents = getw(ooo);
  if (nitemp->numEvents > MAX_NEWINTERACTION_EVENTS) {
    quit("Error: this interaction was saved with a newer version of AGS");
    return NULL;
  }
  fread (&nitemp->eventTypes[0], sizeof(int), nitemp->numEvents, ooo);
  //fread (&nitemp->response[0], sizeof(void*), nitemp->numEvents, ooo);
  for (a = 0; a < nitemp->numEvents; a++)
    nitemp->response[a] = (NewInteractionCommandList*)getw(ooo);

  for (a = 0; a < nitemp->numEvents; a++) {
    if (nitemp->response[a] != NULL)
      nitemp->response[a] = deserialize_command_list (ooo);
    nitemp->timesRun[a] = 0;
  }
  return nitemp;
}

void NewInteractionCommandList::reset () {
  int j;
  for (j = 0; j < numCommands; j++) {
    if (command[j].children != NULL) {
      // using this Reset crashes it for some reason
      //command[j].reset ();
      command[j].get_child_list()->reset();
      delete command[j].children;
      command[j].children = NULL;
    }
    command[j].remove();
  }
  numCommands = 0;
  timesRun = 0;
}



int ff;
/*void roomstruct::allocall() {
  // These all get recreated when a room is loaded anyway
  walls = create_bitmap_ex(8, 320, 200);
  object = create_bitmap_ex(8, 320, 200);
  lookat = create_bitmap_ex(8, 320, 200);
  bscene = create_bitmap_ex(8, 320, 200);
  shading = create_bitmap_ex(8, 320, 200);

  if (shading == NULL)
    quit("roomstruct::allocall: out of memory");

 //  printf("Before %ld\n",farcoreleft());
  for (ff=0;ff<5;ff++) { //backups[ff]=wnewblock(0,0,319,199);
    backups[ff]=wallocblock(320,200);
//    printf("%d ",ff); if (kbhit()) break;
    if (backups[ff]==NULL) quit("ROOM.C, AllocMem: Out of memory"); }
  walls=::backups[0];  // this is because blocks in a struct don't work
  object=::backups[1]; // properly
  lookat=::backups[2];
  bscene=::backups[3];
  shading=::backups[4];
//  printf("After %ld\n",farcoreleft());
  }
*/
void roomstruct::freemessage() {
  for (int f = 0; f < nummes; f++) {
    if (message[f] != NULL)
      free(message[f]);
  }
}

/*void roomstruct::freeall() {
//  for (int f=0;f<4;f++) wfreeblock(::backups[f]);
  wfreeblock(walls);
  wfreeblock(lookat);
  wfreeblock(ebscene[0]);
  wfreeblock(object);
  
  if (shading != NULL)
    wfreeblock(shading);

  freemessage();
}*/

/*void roomstruct::freeall() { wfreeblock(walls); wfreeblock(bscene);
  wfreeblock(object); wfreeblock(lookat);
  for (int f=0;f<nummes;f++) if (message[f]!=NULL) free(message[f]); }*/

struct room_file_header {
  short version PCKD;
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    version = __getshort__bigendian(fp);
  }
#endif
};

int _acroom_bpp = 1;  // bytes per pixel of currently loading room

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(BITMAP*bmpt) {
  if (bitmap_color_depth(bmpt) == 15)
    return 2;

  return bitmap_color_depth(bmpt) / 8;
}

#ifdef LOADROOM_DO_POLL
extern void update_polled_stuff();
#else
static void update_polled_stuff() { }
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)
extern void lzwcompress(FILE *,FILE *);
extern void lzwexpand(FILE *,FILE *);
extern unsigned char *lzwexpand_to_mem(FILE *);
extern long maxsize, outbytes, putbytes;
char *lztempfnm = "~aclzw.tmp";

long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe) {
  FILE  *ooo, *iii;
  long  fll, toret, gobacto;

  ooo = ci_fopen(lztempfnm, "wb");
  putw(bmpp->w * bmp_bpp(bmpp), ooo);
  putw(bmpp->h, ooo);
  fwrite(&bmpp->line[0][0], bmpp->w * bmp_bpp(bmpp), bmpp->h, ooo);
  fclose(ooo);

  iii = ci_fopen(fnn, "r+b");
  fseek(iii, offe, SEEK_SET);

  ooo = ci_fopen(lztempfnm, "rb");
  fll = filelength(fileno(ooo));
  fwrite(&pall[0], sizeof(color), 256, iii);
  fwrite(&fll, 4, 1, iii);
  gobacto = ftell(iii);

  // reserve space for compressed size
  fwrite(&fll, 4, 1, iii);
  lzwcompress(ooo, iii);
  toret = ftell(iii);
  fseek(iii, gobacto, SEEK_SET);
  fll = (toret - gobacto) - 4;
  fwrite(&fll, 4, 1, iii);      // write compressed size
  fclose(ooo);
  fclose(iii);
  unlink(lztempfnm);

  return toret;
}

BITMAP *recalced;
/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  fseek(iii,ooff,SEEK_SET);*/

long load_lzw(FILE *iii, BITMAP *bmm, color *pall) {
  long          uncompsiz, *loptr;
  unsigned char *membuffer;
  int           arin;

  recalced = bmm;
  // MACPORT FIX (HACK REALLY)
  fread(&pall[0], 1, sizeof(color)*256, iii);
  fread(&maxsize, 4, 1, iii);
  fread(&uncompsiz,4,1,iii);

  uncompsiz += ftell(iii);
  outbytes = 0; putbytes = 0;

  update_polled_stuff();
  membuffer = lzwexpand_to_mem(iii);
  update_polled_stuff();

  loptr = (long *)&membuffer[0];
  membuffer += 8;
#ifdef ALLEGRO_BIG_ENDIAN
  loptr[0] = __int_swap_endian(loptr[0]);
  loptr[1] = __int_swap_endian(loptr[1]);
  int bitmapNumPixels = loptr[0]*loptr[1]/_acroom_bpp;
  switch (_acroom_bpp) // bytes per pixel!
  {
    case 1:
    {
      // all done
      break;
    }
    case 2:
    {
      short *sp = (short *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        sp[i] = __short_swap_endian(sp[i]);
      }
      // all done
      break;
    }
    case 4:
    {
      int *ip = (int *)membuffer;
      for (int i = 0; i < bitmapNumPixels; ++i)
      {
        ip[i] = __int_swap_endian(ip[i]);
      }
      // all done
      break;
    }
  }
#endif

  if (bmm!=NULL)
    destroy_bitmap(bmm);

  update_polled_stuff();

  bmm = create_bitmap_ex(_acroom_bpp * 8, (loptr[0] / _acroom_bpp), loptr[1]);
  if (bmm == NULL)
    quit("!load_room: not enough memory to load room background");

  update_polled_stuff();

  acquire_bitmap (bmm);
  recalced = bmm;

  for (arin = 0; arin < loptr[1]; arin++)
    memcpy(&bmm->line[arin][0], &membuffer[arin * loptr[0]], loptr[0]);

  release_bitmap (bmm);

  update_polled_stuff();

  free(membuffer-8);

  if (ftell(iii) != uncompsiz)
    fseek(iii, uncompsiz, SEEK_SET);

  update_polled_stuff();

  return uncompsiz;
}

long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo) {
  unsigned char *wgtbl = (unsigned char *)malloc(bmpp->w * bmpp->h + 4);
  short         *sss = (short *)wgtbl;
  long          toret;

  sss[0] = bmpp->w;
  sss[1] = bmpp->h;

  memcpy(&wgtbl[4], &bmpp->line[0][0], bmpp->w * bmpp->h);

  toret = csavecompressed(fnn, wgtbl, pall, ooo);
  free(wgtbl);
  return toret;
}

long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo) {
  short widd,hitt;
  int   ii;

  BITMAP *bim = *bimpp;
  if (bim != NULL)
    destroy_bitmap(bim);

  fread(&widd,2,1,fpp);
  fread(&hitt,2,1,fpp);
  bim = create_bitmap_ex(8, widd, hitt);
  if (bim == NULL)
    quit("!load_room: not enough memory to decompress masks");
  *bimpp = bim;

  for (ii = 0; ii < hitt; ii++) {
    cunpackbitl(&bim->line[ii][0], widd, fpp);
    if (ii % 20 == 0)
      update_polled_stuff();
  }

  fseek(fpp, 768, SEEK_CUR);  // skip palette

  return ftell(fpp);
}
#endif

#define BLOCKTYPE_MAIN        1
#define BLOCKTYPE_SCRIPT      2
#define BLOCKTYPE_COMPSCRIPT  3
#define BLOCKTYPE_COMPSCRIPT2 4
#define BLOCKTYPE_OBJECTNAMES 5
#define BLOCKTYPE_ANIMBKGRND  6
#define BLOCKTYPE_COMPSCRIPT3 7     // new CSCOMP script instead of SeeR
#define BLOCKTYPE_PROPERTIES  8
#define BLOCKTYPE_OBJECTSCRIPTNAMES 9
#define BLOCKTYPE_EOF         0xff

extern void load_script_configuration(FILE *);
extern void save_script_configuration(FILE *);
extern void load_graphical_scripts(FILE *, roomstruct *);
extern void save_graphical_scripts(FILE *, roomstruct *);
static char *passwencstring = "Avis Durgan";

void decrypt_text(char*toenc) {
  int adx = 0;

  while (1) {
    toenc[0] -= passwencstring[adx];
    if (toenc[0] == 0)
      break;

    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

#ifndef NO_SAVE_FUNCTIONS

void encrypt_text(char *toenc) {
  int adx = 0, tobreak = 0;

  while (tobreak == 0) {
    if (toenc[0] == 0)
      tobreak = 1;

    toenc[0] += passwencstring[adx];
    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void write_string_encrypt(FILE *ooo, char *sss) {
  int stlent = (int)strlen(sss) + 1;

  putw(stlent, ooo);
  encrypt_text(sss);
  fwrite(sss, stlent, 1, ooo);
  decrypt_text(sss);
}

void write_dictionary (WordsDictionary *dict, FILE *writeto) {
  int ii;

  putw(dict->num_words, writeto);
  for (ii = 0; ii < dict->num_words; ii++) {
    write_string_encrypt (writeto, dict->word[ii]);
#ifndef ALLEGRO_BIG_ENDIAN
    fwrite(&dict->wordnum[ii], sizeof(short), 1, writeto);
#else
    __putshort__lilendian(dict->wordnum[ii], writeto);
#endif  // ALLEGRO_BIG_ENDIAN
  }
}
#endif  // NO_SAVE_FUCNTIONS

void read_string_decrypt(FILE *ooo, char *sss) {
  int newlen = getw(ooo);
  if ((newlen < 0) || (newlen > 5000000))
    quit("ReadString: file is corrupt");

  // MACPORT FIX: swap as usual
  fread(sss, sizeof(char), newlen, ooo);
  sss[newlen] = 0;
  decrypt_text(sss);
}

void read_dictionary (WordsDictionary *dict, FILE *writeto) {
  int ii;

  dict->allocate_memory(getw(writeto));
  for (ii = 0; ii < dict->num_words; ii++) {
    read_string_decrypt (writeto, dict->word[ii]);
    fread(&dict->wordnum[ii], sizeof(short), 1, writeto);
  }
}

void freadmissout(short *pptr, FILE *opty) {
  fread(&pptr[0], 2, 5, opty);
  fread(&pptr[7], 2, NUM_CONDIT - 7, opty);
  pptr[5] = pptr[6] = 0;
}

#define HS_STANDON    0
#define HS_LOOKAT     1
#define HS_INTERACT   2
#define HS_USEINV     3
#define HS_TALKTO     4
#define OBJ_LOOKAT    0
#define OBJ_INTERACT  1
#define OBJ_TALKTO    2
#define OBJ_USEINV    3

void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr) {
  evpt->list[evpt->numcmd] = evnt;
  evpt->respond[evpt->numcmd] = whatac;
  evpt->respondval[evpt->numcmd] = val1;
  evpt->data[evpt->numcmd] = data;
  evpt->score[evpt->numcmd] = scorr;
  evpt->numcmd++;
}

int usesmisccond = 0;

void deserialize_interaction_scripts(FILE *iii, InteractionScripts *scripts)
{
  int numEvents = getw(iii);
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
    quit("Too many interaction script events");
  scripts->numEvents = numEvents;

  char buffer[200];
  for (int i = 0; i < numEvents; i++)
  {
    fgetstring_limit(buffer, iii, sizeof(buffer));
    scripts->scriptFuncNames[i] = new char[strlen(buffer) + 1];
    strcpy(scripts->scriptFuncNames[i], buffer);
  }
}

void load_main_block(roomstruct *rstruc, char *files, FILE *opty, room_file_header rfh) {
  int   f, gsmod, NUMREAD;
  char  buffre[3000];
  long  tesl;

  usesmisccond = 0;
  rstruc->width = 320;
  rstruc->height = 200;
  rstruc->resolution = 1;
  rstruc->numwalkareas = 0;
  rstruc->numhotspots = 0;

  memset(&rstruc->shadinginfo[0], 0, sizeof(short) * 16);
  memset(&rstruc->sprs[0], 0, sizeof(sprstruc) * MAX_INIT_SPR);
  memset(&rstruc->objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
  memset(&rstruc->objectFlags[0], 0, sizeof(short) * MAX_INIT_SPR);
  memset(&rstruc->hswalkto[0], 0, sizeof(_Point) * MAX_HOTSPOTS);
  memset(&rstruc->walk_area_zoom[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
  memset(&rstruc->walk_area_light[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));

  for (f = 0; f < MAX_HOTSPOTS; f++) {
    rstruc->hotspotScriptNames[f][0] = 0;
	if (rstruc->hotspotnames[f] != NULL)
		free(rstruc->hotspotnames[f]);

	rstruc->hotspotnames[f] = (char*)malloc(20);
    sprintf(rstruc->hotspotnames[f], "Hotspot %d", f);

    if (f == 0)
      strcpy(rstruc->hotspotnames[f], "No hotspot");
  }

/*  memset(&rstruc->hscond[0], 0, sizeof(EventBlock) * MAX_HOTSPOTS);
  memset(&rstruc->objcond[0], 0, sizeof(EventBlock) * MAX_INIT_SPR);
  memset(&rstruc->misccond, 0, sizeof(EventBlock));*/

  if (rfh.version >= 12)
    _acroom_bpp = getw(opty);
  else
    _acroom_bpp = 1;

  if (_acroom_bpp < 1)
    _acroom_bpp = 1;

  rstruc->bytes_per_pixel = _acroom_bpp;
  fread(&rstruc->numobj, 2, 1, opty);
  if (rstruc->numobj > MAX_OBJ)
    quit("!room newer than this version - too many walk-behinds");

  NUMREAD = NUM_CONDIT;
  fread(&rstruc->objyval[0], 2, rstruc->numobj, opty);

  gsmod = 0;

  fread(&rstruc->numhotspots, sizeof(int), 1, opty);
  if (rstruc->numhotspots == 0)
    rstruc->numhotspots = 20;
	if (rstruc->numhotspots > MAX_HOTSPOTS)
		quit("room has too many hotspots: need newer version of AGS?");

#ifndef ALLEGRO_BIG_ENDIAN
    fread(&rstruc->hswalkto[0], sizeof(_Point), rstruc->numhotspots, opty);
#else
    // Points are a pair of shorts
    fread(&rstruc->hswalkto[0], sizeof(short), 2*rstruc->numhotspots, opty);
#endif

	for (f = 0; f < rstruc->numhotspots; f++)
	{
		free(rstruc->hotspotnames[f]);
		if (rfh.version >= 28)
		{
			fgetstring_limit(buffre, opty, 2999);
			rstruc->hotspotnames[f] = (char*)malloc(strlen(buffre) + 1);
			strcpy(rstruc->hotspotnames[f], buffre);
		}
		else
		{
			rstruc->hotspotnames[f] = (char*)malloc(30);
			fread(rstruc->hotspotnames[f], 30, 1, opty);
		}
	}

  if (rfh.version >= 24)
    fread(&rstruc->hotspotScriptNames[0], MAX_SCRIPT_NAME_LEN, rstruc->numhotspots, opty);
    
  fread(&rstruc->numwalkareas, 4, 1, opty);
  // MACPORT FIX: read polypoints
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rstruc->wallpoints[0], sizeof(PolyPoints), rstruc->numwalkareas, opty);
#else
  for (int iteratorCount = 0; iteratorCount < rstruc->numwalkareas; ++iteratorCount)
  {
    rstruc->wallpoints[iteratorCount].ReadFromFile(opty);
  }
#endif
  
  update_polled_stuff();

  fread(&rstruc->top, 2, 1, opty);
  fread(&rstruc->bottom, 2, 1, opty);
  fread(&rstruc->left, 2, 1, opty);
  fread(&rstruc->right, 2, 1, opty);

  fread(&rstruc->numsprs, 2, 1, opty);
  // MACPORT FIX: read sprstrucs
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rstruc->sprs[0], sizeof(sprstruc), rstruc->numsprs, opty);
#else
  for (int iteratorCount = 0; iteratorCount < rstruc->numsprs; ++iteratorCount)
  {
    rstruc->sprs[iteratorCount].ReadFromFile(opty);
  }
#endif

  if (rfh.version >= 19) {
    rstruc->numLocalVars = getw(opty);
    if (rstruc->numLocalVars > 0) {
      rstruc->localvars = (InteractionVariable*)malloc (sizeof(InteractionVariable) * rstruc->numLocalVars);
#ifndef ALLEGRO_BIG_ENDIAN
      fread (&rstruc->localvars[0], sizeof(InteractionVariable), rstruc->numLocalVars, opty);
#else
      for (int iteratorCount = 0; iteratorCount < rstruc->numLocalVars; ++iteratorCount)
      {
        rstruc->localvars[iteratorCount].ReadFromFile(opty);
      }
#endif
    }
  }
  
  rstruc->numRegions = 0;

  if (rfh.version >= 15) {
    if ((rstruc->numhotspots > MAX_HOTSPOTS) || (rstruc->numsprs > MAX_INIT_SPR))
      quit("load_room: room file created with newer version (too many hotspots/objects)");

    // free all of the old interactions
    for (f = 0; f < MAX_HOTSPOTS; f++) {
      if (rstruc->intrHotspot[f] != NULL) {
        delete rstruc->intrHotspot[f];
        rstruc->intrHotspot[f] = NULL;
      }

	  if (rfh.version < 26) 
	  {
		  if (f < rstruc->numhotspots)
			rstruc->intrHotspot[f] = deserialize_new_interaction (opty);
		  else
			rstruc->intrHotspot[f] = new NewInteraction();
	  }
    }

    for (f = 0; f < MAX_INIT_SPR; f++) {
      if (rstruc->intrObject[f] != NULL) {
        delete rstruc->intrObject[f];
        rstruc->intrObject[f] = NULL;
      }

	  if (rfh.version < 26) 
	  {
		  if (f < rstruc->numsprs)
			rstruc->intrObject[f] = deserialize_new_interaction (opty);
		  else
			rstruc->intrObject[f] = new NewInteraction();
	  }
    }

	if (rfh.version < 26) 
	{
	    delete rstruc->intrRoom;
		rstruc->intrRoom = deserialize_new_interaction (opty);
	}

    for (f = 0; f < MAX_REGIONS; f++) {
      if (rstruc->intrRegion[f] != NULL)
        delete rstruc->intrRegion[f];
      rstruc->intrRegion[f] = new NewInteraction();
    }

    if (rfh.version >= 21) {
      rstruc->numRegions = getw (opty);
      if (rstruc->numRegions > MAX_REGIONS)
        quit("load_room: needs newer version of AGS - too many regions");

  	  if (rfh.version < 26) 
	  {
        for (f = 0; f < rstruc->numRegions; f++) {
          delete rstruc->intrRegion[f];
          rstruc->intrRegion[f] = deserialize_new_interaction (opty);
		}
      }
    }

	if (rfh.version >= 26)
	{
	  rstruc->hotspotScripts = new InteractionScripts*[rstruc->numhotspots];
	  rstruc->objectScripts = new InteractionScripts*[rstruc->numsprs];
      rstruc->regionScripts = new InteractionScripts*[rstruc->numRegions];
	  rstruc->roomScripts = new InteractionScripts();
	  deserialize_interaction_scripts(opty, rstruc->roomScripts);
	  int bb;
      for (bb = 0; bb < rstruc->numhotspots; bb++) {
        rstruc->hotspotScripts[bb] = new InteractionScripts();
        deserialize_interaction_scripts(opty, rstruc->hotspotScripts[bb]);
      }
      for (bb = 0; bb < rstruc->numsprs; bb++) {
        rstruc->objectScripts[bb] = new InteractionScripts();
        deserialize_interaction_scripts(opty, rstruc->objectScripts[bb]);
      }
	  for (bb = 0; bb < rstruc->numRegions; bb++) {
        rstruc->regionScripts[bb] = new InteractionScripts();
        deserialize_interaction_scripts(opty, rstruc->regionScripts[bb]);
      }

	}
  }

  if (rfh.version >= 9) {
    fread(&rstruc->objbaseline[0], sizeof(int), rstruc->numsprs, opty);
    fread(&rstruc->width, 2, 1, opty);
    fread(&rstruc->height, 2, 1, opty); 
  }

  if (rfh.version >= 23)
    fread(&rstruc->objectFlags[0], sizeof(short), rstruc->numsprs, opty);

  if (rfh.version >= 11)
    fread(&rstruc->resolution, 2, 1, opty);

  int num_walk_areas = MAX_WALK_AREAS;
  if (rfh.version >= 14)
    num_walk_areas = getw (opty);
    
  if (num_walk_areas > MAX_WALK_AREAS + 1)
    quit("load_room: Too many walkable areas, need newer version");

  if (rfh.version >= 10)
    fread(&rstruc->walk_area_zoom[0], sizeof(short), num_walk_areas, opty);

  if (rfh.version >= 13)
    fread(&rstruc->walk_area_light[0], sizeof(short), num_walk_areas, opty);

  if (rfh.version >= 18) {
    fread(&rstruc->walk_area_zoom2[0], sizeof(short), num_walk_areas, opty);
    fread(&rstruc->walk_area_top[0], sizeof(short), num_walk_areas, opty);
    fread(&rstruc->walk_area_bottom[0], sizeof(short), num_walk_areas, opty);

    for (f = 0; f < num_walk_areas; f++) {
      // if they set a contiuously scaled area where the top
      // and bottom zoom levels are identical, set it as a normal
      // scaled area
      if (rstruc->walk_area_zoom[f] == rstruc->walk_area_zoom2[f])
        rstruc->walk_area_zoom2[f] = NOT_VECTOR_SCALED;
    }
  }

  fread(&rstruc->password[0], 11, 1, opty);
  fread(&rstruc->options[0], 10, 1, opty);
  fread(&rstruc->nummes, 2, 1, opty);

  if (rfh.version >= 25)
    rstruc->gameId = getw(opty);

  if (rfh.version >= 3)
#ifndef ALLEGRO_BIG_ENDIAN
    fread(&rstruc->msgi[0], sizeof(MessageInfo), rstruc->nummes, opty);
#else
  {
    for (int iteratorCount = 0; iteratorCount < rstruc->nummes; ++iteratorCount)
    {
      rstruc->msgi[iteratorCount].ReadFromFile(opty);
    }
  }
#endif
  else
    memset(&rstruc->msgi[0], 0, sizeof(MessageInfo) * MAXMESS);

  for (f = 0;f < rstruc->nummes; f++) {
    if (rfh.version >= 22)
      read_string_decrypt(opty, buffre);
    else
      fgetstring_limit(buffre, opty, 2999);

    rstruc->message[f] = (char *)malloc(strlen(buffre) + 2);
    strcpy(rstruc->message[f], buffre);

    if (buffre[strlen(buffre)-1] == (char)200) {
      rstruc->message[f][strlen(buffre)-1] = 0;
      rstruc->msgi[f].flags |= MSG_DISPLAYNEXT;
    }
  }

  rstruc->numanims = 0;
  if (rfh.version >= 6) {
    fread(&rstruc->numanims, 2, 1, opty);

    if (rstruc->numanims > 0)
      fseek (opty, sizeof(FullAnimation) * rstruc->numanims, SEEK_CUR);
//      fread(&rstruc->anims[0], sizeof(FullAnimation), rstruc->numanims, opty);
  }
  else {
    rstruc->numanims = 0;
    memset(&rstruc->anims[0], 0, sizeof(FullAnimation) * MAXANIMS);
  }

  if ((rfh.version >= 4) && (rfh.version < 16)) {
    load_script_configuration(opty);
    load_graphical_scripts(opty, rstruc);
  }

  if (rfh.version >= 8)
    fread(&rstruc->shadinginfo[0], sizeof(short), 16, opty);

  if (rfh.version >= 21) {
    fread (&rstruc->regionLightLevel[0], sizeof(short), rstruc->numRegions, opty);
    fread (&rstruc->regionTintLevel[0], sizeof(int), rstruc->numRegions, opty);
  }

  update_polled_stuff();

  if (rfh.version >= 5) {
    tesl = load_lzw(opty, rstruc->ebscene[0], rstruc->pal);
    rstruc->ebscene[0] = recalced;
  }
  else
    tesl = loadcompressed_allegro(opty, &rstruc->ebscene[0], rstruc->pal, ftell(opty));

  if ((rstruc->ebscene[0]->w > 320) & (rfh.version < 11))
    rstruc->resolution = 2;

  update_polled_stuff();
  if (rfh.version >= 21)
    tesl = loadcompressed_allegro(opty, &rstruc->regions, rstruc->pal, tesl);
  else if (rfh.version >= 8) {
    tesl = loadcompressed_allegro(opty, &rstruc->regions, rstruc->pal, tesl);
    // an old version - clear the 'shadow' area into a blank regions bmp
    wfreeblock (rstruc->regions);
    rstruc->regions = NULL;
  }

  update_polled_stuff();
  tesl = loadcompressed_allegro(opty, &rstruc->walls, rstruc->pal, tesl);

  update_polled_stuff();
  tesl = loadcompressed_allegro(opty, &rstruc->object, rstruc->pal, tesl);

  update_polled_stuff();
  tesl = loadcompressed_allegro(opty, &rstruc->lookat, rstruc->pal, tesl);

  if (rfh.version < 21) {
    // Old version - copy walkable areas to Regions
    if (rstruc->regions == NULL)
      rstruc->regions = create_bitmap_ex (8, rstruc->walls->w, rstruc->walls->h);
    clear (rstruc->regions);

    blit (rstruc->walls, rstruc->regions, 0, 0, 0, 0, rstruc->regions->w, rstruc->regions->h);
    for (f = 0; f <= 15; f++) {
      rstruc->regionLightLevel[f] = rstruc->walk_area_light[f];
      rstruc->regionTintLevel[f] = 0;
    }
  }

  if (rfh.version < 9) {
    for (f = 0; f < 11; f++)
      rstruc->password[f] += 60;
  }
  else {
    for (f = 0; f < 11; f++)
      rstruc->password[f] += passwencstring[f];
  }
}

void load_room(char *files, roomstruct *rstruc, bool gameIsHighRes) {
  FILE              *opty;
  room_file_header  rfh;
  int i;

  rstruc->freemessage();
  if (rstruc->scripts != NULL) {
    free(rstruc->scripts);
    rstruc->scripts = NULL;
  }

  if (rstruc->compiled_script != NULL)
    ccFreeScript(rstruc->compiled_script);

  rstruc->compiled_script = NULL;
  if (rstruc->num_bscenes > 1) {
    int ff;

    for (ff = 1; ff < rstruc->num_bscenes; ff++) {
      wfreeblock(rstruc->ebscene[ff]);
      rstruc->ebscene[ff] = NULL;
    }
    update_polled_stuff();
  }

  rstruc->num_bscenes = 1;
  rstruc->bscene_anim_speed = 5;
  memset (&rstruc->objectnames[0][0], 0, MAX_INIT_SPR * MAXOBJNAMELEN);
  memset (&rstruc->objectscriptnames[0][0], 0, MAX_INIT_SPR * MAX_SCRIPT_NAME_LEN);
  memset (&rstruc->regionLightLevel[0], 0, sizeof(short) * MAX_REGIONS);
  memset (&rstruc->regionTintLevel[0], 0, sizeof(int) * MAX_REGIONS);

  for (i = 0; i <= MAX_WALK_AREAS; i++) {
    rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
    rstruc->walk_area_top[i] = -1;
    rstruc->walk_area_bottom[i] = -1;
  }

  for (i = 0; i < rstruc->numhotspots; i++)
    rstruc->hsProps[i].reset();
  for (i = 0; i < rstruc->numsprs; i++)
    rstruc->objProps[i].reset();
  rstruc->roomProps.reset();

  if (rstruc->roomScripts != NULL) 
  {
	  delete rstruc->roomScripts;
	  rstruc->roomScripts = NULL;
  }
  if (rstruc->hotspotScripts != NULL)
  {
	  for (i = 0; i < rstruc->lastLoadNumHotspots; i++)
	  {
		  delete rstruc->hotspotScripts[i];
	  }
	  delete rstruc->hotspotScripts;
	  rstruc->hotspotScripts = NULL;
  }
  if (rstruc->objectScripts != NULL)
  {
	  for (i = 0; i < rstruc->lastLoadNumObjects; i++)
	  {
		  delete rstruc->objectScripts[i];
	  }
	  delete rstruc->objectScripts;
	  rstruc->objectScripts = NULL;
  }
  if (rstruc->regionScripts != NULL)
  {
	  for (i = 0; i < rstruc->lastLoadNumRegions; i++)
	  {
		  delete rstruc->regionScripts[i];
	  }
	  delete rstruc->regionScripts;
	  rstruc->regionScripts = NULL;
  }

  if (rstruc->localvars != NULL)
    free (rstruc->localvars);
  rstruc->localvars = NULL;
  rstruc->numLocalVars = 0;

  memset(&rstruc->ebpalShared[0], 0, MAX_BSCENE);

  update_polled_stuff();

  opty = clibfopen(files, "rb");
  if (opty == NULL) {
    char errbuffr[500];
    sprintf(errbuffr,"Load_room: Unable to load the room file '%s'\n"
      "Make sure that you saved the room to the correct folder (it should be\n"
      "in your game's sub-folder of the AGS directory).\n"
      "Also check that the player character's starting room is set correctly.\n",files);
    quit(errbuffr);
  }
  update_polled_stuff();  // it can take a while to load the file sometimes

#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rfh, sizeof(rfh), 1, opty);
#else
  rfh.ReadFromFile(opty);
#endif
  //fclose(opty);
  rstruc->wasversion = rfh.version;
#ifdef THIS_IS_THE_ENGINE
  if ((rstruc->wasversion < 17) | (rstruc->wasversion > ROOM_FILE_VERSION))
#else
  if ((rstruc->wasversion < 15) || (rstruc->wasversion > ROOM_FILE_VERSION))
#endif
  {
    fclose(opty);
    quit("Load_Room: Bad packed file. Either the file requires a newer or older version of\n"
      "this program or the file is corrupt.\n");
  }

  int   thisblock = 0;
  long  bloklen;
  FILE *optywas;

  while (thisblock != BLOCKTYPE_EOF) {
    update_polled_stuff();
    thisblock = fgetc(opty);

    if (thisblock == BLOCKTYPE_EOF)
      break;

    fread(&bloklen, 4, 1, opty);
    bloklen += ftell(opty);  // make it the new position for after block read
    optywas = opty;

    if (thisblock == BLOCKTYPE_MAIN)
      load_main_block(rstruc, files, opty, rfh);
    else if (thisblock == BLOCKTYPE_SCRIPT) {
      long  lee;
      int   hh;

      fread(&lee, 4, 1, opty);
      rstruc->scripts = (char *)malloc(lee + 5);
      // MACPORT FIX: swap
      fread(rstruc->scripts, sizeof(char), lee, opty);
      rstruc->scripts[lee] = 0;

      for (hh = 0; hh < lee; hh++)
        rstruc->scripts[hh] += passwencstring[hh % 11];
    }
    else if (thisblock == BLOCKTYPE_COMPSCRIPT3) {
      rstruc->compiled_script = fread_script(opty);
      if (rstruc->compiled_script == NULL)
        quit("Load_room: Script load failed; need newer version?");
    }
    else if ((thisblock == BLOCKTYPE_COMPSCRIPT) || (thisblock == BLOCKTYPE_COMPSCRIPT2))
#ifdef LOADROOM_ALLOWOLD
      rstruc->compiled_script = NULL;
#else
      quit("Load_room: old room format. Please upgrade the room.");
#endif
    else if (thisblock == BLOCKTYPE_OBJECTNAMES) {
      if (fgetc(opty) != rstruc->numsprs)
        quit("Load_room: inconsistent blocks for object names");

      fread(&rstruc->objectnames[0][0], MAXOBJNAMELEN, rstruc->numsprs, opty);
    }
    else if (thisblock == BLOCKTYPE_OBJECTSCRIPTNAMES) {
      if (fgetc(opty) != rstruc->numsprs)
        quit("Load_room: inconsistent blocks for object script names");

      fread(&rstruc->objectscriptnames[0][0], MAX_SCRIPT_NAME_LEN, rstruc->numsprs, opty);
    }
    else if (thisblock == BLOCKTYPE_ANIMBKGRND) {
      int   ct;
      long  fpos;

      rstruc->num_bscenes = fgetc(opty);
      rstruc->bscene_anim_speed = fgetc(opty);

      if (rfh.version >= 20)
        fread(&rstruc->ebpalShared[0], 1, rstruc->num_bscenes, opty);
      else
        memset (&rstruc->ebpalShared[0], 0, rstruc->num_bscenes);

      fpos = ftell(opty);
//        fclose(opty);

      for (ct = 1; ct < rstruc->num_bscenes; ct++) {
        update_polled_stuff();
//          fpos = load_lzw(files,rstruc->ebscene[ct],rstruc->pal,fpos);
        fpos = load_lzw(opty, rstruc->ebscene[ct], rstruc->bpalettes[ct]);
        rstruc->ebscene[ct] = recalced;
      }
//        opty = clibfopen(files, "rb");
//        fseek(opty, fpos, SEEK_SET);
    }
    else if (thisblock == BLOCKTYPE_PROPERTIES) {
      // Read custom properties
      if (getw(opty) != 1)
        quit("LoadRoom: unknown Custom Properties block encounreted");

      int errors = 0, gg;

      if (rstruc->roomProps.UnSerialize (opty))
        quit("LoadRoom: error reading custom properties block");

      for (gg = 0; gg < rstruc->numhotspots; gg++)
        errors += rstruc->hsProps[gg].UnSerialize (opty);
      for (gg = 0; gg < rstruc->numsprs; gg++)
        errors += rstruc->objProps[gg].UnSerialize (opty);

      if (errors > 0)
        quit("LoadRoom: errors encountered reading custom props");
    }
    else if (thisblock == -1)
    {
      fclose(opty);
      quit("LoadRoom: unexpected end of file while loading room");
      return;
    }
    else {
      char  tempbfr[90];
      sprintf(tempbfr, "LoadRoom: unknown block type %d encountered in '%s'", thisblock, files);
      quit(tempbfr);
    }

    // The ftell call below has caused crashes
    if (ftell(opty) != bloklen)
      fseek(opty, bloklen, SEEK_SET);
  }

  // sync bpalettes[0] with room.pal
  memcpy (&rstruc->bpalettes[0][0], &rstruc->pal[0], sizeof(color) * 256);

  fclose(opty);

  if ((rfh.version < 29) && (gameIsHighRes))
  {
	  // Pre-3.0.3, multiply up co-ordinates
      // If you change this, also change convert_room_coordinates_to_low_res
      // function in the engine
	  int f;
	  for (f = 0; f < rstruc->numsprs; f++)
	  {
		  rstruc->sprs[f].x *= 2;
		  rstruc->sprs[f].y *= 2;
		  if (rstruc->objbaseline[f] > 0)
		  {
			  rstruc->objbaseline[f] *= 2;
		  }
	  }

	  for (f = 0; f < rstruc->numhotspots; f++)
	  {
		  rstruc->hswalkto[f].x *= 2;
		  rstruc->hswalkto[f].y *= 2;
	  }

	  for (f = 0; f < rstruc->numobj; f++)
	  {
		  rstruc->objyval[f] *= 2;
	  }

	  rstruc->left *= 2;
	  rstruc->top *= 2;
	  rstruc->bottom *= 2;
	  rstruc->right *= 2;
	  rstruc->width *= 2;
	  rstruc->height *= 2;
  }

}
#endif  // NOFUNCTIONS

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

#ifndef CROOM_NOFUNCTIONS

void ViewStruct::Initialize(int loopCount)
{
  numLoops = loopCount;
  if (numLoops > 0)
  {
    loops = (ViewLoopNew*)calloc(numLoops, sizeof(ViewLoopNew));
  }
}

void ViewStruct::Dispose()
{
  if (numLoops > 0)
  {
    free(loops);
    numLoops = 0;
  }
}

void ViewStruct::WriteToFile(FILE *ooo)
{
  putshort(numLoops, ooo);
  for (int i = 0; i < numLoops; i++)
  {
    loops[i].WriteToFile(ooo);
  }
}

void ViewStruct::ReadFromFile(FILE *iii)
{
#ifdef ALLEGRO_BIG_ENDIAN
  Initialize(__getshort__bigendian(fp));
#else
  Initialize(getshort(iii));
#endif

  for (int i = 0; i < numLoops; i++)
  {
    loops[i].ReadFromFile(iii);
  }
}

void ViewLoopNew::Initialize(int frameCount)
{
  numFrames = frameCount;
  flags = 0;
  frames = (ViewFrame*)calloc(numFrames + 1, sizeof(ViewFrame));
}

void ViewLoopNew::Dispose()
{
  if (frames != NULL)
  {
    free(frames);
    frames = NULL;
    numFrames = 0;
  }
}

void ViewLoopNew::WriteToFile(FILE *ooo)
{
  fwrite(&numFrames, sizeof(short), 1, ooo);
  fwrite(&flags, sizeof(int), 1, ooo);
  fwrite(frames, sizeof(ViewFrame), numFrames, ooo);
}


void ViewLoopNew::ReadFromFile(FILE *iii)
{
#ifdef ALLEGRO_BIG_ENDIAN

  STEVE PLEASE VALIDATE THAT THIS CODE IS OK

  Initialize(__getshort__bigendian(fp));
  flags = getw(iii);

  for (int i = 0; i < numFrames; ++i)
  {
    frames[i].ReadFromFile(iii);
  }

#else

  Initialize(getshort(iii));
  flags = getw(iii);

  fread(frames, sizeof(ViewFrame), numFrames, iii);

#endif

  // an extra frame is allocated in memory to prevent
  // crashes with empty loops -- set its picture to teh BLUE CUP!!
  frames[numFrames].pic = 0;
}

#endif

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


#ifndef CROOM_NOFUNCTIONS

int CharacterInfo::get_effective_y() {
  return y - z;
}
int CharacterInfo::get_baseline() {
  if (baseline < 1)
    return y;
  return baseline;
}
int CharacterInfo::get_blocking_top() {
  if (blocking_height > 0)
    return y - blocking_height / 2;
  return y - 2;
}
int CharacterInfo::get_blocking_bottom() {
  // the blocking_bottom should be 1 less than the top + height
  // since the code does <= checks on it rather than < checks
  if (blocking_height > 0)
    return (y + (blocking_height + 1) / 2) - 1;
  return y + 3;
}

#define COPY_CHAR_VAR(name) ci->name = oci->name
void ConvertOldCharacterToNew (OldCharacterInfo *oci, CharacterInfo *ci) {
  COPY_CHAR_VAR (defview);
  COPY_CHAR_VAR (talkview);
  COPY_CHAR_VAR (view);
  COPY_CHAR_VAR (room);
  COPY_CHAR_VAR (prevroom);
  COPY_CHAR_VAR (x);
  COPY_CHAR_VAR (y);
  COPY_CHAR_VAR (wait);
  COPY_CHAR_VAR (flags);
  COPY_CHAR_VAR (following);
  COPY_CHAR_VAR (followinfo);
  COPY_CHAR_VAR (idleview);
  COPY_CHAR_VAR (idletime);
  COPY_CHAR_VAR (idleleft);
  COPY_CHAR_VAR (transparency);
  COPY_CHAR_VAR (baseline);
  COPY_CHAR_VAR (activeinv);
  COPY_CHAR_VAR (loop);
  COPY_CHAR_VAR (frame);
  COPY_CHAR_VAR (walking);
  COPY_CHAR_VAR (animating);
  COPY_CHAR_VAR (walkspeed);
  COPY_CHAR_VAR (animspeed);
  COPY_CHAR_VAR (actx);
  COPY_CHAR_VAR (acty);
  COPY_CHAR_VAR (on);
  strcpy (ci->name, oci->name);
  strcpy (ci->scrname, oci->scrname);
  memcpy (&ci->inv[0], &oci->inv[0], sizeof(short) * 100);
  // move the talking colour into the struct and remove from flags
  ci->talkcolor = (oci->flags & OCHF_SPEECHCOL) >> OCHF_SPEECHCOLSHIFT;
  ci->flags = ci->flags & (~OCHF_SPEECHCOL);
}

void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss) {
  int i;
  strcpy (gss->gamename, ogss->gamename);
  for (i = 0; i < 20; i++)
    gss->options[i] = ogss->options[i];
  memcpy (&gss->paluses[0], &ogss->paluses[0], 256);
  memcpy (&gss->defpal[0],  &ogss->defpal[0],  256 * sizeof(color));
  gss->numviews = ogss->numviews;
  gss->numcharacters = ogss->numcharacters;
  gss->playercharacter = ogss->playercharacter;
  gss->totalscore = ogss->totalscore;
  gss->numinvitems = ogss->numinvitems;
  gss->numdialog = ogss->numdialog;
  gss->numdlgmessage = ogss->numdlgmessage;
  gss->numfonts = ogss->numfonts;
  gss->color_depth = ogss->color_depth;
  gss->target_win = ogss->target_win;
  gss->dialog_bullet = ogss->dialog_bullet;
  gss->hotdot = ogss->hotdot;
  gss->hotdotouter = ogss->hotdotouter;
  gss->uniqueid = ogss->uniqueid;
  gss->numgui = ogss->numgui;
  memcpy (&gss->fontflags[0], &ogss->fontflags[0], 10);
  memcpy (&gss->fontoutline[0], &ogss->fontoutline[0], 10);
  memcpy (&gss->spriteflags[0], &ogss->spriteflags[0], 6000);
  memcpy (&gss->invinfo[0], &ogss->invinfo[0], 100 * sizeof(InventoryItemInfo));
  memcpy (&gss->mcurs[0], &ogss->mcurs[0], 10 * sizeof(MouseCursor));
  for (i = 0; i < MAXGLOBALMES; i++)
    gss->messages[i] = ogss->messages[i];
  gss->dict = ogss->dict;
  gss->globalscript = ogss->globalscript;
  gss->chars = NULL; //ogss->chars;
  gss->compiled_script = ogss->compiled_script;
  gss->numcursors = 10;
}

void Convert272ViewsToNew (int numof, ViewStruct272 *oldv, ViewStruct *newv) {
  
  for (int a = 0; a < numof; a++) {
    newv[a].Initialize(oldv[a].numloops);
    
    for (int b = 0; b < oldv[a].numloops; b++) 
    {
      newv[a].loops[b].Initialize(oldv[a].numframes[b]);

      if ((oldv[a].numframes[b] > 0) &&
          (oldv[a].frames[b][oldv[a].numframes[b] - 1].pic == -1))
      {
        newv[a].loops[b].flags = LOOPFLAG_RUNNEXTLOOP;
        newv[a].loops[b].numFrames--;
      }
      else
        newv[a].loops[b].flags = 0;

      for (int c = 0; c < newv[a].loops[b].numFrames; c++)
        newv[a].loops[b].frames[c] = oldv[a].frames[b][c];
    }
  }
}

#endif


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
