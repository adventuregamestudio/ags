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
#include "data/mfl_utils.h"
#include <memory>
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

// TODO: might replace "printf" with the logging functions,
// but then we'd also need to make sure they are initialized in tools

static void ExportAsset(Stream *lib_in, const AssetInfo &asset, const String &dst_dir)
{
    String dst_f = Path::ConcatPaths(dst_dir, asset.FileName);
    String sub_dir = Path::GetParent(asset.FileName);
    if (!sub_dir.IsEmpty() && sub_dir != "." &&
        !Directory::CreateAllDirectories(dst_dir, sub_dir))
    {
        printf("Error: unable to create a subdirectory: %s\n", sub_dir.GetCStr());
        return;
    }
    std::unique_ptr<Stream> out(File::CreateFile(dst_f));
    if (!out)
    {
        printf("Error: unable to open a file for writing: %s\n", asset.FileName.GetCStr());
        return;
    }
    lib_in->Seek(asset.Offset, kSeekBegin);
    soff_t wrote = CopyStream(lib_in, out.get(), asset.Size);
    if (wrote == asset.Size)
        printf("+ %s\n", asset.FileName.GetCStr());
    else
        printf("Error: file was not written correctly: %s\n Expected: %jd, wrote: %jd bytes\n",
               asset.FileName.GetCStr(), static_cast<intmax_t>(asset.Size), static_cast<intmax_t>(wrote));
}

HError ExportFromLibrary(const AssetLibInfo &lib, const String &lib_dir, const String &dst_dir,
    const std::function<bool(const String &)> *asset_filter)
{
    for (size_t i = 0; i < lib.LibFileNames.size(); ++i)
    {
        String lib_f = lib.LibFileNames[i];
        String path = Path::ConcatPaths(lib_dir, lib_f);
        std::unique_ptr<Stream> lib_in(File::OpenFileRead(path));
        if (!lib_in)
        {
            return new Error(String::FromFormat("Failed to open a library file for reading: %s",
                lib_f.GetCStr()));
        }
        printf("Extracting %s:\n", lib_f.GetCStr());
        for (const auto &asset : lib.AssetInfos)
        {
            if (asset.LibUid != i)
                continue;

            if (asset_filter)
            {
                if ((*asset_filter)(asset.FileName))
                    ExportAsset(lib_in.get(), asset, dst_dir);
            }
            else
            {
                ExportAsset(lib_in.get(), asset, dst_dir);
            }
        }
    }
    return HError::None();
}

HError ExportFromLibrary(const AssetLibInfo &lib, const String &lib_dir, const String &dst_dir)
{
    return ExportFromLibrary(lib, lib_dir, dst_dir, nullptr);
}

HError ExportFromLibrary(const AssetLibInfo &lib, const String &lib_dir, const String &dst_dir,
    const std::function<bool(const String &)> &asset_filter)
{
    return ExportFromLibrary(lib, lib_dir, dst_dir, &asset_filter);
}

HError MakeListOfFiles(std::vector<String> &files, const String &asset_dir, bool do_subdirs)
{
    const String &parent = asset_dir;

    for (FindFile ff = FindFile::Open(parent, "*", true, false, do_subdirs ? -1 : 0);
         !ff.AtEnd(); ff.Next())
    {
        String filename = ff.Current();
        files.push_back(filename);
    }
    return HError::None();
}

HError MakeAssetListFromFileList(const std::vector<String> &files, std::vector<AssetInfo> &assets, const String &asset_dir)
{
    String fpath;
    const String &parent = asset_dir;
    for (const auto &file : files)
    {
        AssetInfo asset;
        asset.FileName = file;
        Path::ConcatPaths(fpath, parent, asset.FileName);
        asset.Size = File::GetFileSize(fpath);
        assets.push_back(asset);
    }
    return HError::None();
}

HError MakeAssetList(std::vector<AssetInfo> &assets, const String &asset_dir,
    bool do_subdirs, const String &lib_basefile)
{
    String dpath;
    String fpath;
    const String &base = asset_dir;
    const String &parent = asset_dir;

    for (FindFile ff = FindFile::Open(parent, "*", true, false, do_subdirs ? -1 : 0);
        !ff.AtEnd(); ff.Next())
    {
        AssetInfo asset;
        asset.FileName = ff.Current();
        Path::ConcatPaths(fpath, parent, asset.FileName);
        asset.Size = File::GetFileSize(fpath);
        assets.push_back(asset);
    }
    return HError::None();
}

HError MakeAssetLib(AssetLibInfo &lib, const String &lib_basefile,
    std::vector<AssetInfo> &assets, soff_t part_size, bool append_first)
{
    int lib_id = 0;
    if (part_size > 0)
    {
        soff_t start_size = 0;
        if (append_first)
        {
            start_size = File::GetFileSize(lib_basefile);
        }

        // Split assets into multiple libs
        for (soff_t lib_size = start_size; assets.size() > 0 && lib_id < MFLUtil::MaxMultiLibFiles; ++lib_id, lib_size = 0)
        {
            for (auto it = assets.begin(); it != assets.end() && lib_size < part_size;)
            {
                // We accept a single asset to be larger than the limit,
                // because we cannot split assets themselves in parts
                if (lib_size > 0 && (lib_size + it->Size > part_size))
                {
                    ++it;
                    continue;
                }
                lib_size += it->Size;
                it->LibUid = lib_id;
                lib.AssetInfos.push_back(*it);
                it = assets.erase(it);
            }
        }
        // Did we succeed arranging everything?
        if (assets.size() > 0)
        {
            return new Error(String::FromFormat(
                "Failed trying to split assets in parts of %zu MB. Max number of package partitions exceeded.\n", part_size));
        }
    }
    else
    {
        // Put everything in one lib
        lib.AssetInfos = std::move(assets);
    }
    // Set up lib filenames
    String filename = Path::GetFilename(lib_basefile);
    String base_filename = Path::RemoveExtension(filename);
    lib.LibFileNames.push_back(filename);
    for (int id = 1; id < lib_id; ++id)
    {
        filename.Format("%s.%03d", base_filename.GetCStr(), id);
        lib.LibFileNames.push_back(filename);
    }
    return HError::None();
}

HError WriteLibraryFile(AssetLibInfo &lib, const String &asset_dir, const String &lib_filename,
                        MFLUtil::MFLVersion lib_version, int lib_index, bool append, bool verbose)
{
    append &= File::IsFile(lib_filename); // append only if this file exists, otherwise create
    std::unique_ptr<Stream> out(append ? File::CreateTempFile() : File::CreateFile(lib_filename));
    if (!out)
    {
        if (append)
            return new Error("Error: failed to open a temporary file for writing pack data.");
        else
            return new Error("Error: failed to open pack file for writing.");
    }

    soff_t s_offset = out->GetPosition();
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, lib_index, out.get());
    for (auto &asset : lib.AssetInfos)
    {
        if (asset.LibUid == lib_index)
        {
            asset.Offset = out->GetPosition() - s_offset;
            String path = Path::ConcatPaths(asset_dir, asset.FileName);
            std::unique_ptr<Stream> in(File::OpenFileRead(path));
            if (!in)
                return new Error(String::FromFormat("Failed to open the asset file for reading: %s", path.GetCStr()));
            if (CopyStream(in.get(), out.get(), asset.Size) < asset.Size)
                return new Error(String::FromFormat("Failed to write the asset '%s'.", asset.FileName.GetCStr()));
            if (verbose)
                printf("Info: wrote asset '%s'\n", asset.FileName.GetCStr());
        }
    }
    lib.BaseFileOffset = s_offset;
    out->Seek(s_offset, kSeekBegin);
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, lib_index, out.get());
    out->Seek(0, kSeekEnd);
    MFLUtil::WriteEnder(s_offset, lib_version, out.get());
    out->Flush();

    // If appending, then write to the final file
    if (append)
    {
        std::unique_ptr<Stream> final(File::OpenFileWrite(lib_filename));
        if (!final)
            return new Error("Error: failed to open destination file for writing.");
        final->Seek(0, kSeekEnd);
        lib.BaseFileOffset = final->GetPosition();
        out->Seek(0, kSeekBegin);
        CopyStream(out->GetStreamBase(), final->GetStreamBase());
        OverwriteEnder(lib.BaseFileOffset, lib_version, final.get());
    }
    return HError::None();
}

HError WriteLibrary(AssetLibInfo &lib, const String &asset_dir, const String &dst_dir, MFLUtil::MFLVersion lib_version,
                    bool append_first, bool verbose)
{
    for (size_t id = 0; id < lib.LibFileNames.size(); ++id)
    {
        String dst_file = Path::ConcatPaths(dst_dir, lib.LibFileNames[id]);
        if (verbose)
            printf("Info: writing pack file '%s' ...\n", dst_file.GetCStr());

        HError err = WriteLibraryFile(lib, asset_dir, dst_file, lib_version, id, append_first && (id == 0), verbose);
        if (!err)
        {
            if (verbose)
                printf("Error: failed to write pack file '%s'.\n", dst_file.GetCStr());
            return err;
        }

        // Post-check
        err = TestLibraryFile(dst_file, &lib);
        if (!err)
        {
            if (verbose)
                printf("Error: post-check failed for file '%s'.\n", dst_file.GetCStr());
            return new Error("Pack post-check failed", err);
        }
    }
    return HError::None();
}

HError WriteLibraryFromPaths(const String &output_path,
    const std::vector<AssetFileEntry> &entries,
    MFLUtil::MFLVersion lib_version)
{
    std::unique_ptr<Stream> out(File::CreateFile(output_path));
    if (!out)
        return new Error("Failed to open pack file for writing.");

    AssetLibInfo lib;
    lib.LibFileNames.push_back(Path::GetFilename(output_path));

    for (const auto &entry : entries)
    {
        AssetInfo ai;
        ai.FileName = entry.FileName;
        ai.LibUid = 0;
        ai.Size = File::GetFileSize(entry.SourcePath);
        lib.AssetInfos.push_back(ai);
    }

    soff_t s_offset = out->GetPosition();
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, 0, out.get());

    for (size_t i = 0; i < entries.size(); ++i)
    {
        lib.AssetInfos[i].Offset = out->GetPosition() - s_offset;
        std::unique_ptr<Stream> in(File::OpenFileRead(entries[i].SourcePath));
        if (!in)
            return new Error(String::FromFormat(
                "Failed to open asset '%s' for reading.", entries[i].FileName.GetCStr()));
        if (CopyStream(in.get(), out.get(), lib.AssetInfos[i].Size) < lib.AssetInfos[i].Size)
            return new Error(String::FromFormat(
                "Failed to write asset '%s'.", entries[i].FileName.GetCStr()));
    }

    out->Seek(s_offset, kSeekBegin);
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, 0, out.get());
    out->Seek(0, kSeekEnd);
    MFLUtil::WriteEnder(s_offset, lib_version, out.get());
    return HError::None();
}

HError TestLibraryFile(const String &lib_file, const AssetLibInfo *compare_lib)
{
    std::unique_ptr<Stream> in(File::OpenFileRead(lib_file));
    if (!in)
        return new Error("Error: failed to open pack file for reading.");

    AssetLibInfo lib;
    MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(lib, in.get());
    if (mfl_err != MFLUtil::kMFLNoError)
        return new Error("Failed to parse pack file.", MFLUtil::GetMFLErrorText(mfl_err).GetCStr());

    if (compare_lib)
    {
        if (lib.BaseFileOffset != compare_lib->BaseFileOffset)
            return new Error(String::FromFormat("Base library offset does not match: %lld vs %lld", lib.BaseFileOffset, compare_lib->BaseFileOffset));
        if (lib.AssetInfos.size() != compare_lib->AssetInfos.size())
            return new Error(String::FromFormat("Number of assets does not match: %zu vs %zu", lib.AssetInfos.size(), compare_lib->AssetInfos.size()));
        for (size_t i = 0; i < lib.AssetInfos.size(); ++i)
        {
            const auto &asset1 = lib.AssetInfos[i];
            const auto &asset2 = compare_lib->AssetInfos[i];
            if (asset1.FileName.CompareNoCase(asset2.FileName) != 0)
                return new Error(String::FromFormat("Asset %zu does not match filename: %s vs %s", i, asset1.FileName.GetCStr(), asset2.FileName.GetCStr()));
            if (asset1.Size != asset2.Size)
                return new Error(String::FromFormat("Asset %zu does not match size: %lld vs %lld", i, asset1.Size, asset2.Size));
            if (asset1.Offset - lib.BaseFileOffset != asset2.Offset)
                return new Error(String::FromFormat("Asset %zu does not match offset: %lld vs %lld", i, asset1.Offset - lib.BaseFileOffset, asset2.Offset));
        }
    }

    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
