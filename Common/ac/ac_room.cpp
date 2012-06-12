
#include <stdio.h>
#include "wgt2allg_nofunc.h"
#include "ac/ac_room.h"


#ifdef ALLEGRO_BIG_ENDIAN
void sprstruc::ReadFromFile(FILE *fp)
{
    sprnum = __getshort__bigendian(fp);
    x = __getshort__bigendian(fp);
    y = __getshort__bigendian(fp);
    room = __getshort__bigendian(fp);
    on = __getshort__bigendian(fp);
}
#endif

roomstruct::roomstruct() {
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


