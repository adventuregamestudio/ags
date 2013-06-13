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

#ifndef __AC_GUITEXTBOX_H
#define __AC_GUITEXTBOX_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"

#define GTF_NOBORDER  1
struct GUITextBox:public GUIObject
{
  char text[200];
  int font, textcol, exflags;

  virtual void WriteToFile(Common::Stream *out);
  virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
  virtual void Draw(Common::Bitmap *ds);
  void KeyPress(int);

  void MouseMove(int x, int y)
  {
  }

  void MouseOver()
  {
  }

  void MouseLeave()
  {
  }

  void MouseUp()
  {
  }

  void reset()
  {
    GUIObject::init();
    font = 0;
    textcol = 0;
    text[0] = 0;
    numSupportedEvents = 1;
    supportedEvents[0] = "Activate";
    supportedEventArgs[0] = "GUIControl *control";
  }

  GUITextBox() {
    reset();
  }

private:
  void Draw_text_box_contents(Common::Bitmap *ds, color_t text_color);
};

extern DynamicArray<GUITextBox> guitext;
extern int numguitext;

#endif // __AC_GUITEXTBOX_H
