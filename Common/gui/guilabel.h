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

#ifndef __AC_GUILABEL_H
#define __AC_GUILABEL_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"

struct GUILabel:public GUIObject
{
private:
  char *text;
  int textBufferLen;
public:
  int font, textcol, align;

  virtual void WriteToFile(Common::Stream *out);
  virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
  virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGUIVersion gui_version);
  virtual void Draw(Common::Graphics *g);
  void printtext_align(Common::Graphics *g, int yy, char *teptr);
  void SetText(const char *newText);
  const char *GetText();

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

  void KeyPress(int kp)
  {
  }

  void reset()
  {
    GUIObject::init();
    align = GALIGN_LEFT;
    font = 0;
    textcol = 0;
    numSupportedEvents = 0;
    text = "";
    textBufferLen = 0;
  }

  GUILabel() {
    reset();
  }

  virtual ~GUILabel()
  {
    if (textBufferLen > 0)
      free(text);
  }

private:
  void Draw_replace_macro_tokens(char *oritext, char *text);
  void Draw_split_lines(char *teptr, int wid, int font, int &numlines);
};

extern DynamicArray<GUILabel> guilabels;
extern int numguilabels;

#endif // __AC_GUILABEL_H
