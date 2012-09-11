#ifndef __AC_AGSFONTRENDERER_H
#define __AC_AGSFONTRENDERER_H

namespace AGS { namespace Common { class IBitmap; }}
using namespace AGS; // FIXME later

//#include "ac/gamesetupstruct.h"	// constants
// temporary define copy
#ifndef MAX_FONTS
#define MAX_FONTS           15
#endif

typedef unsigned char* wgtfont;

class IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize) = 0;
  virtual void FreeMemory(int fontNumber) = 0;
  virtual bool SupportsExtendedCharacters(int fontNumber) = 0;
  virtual int GetTextWidth(const char *text, int fontNumber) = 0;
  virtual int GetTextHeight(const char *text, int fontNumber) = 0;
  virtual void RenderText(const char *text, int fontNumber, Common::IBitmap *destination, int x, int y, int colour) = 0;
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber) = 0;
  virtual void EnsureTextValidForFont(char *text, int fontNumber) = 0;
};

extern IAGSFontRenderer* fontRenderers[MAX_FONTS];
extern wgtfont fonts[MAX_FONTS];

#endif // __AC_AGSFONTRENDERER_H