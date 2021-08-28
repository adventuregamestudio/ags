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
// SpriteFile class handles sprite file loading and streaming.
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
    sprkey_t LastSlot = -1;
    size_t SpriteCount = 0u;
    std::vector<int16_t> Widths;
    std::vector<int16_t> Heights;
    std::vector<soff_t>  Offsets;
};


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
    void        Reset();

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

    HError      LoadSprite(sprkey_t index, Bitmap *&sprite);
    HError      LoadSpriteData(sprkey_t index, Size &metric, int &bpp, std::vector<char> &data);

    // Saves all sprites to file; fills in index data for external use
    // TODO: refactor to be able to save main file and index file separately (separate function for gather data?)
    static int  SaveToFile(const String &save_to_file,
        const std::vector<Bitmap*> &sprites, // available sprites (may contain nullptrs)
        SpriteFile *read_from_file, // optional file to read missing sprites from
        bool compressOutput, SpriteFileIndex &index);
    // Saves sprite index table in a separate file
    static int  SaveSpriteIndex(const String &filename, const SpriteFileIndex &index);

private:
    // Finds the topmost occupied slot index. Warning: may be slow.
    static sprkey_t FindTopmostSprite(const std::vector<Bitmap*> &sprites);
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

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_AC__SPRFILE_H
