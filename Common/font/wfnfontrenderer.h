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

#ifndef __AC_WFNFONTRENDERER_H
#define __AC_WFNFONTRENDERER_H

#include "font/agsfontrenderer.h"
#include "font/wfnfont.h"

class WFNFontRenderer : public IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize);
  virtual void FreeMemory(int fontNumber);
  virtual bool SupportsExtendedCharacters(int fontNumber);
  virtual int GetTextWidth(const char *text, int fontNumber);
  virtual int GetTextHeight(const char *text, int fontNumber);
  virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour);
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber);
  virtual void EnsureTextValidForFont(char *text, int fontNumber);

private:
  inline unsigned char GetCharCode(unsigned char wanted_code, const WFNFont *font) const
  {
    return wanted_code < font->GetCharCount() ? wanted_code : '?';
  }
  int RenderChar(Common::Bitmap *ds, const int at_x, const int at_y, const WFNChar &wfn_char, const color_t text_color);
};

extern WFNFontRenderer wfnRenderer;

#endif // __AC_WFNFONTRENDERER_H
