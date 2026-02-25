//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_WFNFONTRENDERER_H
#define __AC_WFNFONTRENDERER_H

#include <map>
#include "data/assetmanager.h"
#include "font/agsfontrenderer.h"

class WFNFont;

class WFNFontRenderer : public IAGSFontRendererInternal {
public:
  // IAGSFontRenderer implementation
  bool LoadFromDisk(int fontNumber, int fontSize) override;
  void FreeMemory(int fontNumber) override;
  bool SupportsExtendedCharacters(int fontNumber) override;
  int GetTextWidth(const char *text, int fontNumber) override;
  int GetTextHeight(const char *text, int fontNumber) override;
  void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) override;
  void AdjustYCoordinateForFont(int *ycoord, int fontNumber) override;
  void EnsureTextValidForFont(char *text, int fontNumber) override;

  // IAGSFontRenderer2 implementation
  int GetVersion() override { return 26; /* first compatible engine API version */ }
  const char *GetRendererName() override { return "WFNFontRenderer"; }
  const char *GetFontName(int /*fontNumber*/) override { return ""; }
  int GetFontHeight(int fontNumber) override;
  int GetLineSpacing(int fontNumber) override { return 0; /* no specific spacing */ }

  // IAGSFontRendererInternal implementation
  bool IsBitmapFont() override;
  bool LoadFromDiskEx(int fontNumber, int fontSize, const String &filename, String *src_filename,
      const FontRenderParams *params, FontMetrics *metrics) override;
  void GetFontMetrics(int fontNumber, FontMetrics *metrics) override;
  void AdjustFontForAntiAlias(int /*fontNumber*/, bool /*aa_mode*/) override { /* do nothing */}
  void GetCharCodeRange(int fontNumber, std::pair<int, int> *char_codes) override;
  void GetValidCharCodes(int fontNumber, std::vector<int> &char_codes) override;
  void SetCharacterSpacing(int fontNumber, int spacing) override;

  WFNFontRenderer(AGS::Common::AssetManager *mgr)
      : _amgr(mgr) {}
  virtual ~WFNFontRenderer() = default;

private:
  void FillMetrics(const WFNFont *font, FontMetrics *metrics);

  struct FontData
  {
    WFNFont         *Font;
    FontRenderParams Params;
    int              CharacterSpacing;
  };
  std::map<int, FontData> _fontData;
  AGS::Common::AssetManager *_amgr = nullptr;
};

#endif // __AC_WFNFONTRENDERER_H
