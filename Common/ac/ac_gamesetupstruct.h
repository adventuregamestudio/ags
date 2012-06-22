
#ifndef __AC_GAMESETUPSTRUCT_H
#define __AC_GAMESETUPSTRUCT_H

#include "platform/file.h"
#include "ac/dynobj/scriptaudioclip.h"   // ScriptAudioClip
#include "ac/ac_audioclip.h"        // AudioClipType
#include "ac/ac_characterinfo.h"    // OldCharacterInfo, CharacterInfo
#include "ac/ac_customproperties.h" // CustomProperties, CustomPropertySchema
#include "ac/ac_dialog.h"           // constants
#ifdef UNUSED_CODE
#include "ac/ac_eventblock.h"       // EventBlock
#endif
#include "ac/ac_interaction.h"      // NewInteraction
#include "ac/ac_interfaceelement.h" // InterfaceElement
#include "ac/ac_inventoryiteminfo.h"// InventoryItemInfo
#include "ac/ac_mousecursor.h"      // MouseCursor
#include "ac/ac_wordsdictionary.h"  // WordsDictionary
#include "ac/dynobj/scriptaudioclip.h" // ScriptAudioClip
#include "cs/cc_script.h"           // ccScript

#define POPUP_NONE      0
#define POPUP_MOUSEY    1
#define POPUP_SCRIPT    2
#define POPUP_NOAUTOREM 3  // don't remove automatically during cutscene
#define POPUP_NONEINITIALLYOFF 4   // normal GUI, initially off
#define VTA_LEFT        0
#define VTA_RIGHT       1
#define VTA_CENTRE      2
#define IFLG_TEXTWINDOW 1

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
#ifdef UNUSED_CODE
    EventBlock        __charcond[50];   // [IKM] 2012-06-22: does not seem to be used anywhere
    EventBlock        __invcond[100];   // same
#endif
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

    void ReadFromFile(FILE *fp);
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

void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss);

#endif // __AC_GAMESETUPSTRUCT_H