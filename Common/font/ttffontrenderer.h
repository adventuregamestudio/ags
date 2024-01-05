//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_TTFFONTRENDERER_H
#define __AC_TTFFONTRENDERER_H

#include <map>
#include "font/agsfontrenderer.h"
#include "util/string.h"

struct ALFONT_FONT;

class TTFFontRenderer : public IAGSFontRendererInternal {
public:
  // IAGSFontRenderer implementation
  bool LoadFromDisk(int fontNumber, int fontSize) override;
  void FreeMemory(int fontNumber) override;
  bool SupportsExtendedCharacters(int /*fontNumber*/) override { return true; }
  int GetTextWidth(const char *text, int fontNumber) override;
  int GetTextHeight(const char *text, int fontNumber) override;
  void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) override ;
  void AdjustYCoordinateForFont(int *ycoord, int fontNumber) override;
  void EnsureTextValidForFont(char *text, int fontNumber) override;

  // IAGSFontRenderer2 implementation
  int GetVersion() override { return 26; /* first compatible engine API version */ }
  const char *GetRendererName() override { return "TTFFontRenderer"; }
  const char *GetFontName(int fontNumber) override;
  int GetFontHeight(int fontNumber) override;
  int GetLineSpacing(int fontNumber) override { return 0; /* no specific spacing */ }

  // IAGSFontRendererInternal implementation
  bool IsBitmapFont() override;
  bool LoadFromDiskEx(int fontNumber, int fontSize, const FontRenderParams *params,
      FontMetrics *metrics) override;
  void GetFontMetrics(int fontNumber, FontMetrics *metrics) override;
  void AdjustFontForAntiAlias(int fontNumber, bool aa_mode) override;

  //
  // Utility functions
  //
  // Try load the TTF font using provided point size, and report its metrics
  static bool MeasureFontOfPointSize(const AGS::Common::String &filename, int size_pt, FontMetrics *metrics);
  // Try load the TTF font, find the point size which results in pixel height
  // as close to the requested as possible; report its metrics
  static bool MeasureFontOfPixelHeight(const AGS::Common::String &filename, int pixel_height, FontMetrics *metrics);

private:
    struct FontData
    {
        ALFONT_FONT     *AlFont;
        FontRenderParams Params;
    };
    std::map<int, FontData> _fontData;
};

#endif // __AC_TTFFONTRENDERER_H
