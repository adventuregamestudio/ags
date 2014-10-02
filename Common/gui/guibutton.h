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

#ifndef __AC_GUIBUTTON_H
#define __AC_GUIBUTTON_H

#include <vector>
#include "gui/guiobject.h"

#define GBUT_ALIGN_TOPMIDDLE    0
#define GBUT_ALIGN_TOPLEFT      1
#define GBUT_ALIGN_TOPRIGHT     2
#define GBUT_ALIGN_MIDDLELEFT   3 
#define GBUT_ALIGN_CENTRED      4
#define GBUT_ALIGN_MIDDLERIGHT  5
#define GBUT_ALIGN_BOTTOMLEFT   6
#define GBUT_ALIGN_BOTTOMMIDDLE 7
#define GBUT_ALIGN_BOTTOMRIGHT  8

struct GUIButton:public GUIObject
{
  char text[50];
  int pic, overpic, pushedpic;
  int usepic, ispushed, isover;
  int font, textcol;
  int leftclick, rightclick;
  int lclickdata, rclickdata;
  int textAlignment, reserved1;

  virtual void WriteToFile(Common::Stream *out);
  virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
  virtual void Draw(Common::Bitmap *ds);
  void MouseUp();

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
    if (ispushed)
      usepic = pushedpic;
    else
      usepic = overpic;

    isover = 1;
  }

  void MouseLeave()
  {
    usepic = pic;
    isover = 0;
  }

  virtual int MouseDown()
  {
    if (pushedpic > 0)
      usepic = pushedpic;

    ispushed = 1;
    return 0;
  }

  void KeyPress(int keycode)
  {
  }

  void reset()
  {
    GUIObject::init();
    usepic = -1;
    pic = -1;
    overpic = -1;
    pushedpic = -1;
    ispushed = 0;
    isover = 0;
    text[0] = 0;
    font = 0;
    textcol = 0;
    leftclick = 2;
    rightclick = 0;
    activated = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Click";
    supportedEventArgs[0] = "GUIControl *control, MouseButton button";
  }

  GUIButton() {
    reset();
  }

private:
  void Draw_set_oritext(char *oritext, const char *text);
};

extern std::vector<GUIButton> guibuts;
extern int numguibuts;

int UpdateAnimatingButton(int bu);
void StopButtonAnimation(int idxn);

#endif // __AC_GUIBUTTON_H
