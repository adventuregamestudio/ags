
#ifndef __CROOM_ROOM_H
#define __CROOM_ROOM_H

#include "cs/cc_script.h"   // ccScript


// thisroom.options[0] = startup music
// thisroom.options[1] = can save/load on screen (0=yes, 1=no)
// thisroom.options[2] = player character disabled? (0=no, 1=yes)
// thisroom.options[3] = player special view (0=normal)
//                 [4] = music volume (0=normal, <0 quiter, >0 louder)

const int ST_TUNE = 0, ST_SAVELOAD = 1, ST_MANDISABLED = 2, ST_MANVIEW = 3, ST_VOLUME = 4;

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

#endif // __CROOM_ROOM_H