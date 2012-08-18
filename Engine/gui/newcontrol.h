//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_GUI__NEWCONTROL_H
#define __AGS_EE_GUI__NEWCONTROL_H

struct NewControl
{
  int x, y, wid, hit, state, typeandflags, wlevel;
  char visible, enabled;        // not implemented
  char needredraw;
  virtual void draw() = 0;
  virtual int pressedon() = 0;
  virtual int processmessage(int, int, long) = 0;

  NewControl(int xx, int yy, int wi, int hi);
  NewControl();
  int mouseisinarea();
  void drawifneeded();
  void drawandmouse();
};

#endif // __AGS_EE_GUI__NEWCONTROL_H