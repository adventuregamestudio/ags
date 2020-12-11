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
#include "util/misc.h" // ci_fopen
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

/* static */ bool AssetManager::IsDataFile(const String &data_file)
{
    Stream *in = ci_fopen(data_file, Common::kFile_Open, Common::kFile_Read);
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
    Stream *in = ci_fopen(data_file, Common::kFile_Open, Common::kFile_Read);
    if (in)
    {
        MFLUtil::MFLError err = MFLUtil::ReadHeader(lib, in);
        delete in;
        return (err != MFLUtil::kMFLNoError) ? kAssetErrLibParse : kAssetNoError;
    }
    return kAssetErrNoLibFile;
}

AssetManager::AssetManager()
    : _searchPriority(kAssetPriorityDir)
{
}

void AssetManager::SetSearchPriority(AssetSearchPriority priority)
{
    _searchPriority = priority;
}

AssetSearchPriority AssetManager::GetSearchPriority() const
{
    return _searchPriority;
}

AssetError AssetManager::AddLibrary(const String &data_file)
{
    if (data_file.IsEmpty())
        return kAssetErrNoLibFile;

    for (const auto &lib : _libs)
    {
        if (Path::ComparePaths(lib->BaseFilePath, data_file) == 0)
            return kAssetNoError; // already present
    }

    AssetLibInfo *lib;
    AssetError err = RegisterAssetLib(data_file, lib);
    if (err != kAssetNoError)
        return err;
    _activeLibs.push_back(lib);
    return kAssetNoError;
}

void AssetManager::RemoveLibrary(const String &data_file)
{
    for (auto it = _libs.cbegin(); it != _libs.cend(); ++it)
    {
        if (Path::ComparePaths((*it)->BaseFilePath, data_file) == 0)
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

void AssetManager::SetLibrarySearch(const String &data_file, bool on)
{
    for (const auto &lib : _libs)
    {
        if (Path::ComparePaths(lib->BaseFilePath, data_file) == 0)
        {
            auto it = std::find(_activeLibs.begin(), _activeLibs.end(), lib.get());
            if (on && it == _activeLibs.end())
                _activeLibs.push_back(lib.get());
            else if (!on && it != _activeLibs.end())
                _activeLibs.erase(it);
        }
    }
}

size_t AssetManager::GetLibraryCount() const
{
    return _libs.size();
}

const AssetLibInfo *AssetManager::GetLibraryTOC(size_t index) const
{
    return index < _libs.size() ? _libs[index].get() : nullptr;
}

bool AssetManager::DoesAssetExist(const String &asset_name) const
{
    return FindAssetInLibs(asset_name, nullptr, nullptr) ||
        File::TestReadFile(asset_name);
}

AssetError AssetManager::RegisterAssetLib(const String &data_file, AssetLibInfo *&out_lib)
{
    // open data library
    Stream *in = ci_fopen(data_file, Common::kFile_Open, Common::kFile_Read);
    if (!in)
        return kAssetErrNoLibFile; // can't be opened, return error code

    std::unique_ptr<AssetLibInfo> lib(new AssetLibInfo());
    MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(*lib, in);
    delete in;

    if (mfl_err != MFLUtil::kMFLNoError)
        return kAssetErrLibParse;

    lib->BaseFileName = Path::GetFilename(data_file);
    lib->LibFileNames[0] = lib->BaseFileName;
    lib->BaseFilePath = Path::MakeAbsolutePath(data_file);
    lib->BasePath = Path::GetDirectoryPath(data_file);
    out_lib = lib.get();
    _libs.push_back(std::move(lib));
    return kAssetNoError;
}

bool AssetManager::FindAssetInLibs(const String &asset_name, const AssetInfo **out_asset, const AssetLibInfo **out_lib) const
{
    for (const auto *lib : _activeLibs)
    {
        for (const auto &asset : lib->AssetInfos)
        {
            if (asset.FileName.CompareNoCase(asset_name) == 0)
            {
                if (out_asset)
                    *out_asset = &asset;
                if (out_lib)
                    *out_lib = lib;
                return true;
            }
        }
    }
    return false;
}

bool AssetManager::GetAssetFromLib(const String &asset_name, AssetLocation &loc, FileOpenMode open_mode, FileWorkMode work_mode) const
{
    if (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read)
        return false; // creating/writing is allowed only for common files on disk

    const AssetInfo *asset;
    const AssetLibInfo *lib;
    if (!FindAssetInLibs(asset_name, &asset, &lib))
        return false; // asset not found

    String libfile = cbuf_to_string_and_free( ci_find_file(lib->BasePath, lib->LibFileNames[asset->LibUid]) );
    if (libfile.IsEmpty())
        return false;
    loc.FileName = libfile;
    loc.Offset = asset->Offset;
    loc.Size = asset->Size;
    return true;
}

bool AssetManager::GetAssetFromDir(const String &file_name, AssetLocation &loc, FileOpenMode open_mode, FileWorkMode work_mode) const
{
    String exfile = cbuf_to_string_and_free( ci_find_file(nullptr, file_name) );
    if (exfile.IsEmpty() || !Path::IsFile(exfile))
        return false;
    loc.FileName = exfile;
    loc.Offset = 0;
    loc.Size = File::GetFileSize(exfile);
    return true;
}

bool AssetManager::GetAssetByPriority(const String &asset_name, AssetLocation &loc, FileOpenMode open_mode, FileWorkMode work_mode) const
{
    if (_searchPriority == kAssetPriorityDir)
    {
        // check for disk, otherwise use datafile
        return GetAssetFromDir(asset_name, loc, open_mode, work_mode) ||
            GetAssetFromLib(asset_name, loc, open_mode, work_mode);
    }
    else if (_searchPriority == kAssetPriorityLib)
    {
        // check datafile first, then scan directory
        return GetAssetFromLib(asset_name, loc, open_mode, work_mode) ||
            GetAssetFromDir(asset_name, loc, open_mode, work_mode);
    }
    return false;
}

bool AssetManager::GetAssetLocation(const String &asset_name, AssetLocation &loc) const
{
    return GetAssetByPriority(asset_name, loc, kFile_Open, kFile_Read);
}

Stream *AssetManager::OpenAsset(const String &asset_name, soff_t *asset_size, FileOpenMode open_mode, FileWorkMode work_mode) const
{
    AssetLocation loc;
    if (GetAssetByPriority(asset_name, loc, open_mode, work_mode))
    {
        Stream *s = File::OpenFile(loc.FileName, open_mode, work_mode);
        if (s)
        {
            s->Seek(loc.Offset, kSeekBegin);
            if (asset_size)
                *asset_size = loc.Size;
        }
        return s;
    }
    return nullptr;
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
