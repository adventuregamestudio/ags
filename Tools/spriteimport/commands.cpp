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
#include "commands.h"
#include <allegro.h>
#include <vector>
#include <map>
#include "data/agfreader.h"
#include "data/game_utils.h"
#include "data/sprite_utils.h"
#include "game/room_file.h"
#include "gfx/bitmapdata.h"
#include "gfx/image_file.h"
#include "util/file.h"
#include "util/memory_compat.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;

namespace SpriteImport
{

void Init()
{
    // Init Allegro RGB shifts; necessary for doing color conversions
    set_rgb_shifts(10, 5, 0, 11, 5, 0, 16, 8, 0, 16, 8, 0, 24);
}

// SpriteWriter is a helper class, which either compiles sprites into the
// spritefile pack (using SpriteFileWriter), or writes sprites as individual
// image files (provided with a destination directory and filename pattern).
class SpriteWriter
{
public:
    SpriteWriter(std::unique_ptr<SpriteFileWriter> &&sf_writer, int store_flags, SpriteCompression compress,
            bool use_sequential_index = false)
        : _sfWriter(std::move(sf_writer))
        , _sfStoreFlags(store_flags)
        , _sfCompress(compress)
        , _useSequentialIndex(use_sequential_index)
    {}

    SpriteWriter(const String &out_dir, const String &image_pattern, bool use_sequential_index = false)
        : _outDir(out_dir)
        , _imagePattern(image_pattern)
        , _useSequentialIndex(use_sequential_index)
    {}

    HError Begin(sprkey_t topmost_sprite)
    {
        if (_sfWriter)
        {
            _sfWriter->Begin(_sfStoreFlags, _sfCompress, topmost_sprite);
        }
        return HError::None();
    }
    HError WriteSprite(const BitmapData &image, int slot)
    {
        if (_useSequentialIndex)
            slot = ++_lastSlot;
        _lastSlot = slot;

        if (_sfWriter)
        {
            // Spritefile must have all the gaps filled with "empty slot" entries
            for (sprkey_t last_slot = _sfWriter->GetLastWrittenSlot() + 1; last_slot < slot; ++last_slot)
                _sfWriter->WriteEmptySlot();

            if (image)
                _sfWriter->WriteBitmap(image);
            else
                _sfWriter->WriteEmptySlot();
            return HError::None();
        }
        else if (image)
        {
            String filename = String::FromFormat(_imagePattern.GetCStr(), slot);
            if (!ImageFile::SaveImage(image, Path::ConcatPaths(_outDir, filename)))
            {
                return new Error(String::FromFormat("Failed to save sprite %d as the image file '%s'", slot, filename.GetCStr()));
            }
        }
        return HError::None();
    }
    HError End()
    {
        if (_sfWriter)
        {
            _sfWriter->Finalize();
            _sfWriter = nullptr;
        }
        return HError::None();
    }

private:
    std::unique_ptr<SpriteFileWriter> _sfWriter;
    int _sfStoreFlags = 0;
    SpriteCompression _sfCompress = kSprCompress_None;
    String _outDir;
    String _imagePattern;
    bool _useSequentialIndex = false;
    int _lastSlot = -1;
};

HError GatherSpriteSpecsFromAgf(const String &src_agf, std::vector<SpriteData> &sprites, GameColorSettings &game_color_opts,
    std::vector<std::pair<int, String>> &room_list, bool verbose)
{
    AGF::AGFReader reader;
    HError err = reader.Open(src_agf.GetCStr());
    if (!err)
        return new Error(String::FromFormat("Failed to open source AGF '%s':\n", src_agf.GetCStr()), err);
    GameSettings opt;
    AGF::ReadGameSettings(opt, reader.GetGameRoot());
    game_color_opts.ColorDepth = opt.ColorDepth;
    AGF::ReadSpriteList(sprites, reader.GetGameRoot());
    std::vector<PaletteEntryData> pal_entries(256);
    AGF::ReadGamePalette(pal_entries, reader.GetGameRoot());
    for (size_t i = 0; i < pal_entries.size(); ++i)
    {
        game_color_opts.Palette[i].r = pal_entries[i].Red;
        game_color_opts.Palette[i].g = pal_entries[i].Green;
        game_color_opts.Palette[i].b = pal_entries[i].Blue;
        game_color_opts.Palette[i].a = 255;
        game_color_opts.PalUses[i] = pal_entries[i].ColourType;
    }
    AGF::ReadRoomList(room_list, reader.GetGameRoot());
    return HError::None();
}

void MapSpritesToSources(const String &src_dir, const std::vector<SpriteData> &sprites, std::multimap<String, SpriteData> &source_to_sprite)
{
    for (const auto &sprite : sprites)
    {
        if (!sprite.SourceFile.IsEmpty())
        {
            String sprite_path = Path::IsRelativePath(sprite.SourceFile) ?
                Path::ConcatPaths(src_dir, sprite.SourceFile) : sprite.SourceFile;
            source_to_sprite.insert(std::make_pair(sprite_path, sprite));
        }
        else
        {
            printf("Warning: sprite %d does not have a source data: won't be able to import\n", sprite.Slot);
        }
    }
}

void CacheRoomPalettes(RoomPaletteCache &room_cache, const String &room_dir,
    const std::vector<std::pair<int, String>> &room_list, const std::vector<SpriteData> &sprites, bool verbose)
{
    for (const auto &room_ref : room_list)
    {
        const String filepath = Path::ConcatPaths(room_dir, String::FromFormat("room%d.crm", room_ref.first));
        RoomDataSource src;
        HRoomFileError err = OpenRoomFile(filepath, src);
        if (!err)
        {
            printf("Error: failed to open room file '%s' for reading:\n", filepath.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
            continue;
        }
        
        // For the room palette we must read its primary background image
        // found in the kRoomFblk_Main
        RoomData room;
        err = ReadRoomData(&room, nullptr, std::move(src.InputStream), src.DataVersion, {{ kRoomFblk_Main, "" }}, {});
        if (!err)
        {
            printf("Error: failed to read room file '%s':\n", filepath.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
            continue;
        }

        std::unique_ptr<Palette> pal = std::make_unique<Palette>();
        std::copy(room.Palette, room.Palette + PAL_SIZE, pal->data());
        room_cache[room_ref.first] = std::move(pal);
        if (verbose)
            printf("Loaded palette for room %d\n", room_ref.first);
    }
}

HError CutSpritesAndWrite(const String &src_file, const std::vector<SpriteData> &sprites, const GameColorSettings &game_color_opts,
    const RoomPaletteCache &room_cache, SpriteWriter &writer, bool verbose)
{
    PixelFormat px_fmt;
    Palette pal;
    // TODO: implement loading distinct frame(s) from formats such as GIF
    PixelBuffer image = ImageFile::LoadImage(src_file, &px_fmt, pal.data());
    if (!image)
    {
        // In case we output to the spritefile, we must fill all gaps with empty slot entries
        for (const auto &sprite : sprites)
        {
            writer.WriteSprite({}, sprite.Slot);
        }
        return new Error(String::FromFormat("Failed to load image file %s", src_file.GetCStr()));
    }

    for (const auto &sprite : sprites)
    {
        PixelBuffer tile = PixelOp::CopyPixelsRegion(image, sprite.ImportOffsetX, sprite.ImportOffsetY, sprite.ImportWidth, sprite.ImportHeight);
        if (tile)
        {
            PixelBuffer conv_tile;
            // NOTE: GameColorDepth's values match byte per pixel of a respective color depth type
            HError err = ConvertSpriteForGame(tile, &pal, conv_tile, game_color_opts, &room_cache, sprite);
            if (err)
            {
                writer.WriteSprite(conv_tile, sprite.Slot);
            }
            else
            {
                printf("Error: failed to convert the sprite %d for the game:\n", sprite.Slot);
                printf("%s\n", err->FullMessage().GetCStr());
            }
        }
    }
    return HError::None();
}

HError ImportSpritesImpl(const std::multimap<String, SpriteData> &source_to_sprite, const GameColorSettings &game_color_opts,
    const RoomPaletteCache &room_cache, SpriteWriter &writer, std::map<sprkey_t, sprkey_t> *out_sprite_order, bool verbose)
{
    writer.Begin(source_to_sprite.size() > 0 ? source_to_sprite.size() - 1 : -1);
    // Multimap stores items ordered by keys, meaning the duplicates will be together.
    // So lookup for each next key group, collect all sprite indexes from that group,
    // and pass them into the import as a single collection.
    std::vector<SpriteData> sprite_group;
    sprkey_t out_index = 0;
    for (auto it_spr_src = source_to_sprite.begin(); it_spr_src != source_to_sprite.end();)
    {
        sprite_group.clear();
        const String &this_source = it_spr_src->first;
        for (; (it_spr_src != source_to_sprite.end()) && (this_source == it_spr_src->first); ++it_spr_src)
            sprite_group.push_back(it_spr_src->second);

        if (out_sprite_order)
        {
            for (const auto s : sprite_group)
                (*out_sprite_order)[s.Slot] = out_index++;
        }

        HError err = CutSpritesAndWrite(this_source, sprite_group, game_color_opts, room_cache, writer, verbose);
        if (!err)
        {
            printf("Error: failed to import sprite(s) from source file '%s':\n", this_source.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
        }
    }
    writer.End();
    return HError::None();
}

HError ImportToSpritePak(const std::multimap<String, SpriteData> &source_to_sprite, const GameColorSettings &game_color_opts,
    const RoomPaletteCache &room_cache, const String &dst_path, const CommandOptions &opts, bool verbose)
{
    // The strategy: because we sort sprites by sources, we might end up having them
    // in the non-sequential order. While spritefile is supposed to have them strictly
    // ordered. As a workaround we do a two-step-operation:
    // Step 1. Write the spritefile into the temporary file, while recording the
    // order in which the sprites are written there.
    // Step 2. Write another spritefile into the final file, copying raw chunks
    // from the temporary file over (simply copy, faster than import itself).
    std::unique_ptr<Stream> temp_s = File::CreateTempFile();
    if (!temp_s)
        return new Error("Failed to open temporary spritefile for writing");

    // As we open a temporary file, it gets deleted as soon as the stream closes.
    // SpriteFileWriter closes its stream as soon as the pack is finalized.
    // In order to prevent a temp file closing, here we make a "proxy stream",
    // which will use the base but not own one, and thus won't close.
    // TODO: this is a quick and clumsy solution for now, revise this later
    auto proxy_out = std::make_unique<Stream>(std::make_unique<StreamSection>(temp_s->GetStreamBase(), 0, temp_s->GetStreamBase()->GetLength()));
    std::unique_ptr<SpriteFileWriter> sf_writer(new SpriteFileWriter(std::move(proxy_out)));
    SpriteWriter writer(std::move(sf_writer), opts.StorageFlags, opts.Compress, true);
    std::map<sprkey_t, sprkey_t> out_sprite_order;
    HError err = ImportSpritesImpl(source_to_sprite, game_color_opts, room_cache, writer, &out_sprite_order, verbose);
    if (!err)
        return err;

    std::unique_ptr<Stream> out = File::CreateFile(dst_path);
    if (!out)
        return new Error(String::FromFormat("Failed to open destination spritefile for writing: %s", dst_path.GetCStr()));

    SpriteFile spr_reader;
    temp_s->Seek(0, kSeekBegin);
    err = spr_reader.OpenFile(std::move(temp_s), {});
    if (!err)
        return err;

    SpriteFileWriter spr_writer(std::move(out));
    SpriteDatHeader hdr;
    std::vector<uint8_t> data;
    spr_writer.Begin(opts.StorageFlags, opts.Compress, spr_reader.GetTopmostSprite());
    sprkey_t last_slot = -1;
    for (const auto &temp_slot : out_sprite_order)
    {
        // Spritefile must have all the gaps filled with "empty slot" entries
        while (++last_slot < temp_slot.first)
            spr_writer.WriteEmptySlot();
        HError err = spr_reader.LoadRawData(temp_slot.second, hdr, data);
        if (err)
            spr_writer.WriteRawData(hdr, data);
        else
            spr_writer.WriteEmptySlot();
    }
    spr_writer.Finalize();

    // Consider index file to be non-obligatory, as the index may be restored from the spritefile
    if (!opts.IndexFile.IsEmpty())
    {
        HError err = SaveSpriteIndex(opts.IndexFile, spr_writer.GetIndex());
        if (err)
        {
            printf("Index file written successfully.\n");
        }
        else
        {
            printf("Error: failed to write index file (%s):\n", opts.IndexFile.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
        }
    }
    return HError::None();
}

HError ImportToDirectory(const std::multimap<String, SpriteData> &source_to_sprite, const GameColorSettings &game_color_opts,
    const RoomPaletteCache &room_cache, const String &dst_path, const CommandOptions &opts, bool verbose)
{
    if (!File::IsDirectory(dst_path))
        return new Error(String::FromFormat("Not a valid directory: %s", dst_path.GetCStr()));

    String image_pattern;
    if (!ResolveImageFilePattern(opts.ImageFilePattern, image_pattern))
        return new Error(String::FromFormat("Image file pattern \"%s\" is not a valid pattern.\n", opts.ImageFilePattern.GetCStr()));
    SpriteWriter writer(dst_path, image_pattern);
    return ImportSpritesImpl(source_to_sprite, game_color_opts, room_cache, writer, nullptr, verbose);
}

int Command_Import(const String &src_agf, const String &dst_path, const CommandOptions &opts, bool verbose)
{
    std::vector<SpriteData> sprites;
    GameColorSettings game_color_opts;
    std::vector<std::pair<int, String>> room_list;
    HError err = GatherSpriteSpecsFromAgf(src_agf, sprites, game_color_opts, room_list, verbose);
    if (!err)
    {
        printf("Error: failed to gather sprite specs from '%s':\n", src_agf.GetCStr());
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    // CHECKME: do we need to provide a separate cmd arg for this?
    const String src_dir = Path::GetParent(src_agf);

    std::multimap<String, SpriteData> source_to_sprite;
    MapSpritesToSources(src_dir, sprites, source_to_sprite);

    RoomPaletteCache room_cache;
    if (game_color_opts.ColorDepth == kGameColorDepth_Palette)
    {
        String room_dir = !opts.RoomDirectory.IsEmpty() ? opts.RoomDirectory : Path::GetParent(src_agf);
        CacheRoomPalettes(room_cache, room_dir, room_list, sprites, verbose);
        if (room_cache.size() < room_list.size())
        {
            printf("Warning: failed to precache all the room palettes needed for this 8-bit game. "
                "The imported 8-bit sprites may have incorrect colors if they were refering room background palette slots.\n");
        }
    }

    if (opts.OutputToSpritePak)
        err = ImportToSpritePak(source_to_sprite, game_color_opts, room_cache, dst_path, opts, verbose);
    else
        err = ImportToDirectory(source_to_sprite, game_color_opts, room_cache, dst_path, opts, verbose);
    if (!err)
    {
        printf("Error: failed to write imported sprites to their destination:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    printf("Done.\n");
    return 0;
}

} // namespace SpriteImport
