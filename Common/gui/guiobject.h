/*
** Adventure Game Studio GUI routines
** Copyright (C) 2000-2005, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __AC_GUIOBJECT_H
#define __AC_GUIOBJECT_H

#include "gui/guidefines.h"
#include "util/file.h"

#define GUIDIS_GREYOUT   1
#define GUIDIS_BLACKOUT  2
#define GUIDIS_UNCHANGED 4
#define GUIDIS_GUIOFF  0x80

#define GUIF_DEFAULT  1
#define GUIF_CANCEL   2
#define GUIF_DISABLED 4
#define GUIF_TABSTOP  8
#define GUIF_INVISIBLE 0x10
#define GUIF_CLIP     0x20
#define GUIF_NOCLICKS 0x40
#define GUIF_DELETED  0x8000
#define BASEGOBJ_SIZE 7
#define GALIGN_LEFT   0
#define GALIGN_RIGHT  1
#define GALIGN_CENTRE 2
#define MAX_GUIOBJ_SCRIPTNAME_LEN 25
#define MAX_GUIOBJ_EVENTS 10
#define MAX_GUIOBJ_EVENTHANDLER_LEN 30
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

  virtual void MouseMove(int, int) = 0; // x,y relative to gui
  virtual void MouseOver() = 0; // mouse moves onto object
  virtual void MouseLeave() = 0;        // mouse moves off object
  virtual int  MouseDown() { // button down - return 1 to lock focus
    return 0;
  }
  virtual void MouseUp() = 0;   // button up
  virtual void KeyPress(int) = 0;
  virtual void Draw() = 0;
  // overridable routine to determine whether the mouse is over
  // the control
  virtual int  IsOverControl(int p_x, int p_y, int p_extra) {
    if ((p_x >= x) && (p_y >= y) && (p_x < x + wid + p_extra) && (p_y < y + hit + p_extra))
      return 1;
    return 0;
  }
  // we can't just fread/fwrite inherited objects because of vtbl, so use:
  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
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

protected:
  const char *supportedEvents[MAX_GUIOBJ_EVENTS];
  const char *supportedEventArgs[MAX_GUIOBJ_EVENTS];
  int numSupportedEvents;
};

#endif // __AC_GUIOBJECT_H