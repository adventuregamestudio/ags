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

#include <string.h>
#include "font/wfnfont.h"

using namespace AGS;
using AGS::Common::Stream;

const char   *WFN_FILE_SIGNATURE = "WGT Font File  ";
const size_t  WFN_FILE_SIG_LENGTH = 15;

WFNFont::WFNChar::WFNChar()
    : Width(0)
    , Height(0)
    , Data(NULL)
{
}

WFNFont::WFNFont()
    : CharCount(0)
    , Chars(NULL)
    , CharData(NULL)
{
}

WFNFont::~WFNFont()
{
    delete [] Chars;
    delete [] CharData;
}

bool WFNFont::ReadFromFile(Stream *in, const size_t data_size)
{
    const size_t used_data_size = data_size > 0 ? data_size : in->GetLength();

    char sig[WFN_FILE_SIG_LENGTH];
    in->ReadArray(sig, WFN_FILE_SIG_LENGTH, 1);
    if (strncmp(sig, WFN_FILE_SIGNATURE, WFN_FILE_SIG_LENGTH) != 0)
        return false; // bad format

    const size_t table_addr = in->ReadInt16(); // offset table relative address
    if (table_addr < WFN_FILE_SIG_LENGTH + sizeof(int16_t) || table_addr >= used_data_size)
        return false; // bad table address

    const size_t offset_table_size = used_data_size - table_addr; // size of offset table
    CharCount = offset_table_size / sizeof(uint16_t);
    const size_t total_char_data = used_data_size - offset_table_size - WFN_FILE_SIG_LENGTH - sizeof(int16_t);
    const size_t char_pixel_data_size = total_char_data - sizeof(uint16_t) * 2 * CharCount;

    // Read characters array
    Chars = new WFNChar[CharCount];
    CharData = new uint8_t[char_pixel_data_size];
    WFNChar *char_ptr = Chars;
    uint8_t *char_data_ptr = CharData;
    for (size_t i = 0; i < CharCount; ++i, ++char_ptr)
    {
        char_ptr->Width = in->ReadInt16();
        char_ptr->Height = in->ReadInt16();
        if (char_ptr->Width == 0 || char_ptr->Height == 0)
            continue;
        char_ptr->Data = char_data_ptr;
        const size_t char_data_size = ((char_ptr->Width - 1) / 8 + 1) * char_ptr->Height;
        in->Read(char_data_ptr, char_data_size);
        char_data_ptr += char_data_size;
    }
    return true;
}
