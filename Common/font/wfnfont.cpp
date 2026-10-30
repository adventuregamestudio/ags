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
#include <algorithm>
#include "font/wfnfont.h"
#include "debug/out.h"
#include "util/memory.h"
#include "util/stream.h"

using namespace AGS::Common;

static const char   *WFN_FILE_SIGNATURE  = "WGT Font File  ";
static const size_t  WFN_FILE_SIG_LENGTH = 15;
static const size_t  MinCharDataSize     = sizeof(uint16_t) * 2;
static const char* WFN_EXT_SIGNATURE = "WGT Font File 2";

struct WFNExtHeader
{
    uint32_t HeaderSize;
    uint32_t Flags;
    uint32_t BitsPerPixel;
    uint32_t GlyphTableAddress;
    uint32_t GlyphTableSize;
    uint32_t CharacterTableAddress;
    uint32_t CharacterTableSize;
    uint32_t BaseShadeIndex;
};

WFNChar::WFNChar()
    : Width(0)
    , Height(0)
    , Data(nullptr)
{
}

void WFNChar::RestrictToBytes(size_t bytes)
{
    if (bytes < GetRequiredPixelSize())
        Height = static_cast<uint16_t>(bytes / GetRowByteCount());
}

static bool ReadWFNExtHeader(Stream* in, const soff_t file_size,
    WFNExtHeader& header)
{
    constexpr uint32_t kMinHeaderSize = 8u * sizeof(uint32_t);

    if (file_size < static_cast<soff_t>(WFN_FILE_SIG_LENGTH + kMinHeaderSize))
        return false;

    header.HeaderSize = static_cast<uint32_t>(in->ReadInt32());
    header.Flags = static_cast<uint32_t>(in->ReadInt32());
    header.BitsPerPixel = static_cast<uint32_t>(in->ReadInt32());
    header.GlyphTableAddress = static_cast<uint32_t>(in->ReadInt32());
    header.GlyphTableSize = static_cast<uint32_t>(in->ReadInt32());
    header.CharacterTableAddress = static_cast<uint32_t>(in->ReadInt32());
    header.CharacterTableSize = static_cast<uint32_t>(in->ReadInt32());
    header.BaseShadeIndex = static_cast<uint32_t>(in->ReadInt32());

    if (header.HeaderSize < kMinHeaderSize)
        return false;

    if (static_cast<uint64_t>(WFN_FILE_SIG_LENGTH) + header.HeaderSize >
        static_cast<uint64_t>(file_size))
        return false;

    if (header.BitsPerPixel != 4)
        return false;

    // The base shade must be a non-transparent palette index.
    if (header.BaseShadeIndex < 1 || header.BaseShadeIndex > 15)
        return false;

    const uint64_t header_end =
        static_cast<uint64_t>(WFN_FILE_SIG_LENGTH) + header.HeaderSize;

    const uint64_t glyph_end =
        static_cast<uint64_t>(header.GlyphTableAddress) +
        header.GlyphTableSize;

    const uint64_t character_end =
        static_cast<uint64_t>(header.CharacterTableAddress) +
        header.CharacterTableSize;

    if (header.GlyphTableAddress < header_end ||
        glyph_end > static_cast<uint64_t>(file_size))
        return false;

    if (header.CharacterTableAddress < header_end ||
        character_end > static_cast<uint64_t>(file_size))
        return false;

    if ((header.CharacterTableSize % (2u * sizeof(uint32_t))) != 0)
        return false;

    // The two data tables must not overlap.
    if (header.GlyphTableSize != 0 &&
        header.CharacterTableSize != 0 &&
        static_cast<uint64_t>(header.GlyphTableAddress) < character_end &&
        static_cast<uint64_t>(header.CharacterTableAddress) < glyph_end)
        return false;

    return true;
}

const WFNChar WFNFont::_emptyChar;

void WFNFont::Clear()
{
    _refs.clear();
    _items.clear();
    _pixelData.clear();
    _height = 0;
    _bbox = Rect();
    _baseShadeIndex = 8;
    _bitsPerPixel = 1;
}

WFNError WFNFont::Read4BitFromFile(Stream* in, const soff_t used_data_size)
{
    WFNExtHeader header;

    if (!ReadWFNExtHeader(in, used_data_size, header))
        return kWFNErr_BadTableAddress;

    _bitsPerPixel = 4;
    _baseShadeIndex = static_cast<uint8_t>(header.BaseShadeIndex);

    // Unknown flags may change the meaning of the stored data.
    if (header.Flags != 0)
    {
        Debug::Printf(kDbgMsg_Error,
            "\tWFN: unsupported extended header flags: %u",
            header.Flags);
        return kWFNErr_BadTableAddress;
    }

    // Future versions may append additional header fields.
    if (header.HeaderSize > 8u * sizeof(uint32_t))
    {
        Debug::Printf(
            "\tWFN: warning: extended header contains unknown fields");
    }

    const size_t glyph_data_size =
        static_cast<size_t>(header.GlyphTableSize);

    const size_t char_count =
        static_cast<size_t>(header.CharacterTableSize) /
        (2u * sizeof(uint32_t));

    // Read the glyph-data table.
    std::vector<uint8_t> raw_data(glyph_data_size);

    in->Seek(header.GlyphTableAddress, kSeekBegin);

    if (glyph_data_size != 0 &&
        in->Read(raw_data.data(), glyph_data_size) != glyph_data_size)
        return kWFNErr_BadTableAddress;

    // Read the sparse character map.
    struct CharacterMapEntry
    {
        uint32_t Code;
        uint32_t Offset;
    };

    std::vector<CharacterMapEntry> character_map(char_count);

    in->Seek(header.CharacterTableAddress, kSeekBegin);

    for (size_t i = 0; i < char_count; ++i)
    {
        character_map[i].Code =
            static_cast<uint32_t>(in->ReadInt32());

        character_map[i].Offset =
            static_cast<uint32_t>(in->ReadInt32());
    }

    // Character codes must be sorted and unique.
    for (size_t i = 1; i < char_count; ++i)
    {
        if (character_map[i].Code <= character_map[i - 1].Code)
            return kWFNErr_BadTableAddress;
    }

    // AGS currently supports character indexes up to 65535.
    if (!character_map.empty() &&
        character_map.back().Code >= 65536u)
        return kWFNErr_BadTableAddress;

    WFNError err = kWFNErr_NoError;

    // Collect valid glyph offsets, excluding duplicates.
    std::vector<uint32_t> offs;
    offs.reserve(char_count);

    for (size_t i = 0; i < char_count; ++i)
    {
        const uint32_t off = character_map[i].Offset;

        if (static_cast<uint64_t>(off) + MinCharDataSize >
            glyph_data_size)
        {
            err = kWFNErr_HasBadCharacters;
            continue;
        }

        offs.push_back(off);
    }

    std::sort(offs.begin(), offs.end());
    offs.erase(std::unique(offs.begin(), offs.end()), offs.end());

    // Each distinct glyph must have room for its own width/height header.
    for (size_t i = 1; i < offs.size(); ++i)
    {
        if (static_cast<uint64_t>(offs[i - 1]) + MinCharDataSize > offs[i])
            return kWFNErr_BadTableAddress;
    }

    _items.resize(offs.size());

    size_t total_pixel_size = 0;

    // Read glyph dimensions and validate pixel data.
    for (size_t i = 0; i < offs.size(); ++i)
    {
        const size_t pos = static_cast<size_t>(offs[i]);

        WFNChar ch;

        ch.Width = static_cast<uint16_t>(
            Memory::ReadInt16LE(&raw_data[pos]));

        ch.Height = static_cast<uint16_t>(
            Memory::ReadInt16LE(&raw_data[pos + sizeof(uint16_t)]));

        ch.BitsPerPixel = 4;

        const size_t expected_size = ch.GetRequiredPixelSize();

        const size_t next_pos =
            (i + 1 < offs.size())
            ? static_cast<size_t>(offs[i + 1])
            : glyph_data_size;

        const size_t pixel_pos = pos + MinCharDataSize;

        const size_t available =
            next_pos > pixel_pos ? next_pos - pixel_pos : 0;

        if (available < expected_size)
        {
            err = kWFNErr_HasBadCharacters;
            ch.RestrictToBytes(available);
        }

        total_pixel_size += ch.GetRequiredPixelSize();

        _items[i] = ch;

        if (ch.Width != 0 && ch.Height != 0)
        {
            _bbox.Right = std::max(
                _bbox.Right, static_cast<int>(ch.Width) - 1);

            _bbox.Bottom = std::max(
                _bbox.Bottom, static_cast<int>(ch.Height) - 1);
        }
    }

    // Allocate and copy pixel data.
    _pixelData.resize(total_pixel_size);

    size_t pixel_dest = 0;

    for (size_t i = 0; i < offs.size(); ++i)
    {
        const size_t count = _items[i].GetRequiredPixelSize();

        if (count == 0)
            continue;

        const size_t pos =
            static_cast<size_t>(offs[i]) + MinCharDataSize;

        std::copy(
            raw_data.begin() + pos,
            raw_data.begin() + pos + count,
            _pixelData.begin() + pixel_dest);

        _items[i].Data = _pixelData.data() + pixel_dest;

        pixel_dest += count;
    }

    // Build the character reference array.
    const size_t ref_count = character_map.empty()
        ? 0u
        : static_cast<size_t>(character_map.back().Code) + 1u;

    _refs.resize(ref_count, &_emptyChar);

    for (size_t i = 0; i < char_count; ++i)
    {
        const uint32_t off = character_map[i].Offset;

        const auto at =
            std::lower_bound(offs.begin(), offs.end(), off);

        if (at != offs.end() && *at == off)
        {
            _refs[character_map[i].Code] =
                &_items[at - offs.begin()];
        }
    }

    _height = _bbox.GetHeight();

    return err;
}

WFNError WFNFont::ReadFromFile(Stream *in, const soff_t data_size)
{
    Clear();

    const soff_t used_data_size = data_size > 0 ? data_size : in->GetLength();

    // Read font header
    char sig[WFN_FILE_SIG_LENGTH];
    in->Read(sig, WFN_FILE_SIG_LENGTH);
    const bool is_legacy_wfn =
        strncmp(sig, WFN_FILE_SIGNATURE, WFN_FILE_SIG_LENGTH) == 0;

    const bool is_4bit_wfn =
        strncmp(sig, WFN_EXT_SIGNATURE, WFN_FILE_SIG_LENGTH) == 0;

    if (!is_legacy_wfn && !is_4bit_wfn)
    {
        Debug::Printf(kDbgMsg_Error, "\tWFN: bad format signature");
        return kWFNErr_BadSignature;
    }

    if (is_4bit_wfn)
    {
        return Read4BitFromFile(in, used_data_size);
    }

    const soff_t table_addr = static_cast<uint16_t>(in->ReadInt16()); // offset table relative address
    if (table_addr < WFN_FILE_SIG_LENGTH + sizeof(uint16_t) || table_addr >= used_data_size)
    {
        Debug::Printf(kDbgMsg_Error, "\tWFN: bad table address: %jd (%jd - %jd)", static_cast<intmax_t>(table_addr),
            static_cast<intmax_t>(WFN_FILE_SIG_LENGTH + sizeof(uint16_t)), static_cast<intmax_t>(used_data_size));
        return kWFNErr_BadTableAddress; // bad table address
    }

    const soff_t offset_table_size = used_data_size - table_addr;
    const soff_t raw_data_offset = WFN_FILE_SIG_LENGTH + sizeof(uint16_t);
    const size_t total_char_data = static_cast<size_t>(table_addr - raw_data_offset);
    const size_t char_count = static_cast<size_t>(offset_table_size / sizeof(uint16_t));

    // We process character data in three steps:
    // 1. For every character store offset of character item, excluding
    //    duplicates.
    // 2. Allocate memory for character items and pixel array and copy
    //    appropriate data; test for possible format corruption.
    // 3. Create array of references from characters to items; same item may be
    //    referenced by many characters.
    WFNError err = kWFNErr_NoError;

    if (total_char_data == 0u || char_count == 0u)
        return kWFNErr_NoError; // no items

    // Read character data array
    std::vector<uint8_t> raw_data(total_char_data);
    in->Read(raw_data.data(), total_char_data);

    // Read offset table
    std::vector<uint16_t> offset_table(char_count);
    in->ReadArrayOfInt16(reinterpret_cast<int16_t*>(offset_table.data()), char_count);

    // Read all referenced offsets in an unsorted vector
    std::vector<uint16_t> offs;
    offs.reserve(char_count); // reserve max possible offsets
    for (size_t i = 0; i < char_count; ++i)
    {
        const uint16_t off = offset_table[i];
        if (off == 0)
        {
            // character does not has a glyph, this is allowed
            continue;
        }
        else if (off < raw_data_offset || off + MinCharDataSize > table_addr)
        {
            Debug::Printf("\tWFN: character %zu -- bad item offset: %d (%" PRId64 " - %" PRId64 ", +%zu)",
                i, off, raw_data_offset, table_addr, MinCharDataSize);
            err = kWFNErr_HasBadCharacters; // warn about potentially corrupt format
            continue; // bad character offset
        }
        offs.push_back(off);
    }
    // sort offsets vector and remove any duplicates
    std::sort(offs.begin(), offs.end());
    std::vector<uint16_t>(offs.begin(), std::unique(offs.begin(), offs.end())).swap(offs);

    // Now that we know number of valid character items, parse and store character data
    WFNChar init_ch;
    _items.resize(offs.size());
    size_t total_pixel_size = 0;
    for (size_t i = 0; i < _items.size(); ++i)
    {
        const uint8_t *p_data = &raw_data[offs[i] - raw_data_offset];
        init_ch.Width  = Memory::ReadInt16LE(p_data);
        init_ch.Height = Memory::ReadInt16LE(p_data + sizeof(uint16_t));
        total_pixel_size += init_ch.GetRequiredPixelSize();
        _items[i] = init_ch;

        _bbox.Right = std::max(_bbox.Right, init_ch.Width - 1);
        _bbox.Bottom = std::max(_bbox.Bottom, init_ch.Height - 1);
    }

    // Now that we know actual size of pixels in use, create pixel data array;
    // since the items are sorted, the pixel data will be stored sequentially as well.
    // At this point offs and _items have related elements in the same order.
    _pixelData.resize(total_pixel_size);
    std::vector<uint8_t>::iterator pixel_it = _pixelData.begin(); // write ptr
    for (size_t i = 0; i < _items.size(); ++i)
    {
        const size_t pixel_data_size = _items[i].GetRequiredPixelSize();
        if (pixel_data_size == 0)
        {
            continue; // just an empty character, this is allowed
        }
        const uint16_t raw_off  = offs[i] - raw_data_offset + MinCharDataSize; // offset in raw array
        size_t src_size = pixel_data_size;
        if (i + 1 != _items.size() && raw_off + src_size > offs[i + 1] - raw_data_offset)
        {   // character pixel data overlaps next character
            Debug::Printf("\tWFN: item at off %d -- pixel data overlaps next known item (at %d, +%zu)",
                        offs[i], offs[i + 1], MinCharDataSize + src_size);
            err = kWFNErr_HasBadCharacters; // warn about potentially corrupt format
            src_size = offs[i + 1] - offs[i] - MinCharDataSize;
        }
        
        if (raw_off + src_size > total_char_data)
        {   // character pixel data overflow buffer
            Debug::Printf("\tWFN: item at off %d -- pixel data exceeds available data (at %" PRId64 ", +%zu)",
                        offs[i], table_addr, MinCharDataSize + src_size);
            err = kWFNErr_HasBadCharacters; // warn about potentially corrupt format
            src_size = total_char_data - raw_off;
        }
        _items[i].RestrictToBytes(src_size);

        assert(pixel_it + pixel_data_size <= _pixelData.end()); // should not normally fail
        std::copy(raw_data.begin() + raw_off, raw_data.begin() + (raw_off + src_size), pixel_it);
        _items[i].Data = &(*pixel_it);
        pixel_it += pixel_data_size;
    }

    // Create final reference array
    _refs.resize(char_count);
    for (size_t i = 0; i < char_count; ++i)
    {
        const uint16_t off = offset_table[i];
        // if bad character offset - reference empty character
        if (off < raw_data_offset || off + MinCharDataSize > table_addr)
        {
            _refs[i] = &_emptyChar;
        }
        else
        {
            // in usual case the offset table references items in strict order
            if (i < _items.size() && offs[i] == off)
                _refs[i] = &_items[i];
            else
            {
                // we know beforehand that such item must exist
                std::vector<uint16_t>::const_iterator at = std::lower_bound(offs.begin(), offs.end(), off);
                assert(at != offs.end() && *at == off && // should not normally fail
                       at - offs.begin() >= 0 && static_cast<size_t>(at - offs.begin()) < _items.size());
                _refs[i] = &_items[at - offs.begin()]; // set up reference to item
            }
        }
    }

    // Save font metrics
    _height = _bbox.GetHeight();

    return err;
}
