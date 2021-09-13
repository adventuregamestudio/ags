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
// SpriteFile class handles sprite file parsing and streaming sprites.
// SpriteFileWriter manages writing sprites into the output stream one by one,
// accumulating index information, and may therefore be suitable for a variety
// of situations.
//
//=============================================================================
#ifndef __AGS_CN_AC__SPRFILE_H
#define __AGS_CN_AC__SPRFILE_H

#include <memory>
#include <vector>
#include "core/types.h"
#include "util/error.h"
#include "util/geometry.h"
#include "util/stream.h"
#include "util/string.h"


namespace AGS
{
namespace Common
{

class Bitmap;

// TODO: research old version differences
enum SpriteFileVersion
{
    kSprfVersion_Uncompressed = 4,
    kSprfVersion_Compressed = 5,
    kSprfVersion_Last32bit = 6,
    kSprfVersion_64bit = 10,
    kSprfVersion_HighSpriteLimit = 11,
    kSprfVersion_Current = kSprfVersion_HighSpriteLimit
};

enum SpriteIndexFileVersion
{
    kSpridxfVersion_Initial = 1,
    kSpridxfVersion_Last32bit = 2,
    kSpridxfVersion_64bit = 10,
    kSpridxfVersion_HighSpriteLimit = 11,
    kSpridxfVersion_Current = kSpridxfVersion_HighSpriteLimit
};

typedef int32_t sprkey_t;

// SpriteFileIndex contains sprite file's table of contents
struct SpriteFileIndex
{
    int SpriteFileIDCheck = 0; // tag matching sprite file and index file
    std::vector<int16_t> Widths;
    std::vector<int16_t> Heights;
    std::vector<soff_t>  Offsets;

    inline size_t GetCount() const { return Offsets.size(); }
    inline sprkey_t GetLastSlot() const { return (sprkey_t)GetCount() - 1; }
};


// SpriteFile opens a sprite file for reading, reports general information,
// and lets read sprites in any order.
class SpriteFile
{
public:
    // Standart sprite file and sprite index names
    static const String DefaultSpriteFileName;
    static const String DefaultSpriteIndexName;

    SpriteFile();
    // Loads sprite reference information and inits sprite stream
    HError      OpenFile(const String &filename, const String &sprindex_filename,
        std::vector<Size> &metrics);
    // Closes stream; no reading will be possible unless opened again
    void        Close();

    // Tells if bitmaps in the file are compressed
    bool        IsFileCompressed() const;
    // Tells the highest known sprite index
    sprkey_t    GetTopmostSprite() const;

    // Loads sprite index file
    bool        LoadSpriteIndexFile(const String &filename, int expectedFileID,
        soff_t spr_initial_offs, sprkey_t topmost, std::vector<Size> &metrics);
    // Rebuilds sprite index from the main sprite file
    HError      RebuildSpriteIndex(Stream *in, sprkey_t topmost, SpriteFileVersion vers,
        std::vector<Size> &metrics);

    // Loads an image data and creates a ready bitmap
    HError      LoadSprite(sprkey_t index, Bitmap *&sprite);
    // Loads an image data into the buffer, reports the bitmap metrics and color depth
    HError      LoadSpriteData(sprkey_t index, Size &metric, int &bpp, std::vector<char> &data);

private:
    // Seek stream to sprite
    void        SeekToSprite(sprkey_t index);

    // Internal sprite reference
    struct SpriteRef
    {
        soff_t Offset = 0; // data offset
        size_t Size = 0;   // cache size of element, in bytes
    };

    // Array of sprite references
    std::vector<SpriteRef> _spriteData;
    std::unique_ptr<Stream> _stream; // the sprite stream
    bool _compressed; // are sprites compressed
    sprkey_t _curPos; // current stream position (sprite slot)
};


// SpriteFileWriter class writes a sprite file in a requested format.
// Start using it by calling Begin, write ready bitmaps or copy raw sprite data
// over slot by slot, then call Finalize to let it close the format correctly.
class SpriteFileWriter
{
public:
    SpriteFileWriter(std::unique_ptr<Stream> &&out)
        : _out(std::move(out)) {}
    ~SpriteFileWriter() = default;

    // Get the sprite index, accumulated after write
    const SpriteFileIndex &GetIndex() const { return _index; }

    // Initializes new sprite file format
    void Begin(bool compress, sprkey_t last_slot = -1);
    // Writes a bitmap into file, compressing if necessary
    void WriteBitmap(Bitmap *image);
    // Writes an empty slot marker
    void WriteEmptySlot();
    // Writes a raw sprite data without additional processing
    void WriteSpriteData(const char *pbuf, size_t len, int w, int h, int bpp);
    void WriteSpriteData(const std::vector<char> &buf, int w, int h, int bpp)
        { WriteSpriteData(&buf[0], buf.size(), w, h, bpp); }
    // Finalizes current format; no further writing is possible after this
    void Finalize();

private:
    std::unique_ptr<Stream> _out;
    bool _compress = false;
    soff_t _lastSlotPos = -1; // last slot save position in file
    // sprite index accumulated on write for reporting back to user
    SpriteFileIndex _index;
    // compression buffer
    std::vector<char> _membuf;
};


// Saves all sprites to file; fills in index data for external use
// TODO: refactor to be able to save main file and index file separately (separate function for gather data?)
int SaveSpriteFile(const String &save_to_file,
    const std::vector<Bitmap*> &sprites, // available sprites (may contain nullptrs)
    SpriteFile *read_from_file, // optional file to read missing sprites from
    bool compressOutput, SpriteFileIndex &index);
// Saves sprite index table in a separate file
int SaveSpriteIndex(const String &filename, const SpriteFileIndex &index);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_AC__SPRFILE_H
