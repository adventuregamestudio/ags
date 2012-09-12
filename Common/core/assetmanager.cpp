
#include "core/assetmanager.h"
#include "debug/assert.h"
#include "util/datastream.h"

extern "C"
{
    int csetlib(const char *namm, const char *passw);
    char *clibgetdatafile(const char *fill);
    AGS::Common::DataStream *clibfopen(const char *filnamm,
        AGS::Common::FileOpenMode open_mode, AGS::Common::FileWorkMode work_mode);
    long cliboffset(const char *);
    long clibfilesize(const char *);
    extern long last_opened_size;
    int clibGetNumFiles();
    const char *clibGetFileName(int index);
    extern int cfopenpriority;
    const char *clibgetoriginalfilename();
}

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
    _theAssetManager->SetSearchPriority(kAssetPriority_File);
    return _theAssetManager != NULL; // well, we should return _something_
}

/* static */ void AssetManager::DestroyInstance()
{
    delete _theAssetManager;
    _theAssetManager = NULL;
}

AssetManager::~AssetManager()
{
}

#define PR_DATAFIRST 1
#define PR_FILEFIRST 2

/* static */ bool AssetManager::SetSearchPriority(AssetsSearchPriority priority)
{
    assert(_theAssetManager != NULL);
    if (_theAssetManager)
    {
        _theAssetManager->_searchPriority = priority;
        switch (_theAssetManager->_searchPriority)
        {
        case kAssetPriority_Data:
            cfopenpriority = PR_DATAFIRST;
            break;
        case kAssetPriority_File:
            cfopenpriority = PR_FILEFIRST;
            break;
        default:
            return false;
        }
        return true;
    }
    return false;
}

/* static */ AssetsSearchPriority AssetManager::GetSearchPriority()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_searchPriority : kAssetPriority_Undefined;
}

/* static */ int AssetManager::SetDataFile(const String &data_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ?
        csetlib(data_file, "") : -1; // NOTE: passwords should be kept in AssetManager
}

/* static */ String AssetManager::GetAssetFilePath(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return clibgetdatafile(asset_file);
}

/* static */ long AssetManager::GetAssetOffset(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? cliboffset(asset_file) : 0;
}

/* static */ long AssetManager::GetAssetSize(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? clibfilesize(asset_file) : 0;
}

/* static */ long AssetManager::GetLastAssetSize()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? last_opened_size : 0;
}

/* static */ int AssetManager::GetAssetCount()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? clibGetNumFiles() : 0;
}

/* static */ String AssetManager::GetAssetFileByIndex(int index)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? clibGetFileName(index) : "";
}

/* static */ String AssetManager::GetOriginalDataFile()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? clibgetoriginalfilename() : "";
}

/* static */ DataStream *AssetManager::OpenAsset(const String &asset_file,
                                                  FileOpenMode open_mode,
                                                  FileWorkMode work_mode)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    return clibfopen(asset_file, open_mode, work_mode);
}

/* static */ DataStream *AssetManager::OpenAsset(const String &data_file,
                                                  const String &asset_file,
                                                  FileOpenMode open_mode,
                                                  FileWorkMode work_mode)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    if (_theAssetManager->_currentDataFile.Compare(data_file) != 0)
    {
        if (AssetManager::SetDataFile(data_file) != 0)
        {
            return NULL;
        }
        _theAssetManager->_currentDataFile = data_file;
    }
    return clibfopen(asset_file, open_mode, work_mode);
}

AssetManager::AssetManager()
{
}

} // namespace AGS
} // namespace Common
