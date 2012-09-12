
//=============================================================================
//
// Asset manager class fore reading and writing game resources
// Asset manager is a singleton
//
//-----------------------------------------------------------------------------
//
// [IKM] 2012-09-11: I am planning to complete AssetManager class in a number
// of steps. First step will be to write a primitive wrapper over clib32
// library. Then hide most of its functions making them private.
// Ideally AssetManager should take care of enumerating all existing data
// packages and all files in them, while the game itself should not know where
// it receives the data from.
// Files should be registered based on their source package and their types.
// The user must not have access to this information, but is allowed to query
// all files of certain type (perhaps, filtered and ordered by their id).
//
//=============================================================================
#ifndef __AGS_CN_GAME__ASSETMANAGER_H
#define __AGS_CN_GAME__ASSETMANAGER_H

#include "util/file.h"

namespace AGS
{
namespace Common
{

class DataStream;

enum AssetsSearchPriority
{
    // TODO: rename this to something more obvious
    kAssetPriority_Undefined,
    kAssetPriority_Data,
    kAssetPriority_File
};

class AssetManager
{
public:
    static bool     CreateInstance();
    static void     DestroyInstance();
    ~AssetManager();

    static bool     SetSearchPriority(AssetsSearchPriority priority);
    static AssetsSearchPriority GetSearchPriority();

    // NOTE: this group of methods are only temporarily public
    // Return value is error code from clib32 -- for now
    static int      SetDataFile(const String &data_file);
    static String   GetAssetFilePath(const String &asset_file);
    static long     GetAssetOffset(const String &asset_file);
    static long     GetAssetSize(const String &asset_file);
    static long     GetLastAssetSize();
    static int      GetAssetCount();
    static String   GetAssetFileByIndex(int index);
    static String   GetOriginalDataFile();


    static DataStream *OpenAsset(const String &asset_file,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read);
    static DataStream *OpenAsset(const String &data_file, const String &asset_file,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read);

private:
    AssetManager();

    static AssetManager     *_theAssetManager;
    AssetsSearchPriority    _searchPriority;
    String                  _currentDataFile;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_EE_GAME__ASSETMANAGER_H
