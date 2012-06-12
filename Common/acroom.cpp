//
// Implementation from acroom.h that was previously put under CROOM_NOFUNCTIONS
// macro control
//


// MACPORT FIX: endian support
#include "bigend.h"
#include "misc.h"
#include "wgt2allg_nofunc.h"
#include "acroom_func.h"

#include "cs/cs_utils.h" // fputstring, etc

extern "C" {
	extern FILE *clibfopen(char *, char *);
}


char *croom_h_copyright = "ChrisRoom v2.00 - CRM reader/writer copyright (c) 1995, 1998, 1999 by Chris Jones.";
char *game_file_sig = "Adventure Creator Game File v2";

block backups[5];


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

int _acroom_bpp = 1;  // bytes per pixel of currently loading room

// returns bytes per pixel for bitmap's color depth
int bmp_bpp(BITMAP*bmpt) {
  if (bitmap_color_depth(bmpt) == 15)
    return 2;

  return bitmap_color_depth(bmpt) / 8;
}



#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)
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

  update_polled_stuff_if_runtime();
  membuffer = lzwexpand_to_mem(iii);
  update_polled_stuff_if_runtime();

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
#endif // ALLEGRO_BIG_ENDIAN

  if (bmm!=NULL)
    destroy_bitmap(bmm);

  update_polled_stuff_if_runtime();

  bmm = create_bitmap_ex(_acroom_bpp * 8, (loptr[0] / _acroom_bpp), loptr[1]);
  if (bmm == NULL)
    quit("!load_room: not enough memory to load room background");

  update_polled_stuff_if_runtime();

  acquire_bitmap (bmm);
  recalced = bmm;

  for (arin = 0; arin < loptr[1]; arin++)
    memcpy(&bmm->line[arin][0], &membuffer[arin * loptr[0]], loptr[0]);

  release_bitmap (bmm);

  update_polled_stuff_if_runtime();

  free(membuffer-8);

  if (ftell(iii) != uncompsiz)
    fseek(iii, uncompsiz, SEEK_SET);

  update_polled_stuff_if_runtime();

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
      update_polled_stuff_if_runtime();
  }

  fseek(fpp, 768, SEEK_CUR);  // skip palette

  return ftell(fpp);
}
#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER



char *passwencstring = "Avis Durgan";


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
  
  update_polled_stuff_if_runtime();

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

#ifndef ALLEGRO_BIG_ENDIAN
  fread(&rfh, sizeof(rfh), 1, opty);
#else
  rfh.ReadFromFile(opty);
#endif
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

