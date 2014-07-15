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
//
// WFNFont - an immutable AGS font object.
//
//-----------------------------------------------------------------------------
//
// WFN format:
// - signature            ( 15 )
// - offsets table offset (  2 )
// - characters table (for X chars):
// -     width            (  2 )
// -     height           (  2 )
// -     pixel bits       ( (width / 8 + 1) * height )
// - offsets table (for X chars):
// -     character offset (  2 )
//
// NOTE: unfortunately, at the moment the format does not provide means to
// know the number of supported characters for certain, and the size of the
// data (file) is used to determine that.
//
//=============================================================================

#ifndef __AGS_CN_FONT__WFNFONT_H
#define __AGS_CN_FONT__WFNFONT_H

#include "util/stream.h"

class WFNFont
{
public:
    struct WFNChar
    {
        uint16_t       Width;
        uint16_t       Height;
        const uint8_t *Data;

        WFNChar();
        inline size_t GetRowByteCount() const
        {
            return ((Width - 1) / 8 + 1);
        }
        inline size_t GetRequiredDataSize() const
        {
            return GetRowByteCount() * Height;
        }
    };

public:
    WFNFont();
    ~WFNFont();

    inline uint16_t GetCharCount() const
    {
        return _charCount;
    }

    // Get WFN character for the given code; if the character is missing, returns empty character
    inline const WFNChar &GetChar(uint8_t code) const
    {
        return code < _charCount ? _chars[code] : _emptyChar;
    }

    void Clear();
    // Reads WFNFont object, using data_size bytes from stream; if data_size = 0,
    // the available stream's length is used instead. Returns false on error.
    bool ReadFromFile(AGS::Common::Stream *in, const size_t data_size = 0);

protected:
    uint16_t  _charCount;
    WFNChar  *_chars;
    uint8_t  *_charData;

    static WFNChar _emptyChar;
};

#endif // __AGS_CN_FONT__WFNFONT_H
