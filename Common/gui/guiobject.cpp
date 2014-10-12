//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <stdio.h>
#include "gui/guiobject.h"
#include "gui/guimain.h"
#include "util/string_utils.h"  // fputstring, etc
#include "ac/common.h"		// quit()
#include "util/stream.h"

using AGS::Common::Stream;

GUIObject::GUIObject()
{
  guin = objn = 0;
  flags = 0;
  x = y = 0;
  wid = hit = 0;
  zorder = 0;
  activated = 0;
  init();
}

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

void GUIObject::WriteToFile(Stream *out)
{
  // MACPORT FIX: swap
  out->WriteArrayOfInt32((int32_t*)&flags, BASEGOBJ_SIZE);
  fputstring(scriptName, out);

  out->WriteInt32(GetNumEvents());
  for (int kk = 0; kk < GetNumEvents(); kk++)
    fputstring(eventHandlers[kk], out);
}

void GUIObject::ReadFromFile(Stream *in, GuiVersion gui_version)
{
  // MACPORT FIX: swap
  in->ReadArrayOfInt32((int32_t*)&flags, BASEGOBJ_SIZE);
  if (gui_version >= kGuiVersion_unkn_106)
    fgetstring_limit(scriptName, in, MAX_GUIOBJ_SCRIPTNAME_LEN);
  else
    scriptName[0] = 0;

  int kk;
  for (kk = 0; kk < GetNumEvents(); kk++)
    eventHandlers[kk][0] = 0;

  if (gui_version >= kGuiVersion_unkn_108) {
    int numev = in->ReadInt32();
    if (numev > GetNumEvents())
      quit("Error: too many control events, need newer version");

    // read in the event handler names
    for (kk = 0; kk < numev; kk++)
      fgetstring_limit(eventHandlers[kk], in, MAX_GUIOBJ_EVENTHANDLER_LEN + 1);
  }
}
