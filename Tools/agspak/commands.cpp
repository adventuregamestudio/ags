//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
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

// Filters the list of valid file paths using a list of regex patterns;
// returns the filtered result.
static std::vector<String> FilterFileList(const std::vector<String> &files, const std::vector<std::regex> &patterns)
{
    std::vector<String> filtered_files;
    for (const auto &full_filepath : files)
    {
        String fn_only = Path::GetFilename(full_filepath);
        for (const auto &p : patterns)
        {
            if (std::regex_match(fn_only.GetCStr(), p))
            {
                filtered_files.push_back(full_filepath);
                break;
            }
        }
    }
    return filtered_files;
}

static HError OpenAssetLib(const String &pak_file, AssetLibInfo &lib)
{
    auto in = File::OpenFileRead(pak_file);
    if (!in)
        return new Error("Failed to open pack file for reading.");

    MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(lib, in.get());
    if (mfl_err != MFLUtil::kMFLNoError)
        return new Error("Failed to parse pack file.\n%s", MFLUtil::GetMFLErrorText(mfl_err).GetCStr());
    return HError::None();
}


int Command_Create(const String &src_dir, const String &dst_pak, const std::vector<std::regex> &pattern_list,
                   const String &pattern_file, bool do_subdirs, size_t part_size_mb, bool verbose)
{
    printf("Input directory: %s\n", src_dir.GetCStr());
    printf("Output pack file: %s\n", dst_pak.GetCStr());
    const bool has_pattern_file = !pattern_file.IsEmpty();
    if (has_pattern_file)
        printf("Pattern file name: %s\n", pattern_file.GetCStr());

    if (!File::IsDirectory(src_dir))
    {
        printf("Error: not a valid input directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Gather list of files and set up library info
    //-----------------------------------------------------------------------//
    const String &asset_dir = src_dir;
    const String &lib_basefile = dst_pak;

    std::vector<String> files;
    HError err = MakeListOfFiles(files, asset_dir, do_subdirs);
    if (!err)
    {
        printf("Error: failed to gather list of files:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    // Apply the explicit file list, if provided
    if (!pattern_list.empty())
    {
        files = std::move(FilterFileList(files, pattern_list));
    }

    // Apply the include/exclude pattern file as a filter
    if (has_pattern_file)
    {
        std::vector<String> output_files;
        err = IncludeFiles(files, output_files, asset_dir, pattern_file, verbose);
        if (!err)
        {
            printf("Error: failed to processes %s file:\n", pattern_file.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
            return -1;
        }

        files = std::move(output_files);
    }

    std::vector<AssetInfo> assets;
    err = MakeAssetListFromFileList(files, assets, asset_dir);
    if (!err)
    {
        printf("Error: failed to prepare list of assets:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    if (assets.size() == 0)
    {
        printf("No valid assets found in the provided directory.\nDone.\n");
        return 0;
    }

    AssetLibInfo lib;
    soff_t part_size_b = part_size_mb * 1024 * 1024; // MB to bytes
    err = MakeAssetLib(lib, lib_basefile, assets, part_size_b);
    if (!err)
    {
        printf("Error: failed to configure asset library:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write pack file
    //-----------------------------------------------------------------------//
    String lib_dir = Path::GetParent(lib_basefile);
    err = WriteLibrary(lib, asset_dir, lib_dir, MFLUtil::kMFLVersion_MultiV30, verbose);
    if (!err)
    {
        printf("Error: failed to write pack file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Pack file(s) written successfully.\nDone.\n");
    return 0;
}

int Command_Export(const String &src_pak, const String &dst_dir, const std::vector<std::regex> &pattern_list)
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
        err = ExportFromLibrary(lib, lib_dir, dst_dir);
    else
        err = ExportFromLibrary(lib, lib_dir, dst_dir, &pattern_list);
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
    if (lib.AssetInfos.size() == 0)
    {
        printf("Pack file has no assets.\nDone.\n");
        return 0;
    }

    printf("Pack file assets (%zu total):\n", lib.AssetInfos.size());
    // TODO: print more info, but perhaps require cmd arguments for that? (because it's not always useful)
    for (const auto &asset : lib.AssetInfos)
    {
        printf("* %s\n", asset.FileName.GetCStr());
    }
    printf("Done.\n");
    return 0;
}

} // namespace AGSPak
