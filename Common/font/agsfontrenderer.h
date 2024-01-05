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

#ifndef __AC_AGSFONTRENDERER_H
#define __AC_AGSFONTRENDERER_H

struct BITMAP;

// Basic font renderer interface.
// WARNING: this interface is exposed for plugins and declared for the second time in agsplugin.h
class IAGSFontRenderer
{
public:
    virtual bool LoadFromDisk(int fontNumber, int fontSize) = 0;
    virtual void FreeMemory(int fontNumber) = 0;
    virtual bool SupportsExtendedCharacters(int fontNumber) = 0;
    virtual int GetTextWidth(const char *text, int fontNumber) = 0;
    // Get actual height of the given line of text
    virtual int GetTextHeight(const char *text, int fontNumber) = 0;
    virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) = 0;
    virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber) = 0;
    virtual void EnsureTextValidForFont(char *text, int fontNumber) = 0;
protected:
    IAGSFontRenderer() = default;
    ~IAGSFontRenderer() = default;
};

// Extended font renderer interface.
// WARNING: this interface is exposed for plugins and declared for the second time in agsplugin.h
class IAGSFontRenderer2 : public IAGSFontRenderer
{
public:
    // Returns engine API version this font renderer complies to.
    // Must not be lower than 26 (this interface was added at API v26).
    virtual int GetVersion() = 0;
    // Returns an arbitrary renderer name; this is for informational
    // purposes only.
    virtual const char *GetRendererName() = 0;
    // Returns given font's name (if available).
    virtual const char *GetFontName(int fontNumber) = 0;
    // Returns the given font's height: that is the maximal vertical size
    // that the font glyphs may occupy.
    virtual int GetFontHeight(int fontNumber) = 0;
    // Returns the given font's linespacing;
    // is allowed to return 0, telling that no specific linespacing
    // is assigned for this font.
    virtual int GetLineSpacing(int fontNumber) = 0;

protected:
    IAGSFontRenderer2() = default;
    ~IAGSFontRenderer2() = default;
};

// Font render params, mainly for dealing with various compatibility issues.
struct FontRenderParams
{
    // Font's render multiplier
    int SizeMultiplier = 1;
    int LoadMode = 0; // contains font flags from FFLG_LOADMODEMASK
};

// Describes loaded font's properties
struct FontMetrics
{
    int Height = 0; // formal font height value
    int RealHeight = 0; // real graphical height of a font
    int CompatHeight = 0; // either formal or real height, depending on compat settings
};

// The strictly internal font renderer interface, not to use in plugin API.
// Contains methods necessary for built-in font renderers.
class IAGSFontRendererInternal : public IAGSFontRenderer2
{
public:
    // Tells if this is a bitmap font (otherwise it's a vector font)
    virtual bool IsBitmapFont() = 0;
    // Load font, applying extended font rendering parameters
    virtual bool LoadFromDiskEx(int fontNumber, int fontSize, const FontRenderParams *params,
        FontMetrics *metrics) = 0;
    // Fill FontMetrics struct; note that it may be left cleared if this is not supported
    virtual void GetFontMetrics(int fontNumber, FontMetrics *metrics) = 0;
    // Perform any necessary adjustments when the AA mode is toggled
    virtual void AdjustFontForAntiAlias(int fontNumber, bool aa_mode) = 0;

protected:
    IAGSFontRendererInternal() = default;
    ~IAGSFontRendererInternal() = default;
};

#endif // __AC_AGSFONTRENDERER_H
