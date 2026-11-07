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
//
// WFNFont - an immutable AGS font object.
//
//-----------------------------------------------------------------------------
//
// WFN format:
// - signature            ( 15 )
// - offsets table offset (  2 )
// - characters table (for as many char items as can fit until offsets table):
// -     width            (  2 )
// -     height           (  2 )
// -     pixel bits       ( (width / 8 + 1) * height )
// - offsets table (for X chars):
// -     character offset (  2 )
//
// Offsets table has its index equal to a character index in the char table
// which this font corresponds to (ASCII, ANSI, Unicode). In offsets table
// all char indexes must be sequential, meaning that the format may support a
// range of 0-64k glyph indexes.
// 
// All offsets in this format are relative to the beginning of the WFN data.
// 
// NOTE: unfortunately, at the moment the format does not provide means to
// know the number of supported characters for certain, and the size of the
// data (file) is used to determine that.
//
//-----------------------------------------------------------------------------
//
// Extended WFN format:
//
// - signature                ( 15 ) "WGT Font File 2"
// - header size              (  4 )
// - flags                    (  4 ) currently must be 0
// - bits per pixel           (  4 ) currently supports 4
// - glyph table address      (  4 )
// - glyph table size         (  4 )
// - character table address  (  4 )
// - character table size     (  4 )
// - base shade index         (  4 ) nominal range 1-15
//
// The header size is measured in bytes, excluding the 15-byte signature.
// Additional header fields may be appended in future versions.
//
// Glyph table (variable size):
// - glyph width              (  2 )
// - glyph height             (  2 )
// - pixel data               ( ceil(width * bits_per_pixel / 8) * height )
//
// Character table (8 bytes per entry):
// - character code           (  4 )
// - glyph offset             (  4 )
//
// All addresses are relative to the beginning of the WFN data.
// Glyph offsets are relative to the beginning of the glyph table.
//
// The character table maps character codes to glyph offsets, allowing sparse
// character indexes and multiple characters to reference the same glyph.
// Character entries need not be stored in character-code order.
// AGS currently supports character codes from 0 to 65535.
//
// In 4-bit fonts, each pixel stores a shade index from 0 to 15.
// The base shade index represents the exact requested text color.
// Index 0 is transparent; indices 1-15 represent visible shades.
//
//=============================================================================

#ifndef __AGS_CN_FONT__WFNFONT_H
#define __AGS_CN_FONT__WFNFONT_H

#include <vector>
#include "platform/types.h"
#include "util/geometry.h"

namespace AGS { namespace Common { class Stream; } }

enum WFNError
{
    kWFNErr_NoError,
    kWFNErr_BadSignature,
    kWFNErr_BadTableAddress,
    kWFNErr_HasBadCharacters,
    kWFNErr_BadHeader,
    kWFNErr_BadCharacterTable,
    kWFNErr_UnsupportedFlags
};

struct WFNChar
{
    uint16_t       Width;
    uint16_t       Height;
    const uint8_t *Data;
    uint8_t BitsPerPixel = 1;

    WFNChar();

    inline bool IsValid() const
    {
        return Width != 0 && Height != 0;
    }

    inline size_t GetRowByteCount() const
    {
        return (BitsPerPixel * Width + 7) / 8;
    }

    inline size_t GetRequiredPixelSize() const
    {
        return GetRowByteCount() * Height;
    }

    // Ensure character's width & height fit in given number of pixel bytes
    void RestrictToBytes(size_t bytes);
};


class WFNFont
{
public:
    inline size_t GetCharCount() const
    {
        return _refs.size();
    }

    // Get WFN character for the given code; if the character is missing, returns empty character
    inline const WFNChar &GetChar(uint16_t code) const
    {
        return code < _refs.size() ? *_refs[code] : _emptyChar;
    }

    // The font's real height is calculated from the highest glyph found
    inline int GetHeight() const { return _height; }
    // The font's BBox defines max bounds of all the glyphs in the font
    inline const Rect &GetBBox() const { return _bbox; }
    // Base shade index for font formats that support shades (1-15).
    // This shade represents the exact AGS text color.
    inline uint32_t GetBaseShadeIndex() const { return _baseShadeIndex; }
    // Font pixel depth in bits per pixel.
    inline uint8_t GetBitsPerPixel() const { return _bitsPerPixel; }

    void Clear();
    // Reads WFNFont object, using data_size bytes from stream; if data_size = 0,
    // the available stream's length is used instead. Returns error code.
    WFNError ReadFromFile(AGS::Common::Stream *in, const soff_t data_size = 0);

private:
    WFNError ReadExtendedFromFile(AGS::Common::Stream* in, const soff_t used_data_size);
    WFNError ReadClassicFromFile(AGS::Common::Stream* in, const soff_t used_data_size);
    std::vector<const WFNChar*> _refs;      // reference array, contains pointers to elements of _items
    std::vector<WFNChar>        _items;     // actual character items
    std::vector<uint8_t>        _pixelData; // pixel data array
    int  _height = 0;   // font's height (calculated from the max glyph's height)
    Rect _bbox;         // bounding box (maximal glyphs extent)
    uint32_t _baseShadeIndex = 8; // base shade representing the exact AGS text color (4-bit fonts)
    uint8_t _bitsPerPixel = 1; // font pixel depth in bits per pixel

    static const WFNChar        _emptyChar; // a dummy character to substitute bad symbols
};

#endif // __AGS_CN_FONT__WFNFONT_H
