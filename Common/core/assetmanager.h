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
//
// Asset manager class for reading and writing game resources.
//-----------------------------------------------------------------------------
//
// The code is based on CLIB32, by Chris Jones (1998-99), DJGPP implementation
// of the CLIB reader.
//
//-----------------------------------------------------------------------------
// TODO: consider replace/merge with PhysFS library in the future.
//
// TODO:
// Ideally AssetManager should take care of enumerating all existing data
// packages and all files in them, while the game itself should not know where
// it receives the data from.
// Files should be registered based on their source package and their types.
// The user must not have access to this information, but is allowed to query
// all files of certain type (perhaps, filtered and ordered by their id).
//
// TODO: support streams that work on a file subsection, limited by size,
// to avoid having to return an asset size separately from a stream.
// TODO: return stream as smart pointer.
//
//=============================================================================
#ifndef __AGS_CN_CORE__ASSETMANAGER_H
#define __AGS_CN_CORE__ASSETMANAGER_H

#include <memory>
#include "core/asset.h"
#include "util/file.h" // TODO: extract filestream mode constants or introduce generic ones

namespace AGS
{
namespace Common
{

class Stream;
struct MultiFileLib;

enum AssetSearchPriority
{
    // TODO: rename this to something more obvious
    kAssetPriorityUndefined,
    kAssetPriorityLib,
    kAssetPriorityDir
};

enum AssetError
{
    kAssetNoError           =  0,
    kAssetErrNoLibFile      = -1, // library file not found or can't be read
    kAssetErrLibParse       = -2, // bad library file format or read error
    kAssetErrNoManager      = -6, // asset manager not initialized
};

// Explicit location of asset data
struct AssetLocation
{
    String      FileName;   // file where asset is located
    soff_t      Offset;     // asset's position in file (in bytes)
    soff_t      Size;       // asset's size (in bytes)

    AssetLocation();
};


class AssetManager
{
public:
    AssetManager();
    ~AssetManager() = default;

    // Test if given file is main data file
    static bool         IsDataFile(const String &data_file);
    // Read data file table of contents into provided struct
    static AssetError   ReadDataFileTOC(const String &data_file, AssetLibInfo &lib);

    // Sets asset search priority (in which order manager will search available locations)
    void         SetSearchPriority(AssetSearchPriority priority);
    // Gets current asset search priority
    AssetSearchPriority GetSearchPriority() const;

    // Add library file to the list of asset locations
    AssetError   AddLibrary(const String &path);
    // Remove library file from the list of asset locations
    void         RemoveLibrary(const String &path);
    // Removes all libraries
    void         RemoveAllLibraries();
    // Includes or excludes particular library in/from asset search while keeping its TOC registered
    void         SetLibrarySearch(const String &path, bool on);

    size_t       GetLibraryCount() const;
    const AssetLibInfo *GetLibraryInfo(size_t index) const;
    bool         DoesAssetExist(const String &asset_name) const;
    // TODO: review this function later;
    // this is a workaround that lets us use back-end specific kind of streams
    // to read the asset data. This is not ideal, because it limits us to reading from file.
    // The better solution could be returning a kind of "io device" object
    // which may be used to initialize both AGS and back-end compatible stream wrappers.
    bool         GetAssetLocation(const String &asset_name, AssetLocation &loc) const;
    Stream      *OpenAsset(const String &asset_name, soff_t *asset_size = nullptr,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read) const;

private:
    // Loads library and registers its contents into the cache
    AssetError  RegisterAssetLib(const String &path, AssetLibInfo *&lib);

    // Searchs for asset specifically in the registered lib TOCs
    bool        FindAssetInLibs(const String &asset_name, const AssetInfo **asset, const AssetLibInfo **lib) const;
    // Tries to find asset in libs, tests if it's possible to open, and fills in AssetLocation
    bool        GetAssetFromLib(const String &asset_name, AssetLocation &loc, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode) const;
    bool        GetAssetFromDir(const String &asset_name, AssetLocation &loc, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode) const;
    bool        GetAssetByPriority(const String &asset_name, AssetLocation &loc, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode) const;

    AssetSearchPriority     _searchPriority;

    std::vector<std::unique_ptr<AssetLibInfo>> _libs;
    std::vector<AssetLibInfo*> _activeLibs;
};


String GetAssetErrorText(AssetError err);

//
// Global AssetManager instance.
// TODO: Move elsewhere, e.g. make a member of global object list or Engine instance at some point
extern std::unique_ptr<AssetManager> AssetMgr;

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_CORE__ASSETMANAGER_H
