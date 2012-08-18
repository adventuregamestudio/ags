
#include <stdio.h>
#include "gui/guiobject.h"
#include "gui/guimain.h"
#include "util/string_utils.h"  // fputstring, etc
#include "ac/common.h"		// quit()

void GUIObject::init() {
  int jj;
  scriptName[0] = 0;
  for (jj = 0; jj < MAX_GUIOBJ_EVENTS; jj++)
    eventHandlers[jj][0] = 0;
}

int GUIObject::IsDisabled() {
  if (flags & GUIF_DISABLED)
    return 1;
  if (all_buttons_disabled)
    return 1;
  return 0;
}

void GUIObject::WriteToFile(FILE * ooo)
{
  // MACPORT FIX: swap
  fwrite(&flags, sizeof(int), BASEGOBJ_SIZE, ooo);
  fputstring(scriptName, ooo);

  putw(GetNumEvents(), ooo);
  for (int kk = 0; kk < GetNumEvents(); kk++)
    fputstring(eventHandlers[kk], ooo);
}

void GUIObject::ReadFromFile(FILE * ooo, int version)
{
  // MACPORT FIX: swap
  fread(&flags, sizeof(int), BASEGOBJ_SIZE, ooo);
  if (version >= 106)
    fgetstring_limit(scriptName, ooo, MAX_GUIOBJ_SCRIPTNAME_LEN);
  else
    scriptName[0] = 0;

  int kk;
  for (kk = 0; kk < GetNumEvents(); kk++)
    eventHandlers[kk][0] = 0;

  if (version >= 108) {
    int numev = getw(ooo);
    if (numev > GetNumEvents())
      quit("Error: too many control events, need newer version");

    // read in the event handler names
    for (kk = 0; kk < numev; kk++)
      fgetstring_limit(eventHandlers[kk], ooo, MAX_GUIOBJ_EVENTHANDLER_LEN + 1);
  }
}
