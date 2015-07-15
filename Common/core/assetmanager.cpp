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

#include <string.h>
#include <stdlib.h>
#include "core/asset.h"
#include "core/assetmanager.h"
#include "debug/assert.h"
#include "util/multifilelib.h"
#include "util/stream.h"
#include "util/misc.h"


namespace AGS
{
namespace Common
{

AssetManager *AssetManager::_theAssetManager = NULL;

/* static */ bool AssetManager::CreateInstance()
{
    // Issue a warning - recreating asset manager is not a normal behavior
    assert(_theAssetManager == NULL);
    delete _theAssetManager;
    _theAssetManager = new AssetManager();
    _theAssetManager->SetSearchPriority(kAssetPriorityDir);
    return _theAssetManager != NULL; // well, we should return _something_
}

/* static */ void AssetManager::DestroyInstance()
{
    delete _theAssetManager;
    _theAssetManager = NULL;
}

AssetManager::~AssetManager()
{
    delete &_assetLib;
}

/* static */ bool AssetManager::SetSearchPriority(AssetSearchPriority priority)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_SetSearchPriority(priority) : false;
}

/* static */ AssetSearchPriority AssetManager::GetSearchPriority()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetSearchPriority() : kAssetPriorityUndefined;
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

AssetError AssetManager::ReadDataFileTOC(const String &data_file, AssetLibInfo &lib)
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

/* static */ AssetError AssetManager::SetDataFile(const String &data_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_SetDataFile(data_file) : kAssetErrNoManager;
}

/* static */ String AssetManager::GetLibraryForAsset(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetLibraryForAsset(asset_name) : "";
}

/* static */ long AssetManager::GetAssetOffset(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetOffset(asset_name) : 0;
}

/* static */ long AssetManager::GetAssetSize(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetSize(asset_name) : 0;
}

/* static */ long AssetManager::GetLastAssetSize()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetLastAssetSize() : 0;
}

/* static */ int AssetManager::GetAssetCount()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetCount() : 0;
}

/* static */ String AssetManager::GetAssetFileByIndex(int index)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetFileByIndex(index) : "";
}

/* static */ String AssetManager::GetLibraryBaseFile()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetLibraryBaseFile() : "";
}

/* static */ bool AssetManager::DoesAssetExist(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    return _theAssetManager->_DoesAssetExist(asset_name);
}

/* static */ Stream *AssetManager::OpenAsset(const String &asset_name,
                                                  FileOpenMode open_mode,
                                                  FileWorkMode work_mode)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    return _theAssetManager->_OpenAsset(asset_name, open_mode, work_mode);
}

/* static */ Stream *AssetManager::OpenAsset(const String &data_file,
                                                  const String &asset_name,
                                                  FileOpenMode open_mode,
                                                  FileWorkMode work_mode)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    if (_theAssetManager->_SetDataFile(data_file) != 0)
    {
        return NULL;
    }
    return _theAssetManager->_OpenAsset(asset_name, open_mode, work_mode);
}

AssetManager::AssetManager()
    : _assetLib(*new AssetLibInfo())
{
}

bool AssetManager::_SetSearchPriority(AssetSearchPriority priority)
{
    _searchPriority = priority;
    return true;
}

AssetSearchPriority AssetManager::_GetSearchPriority()
{
    return _searchPriority;
}

AssetError AssetManager::_SetDataFile(const String &data_file)
{
    if (data_file.IsEmpty())
    {
        return kAssetErrNoLibFile;
    }
    if (_assetLib.BaseFileName.Compare(data_file) == 0)
    {
        return kAssetNoError;
    }
    AssetError err = RegisterAssetLib(data_file, "");
    return err;
}

String AssetManager::_GetLibraryForAsset(const String &asset_name)
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

long AssetManager::_GetAssetOffset(const String &asset_name)
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

long AssetManager::_GetAssetSize(const String &asset_name)
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

long AssetManager::_GetLastAssetSize()
{
    return _lastAssetSize;
}

int AssetManager::_GetAssetCount()
{
    return _assetLib.AssetInfos.size();
}

String AssetManager::_GetAssetFileByIndex(int index)
{
    if ((index < 0) || ((size_t)index >= _assetLib.AssetInfos.size()))
        return NULL;

    return _assetLib.AssetInfos[index].FileName;
}

String AssetManager::_GetLibraryBaseFile()
{
    return _assetLib.BaseFileName;
}

bool AssetManager::_DoesAssetExist(const String &asset_name)
{
    return FindAssetByFileName(asset_name) != NULL ||
        File::TestReadFile(asset_name);
}

Stream *AssetManager::_OpenAsset(const String &asset_name,
                       FileOpenMode open_mode,
                       FileWorkMode work_mode)
{
    return OpenAssetByPriority(asset_name, open_mode, work_mode);
}

AssetError AssetManager::RegisterAssetLib(const String &data_file, const String &password)
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
    // make a backup of the original file name
    _assetLib.BaseFileName = _assetLib.LibFileNames[0];
    _assetLib.BaseFileName.MakeLower();
    return kAssetNoError;
}

AssetInfo *AssetManager::FindAssetByFileName(const String &asset_name)
{
    for (size_t i = 0; i < _assetLib.AssetInfos.size(); ++i)
    {
        if (_assetLib.AssetInfos[i].FileName.CompareNoCase(asset_name) == 0)
        {
            return &_assetLib.AssetInfos[i];
        }
    }
    return NULL;
}

String AssetManager::MakeLibraryFileNameForAsset(const AssetInfo *asset)
{
    // deduce asset library file containing this asset
    return String::FromFormat("%s/%s",_basePath.GetCStr(), _assetLib.LibFileNames[asset->LibUid].GetCStr());
}

Stream *AssetManager::OpenAssetFromLib(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    // creating/writing is allowed only for common files on disk
    if (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read)
    {
        return NULL;
    }

    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (!asset)
    {
        // asset not found
        return NULL;
    }

    String lib_filename = MakeLibraryFileNameForAsset(asset);
    // open library datafile
    Stream *lib_s = ci_fopen(lib_filename, open_mode, work_mode);
    if (lib_s)
    {
        // set stream ptr at the beginning of wanted section
        lib_s->Seek(asset->Offset, kSeekBegin);
        // remember size of opened asset
        _lastAssetSize = asset->Size;
    }
    return lib_s;
}

Stream *AssetManager::OpenAssetFromDir(const String &file_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    Stream *asset_s = ci_fopen(file_name, open_mode, work_mode);
    if (asset_s)
    {
        // remember size of opened file
        _lastAssetSize = asset_s->GetLength();
    }
    return asset_s;
}

Stream *AssetManager::OpenAssetByPriority(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    Stream *asset_s = NULL;
    if (_searchPriority == kAssetPriorityDir)
    {
        // check for disk, otherwise use datafile
        asset_s = OpenAssetFromDir(asset_name, open_mode, work_mode);
        if (!asset_s)
        {
            asset_s = OpenAssetFromLib(asset_name, open_mode, work_mode);
        }
    }
    else if (_searchPriority == kAssetPriorityLib)
    {
        // check datafile first, then scan directory
        asset_s = OpenAssetFromLib(asset_name, open_mode, work_mode);
        if (!asset_s)
        {
            asset_s = OpenAssetFromDir(asset_name, open_mode, work_mode);
        }
    }
    return asset_s;
}

} // namespace Common
} // namespace AGS
