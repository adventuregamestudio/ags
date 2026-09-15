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
#include "ac/spritefile.h"
#include <algorithm>
#include <array>
#include <time.h>
#include "data/assetmanager.h"
#include "gfx/sprite_data_fmt.h"
#include "util/compress.h"
#include "util/file.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

namespace AGS
{
namespace Common
{

const char *spriteFileSig = " Sprite File ";
const char *spindexid = "SPRINDEX";

// TODO: should not be part of SpriteFile, but rather some asset management class?
const String SpriteFile::DefaultSpriteFileName = "acsprset.spr";
const String SpriteFile::DefaultSpriteIndexName = "sprindex.dat";

SpriteFile::SpriteFile()
{
    _curPos = -2;
}

HError SpriteFile::OpenFile(std::unique_ptr<Stream> &&sprite_file,
    std::unique_ptr<Stream> &&index_file)
{
    return OpenFileImpl(std::move(sprite_file), std::move(index_file), nullptr, nullptr);
}

HError SpriteFile::OpenFile(std::unique_ptr<Stream> &&sprite_file,
    std::unique_ptr<Stream> &&index_file, std::vector<Size> &metrics)
{
    return OpenFileImpl(std::move(sprite_file), std::move(index_file), &metrics, nullptr);
}

HError SpriteFile::OpenFile(std::unique_ptr<Stream> &&sprite_file,
    std::unique_ptr<Stream> &&index_file, std::vector<SpriteDatHeader> &metrics)
{
    return OpenFileImpl(std::move(sprite_file), std::move(index_file), nullptr, &metrics);
}

HError SpriteFile::OpenFileImpl(std::unique_ptr<Stream> &&sprite_file,
    std::unique_ptr<Stream> &&index_file, std::vector<Size> *metrics, std::vector<SpriteDatHeader> *metrics2)
{
    Close();

    assert(sprite_file);
    if (!sprite_file)
        return new Error("Invalid spritefile stream.");

    _stream = std::move(sprite_file);

    _version = (SpriteFileVersion)_stream->ReadInt16();
    // read the "Sprite File" signature
    char buff[20];
    _stream->ReadArray(&buff[0], 13, 1);

    if (_version < kSprfVersion_Uncompressed || _version > kSprfVersion_Current)
    {
        _stream.reset();
        return new Error("Unsupported spriteset format (requested %d, supported %d - %d).", _version,
            kSprfVersion_Uncompressed, kSprfVersion_Current);
    }

    // unknown version
    buff[13] = 0;
    if (strcmp(buff, spriteFileSig))
    {
        _stream.reset();
        return new Error("Uknown spriteset format.");
    }

    int spriteFileID = 0;
    _storeFlags = 0;
    if (_version < kSprfVersion_Compressed)
    {
        _compress = kSprCompress_None;
        // skip the palette
        _stream->Seek(256 * 3); // sizeof(RGB) * 256
    }
    else if (_version == kSprfVersion_Compressed)
    {
        _compress = kSprCompress_RLE;
    }
    else if (_version >= kSprfVersion_Last32bit)
    {
        _compress = (SpriteCompression)_stream->ReadInt8();
        spriteFileID = _stream->ReadInt32();
    }

    sprkey_t topmost;
    if (_version < kSprfVersion_HighSpriteLimit)
        topmost = (uint16_t)_stream->ReadInt16();
    else
        topmost = _stream->ReadInt32();
    if (_version < kSprfVersion_Uncompressed)
        topmost = 200;

    _spriteData.resize(topmost + 1);
    if (metrics)
        metrics->resize(topmost + 1);
    if (metrics2)
        metrics2->resize(topmost + 1);

    // Version 12+: read global store flags
    if (_version >= kSprfVersion_StorageFormats)
    {
        _storeFlags = _stream->ReadInt8();
        _stream->ReadInt8(); // reserved
        _stream->ReadInt8();
        _stream->ReadInt8();
    }

    // If there is a sprite index file, then use it,
    // but only if we are not required to fill full sprite info, because
    // index contains only basic parameters.
    if (!metrics2 &&
        LoadSpriteIndexFile(std::move(index_file), spriteFileID, topmost, metrics))
    {
        // Succeeded
        return HError::None();
    }

    // Failed, index file is invalid; index sprites manually
    return RebuildSpriteIndex(_stream.get(), topmost, metrics, metrics2);
}

void SpriteFile::Close()
{
    _stream.reset();
    _spriteData.clear();
    _version = kSprfVersion_Undefined;
    _storeFlags = 0;
    _compress = kSprCompress_None;
    _curPos = -2;
}

int SpriteFile::GetStoreFlags() const
{
    return _storeFlags;
}

SpriteCompression SpriteFile::GetSpriteCompression() const
{
    return _compress;
}

sprkey_t SpriteFile::GetTopmostSprite() const
{
    return _spriteData.size() > 0 ? static_cast<sprkey_t>(_spriteData.size()) - 1 : -1;
}

size_t SpriteFile::GetSpriteCount() const
{
    return _validCount;
}

bool SpriteFile::LoadSpriteIndexFile(std::unique_ptr<Stream> &&fidx,
    int expectedFileID, sprkey_t topmost, std::vector<Size> *metrics)
{
    if (!fidx)
    {
        return false;
    }

    char buffer[9];
    // check "SPRINDEX" id
    fidx->ReadArray(&buffer[0], strlen(spindexid), 1);
    buffer[8] = 0;
    if (strcmp(buffer, spindexid))
    {
        return false;
    }
    // check version
    SpriteIndexFileVersion vers = (SpriteIndexFileVersion)fidx->ReadInt32();
    if (vers < kSpridxfVersion_Initial || vers > kSpridxfVersion_Current)
    {
        return false;
    }
    if (vers >= kSpridxfVersion_Last32bit)
    {
        if (fidx->ReadInt32() != expectedFileID)
        {
            return false;
        }
    }

    sprkey_t topmost_index = fidx->ReadInt32();
    // end index+1 should be the same as num sprites
    if (fidx->ReadInt32() != topmost_index + 1)
    {
        return false;
    }

    if (topmost_index != topmost)
    {
        return false;
    }

    sprkey_t numsprits = topmost_index + 1;
    std::vector<int16_t> spritewidths;
    std::vector<int16_t> spriteheights;
    std::vector<soff_t>  spriteoffs;

    spritewidths.resize(numsprits);
    spriteheights.resize(numsprits);
    fidx->ReadArrayOfInt16(spritewidths.data(), numsprits);
    fidx->ReadArrayOfInt16(spriteheights.data(), numsprits);

    spriteoffs.resize(numsprits);
    if (vers <= kSpridxfVersion_Last32bit)
    {
        for (sprkey_t i = 0; i < numsprits; ++i)
            spriteoffs[i] = fidx->ReadInt32();
    }
    else // large file support
    {
        fidx->ReadArrayOfInt64(spriteoffs.data(), numsprits);
    }

    for (sprkey_t i = 0; i <= topmost_index; ++i)
    {
        if (spriteoffs[i] != 0)
        {
            _spriteData[i].Offset = spriteoffs[i];
            _spriteData[i].HasImage = (spritewidths[i] > 0) && (spriteheights[i] > 0);
            if (metrics)
            {
                (*metrics)[i].Width = spritewidths[i];
                (*metrics)[i].Height = spriteheights[i];
            }
        }
    }
    return true;
}

static inline void ReadSpriteHeader(SpriteDatHeader &hdr, Stream *in, SpriteFileVersion sf_version, SpriteCompression compress)
{
    (sf_version >= kSprfVersion_StorageFormats) ?
        SpriteDataUtils::ReadSpriteHeader_360(hdr, in)
      : SpriteDataUtils::ReadSpriteHeader_321(hdr, in, compress);
}

static inline void SkipSpriteData(SpriteDatHeader &hdr, Stream *in, SpriteFileVersion sf_version, SpriteCompression compress)
{
    (sf_version >= kSprfVersion_StorageFormats) ?
        SpriteDataUtils::SkipSpriteData_360(hdr, in)
      : SpriteDataUtils::SkipSpriteData_321(hdr, in, compress);
}

static inline PixelBuffer ReadSprite(Stream *in, SpriteFileVersion sf_version, SpriteCompression compress, HError &err)
{
    return (sf_version >= kSprfVersion_StorageFormats) ?
        SpriteDataUtils::ReadSprite_360(in, err)
      : SpriteDataUtils::ReadSprite_321(in, compress, err);
}

static inline size_t GetSpriteDataSize(SpriteDatHeader &hdr, Stream *in, SpriteFileVersion sf_version, SpriteCompression compress)
{
    return (sf_version >= kSprfVersion_StorageFormats) ?
        SpriteDataUtils::GetSpriteDataSize_360(hdr, in)
      : SpriteDataUtils::GetSpriteDataSize_321(hdr, in, compress);
}

HError SpriteFile::RebuildSpriteIndex(Stream *in, sprkey_t topmost,
    std::vector<Size> *metrics, std::vector<SpriteDatHeader> *metrics2)
{
    topmost = std::min(topmost, (sprkey_t)_spriteData.size() - 1);
    _validCount = 0;

    for (sprkey_t i = 0; !in->EOS() && (i <= topmost); ++i)
    {
        _spriteData[i].Offset = in->GetPosition();
        SpriteDatHeader hdr;
        ReadSpriteHeader(hdr, _stream.get(), _version, _compress);
        _spriteData[i].HasImage = (hdr.BPP > 0) && (hdr.Width > 0) && (hdr.Height > 0);
        if (hdr.BPP == 0) continue; // empty slot, this is normal
        if (hdr.BPP < 0 || hdr.Width <= 0 || hdr.Height <= 0)
        {
            return new Error("RebuildSpriteIndex: invalid sprite metrics %d (%dx%d %d-bit), cannot deduce pixel data size.",
                i, hdr.Width, hdr.Height, hdr.BPP * 8);
        }
        SkipSpriteData(hdr, _stream.get(), _version, _compress);

        if (metrics)
        {
            (*metrics)[i].Width = hdr.Width;
            (*metrics)[i].Height = hdr.Height;
        }
        if (metrics2)
        {
            (*metrics2)[i] = hdr;
        }
        _validCount++;
    }
    return HError::None();
}

bool SpriteFile::DoesSpriteExist(sprkey_t index)
{
    return (index >= 0) && (static_cast<size_t>(index) < _spriteData.size())
        && (_spriteData[index].HasImage);
}

HError SpriteFile::LoadSprite(sprkey_t index, PixelBuffer &sprite)
{
    sprite = {};
    if (index < 0 || (size_t)index >= _spriteData.size())
        return new Error("LoadSprite: slot index %d out of bounds (%d - %zu).",
            index, 0, _spriteData.size() - 1);

    if (_spriteData[index].Offset == 0)
        return HError::None(); // sprite is not in file

    SeekToSprite(index);
    _curPos = -2; // mark undefined pos

    HError err;
    PixelBuffer image = ReadSprite(_stream.get(), _version, _compress, err);
    if (!err)
    {
        return new Error(err, "LoadSprite: failed to load sprite %d", index);
    }

    sprite = std::move(image);
    _curPos = index + 1; // mark correct pos
    return HError::None();
}

HError SpriteFile::LoadRawData(sprkey_t index, SpriteDatHeader &hdr, std::vector<uint8_t> &data)
{
    hdr = SpriteDatHeader();
    data.resize(0);
    if (index < 0 || (size_t)index >= _spriteData.size())
        return new Error("LoadSprite: slot index %d out of bounds (%d - %zu).",
            index, 0, _spriteData.size() - 1);

    if (_spriteData[index].Offset == 0)
        return HError::None(); // sprite is not in file

    SeekToSprite(index);
    _curPos = -2; // mark undefined pos

    ReadSpriteHeader(hdr, _stream.get(), _version, _compress);
    if (hdr.BPP == 0) return HError::None(); // empty slot, this is normal
    size_t data_size = GetSpriteDataSize(hdr, _stream.get(), _version, _compress);
    // Seek back and read all at once
    data.resize(data_size);
    _stream->Read(data.data(), data_size);

    _curPos = index + 1; // mark correct pos
    return HError::None();
}

HError SpriteFile::LoadSpriteMetrics(std::vector<SpriteDatHeader> &metrics)
{
    metrics.resize(_spriteData.size());
    if (_spriteData.size() == 0)
        return HError::None();

    return RebuildSpriteIndex(_stream.get(), GetTopmostSprite(), nullptr, &metrics);
}

void SpriteFile::SeekToSprite(sprkey_t index)
{
    // If we didn't just load the previous sprite, seek to it
    if (index != _curPos)
    {
        _stream->Seek(_spriteData[index].Offset, kSeekBegin);
        _curPos = index;
    }
}


// Finds the topmost occupied slot index
static sprkey_t FindTopmostSprite(const std::vector<std::pair<bool, BitmapData>> &sprites)
{
    sprkey_t topmost = -1;
    for (sprkey_t i = 0; i < static_cast<sprkey_t>(sprites.size()); ++i)
        if (sprites[i].first)
            topmost = i;
    return topmost;
}

HError SaveSpriteFile(const String& save_to_file,
    const std::vector<std::pair<bool, BitmapData>>& sprites,
    SpriteFile* read_from_file,
    int store_flags, SpriteCompression compress, SpriteFileIndex& index)
{
    std::unique_ptr<Stream> out(File::CreateFile(save_to_file));
    if (out == nullptr)
        return new Error("Failed to open a output file for writing");
    return SaveSpriteFile(std::move(out), sprites, read_from_file, store_flags, compress, index);
}

HError SaveSpriteFile(std::unique_ptr<Stream>&& out,
    const std::vector<std::pair<bool, BitmapData>>& sprites,
    SpriteFile* read_from_file, // optional file to read missing sprites from
    int store_flags, SpriteCompression compress, SpriteFileIndex& index)
{
    sprkey_t lastslot = FindTopmostSprite(sprites);
    SpriteFileWriter writer(std::move(out));
    writer.Begin(store_flags, compress, lastslot);

    PixelBuffer temp_bmp; // for disposing temp sprites
    std::vector<uint8_t> membuf; // for loading raw sprite data

    const bool diff_compress =
        read_from_file &&
        (read_from_file->GetSpriteCompression() != compress ||
        read_from_file->GetStoreFlags() != store_flags);

    for (sprkey_t i = 0; i <= lastslot; ++i)
    {
        if (!sprites[i].first)
        { // empty slot
            writer.WriteEmptySlot();
            continue;
        }

        const BitmapData *image = &sprites[i].second;
        // if compression setting is different, load the sprite into memory
        // (otherwise we will be able to simply copy bytes from one file to another
        if ((!*image) && diff_compress)
        {
            read_from_file->LoadSprite(i, temp_bmp);
            image = &temp_bmp;
        }

        // if managed to load an image - save it according the new compression settings
        if (*image)
        {
            writer.WriteBitmap(*image);
            continue;
        }
        else if (diff_compress)
        {
            // sprite doesn't exist
            writer.WriteEmptySlot();
            continue;
        }

        // Not in memory - and same compression option;
        // Directly copy the sprite bytes from the input file to the output
        SpriteDatHeader hdr;
        read_from_file->LoadRawData(i, hdr, membuf);
        if (hdr.BPP == 0)
        { // empty slot
            writer.WriteEmptySlot();
            continue;
        }
        writer.WriteRawData(hdr, membuf.data(), membuf.size());
    }
    writer.Finalize();

    index = writer.GetIndex();
    return HError::None();
}

HError SaveSpriteIndex(const String &filename, const SpriteFileIndex &index)
{
    // write the sprite index file
    auto out = File::CreateFile(filename);
    if (!out)
        return new Error("Failed to open a output file for writing");
    // write "SPRINDEX" id
    out->WriteArray(spindexid, strlen(spindexid), 1);
    // write version
    out->WriteInt32(kSpridxfVersion_Current);
    out->WriteInt32(index.SpriteFileIDCheck);
    // write last sprite number and num sprites, to verify that
    // it matches the spr file
    out->WriteInt32(index.GetLastSlot());
    out->WriteInt32(index.GetCount());
    if (index.GetCount() > 0)
    {
        out->WriteArrayOfInt16(index.Widths.data(), index.Widths.size());
        out->WriteArrayOfInt16(index.Heights.data(), index.Heights.size());
        out->WriteArrayOfInt64(index.Offsets.data(), index.Offsets.size());
    }
    return HError::None();
}


void SpriteFileWriter::Begin(int store_flags, SpriteCompression compress, sprkey_t last_slot)
{
    if (!_out) return;
    _index.SpriteFileIDCheck = (int)time(nullptr);
    _storeFlags = store_flags;
    _compress = compress;

    // sprite file version
    _out->WriteInt16(kSprfVersion_Current);
    _out->WriteArray(spriteFileSig, strlen(spriteFileSig), 1);
    _out->WriteInt8(_compress);
    _out->WriteInt32(_index.SpriteFileIDCheck);

    // Remember and write provided "last slot" index,
    // but if it's not set (< 0) then we will have to return back later
    // and write correct one; this is done in Finalize().
    _lastSlotPos = _out->GetPosition();
    _out->WriteInt32(last_slot);

    _out->WriteInt8(_storeFlags);
    _out->WriteInt8(0); // reserved
    _out->WriteInt8(0);
    _out->WriteInt8(0);

    if (last_slot >= 0)
    { // allocate buffers to store the indexing info
        sprkey_t numsprits = last_slot + 1;
        _index.Offsets.reserve(numsprits);
        _index.Widths.reserve(numsprits);
        _index.Heights.reserve(numsprits);
    }
}

void SpriteFileWriter::WriteBitmap(const BitmapData &image)
{
    if (!_out) return;
    // Add index entry and write resulting data to the stream
    soff_t sproff = _out->GetPosition();
    _index.Offsets.push_back(sproff);
    _index.Widths.push_back(image.GetWidth());
    _index.Heights.push_back(image.GetHeight());

    SpriteDataUtils::WriteSprite_360(image, _out.get(), _storeFlags, _compress, &_membuf);
}

void SpriteFileWriter::WriteEmptySlot()
{
    if (!_out) return;
    soff_t sproff = _out->GetPosition();
    _index.Offsets.push_back(sproff);
    _index.Widths.push_back(0);
    _index.Heights.push_back(0);

    _out->WriteInt16(0); // 16-bit zero to mark empty slot (bpp + storage format)
}

void SpriteFileWriter::WriteRawData(const SpriteDatHeader &hdr, const uint8_t *data, size_t data_sz)
{
    if (!_out) return;
    soff_t sproff = _out->GetPosition();
    _index.Offsets.push_back(sproff);
    _index.Widths.push_back(hdr.Width);
    _index.Heights.push_back(hdr.Height);

    SpriteDataUtils::WriteRawSpriteData_360(hdr, _out.get(), data, data_sz);
}

void SpriteFileWriter::Finalize()
{
    if (_out)
    {
        if (_lastSlotPos >= 0)
        {
            _out->Seek(_lastSlotPos, kSeekBegin);
            _out->WriteInt32(_index.GetLastSlot());
        }
        _out.reset();
    }
}

} // namespace Common
} // namespace AGS
