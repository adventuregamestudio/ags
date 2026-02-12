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
#include "ac/spritefile.h"
#include "gfx/image_file.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace SpritePak
{

static const String DefaultPattern = "spr%06d";
static const String DefaultRegexPattern = "spr\\d{6}";
static const String DefaultExtension = "bmp"; // TODO: replace with PNG when engine code has support
static const CstrArr<kNumSprCompressTypes> CompressionNames = {{"none", "rle", "lzw", "deflate"}};

void Init()
{
    // Init Allegro RGB shifts; necessary for doing color conversions
    set_rgb_shifts(10, 5, 0, 11, 5, 0, 16, 8, 0, 16, 8, 0, 24);
}

String GetCompressionName(SpriteCompression compress)
{
    return String::Wrapper(StrUtil::SelectCStr<kNumSprCompressTypes>(
        CompressionNames, compress, "unknown"));
}

SpriteCompression CompressionFromName(const String &compress_name)
{
    return StrUtil::ParseEnum<SpriteCompression, kNumSprCompressTypes>(
        compress_name, CompressionNames, kSprCompress_None);
}

static void ResolveImageFilePattern(const String &pattern, String &res_pattern, String &regex_pattern)
{
    if (pattern.IsNullOrSpace())
    {
        res_pattern = String::FromFormat("%s.%s", DefaultPattern.GetCStr(), DefaultExtension.GetCStr());
        regex_pattern = String::FromFormat("%s.%s", DefaultRegexPattern.GetCStr(), DefaultExtension.GetCStr());
    }

    res_pattern = "";
    regex_pattern = "";
    const auto parts = pattern.Split('%');
    // Each second part is assumed a placeholder between two '%'
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i % 2 == 0)
        {
            res_pattern.Append(parts[i]);
            regex_pattern.Append(parts[i]);
        }
        else if (i < parts.size() - 1)
        {
            // Resolve placeholder
            if (parts[i] == "N" || parts[i] == "n")
            {
                res_pattern.Append("%06d");
                regex_pattern.Append("\\d{6}");
            }
            else
            {
                res_pattern.Append("_");
                regex_pattern.Append("_");
            }
        }
    }

    if (Path::GetFileExtension(res_pattern).IsEmpty())
    {
        res_pattern = String::FromFormat("%s.%s", res_pattern.GetCStr(), DefaultExtension.GetCStr());
        regex_pattern = String::FromFormat("%s.%s", regex_pattern.GetCStr(), DefaultExtension.GetCStr());
    }
}

static String ResolveImageFilePattern(const String &pattern)
{
    String res_pattern, regex_pattern;
    ResolveImageFilePattern(pattern, res_pattern, regex_pattern);
    return res_pattern;
}

static HError MakeListOfFiles(std::vector<String> &files, const String &asset_dir, const std::regex &regex)
{
    for (FindFile ff = FindFile::Open(asset_dir, regex, true, false, 0);
         !ff.AtEnd(); ff.Next())
    {
        String filename = ff.Current();
        files.push_back(filename);
    }
    return HError::None();
}

static HError OpenSpriteFile(SpriteFile &reader, const String &sprite_file, const String &index_file,
    std::vector<SpriteDatHeader> *metrics = nullptr)
{
    std::unique_ptr<Stream> file_in = File::OpenFileRead(sprite_file);
    if (!file_in)
        return new Error(String::FromFormat("Failed to open spritefile for reading: %s", sprite_file.GetCStr()));
    std::unique_ptr<Stream> index_in;
    if (!index_file.IsEmpty())
    {
        index_in = File::OpenFileRead(index_file);
        if (!index_in)
        {
            printf("Error: failed to open index for reading: %s\n", index_file.GetCStr());
        }
    }

    printf("Parsing the sprite file...\n");
    HError err;
    if (metrics)
        err = reader.OpenFile(std::move(file_in), std::move(index_in), *metrics);
    else
        err = reader.OpenFile(std::move(file_in), std::move(index_in));
    if (!err)
        return new Error("Failed to initialize sprite file", err);
    return 0;
}

static void SaveIndexFile(const String &index_file, const SpriteFileIndex &index)
{
    if (!index_file.IsEmpty())
    {
        printf("Writing the new index file...\n");
        HError err = SaveSpriteIndex(index_file, index);
        if (err)
        {
            printf("Index file written successfully.\n");
        }
        else
        {
            printf("Error: failed to write index file (%s):\n", index_file.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
        }
    }
}

static void PrintQuickInfo(const SpriteFile &reader)
{
    printf("Sprite file info:\n\tcompression: %s\n\tstorage flags: 0x%08x\n\ttotal sprites: %zu\n\thighest sprite: %d\n",
           GetCompressionName(reader.GetSpriteCompression()).GetCStr(),
           reader.GetStoreFlags(),
           reader.GetSpriteCount(),
           reader.GetTopmostSprite());
}

int Command_Create(const String &src_dir, const String &dst_pak, const CommandOptions &opts, bool verbose)
{
    printf("Input directory: %s\n", src_dir.GetCStr());
    printf("Output spritefile: %s\n", dst_pak.GetCStr());
    if (!opts.IndexFile.IsEmpty())
        printf("Output index file: %s\n", opts.IndexFile.GetCStr());

    if (!File::IsDirectory(src_dir))
    {
        printf("Error: not a valid input directory.\n");
        return -1;
    }

    String image_pattern, regex_pattern;
    ResolveImageFilePattern(opts.ImageFilePattern, image_pattern, regex_pattern);
    printf("Image file pattern: %s\n", image_pattern.GetCStr());
    printf("Sprite file compression: %s\n", GetCompressionName(opts.Compress).GetCStr());
    printf("Sprite file storage flags: 0x%08x\n", opts.StorageFlags);

    //-----------------------------------------------------------------------//
    // Gather image file list
    //-----------------------------------------------------------------------//
    std::vector<String> files;
    printf("Scanning the directory...\n");
    HError err = MakeListOfFiles(files, src_dir, std::regex(regex_pattern.GetCStr(), std::regex_constants::icase));
    if (!err)
    {
        printf("Error: failed to gather list of image files:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    if (files.size() == 0)
    {
        printf("Warning: no pattern-matching files found in the provided directory.\n");
    }
    else
    {
        printf("Found: %zu matching input files.\n", files.size());
    }

    sprkey_t top_index = -1;
    typedef std::pair<sprkey_t, String> SlotImageFilePair;
    std::vector<SlotImageFilePair> sorted_files;
    for (const auto &imf : files)
    {
        sprkey_t slot;
        if ((sscanf(imf.GetCStr(), image_pattern.GetCStr(), &slot) == 1)
            && (slot >= 0) && (slot < INT32_MAX))
        {
            sorted_files.push_back(std::make_pair(slot, imf));
            top_index = std::max(top_index, slot);
        }
    }
    std::sort(sorted_files.begin(), sorted_files.end(),
        [](const SlotImageFilePair &f1, const SlotImageFilePair &f2) { return f1.first < f2.first; });

    //-----------------------------------------------------------------------//
    // Write sprite file
    //-----------------------------------------------------------------------//
    auto out = File::OpenFileWrite(dst_pak);
    if (!out)
    {
        printf("Error: failed to open sprite file for writing: %s\n", dst_pak.GetCStr());
        return -1;
    }
    printf("Writing the new sprite file...\n");
    size_t import_count = 0u;
    SpriteFileWriter writer(std::move(out));
    // TODO: move this process to a separate code module;
    // there's a lot more to this, as the process of sprite file generation may
    // need to include extra image processing, such as transparent color selection,
    // tile cropping, etc, which options we may need to read from some kind of a
    // "sprite definition" file (or Game.agf).
    writer.Begin(opts.StorageFlags, opts.Compress, top_index);
    for (const auto &imf : sorted_files)
    {
        auto im_in = File::OpenFileRead(Path::ConcatPaths(src_dir, imf.second));
        if (!im_in)
        {
            writer.WriteEmptySlot();
            printf("Error: failed to open image file for input: %s\n", imf.second.GetCStr());
            continue;
        }

        PixelBuffer pxbuf = ImageFile::LoadImage(im_in.get(), Path::GetFileExtension(imf.second));
        if (!pxbuf)
        {
            writer.WriteEmptySlot();
            printf("Error: failed to load an image from file: %s\n", imf.second.GetCStr());
            continue;
        }

        writer.WriteBitmap(pxbuf);
        import_count++;
        if (verbose)
            printf("+ [%06d] - '%s'\n", imf.first, imf.second.GetCStr());
    }
    writer.Finalize();
    printf("Sprite file written successfully.\n");
    printf("Imported %zu sprites.\n", import_count);

    SaveIndexFile(opts.IndexFile, writer.GetIndex());
    printf("Done.\n");
    return 0;
}

int Command_Export(const String &src_pak, const String &dst_dir, const CommandOptions &opts, bool verbose)
{
    printf("Input spritefile: %s\n", src_pak.GetCStr());
    if (!opts.IndexFile.IsEmpty())
        printf("Input index file: %s\n", opts.IndexFile.GetCStr());
    printf("Output directory: %s\n", dst_dir.GetCStr());

    if (!File::IsDirectory(dst_dir))
    {
        printf("Error: not a valid output directory.\n");
        return -1;
    }

    String image_pattern = ResolveImageFilePattern(opts.ImageFilePattern);
    printf("Image file pattern: %s\n", image_pattern.GetCStr());

    //-----------------------------------------------------------------------//
    // Read the sprite file
    //-----------------------------------------------------------------------//
    SpriteFile reader;
    HError err = OpenSpriteFile(reader, src_pak, opts.IndexFile);
    if (!err)
    {
        printf("Error: failed to open and/or parse the sprite file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    if (reader.GetSpriteCount() == 0)
    {
        printf("Spritefile has no sprites.\nDone.\n");
        return 0;
    }

    //-----------------------------------------------------------------------//
    // Extract sprites
    //-----------------------------------------------------------------------//
    printf("Exporting sprites...\n");
    const sprkey_t top_sprite = reader.GetTopmostSprite();
    const String file_ext = Path::GetFileExtension(image_pattern);
    size_t export_count = 0;
    for (sprkey_t i = 0; i <= top_sprite; ++i)
    {
        if (reader.DoesSpriteExist(i))
        {
            PixelBuffer pxbuf;
            HError err = reader.LoadSprite(i, pxbuf);
            if (!err)
            {
                printf("Error: failed to unpack sprite %d\n", i);
                continue;
            }
            String image_file = Path::ConcatPaths(dst_dir, String::FromFormat(image_pattern.GetCStr(), i));
            auto out = File::OpenFileWrite(image_file);
            if (!out)
            {
                printf("Error: failed to open image file for writing: %s\n", image_file.GetCStr());
                continue;
            }
            if (!ImageFile::SaveImage(pxbuf, nullptr, out.get(), file_ext))
            {
                printf("Error: failed to save image file: %s\n", image_file.GetCStr());
                continue;
            }
            export_count++;
            if (verbose)
                printf("- [%06d] - '%s'\n", i, image_file.GetCStr());
        }
    }
    printf("Exported %zu sprites.\n", export_count);
    printf("Done.\n");
    return 0;
}

int Command_Info(const String &src_pak, const CommandOptions &opts)
{
    printf("Input spritefile: %s\n", src_pak.GetCStr());
    if (!opts.IndexFile.IsEmpty())
        printf("Input index file: %s\n", opts.IndexFile.GetCStr());

    //-----------------------------------------------------------------------//
    // Read and print the sprite file info
    //-----------------------------------------------------------------------//
    SpriteFile reader;
    HError err = OpenSpriteFile(reader, src_pak, opts.IndexFile);
    if (!err)
    {
        printf("Error: failed to open and/or parse the sprite file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    PrintQuickInfo(reader);
    printf("Done.\n");
    return 0;
}

int Command_List(const String &src_pak, const CommandOptions &opts)
{
    printf("Input spritefile: %s\n", src_pak.GetCStr());
    if (!opts.IndexFile.IsEmpty())
        printf("Input index file: %s\n", opts.IndexFile.GetCStr());

    //-----------------------------------------------------------------------//
    // Read and print the sprite file TOC
    //-----------------------------------------------------------------------//
    SpriteFile reader;
    std::vector<SpriteDatHeader> metrics;
    HError err = OpenSpriteFile(reader, src_pak, opts.IndexFile, &metrics);
    if (!err)
    {
        printf("Error: failed to open and/or parse the sprite file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    PrintQuickInfo(reader);
    if (reader.GetSpriteCount() == 0)
    {
        printf("Sprite file has no sprites.\nDone.\n");
        return 0;
    }

    printf("Sprite file TOC:\n");
    const sprkey_t top_sprite = reader.GetTopmostSprite();
    for (sprkey_t i = 0; i <= top_sprite; ++i)
    {
        const auto &hdr = metrics[i];
        if (hdr.BPP == 0)
            continue;

        printf("* [%06d]: %dx%d %d-bit\n", i, hdr.Width, hdr.Height, hdr.BPP * 8);
    }
    printf("Done.\n");
    return 0;
}

int Command_Rewrite(const String &src_pak, const String &dst_pak, const CommandOptions &opts, bool verbose)
{
    printf("Input spritefile: %s\n", src_pak.GetCStr());
    if (!opts.IndexFile.IsEmpty())
        printf("Input index file: %s\n", opts.IndexFile.GetCStr());
    printf("Output spritefile: %s\n", dst_pak.GetCStr());
    printf("Output file compression: %s\n", GetCompressionName(opts.Compress).GetCStr());
    printf("Output file storage flags: 0x%08x\n", opts.StorageFlags);

    //-----------------------------------------------------------------------//
    // Read the sprite file
    //-----------------------------------------------------------------------//
    SpriteFile reader;
    HError err = OpenSpriteFile(reader, src_pak, opts.IndexFile);
    if (!err)
    {
        printf("Error: failed to open and/or parse the sprite file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write the destination sprite file
    //-----------------------------------------------------------------------//
    // TODO: the SaveSpriteFile variant that does not require sprite array at all,
    // and only works with SpriteFile reader directly!
    std::vector<std::pair<bool, BitmapData>> dummy_sprites;
    const sprkey_t top_sprite = reader.GetTopmostSprite();
    dummy_sprites.resize(top_sprite + 1);
    for (sprkey_t i = 0; i <= top_sprite; ++i)
    {
        dummy_sprites[i].first = reader.DoesSpriteExist(i);
    }

    SpriteFileIndex index;
    printf("Writing the new sprite file...\n");
    err = SaveSpriteFile(dst_pak, dummy_sprites, &reader, opts.StorageFlags, opts.Compress, index);
    if (err)
    {
        printf("Sprite file written successfully.\n");
    }
    else
    {
        printf("Error: failed to write sprite file (%s):\n", dst_pak.GetCStr());
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    SaveIndexFile(opts.OutIndexFile, index);
    printf("Done.\n");
    return 0;
}

} // namespace SpritePak
