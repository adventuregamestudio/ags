//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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

HError UnpackLibrary(const AssetLibInfo &lib, const String &lib_dir, const String &dst_dir)
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
            if (asset.LibUid != i) continue;
            String dst_f = Path::ConcatPaths(dst_dir, asset.FileName);
            String sub_dir = Path::GetParent(asset.FileName);
            if (!sub_dir.IsEmpty() && sub_dir != "." &&
                !Directory::CreateAllDirectories(dst_dir, sub_dir))
            {
                printf("Error: unable to create a subdirectory: %s\n", sub_dir.GetCStr());
                continue;
            }
            std::unique_ptr<Stream> out(File::CreateFile(dst_f));
            if (!out)
            {
                printf("Error: unable to open a file for writing: %s\n", asset.FileName.GetCStr());
                continue;
            }
            lib_in->Seek(asset.Offset, kSeekBegin);
            soff_t wrote = CopyStream(lib_in.get(), out.get(), asset.Size);
            if (wrote == asset.Size)
                printf("+ %s\n", asset.FileName.GetCStr());
            else
                printf("Error: file was not written correctly: %s\n Expected: %jd, wrote: %jd bytes\n",
                    asset.FileName.GetCStr(), static_cast<intmax_t>(asset.Size), static_cast<intmax_t>(wrote));
        }
    }
    return HError::None();
}

HError MakeAssetList(std::vector<AssetInfo> &assets, const String &asset_dir,
    bool do_subdirs, const String &lib_basefile)
{
    String dpath;
    String fpath;
    String base = asset_dir;
    String parent = asset_dir;

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
    std::vector<AssetInfo> &assets, soff_t part_size)
{
    int lib_id = 0;
    if (part_size > 0)
    {
        // Split assets into multiple libs
        for (; assets.size() > 0 && lib_id < MFLUtil::MaxMultiLibFiles; ++lib_id)
        {
            soff_t lib_size = 0;
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

HError WriteLibraryFile(AssetLibInfo &lib, const String &asset_dir,
    const String &lib_filename, MFLUtil::MFLVersion lib_version, int lib_index)
{
    std::unique_ptr<Stream> out(File::CreateFile(lib_filename));
    if (!out)
        return new Error("Error: failed to open pack file for writing.");

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
                return new Error("Failed to open the file for reading.");
            if (CopyStream(in.get(), out.get(), asset.Size) < asset.Size)
                return new Error(String::FromFormat("Failed to write the asset '%s'.", asset.FileName.GetCStr()));
        }
    }
    out->Seek(s_offset, kSeekBegin);
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, lib_index, out.get());
    out->Seek(0, kSeekEnd);
    MFLUtil::WriteEnder(s_offset, lib_version, out.get());
    return HError::None();
}

HError WriteLibrary(AssetLibInfo &lib, const String &asset_dir,
    const String &dst_dir, MFLUtil::MFLVersion lib_version)
{
    for (size_t id = 0; id < lib.LibFileNames.size(); ++id)
    {
        String dst_file = Path::ConcatPaths(dst_dir, lib.LibFileNames[id]);
        HError err = WriteLibraryFile(lib, asset_dir, dst_file, lib_version, id);
        if (!err)
            return err;
    }
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
