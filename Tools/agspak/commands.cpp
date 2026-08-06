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
#include <cinttypes>
#include "commands.h"
#include "data/mfl_utils.h"
#include "data/include_utils.h"
#include "util/file.h"
#include "util/directory.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace AGSPak
{

// Reads asset library TOC from the given file.
static HError OpenAssetLib(const String &pak_file, AssetLibInfo &lib)
{
    auto in = File::OpenFileRead(pak_file);
    if (!in)
        return new Error("Failed to open pack file for reading.");

    MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(lib, in.get());
    if (mfl_err != MFLUtil::kMFLNoError)
        return new Error("Failed to parse pack file.", MFLUtil::GetMFLErrorText(mfl_err).GetCStr());
    return HError::None();
}

static HError CopyAssetLibraryIntoFile(const String &pak_file, const String &dst_file, bool append, bool verbose)
{
    std::unique_ptr<Stream> pak_in = File::OpenFileRead(pak_file);
    if (!pak_in)
        return new Error("Failed to open pack file for reading.");
    MFLUtil::MFLVersion lib_version = MFLUtil::kMFLVersion_Undefined;
    soff_t in_lib_offset = 0;
    MFLUtil::MFLError mfl_err = MFLUtil::ReadOffsetAndVersion(pak_in.get(), lib_version, in_lib_offset);
    if (mfl_err == MFLUtil::kMFLErrNoLibSig)
    {
        printf("No asset pack attachement found.\n");
        return HError::None(); // no library, nothing to do
    }
    else if (mfl_err != MFLUtil::kMFLNoError)
    {
        return new Error(MFLUtil::GetMFLErrorText(mfl_err));
    }
    const bool has_ender = MFLUtil::HasEnderSig(pak_in.get());

    std::unique_ptr<Stream> pak_out = append ? File::OpenFileWrite(dst_file) : File::CreateFile(dst_file);
    if (!pak_in)
        return new Error("Failed to open output file for writing.");
    pak_in->Seek(in_lib_offset, kSeekBegin);
    pak_out->Seek(0, kSeekEnd);
    const soff_t out_lib_offset = pak_out->GetPosition();
    const soff_t copy_count = CopyStream(pak_in.get(), pak_out.get());
    if (verbose)
    {
        append ?
            printf("Info: appended asset pack attachement:\n\t%" PRId64 " bytes copied over.\n", copy_count) :
            printf("Info: written new asset pack:\n\t%" PRId64 " bytes copied over.\n", copy_count);
    }
    if (has_ender)
        MFLUtil::OverwriteEnder(out_lib_offset, lib_version, pak_out.get());
    else
        MFLUtil::WriteEnder(out_lib_offset, lib_version, pak_out.get());
    return HError::None(); // no library, nothing to do
}

// Checks the presence of a asset library in the given file and cuts it out.
// If there's no library found in the file, the does no changes.
// If library is appended to the file, then cuts it off.
// If the whole file is a library, then it becomes zero size.
static HError CutAssetLibrary(const String &pak_file, bool verbose)
{
    if (!File::IsFile(pak_file))
        return HError::None();

    soff_t lib_offset = 0;
    {
        auto in = File::OpenFileRead(pak_file);
        if (!in)
            return new Error("Failed to open pack file for reading.");

        MFLUtil::MFLError mfl_err = MFLUtil::ReadOffset(in.get(), lib_offset);
        if (mfl_err == MFLUtil::kMFLErrNoLibSig)
        {
            printf("No asset pack attachement found.\n");
            return HError::None(); // no library, nothing to do
        }
        // NOTE: we don't really care about supported TOC version here, as we only
        // require valid signature and base offset.
    }

    const soff_t old_size = File::GetFileSize(pak_file);
    if (!File::TruncateFile(pak_file, lib_offset))
        return new Error("Failed to cut existing asset pack attachement.");
    const soff_t new_size = File::GetFileSize(pak_file);
    if (verbose)
        printf("Info: cut existing asset pack attachement:\n\ttruncated from %" PRId64 " to %" PRId64 ", %" PRId64 " bytes cut.\n", old_size, new_size, old_size - new_size);
    return HError::None();
}

// Makes the list of assets from a single input directory, applying pattern list and/or rules from pattern file.
// Fills a map of asset-name -> filepath pairs.
static HError MakeAssetMapFromADir(const String &asset_dir, StringIMap &asset_map,
    const std::vector<String> &pattern_list, const String &pattern_file, bool do_subdirs, bool verbose)
{
    const bool has_pattern_file = !pattern_file.IsEmpty();
    std::vector<String> files;
    HError err = MakeListOfFiles(files, asset_dir, do_subdirs);
    if (!err)
    {
        printf("Error: failed to gather list of files:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return err;
    }

    // Apply the explicit file list, if provided
    if (!pattern_list.empty())
    {
        std::vector<String> output_files;
        err = MatchPatternPaths(files, output_files, pattern_list);
        if (!err)
        {
            printf("Error: failed to filter files:\n");
            printf("%s\n", err->FullMessage().GetCStr());
            return err;
        }
        files = std::move(output_files);
    }

    // Apply the include/exclude pattern file as a filter
    if (has_pattern_file)
    {
        std::vector<String> output_files;
        err = IncludeFiles(files, output_files, Path::ConcatPaths(asset_dir, pattern_file), verbose);
        if (!err)
        {
            printf("Error: failed to processes %s file:\n", pattern_file.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
            return err;
        }

        files = std::move(output_files);
    }

    MakeAssetMapFromFileList(asset_map, files, asset_dir);
    return HError::None();
}

// Configure the full asset library table of contents, by searching in all
// the provided input directories, and picking files according to the pattern
// list and/or pattern file.
// Fills a map of asset-name -> filepath pairs.
static HError PrepareAssetLibrary(AssetLibInfo &lib,
    const std::vector<std::pair<String, bool>> &src_dirs, const String &dst_pak,
    StringIMap &asset_map,
    const std::vector<String> &pattern_list, const String &pattern_file,
    DuplicateAssetAction dup_action,
    size_t part_size_mb, bool verbose)
{
    const String &lib_basefile = dst_pak;
    std::vector<AssetInfo> assets;
    HError err;
    for (const auto &src_dir : src_dirs)
    {
        StringIMap local_asset_map;
        err = MakeAssetMapFromADir(src_dir.first, local_asset_map, pattern_list, pattern_file, src_dir.second, verbose);
        if (!err)
            return err;

        // Handle asset duplicates
        std::vector<String> remove_keys;
        for (const auto &asset_entry : local_asset_map)
        {
            const auto it_exist = asset_map.find(asset_entry.first);
            if (it_exist != asset_map.end())
            {
                switch (dup_action)
                {
                case kAssetDup_Replace:
                    printf("Warning: duplicate asset '%s' [replace]: first found: '%s', new found: '%s'\n", asset_entry.first.GetCStr(),
                        it_exist->second.GetCStr(), asset_entry.second.GetCStr());
                    break;
                case kAssetDup_Skip:
                    printf("Warning: duplicate asset '%s' [skip]: first found: '%s', new found: '%s'\n", asset_entry.first.GetCStr(),
                        it_exist->second.GetCStr(), asset_entry.second.GetCStr());
                    remove_keys.push_back(asset_entry.first);
                    break;
                default:
                    return new Error(String::FromFormat("Duplicate asset '%s': first found: '%s', new found: '%s'\n", asset_entry.first.GetCStr(),
                        it_exist->second.GetCStr(), asset_entry.second.GetCStr()));
                }
            }
        }

        for (const auto &rem_key : remove_keys)
            local_asset_map.erase(rem_key);

        for (const auto &new_item : local_asset_map)
            asset_map[new_item.first] = new_item.second;
    }

    if (asset_map.size() == 0)
    {
        return new Error("No valid assets found in any of the provided input location(s)");
    }

    soff_t part_size_b = part_size_mb * 1024 * 1024; // MB to bytes
    err = MakeAssetLib(lib, lib_basefile, asset_map, part_size_b);
    if (!err)
        return err;

    return HError::None();
}

int Command_Attach(const String &src_pak, const String &dst_file, bool verbose)
{
    printf("Input pack file: %s\n", src_pak.GetCStr());
    printf("Output pack file: %s\n", dst_file.GetCStr());

    //-----------------------------------------------------------------------//
    // Cut assets from the destination file (in case they are present).
    //-----------------------------------------------------------------------//
    HError err = CutAssetLibrary(dst_file, verbose);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Append new assets to the destination file now.
    //-----------------------------------------------------------------------//
    err = CopyAssetLibraryIntoFile(src_pak, dst_file, true /* append */, verbose);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }

    printf("Done.\n");
    return 0;
}

int Command_CutAttachment(const String &src_pak, bool verbose)
{
    printf("Input pack file: %s\n", src_pak.GetCStr());

    HError err = CutAssetLibrary(src_pak, verbose);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }

    printf("Done.\n");
    return 0;
}

int Command_Create(std::vector<std::pair<String, bool>> &src_dirs, const String &dst_pak, bool append,
                   const std::vector<String> &pattern_list, const String &pattern_file, DuplicateAssetAction dup_action,
                   size_t part_size_mb, bool verbose)
{
    if (src_dirs.size() == 1)
    {
        printf("Input directory: %s\n", src_dirs[0].first.GetCStr());
    }
    else if (src_dirs.size() > 1)
    {
        printf("Input directories: %s%s\n", src_dirs[0].first.GetCStr(), src_dirs[0].second ? " [recursive]" : "");
        for (size_t i = 1; i < src_dirs.size(); ++i)
            printf("                   %s%s\n", src_dirs[i].first.GetCStr(), src_dirs[i].second ? " [recursive]" : "");
    }
    printf("Output pack file: %s\n", dst_pak.GetCStr());
    const bool has_pattern_file = !pattern_file.IsEmpty();
    if (has_pattern_file)
        printf("Pattern file name: %s\n", pattern_file.GetCStr());

    size_t valid_src_dirs = src_dirs.size();
    for (const auto &dir : src_dirs)
    {
        if (!File::IsDirectory(dir.first))
        {
            printf("Warning: not a valid input directory: %s.\n", dir.first.GetCStr());
            valid_src_dirs--;
        }
    }

    if (valid_src_dirs == 0)
    {
        printf("Error: no valid input directories provided\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Gather list of files and set up library info
    //-----------------------------------------------------------------------//
    // This will be the list of input file -> asset entry pairs
    const String &lib_basefile = dst_pak;
    StringIMap asset_map;
    AssetLibInfo lib;
    HError err = PrepareAssetLibrary(lib, src_dirs, lib_basefile, asset_map, pattern_list, pattern_file, dup_action, part_size_mb, verbose);
    if (!err)
    {
        printf("Error: failed to prepare asset library:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // If we are appending, then check for the existing library in the
    // destination file, and cut one out.
    //-----------------------------------------------------------------------//
    if (append)
    {
        err = CutAssetLibrary(lib_basefile, verbose);
        if (!err)
        {
            printf("Error: failed to cut existing asset data from the destination file:\n");
            printf("%s\n", err->FullMessage().GetCStr());
            return -1;
        }
    }

    //-----------------------------------------------------------------------//
    // Write pack file
    //-----------------------------------------------------------------------//
    String lib_dir = Path::GetParent(lib_basefile);
    err = WriteLibrary(lib, asset_map, lib_dir, MFLUtil::kMFLVersion_MultiV30, append, verbose);
    if (!err)
    {
        printf("Error: failed to write pack file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Pack file(s) written successfully.\nDone.\n");
    return 0;
}

int Command_Detach(const String &src_pak, const String &dst_pak, bool verbose)
{
    printf("Input pack file: %s\n", src_pak.GetCStr());
    printf("Output pack file: %s\n", dst_pak.GetCStr());

    //-----------------------------------------------------------------------//
    // Copy assets to the new package.
    //-----------------------------------------------------------------------//
    HError err = CopyAssetLibraryIntoFile(src_pak, dst_pak, false /* create */, verbose);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Cut assets from the initial file.
    //-----------------------------------------------------------------------//
    err = CutAssetLibrary(src_pak, verbose);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }

    printf("Done.\n");
    return 0;
}

int Command_Export(const String &src_pak, const String &dst_dir, const std::vector<String> &pattern_list)
{
    printf("Input pack file: %s\n", src_pak.GetCStr());
    printf("Output directory: %s\n", dst_dir.GetCStr());

    if (!File::IsDirectory(dst_dir))
    {
        printf("Error: not a valid output directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Read the library TOC
    //-----------------------------------------------------------------------//
    AssetLibInfo lib;
    HError err = OpenAssetLib(src_pak, lib);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }
    if (lib.AssetInfos.size() == 0)
    {
        printf("Pack file has no assets.\nDone.\n");
        return 0;
    }

    //-----------------------------------------------------------------------//
    // Extract files
    //-----------------------------------------------------------------------//
    String lib_basefile = Path::GetFilename(src_pak);
    String lib_dir = Path::GetParent(src_pak);
    // Replace the file name of the first library part to an actual source
    // file we just opened, because it may be different from the name
    // saved in lib; e.g. if the lib was attached to *.exe.
    lib.LibFileNames[0] = lib_basefile;
    if (pattern_list.empty())
    {
        err = ExportFromLibrary(lib, lib_dir, dst_dir);
    }
    else
    {
        std::vector<Pattern> patterns;
        HError err = CreatePatternList(pattern_list, patterns);
        if (!err)
        {
            printf("Error: failed to filter files:\n");
            printf("%s\n", err->FullMessage().GetCStr());
            return -1;
        }
        err = ExportFromLibrary(lib, lib_dir, dst_dir, PatternMatch(patterns));
    }

    if (!err)
    {
        printf("Failed unpacking the library\n%s", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Done.\n");
    return 0;
}

int Command_List(const String &src_pak)
{
    printf("Input pack file: %s\n", src_pak.GetCStr());

    //-----------------------------------------------------------------------//
    // Read and print the library TOC
    //-----------------------------------------------------------------------//
    AssetLibInfo lib;
    HError err = OpenAssetLib(src_pak, lib);
    if (!err)
    {
        printf("Error: %s\n", err->FullMessage().GetCStr());
        return -1;
    }
    if (lib.AssetInfos.size() == 0)
    {
        printf("Pack file has no assets.\nDone.\n");
        return 0;
    }

    printf("Pack file assets (%zu total):\n", lib.AssetInfos.size());
    // TODO: print more info, but perhaps require cmd arguments for that? (because it's not always useful)
    for (const auto &asset : lib.AssetInfos)
    {
        printf("* %-40s[%10" PRId64 "]\n", asset.FileName.GetCStr(), asset.Size);
    }
    printf("Done.\n");
    return 0;
}

} // namespace AGSPak
