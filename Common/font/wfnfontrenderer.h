#ifndef __AC_WFNFONTRENDERER_H
#define __AC_WFNFONTRENDERER_H

#include "font/agsfontrenderer.h"

namespace AGS { namespace Common { class IBitmap; }}
using namespace AGS; // FIXME later

class WFNFontRenderer : public IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize);
  virtual void FreeMemory(int fontNumber);
  virtual bool SupportsExtendedCharacters(int fontNumber) { return false; }
  virtual int GetTextWidth(const char *text, int fontNumber);
  virtual int GetTextHeight(const char *text, int fontNumber);
  virtual void RenderText(const char *text, int fontNumber, Common::IBitmap *destination, int x, int y, int colour) ;
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber);
  virtual void EnsureTextValidForFont(char *text, int fontNumber);

private:
  int printchar(int xxx, int yyy, wgtfont foo, int charr);
};

extern WFNFontRenderer wfnRenderer;

#endif // __AC_WFNFONTRENDERER_H