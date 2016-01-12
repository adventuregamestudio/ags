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

#ifndef __AC_TTFFONTRENDERER_H
#define __AC_TTFFONTRENDERER_H

#include "font/agsfontrenderer.h"

#include <map>

struct ALFONT_FONT;

class TTFFontRenderer : public IAGSFontRenderer {
public:
    virtual bool LoadFromDisk(int fontNumber, int fontSize) override;
    virtual void FreeMemory(int fontNumber) override;
    virtual bool SupportsExtendedCharacters(int fontNumber) override { return true; }
    virtual int GetTextWidth(const char *text, int fontNumber) override;
    virtual int GetTextHeight(const char *text, int fontNumber) override;
    virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) override;
    virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber) override;
    virtual void EnsureTextValidForFont(char *text, int fontNumber) override;

private:
    std::map<int, ALFONT_FONT*> _fontData;
};

extern TTFFontRenderer ttfRenderer;

#endif // __AC_TTFFONTRENDERER_H
