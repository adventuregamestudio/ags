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
#include "core/assetmanager.h"
#include <algorithm>
#include <regex>
#include "util/directory.h"
#include "util/multifilelib.h"
#include "util/path.h"
#include "util/string_utils.h" // cbuf_to_string_and_free


namespace AGS
{
namespace Common
{

// TODO: move elsewhere later
std::unique_ptr<AssetManager> AssetMgr;



AssetLocation::AssetLocation()
    : Offset(0)
    , Size(0)
{
}


inline static bool IsAssetLibDir(const AssetLibInfo *lib) { return lib->BaseFileName.IsEmpty(); }
inline static bool IsAssetLibFile(const AssetLibInfo *lib) { return !lib->BaseFileName.IsEmpty(); }

bool AssetManager::LibsByPriority::operator()(const AssetLibInfo *lib1, const AssetLibInfo *lib2) const
{
    const bool lib1dir = IsAssetLibDir(lib1);
    const bool lib2dir = IsAssetLibDir(lib2);
    if (lib1dir == lib2dir)
        return false; // both are equal, none is less
    if (Priority == kAssetPriorityLib)
        return !lib1dir; // first element is less if it's library file
    if (Priority == kAssetPriorityDir)
        return lib1dir; // first element is less if it's directory
    return false; // unknown priority, just fail
}


/* static */ bool AssetManager::IsDataFile(const String &data_file)
{
    Stream *in = File::OpenFileCI(data_file, Common::kFile_Open, Common::kFile_Read);
    if (in)
    {
        MFLUtil::MFLError err = MFLUtil::TestIsMFL(in, true);
        delete in;
        return err == MFLUtil::kMFLNoError;
    }
    return false;
}

/* static */ AssetError AssetManager::ReadDataFileTOC(const String &data_file, AssetLibInfo &lib)
{
    Stream *in = File::OpenFileCI(data_file, Common::kFile_Open, Common::kFile_Read);
    if (in)
    {
        MFLUtil::MFLError err = MFLUtil::ReadHeader(lib, in);
        delete in;
        return (err != MFLUtil::kMFLNoError) ? kAssetErrLibParse : kAssetNoError;
    }
    return kAssetErrNoLibFile;
}

AssetManager::AssetManager()
{
    _libsByPriority.Priority = kAssetPriorityDir;
}

void AssetManager::SetSearchPriority(AssetSearchPriority priority)
{
    _libsByPriority.Priority = priority;
    std::sort(_activeLibs.begin(), _activeLibs.end(), _libsByPriority);
}

AssetSearchPriority AssetManager::GetSearchPriority() const
{
    return _libsByPriority.Priority;
}

AssetError AssetManager::AddLibrary(const String &path, const AssetLibInfo **out_lib)
{
    return AddLibrary(path, "", out_lib);
}

AssetError AssetManager::AddLibrary(const String &path, const String &filters, const AssetLibInfo **out_lib)
{
    if (path.IsEmpty())
        return kAssetErrNoLibFile;

    for (const auto &lib : _libs)
    {
        if (Path::ComparePaths(lib->BasePath, path) == 0)
        {
            // already present, only assign new filters
            lib->Filters = filters.Split(',');
            if (out_lib)
                *out_lib = lib.get();
            return kAssetNoError;
        }
    }

    AssetLibEx *lib;
    AssetError err = RegisterAssetLib(path, lib);
    if (err != kAssetNoError)
        return err;
    lib->Filters = filters.Split(',');
    auto place = std::upper_bound(_activeLibs.begin(), _activeLibs.end(), lib, _libsByPriority);
    _activeLibs.insert(place, lib);
    if (out_lib)
        *out_lib = lib;
    return kAssetNoError;
}

void AssetManager::RemoveLibrary(const String &path)
{
    for (auto it = _libs.cbegin(); it != _libs.cend(); ++it)
    {
        if (Path::ComparePaths((*it)->BasePath, path) == 0)
        {
            std::remove(_activeLibs.begin(), _activeLibs.end(), (*it).get());
            _libs.erase(it);
            return;
        }
    }
}

void AssetManager::RemoveAllLibraries()
{
    _libs.clear();
    _activeLibs.clear();
}

size_t AssetManager::GetLibraryCount() const
{
    return _libs.size();
}

const AssetLibInfo *AssetManager::GetLibraryInfo(size_t index) const
{
    return index < _libs.size() ? _libs[index].get() : nullptr;
}

bool AssetManager::DoesAssetExist(const String &asset_name, const String &filter) const
{
    return GetAsset(asset_name, filter, nullptr);
}

void AssetManager::FindAssets(std::vector<String> &assets, const String &wildcard,
    const String &filter) const
{
    String pattern = StrUtil::WildcardToRegex(wildcard);
    const std::regex regex(pattern.GetCStr(), std::regex_constants::icase);
    std::cmatch mr;

    for (const auto *lib : _activeLibs)
    {
        if (filter != "*" &&
            std::find(lib->Filters.begin(), lib->Filters.end(), filter)
            == lib->Filters.end())
            continue; // filter does not match

        bool found = false;
        if (IsAssetLibDir(lib))
        {
            for (FindFile ff = FindFile::OpenFiles(lib->BaseDir, wildcard);
                 !ff.AtEnd(); ff.Next())
                assets.push_back(ff.Current());
        }
        else
        {
            for (const auto &a : lib->AssetInfos)
            {
                if (std::regex_match(a.FileName.GetCStr(), mr, regex))
                    assets.push_back(a.FileName);
            }
        }
    }

    // Sort and remove duplicates
    std::sort(assets.begin(), assets.end());
    assets.erase(std::unique(assets.begin(), assets.end()), assets.end());
}

AssetError AssetManager::RegisterAssetLib(const String &path, AssetLibEx *&out_lib)
{
    // Test for a directory
    std::unique_ptr<AssetLibEx> lib;
    if (File::IsDirectory(path))
    {
        lib.reset(new AssetLibEx());
        lib->BasePath = Path::MakeAbsolutePath(path);
        lib->BaseDir = Path::GetDirectoryPath(lib->BasePath);

        // TODO: maybe parse directory for the file reference? idk if needed
    }
    // ...else try open a data library
    else
    {
        Stream *in = File::OpenFileCI(path, Common::kFile_Open, Common::kFile_Read);
        if (!in)
            return kAssetErrNoLibFile; // can't be opened, return error code

        lib.reset(new AssetLibEx());
        MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(*lib, in);
        delete in;

        if (mfl_err != MFLUtil::kMFLNoError)
            return kAssetErrLibParse;

        lib->BasePath = Path::MakeAbsolutePath(path);
        lib->BaseDir = Path::GetDirectoryPath(lib->BasePath);
        lib->BaseFileName = Path::GetFilename(lib->BasePath);
        lib->LibFileNames[0] = lib->BaseFileName;
    }

    out_lib = lib.get();
    _libs.push_back(std::move(lib));
    return kAssetNoError;
}

bool AssetManager::GetAsset(const String &asset_name, const String &filter,
    AssetLocation *loc) const
{
    for (const auto *lib : _activeLibs)
    {
        if (filter != "*" &&
            std::find(lib->Filters.begin(), lib->Filters.end(), filter)
            == lib->Filters.end())
            continue; // filter does not match

        bool found = false;
        if (IsAssetLibDir(lib))
            found = GetAssetFromDir(lib, asset_name, loc);
        else
            found = GetAssetFromLib(lib, asset_name, loc);
        if (found)
            return true;
    }
    return false;
}

bool AssetManager::GetAssetFromLib(const AssetLibInfo *lib, const String &asset_name,
    AssetLocation *loc) const
{
    const AssetInfo *asset = nullptr;
    for (const auto &a : lib->AssetInfos)
    {
        if (a.FileName.CompareNoCase(asset_name) == 0)
        {
            asset = &a;
            break;
        }
    }
    if (asset == nullptr)
        return false;

    String libfile = File::FindFileCI(lib->BaseDir, lib->LibFileNames[asset->LibUid]);
    if (libfile.IsEmpty())
        return false;
    if (loc)
    {
        loc->FileName = libfile;
        loc->Offset = asset->Offset;
        loc->Size = asset->Size;
    }
    return true;
}

bool AssetManager::GetAssetFromDir(const AssetLibInfo *lib, const String &file_name,
    AssetLocation *loc) const
{
    String found_file = File::FindFileCI(lib->BaseDir, file_name);
    if (found_file.IsEmpty() || !File::IsFile(found_file))
        return false; // not found, or not a file

    if (loc)
    {
        loc->FileName = found_file;
        loc->Offset = 0;
        loc->Size = File::GetFileSize(found_file);
    }
    return true;
}

Stream *AssetManager::OpenAsset(const String &asset_name) const
{
    return OpenAsset(asset_name, "");
}

Stream *AssetManager::OpenAsset(const String &asset_name, const String &filter) const
{
    AssetLocation loc;
    if (!GetAsset(asset_name, filter, &loc))
        return nullptr;
    return File::OpenFile(loc.FileName, loc.Offset, loc.Offset + loc.Size);
}


String GetAssetErrorText(AssetError err)
{
    switch (err)
    {
    case kAssetNoError:
        return "No error.";
    case kAssetErrNoLibFile:
        return "Asset library file not found or could not be opened.";
    case kAssetErrLibParse:
        return "Not an asset library or unsupported format.";
    case kAssetErrNoManager:
        return "Asset manager is not initialized.";
    }
    return "Unknown error.";
}

} // namespace Common
} // namespace AGS
