
#include <stdio.h>
#include "gui/guiobject.h"
#include "gui/guimain.h"
#include "util/string_utils.h"  // fputstring, etc
#include "ac/common.h"		// quit()
#include "util/datastream.h"

using AGS::Common::CDataStream;

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

void GUIObject::WriteToFile(CDataStream *out)
{
  // MACPORT FIX: swap
  out->WriteArrayOfInt32((int32_t*)&flags, BASEGOBJ_SIZE);
  fputstring(scriptName, out);

  out->WriteInt32(GetNumEvents());
  for (int kk = 0; kk < GetNumEvents(); kk++)
    fputstring(eventHandlers[kk], out);
}

void GUIObject::ReadFromFile(CDataStream *in, int version)
{
  // MACPORT FIX: swap
  in->ReadArrayOfInt32((int32_t*)&flags, BASEGOBJ_SIZE);
  if (version >= 106)
    fgetstring_limit(scriptName, in, MAX_GUIOBJ_SCRIPTNAME_LEN);
  else
    scriptName[0] = 0;

  int kk;
  for (kk = 0; kk < GetNumEvents(); kk++)
    eventHandlers[kk][0] = 0;

  if (version >= 108) {
    int numev = in->ReadInt32();
    if (numev > GetNumEvents())
      quit("Error: too many control events, need newer version");

    // read in the event handler names
    for (kk = 0; kk < numev; kk++)
      fgetstring_limit(eventHandlers[kk], in, MAX_GUIOBJ_EVENTHANDLER_LEN + 1);
  }
}
