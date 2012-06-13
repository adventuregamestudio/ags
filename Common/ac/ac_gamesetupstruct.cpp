
#include <stdio.h>
//#include "wgt2allg_nofunc.h"        // macros, typedef
#include "wgt2allg.h"
#include "ac/ac_gamesetupstruct.h"

void GameSetupStructBase::ReadFromFile(FILE *fp)
{
#ifdef ALLEGRO_BIG_ENDIAN
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
#else
    throw "GameSetupStructBase::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
#endif
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
