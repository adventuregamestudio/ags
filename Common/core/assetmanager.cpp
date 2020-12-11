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

AssetManager::~AssetManager()
{
    delete &_assetLib;
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
    : _assetLib(*new AssetLibInfo())
    , _searchPriority(kAssetPriorityDir)
    , _lastAssetSize(0)
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

AssetError AssetManager::SetDataFile(const String &data_file)
{
    if (data_file.IsEmpty())
    {
        return kAssetErrNoLibFile;
    }
    if (Path::ComparePaths(_assetLib.BaseFilePath, data_file) == 0)
    {
        return kAssetNoError;
    }
    AssetError err = RegisterAssetLib(data_file);
    return err;
}

String AssetManager::GetLibraryForAsset(const String &asset_name) const
{
    if (asset_name.IsEmpty())
    {
        return "";
    }
    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (!asset)
    {
        // asset not found
        return "";
    }

    return MakeLibraryFileNameForAsset(asset);
}

soff_t AssetManager::GetAssetOffset(const String &asset_name) const
{
    if (asset_name.IsEmpty())
    {
        return -1;
    }
    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (asset)
    {
        return asset->Offset;
    }
    return -1;
}

soff_t AssetManager::GetAssetSize(const String &asset_name) const
{
    if (asset_name.IsEmpty())
    {
        return -1;
    }
    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (asset)
    {
        return asset->Size;
    }
    return -1;
}

soff_t AssetManager::GetLastAssetSize() const
{
    return _lastAssetSize;
}

size_t AssetManager::GetAssetCount() const
{
    return _assetLib.AssetInfos.size();
}

String AssetManager::GetAssetFileByIndex(size_t index) const
{
    if ((index < 0) || ((size_t)index >= _assetLib.AssetInfos.size()))
        return nullptr;

    return _assetLib.AssetInfos[index].FileName;
}

String AssetManager::GetLibraryBaseFile() const
{
    return _assetLib.BaseFileName;
}

const AssetLibInfo *AssetManager::GetLibraryTOC() const
{
    return &_assetLib;
}

bool AssetManager::DoesAssetExist(const String &asset_name) const
{
    return FindAssetByFileName(asset_name) != nullptr ||
        File::TestReadFile(asset_name);
}

AssetError AssetManager::RegisterAssetLib(const String &data_file)
{
    // base path is current directory
    _basePath = ".";

    // open data library
    Stream *in = ci_fopen(data_file, Common::kFile_Open, Common::kFile_Read);
    if (!in)
        return kAssetErrNoLibFile; // can't be opened, return error code

    // read MultiFileLibrary header (CLIB)
    // PSP: allocate struct on the heap to avoid overflowing the stack.
    MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(_assetLib, in);
    delete in;

    if (mfl_err != MFLUtil::kMFLNoError)
    {
        _assetLib.Unload();
        return kAssetErrLibParse;
    }

    // fixup base library filename
    String nammwas = data_file;
    String data_file_fixed = data_file;
    // TODO: this algorythm should be in path/string utils
    data_file_fixed.TruncateToRightSection('\\');
    data_file_fixed.TruncateToRightSection('/');
    if (data_file_fixed.Compare(nammwas) != 0)
    {
        // store complete path
        _basePath = nammwas;
        _basePath.TruncateToLeft(nammwas.GetLength() - data_file_fixed.GetLength());
        _basePath.TrimRight('\\');
        _basePath.TrimRight('/');
    }

    // set library filename
    _assetLib.LibFileNames[0] = data_file_fixed;
    // make a lowercase backup of the original file name
    _assetLib.BaseFileName = data_file_fixed;
    _assetLib.BaseFileName.MakeLower();
    _assetLib.BaseFilePath = Path::MakeAbsolutePath(data_file);
    return kAssetNoError;
}

AssetInfo *AssetManager::FindAssetByFileName(const String &asset_name) const
{
    for (size_t i = 0; i < _assetLib.AssetInfos.size(); ++i)
    {
        if (_assetLib.AssetInfos[i].FileName.CompareNoCase(asset_name) == 0)
        {
            return &_assetLib.AssetInfos[i];
        }
    }
    return nullptr;
}

String AssetManager::MakeLibraryFileNameForAsset(const AssetInfo *asset) const
{
    // deduce asset library file containing this asset
    return String::FromFormat("%s/%s",_basePath.GetCStr(), _assetLib.LibFileNames[asset->LibUid].GetCStr());
}

bool AssetManager::GetAssetFromLib(const String &asset_name, AssetLocation &loc, FileOpenMode open_mode, FileWorkMode work_mode) const
{
    if (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read)
        return false; // creating/writing is allowed only for common files on disk

    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (!asset)
        return false; // asset not found

    String libfile = cbuf_to_string_and_free( ci_find_file(nullptr, MakeLibraryFileNameForAsset(asset)) );
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

Stream *AssetManager::OpenAsset(const String &asset_name, FileOpenMode open_mode, FileWorkMode work_mode)
{
    AssetLocation loc;
    if (GetAssetByPriority(asset_name, loc, open_mode, work_mode))
    {
        Stream *s = File::OpenFile(loc.FileName, open_mode, work_mode);
        if (s)
        {
            s->Seek(loc.Offset, kSeekBegin);
            _lastAssetSize = loc.Size;
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
