#ifndef __AC_GUILABEL_H
#define __AC_GUILABEL_H

#include "acgui/ac_guiobject.h"
#include "acgui/ac_dynamicarray.h"

struct GUILabel:public GUIObject
{
private:
  char *text;
  int textBufferLen;
public:
  int font, textcol, align;

  virtual void WriteToFile(FILE *);
  virtual void ReadFromFile(FILE *, int);
  void Draw();
  void printtext_align(int yy, char *teptr);
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

private:
  void Draw_replace_macro_tokens(char *oritext, char *text);
  void Draw_split_lines(char *teptr, int wid, int font, int &numlines);
};

extern DynamicArray<GUILabel> guilabels;
extern int numguilabels;

#endif // __AC_GUILABEL_H