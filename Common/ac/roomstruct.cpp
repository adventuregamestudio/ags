
#include <stdio.h>
#include "util/wgt2allg.h"
#include "ac/roomstruct.h"
#include "ac/common.h"
#include "ac/wordsdictionary.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/compress.h"
#include "platform/file.h"

extern "C" {
  extern FILE *clibfopen(char *, char *);
}

extern BITMAP *recalced;

block backups[5];
int _acroom_bpp = 1;  // bytes per pixel of currently loading room

void sprstruc::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    sprnum = getshort(fp);//__getshort__bigendian(fp);
    x = getshort(fp);//__getshort__bigendian(fp);
    y = getshort(fp);//__getshort__bigendian(fp);
    room = getshort(fp);//__getshort__bigendian(fp);
    on = getshort(fp);//__getshort__bigendian(fp);
//#else
//    throw "sprstruc::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

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

void room_file_header::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    version = getshort(fp);//getshort(fp);__getshort__bigendian(fp);
//#else
//    throw "room_file_header::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}


int usesmisccond = 0;

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

//#ifdef ALLEGRO_BIG_ENDIAN
    // Points are a pair of shorts
    // [IKM] TODO: read/write memeber for _Point?
    fread(&rstruc->hswalkto[0], sizeof(short), 2*rstruc->numhotspots, opty);
//#else
//    fread(&rstruc->hswalkto[0], sizeof(_Point), rstruc->numhotspots, opty);
//#endif

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
//#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < rstruc->numwalkareas; ++iteratorCount)
  {
      rstruc->wallpoints[iteratorCount].ReadFromFile(opty);
  }
//#else
//  fread(&rstruc->wallpoints[0], sizeof(PolyPoints), rstruc->numwalkareas, opty);
//#endif
  
  update_polled_stuff_if_runtime();

  fread(&rstruc->top, 2, 1, opty);
  fread(&rstruc->bottom, 2, 1, opty);
  fread(&rstruc->left, 2, 1, opty);
  fread(&rstruc->right, 2, 1, opty);

  fread(&rstruc->numsprs, 2, 1, opty);
  // MACPORT FIX: read sprstrucs
//#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < rstruc->numsprs; ++iteratorCount)
  {
      rstruc->sprs[iteratorCount].ReadFromFile(opty);
  }
//#else
//  fread(&rstruc->sprs[0], sizeof(sprstruc), rstruc->numsprs, opty);
//#endif

  if (rfh.version >= 19) {
    rstruc->numLocalVars = getw(opty);
    if (rstruc->numLocalVars > 0) {
      rstruc->localvars = (InteractionVariable*)malloc (sizeof(InteractionVariable) * rstruc->numLocalVars);
//#ifdef ALLEGRO_BIG_ENDIAN
      for (int iteratorCount = 0; iteratorCount < rstruc->numLocalVars; ++iteratorCount)
      {
          rstruc->localvars[iteratorCount].ReadFromFile(opty);
      }
//#else
//      fread (&rstruc->localvars[0], sizeof(InteractionVariable), rstruc->numLocalVars, opty);
//#endif
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
//#ifdef ALLEGRO_BIG_ENDIAN
  {
      for (int iteratorCount = 0; iteratorCount < rstruc->nummes; ++iteratorCount)
      {
          rstruc->msgi[iteratorCount].ReadFromFile(opty);
      }
  }
//#else
    //fread(&rstruc->msgi[0], sizeof(MessageInfo), rstruc->nummes, opty);
//#endif
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

  update_polled_stuff_if_runtime();

  if (rfh.version >= 5) {
    tesl = load_lzw(opty, rstruc->ebscene[0], rstruc->pal);
    rstruc->ebscene[0] = recalced;
  }
  else
    tesl = loadcompressed_allegro(opty, &rstruc->ebscene[0], rstruc->pal, ftell(opty));

  if ((rstruc->ebscene[0]->w > 320) & (rfh.version < 11))
    rstruc->resolution = 2;

  update_polled_stuff_if_runtime();
  if (rfh.version >= 21)
    tesl = loadcompressed_allegro(opty, &rstruc->regions, rstruc->pal, tesl);
  else if (rfh.version >= 8) {
    tesl = loadcompressed_allegro(opty, &rstruc->regions, rstruc->pal, tesl);
    // an old version - clear the 'shadow' area into a blank regions bmp
    wfreeblock (rstruc->regions);
    rstruc->regions = NULL;
  }

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(opty, &rstruc->walls, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(opty, &rstruc->object, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
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

extern bool load_room_is_version_bad(roomstruct *rstruc);
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
    update_polled_stuff_if_runtime();
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

  update_polled_stuff_if_runtime();

  opty = clibfopen(files, "rb");
  if (opty == NULL) {
    char errbuffr[500];
    sprintf(errbuffr,"Load_room: Unable to load the room file '%s'\n"
      "Make sure that you saved the room to the correct folder (it should be\n"
      "in your game's sub-folder of the AGS directory).\n"
      "Also check that the player character's starting room is set correctly.\n",files);
    quit(errbuffr);
  }
  update_polled_stuff_if_runtime();  // it can take a while to load the file sometimes

//#ifdef ALLEGRO_BIG_ENDIAN
  rfh.ReadFromFile(opty);
//#else
//  fread(&rfh, sizeof(rfh), 1, opty);  
//#endif
  //fclose(opty);
  rstruc->wasversion = rfh.version;

  if (load_room_is_version_bad(rstruc))
  {
    fclose(opty);
    quit("Load_Room: Bad packed file. Either the file requires a newer or older version of\n"
      "this program or the file is corrupt.\n");
  }

  int   thisblock = 0;
  long  bloklen;
  FILE *optywas;

  while (thisblock != BLOCKTYPE_EOF) {
    update_polled_stuff_if_runtime();
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
        update_polled_stuff_if_runtime();
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
