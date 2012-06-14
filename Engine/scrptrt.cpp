/* 
  Script Editor run-time engine component (c) 1998 Chris Jones
  script chunk format:
  00h  1 dword  version - should be 2
  04h  1 dword  sizeof(scriptblock)
  08h  1 dword  number of ScriptBlocks
  0Ch  n STRUCTs ScriptBlocks

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.
*/
#include <stdio.h>
#include <stdlib.h>
#include "wgt2allg.h"
#include "ac/ac_room.h"
#include "bigend.h"

char *scripteditruntimecopr = "Script Editor v1.2 run-time component. (c) 1998 Chris Jones";

#define SCRIPT_CONFIG_VERSION 1
extern void quit(char *);

long getlong(FILE * iii)
{
  long tmm;
  fread(&tmm, 4, 1, iii);
  return tmm;
}

void save_script_configuration(FILE * iii)
{
  quit("ScriptEdit: run-time version can't save");
}

void load_script_configuration(FILE * iii)
{
  int aa;
  if (getlong(iii) != SCRIPT_CONFIG_VERSION)
    quit("ScriptEdit: invliad config version");

  int numvarnames = getlong(iii);
  for (aa = 0; aa < numvarnames; aa++) {
    int lenoft = getc(iii);
    fseek(iii, lenoft, SEEK_CUR);
  }
}

void save_graphical_scripts(FILE * fff, roomstruct * rss)
{
  quit("ScriptEdit: run-time version can't save");
}

char *scripttempn = "~acsc%d.tmp";
extern int route_script_link();

void load_graphical_scripts(FILE * iii, roomstruct * rst)
{
  long ct;
  FILE *te;

  if (route_script_link()) {
    quit("STOP IT.");
    exit(767);
    abort();
  }

  while (1) {
    fread(&ct, 4, 1, iii);
    if ((ct == -1) | (feof(iii) != 0))
      break;

    long lee;
    fread(&lee, 4, 1, iii);

    char thisscn[20];
    sprintf(thisscn, scripttempn, ct);
    te = fopen(thisscn, "wb");

    char *scnf = (char *)malloc(lee);
    // MACPORT FIX: swap size and nmemb
    fread(scnf, sizeof(char), lee, iii);
    fwrite(scnf, sizeof(char), lee, te);
    fclose(te);

    free(scnf);
  }
}
