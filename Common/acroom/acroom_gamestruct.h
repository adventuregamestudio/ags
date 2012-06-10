
#ifndef __CROOM_GAMESETUP_H
#define __CROOM_GAMESETUP_H



#define MAXCOMMANDS 8
struct EventBlock {
    int   list[MAXCOMMANDS];
    int   respond[MAXCOMMANDS];
    int   respondval[MAXCOMMANDS];
    int   data[MAXCOMMANDS];
    int   numcmd;
    short score[MAXCOMMANDS];
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


#endif // __CROOM_GAMESETUP_H