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

#ifndef __AC_AGSFONTRENDERER_H
#define __AC_AGSFONTRENDERER_H

#include "ac/gamestructdefines.h"

typedef unsigned char IFont;
struct BITMAP;

// WARNING: this interface is exposed for plugins and declared for the second time in agsplugin.h
class IAGSFontRenderer {
public:
    virtual bool LoadFromDisk(int fontNumber, int fontSize) = 0;
    virtual void FreeMemory(int fontNumber) = 0;
    virtual bool SupportsExtendedCharacters(int fontNumber) = 0;
    virtual int GetTextWidth(const char *text, int fontNumber) = 0;
    virtual int GetTextHeight(const char *text, int fontNumber) = 0;
    // [IKM] An important note: the AGS font renderers do not use 'destination' parameter at all, probably
    // for simplicity (although that causes confusion): the parameter passed is always a global 'virtual screen'
    // pointer therefore renderers address 'virtual screen' directly.
    // Plugins, on other reason, act differently, since they are not aware of 'abuf'.
    virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) = 0;
    virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber) = 0;
    virtual void EnsureTextValidForFont(char *text, int fontNumber) = 0;
};

extern IAGSFontRenderer* fontRenderers[MAX_FONTS];
extern IFont *fonts[MAX_FONTS];

#endif // __AC_AGSFONTRENDERER_H
