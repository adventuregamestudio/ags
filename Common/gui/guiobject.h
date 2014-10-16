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

#ifndef __AC_GUIOBJECT_H
#define __AC_GUIOBJECT_H

#include "core/types.h"
#include "gfx/bitmap.h"
#include "gui/guidefines.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define GUIDIS_GREYOUT   1
#define GUIDIS_BLACKOUT  2
#define GUIDIS_UNCHANGED 4
#define GUIDIS_GUIOFF  0x80

#define BASEGOBJ_SIZE 7
#define GALIGN_LEFT   0
#define GALIGN_RIGHT  1
#define GALIGN_CENTRE 2

struct GUIObject
{
  int guin, objn;    // gui and object number of this object
  unsigned int flags;
  int x, y;
  int wid, hit;
  int zorder;
  int activated;
  char scriptName[MAX_GUIOBJ_SCRIPTNAME_LEN + 1];
  char eventHandlers[MAX_GUIOBJ_EVENTS][MAX_GUIOBJ_EVENTHANDLER_LEN + 1];

  GUIObject();
  virtual ~GUIObject(){}
  virtual void MouseMove(int, int) = 0; // x,y relative to gui
  virtual void MouseOver() = 0; // mouse moves onto object
  virtual void MouseLeave() = 0;        // mouse moves off object
  virtual int  MouseDown() { // button down - return 1 to lock focus
    return 0;
  }
  virtual void MouseUp() = 0;   // button up
  virtual void KeyPress(int) = 0;
  virtual void Draw(Common::Bitmap *ds) = 0;
  // overridable routine to determine whether the mouse is over
  // the control
  virtual int  IsOverControl(int p_x, int p_y, int p_extra) {
    if ((p_x >= x) && (p_y >= y) && (p_x < x + wid + p_extra) && (p_y < y + hit + p_extra))
      return 1;
    return 0;
  }
  virtual void WriteToFile(Common::Stream *out);
  virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
  // called when the control is resized
  virtual void Resized() { }
  virtual int  GetNumEvents() {
    return numSupportedEvents;
  }
  virtual const char *GetEventName(int idx) {
    if ((idx < 0) || (idx >= numSupportedEvents))
      return NULL;
    return supportedEvents[idx];
  }
  virtual const char *GetEventArgs(int idx) {
    if ((idx < 0) || (idx >= numSupportedEvents))
      return NULL;
    return supportedEventArgs[idx];
  }
  void init();

  int IsDeleted() {
    return flags & GUIF_DELETED;
  }
  int IsDisabled();
  void Enable() {
    flags &= ~GUIF_DISABLED;
  }
  void Disable() {
    flags |= GUIF_DISABLED;
  }
  int IsVisible() {
    if (flags & GUIF_INVISIBLE)
      return 0;
    return 1;
  }
  void Show() {
    flags &= ~GUIF_INVISIBLE;
  }
  void Hide() {
    flags |= GUIF_INVISIBLE;
  }
  int IsClickable();
  void SetClickable(bool newValue) {
    flags &= ~GUIF_NOCLICKS;
    if (!newValue)
      flags |= GUIF_NOCLICKS;
  }

  inline bool IsTranslated() const
  {
     return (flags & GUIF_TRANSLATED) != 0;
  }

protected:
  const char *supportedEvents[MAX_GUIOBJ_EVENTS];
  const char *supportedEventArgs[MAX_GUIOBJ_EVENTS];
  int numSupportedEvents;
};

#endif // __AC_GUIOBJECT_H
