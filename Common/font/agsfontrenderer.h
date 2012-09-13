#ifndef __AC_AGSFONTRENDERER_H
#define __AC_AGSFONTRENDERER_H

//#include "ac/gamesetupstruct.h"	// constants
// temporary define copy
#ifndef MAX_FONTS
#define MAX_FONTS           15
#endif

typedef unsigned char* wgtfont;

// WARNING: this interface is exposed for plugins and declared for the second time in agsplugin.h
class IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize) = 0;
  virtual void FreeMemory(int fontNumber) = 0;
  virtual bool SupportsExtendedCharacters(int fontNumber) = 0;
  virtual int GetTextWidth(const char *text, int fontNumber) = 0;
  virtual int GetTextHeight(const char *text, int fontNumber) = 0;
  // [IKM] An important note: the AGS font renderers do not use 'destination' parameter at all, probably
  // for simplicity (although that cause confusion): the parameter passed is always a global 'abuf'
  // pointer therefore renderers address 'abuf' directly.
  // Plugins, on other reason, act differently, since they are not aware of 'abuf'.
  virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) = 0;
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber) = 0;
  virtual void EnsureTextValidForFont(char *text, int fontNumber) = 0;
};

extern IAGSFontRenderer* fontRenderers[MAX_FONTS];
extern wgtfont fonts[MAX_FONTS];

#endif // __AC_AGSFONTRENDERER_H
