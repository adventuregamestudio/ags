//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
// TODO: support streams that work on a file subsection, limited by size,
// to avoid having to return an asset size separately from a stream.
// TODO: return stream as smart pointer.
//
//=============================================================================
#ifndef __AGS_CN_CORE__ASSETMANAGER_H
#define __AGS_CN_CORE__ASSETMANAGER_H

#include <memory>
#include <functional>
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
    kAssetPriorityDir,
    kAssetPriorityLib
};

enum AssetError
{
    kAssetNoError           =  0,
    kAssetErrNoLibFile      = -1, // library file not found or can't be read
    kAssetErrLibParse       = -2, // bad library file format or read error
    kAssetErrNoManager      = -6, // asset manager not initialized
};

// AssetPath combines asset name and optional library filter, that serves to narrow down the search
struct AssetPath
{
    String Name;
    String Filter;

    AssetPath(const String &name = "", const String &filter = "") : Name(name), Filter(filter) {}
};


class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // Test if given file is main data file
    static bool         IsDataFile(const String &data_file);
    // Read data file table of contents into provided struct
    static AssetError   ReadDataFileTOC(const String &data_file, AssetLibInfo &lib);

    // Sets asset search priority (in which order manager will search available locations)
    void         SetSearchPriority(AssetSearchPriority priority);
    // Gets current asset search priority
    AssetSearchPriority GetSearchPriority() const;

    // Add library location to the list of asset locations
    AssetError   AddLibrary(const String &path, const AssetLibInfo **lib = nullptr);
    // Add library location, specifying comma-separated list of filters;
    // if library was already added before, this method will overwrite the filters only
    AssetError   AddLibrary(const String &path, const String &filters, const AssetLibInfo **lib = nullptr);
    // Remove library location from the list of asset locations
    void         RemoveLibrary(const String &path);
    // Removes all libraries
    void         RemoveAllLibraries();

    size_t       GetLibraryCount() const;
    const AssetLibInfo *GetLibraryInfo(size_t index) const;
    // Tells whether asset exists in any of the registered search locations
    bool         DoesAssetExist(const String &asset_name, const String &filter = "") const;
    inline bool  DoesAssetExist(const AssetPath &apath) const { return DoesAssetExist(apath.Name, apath.Filter); }
    // Searches in all the registered locations and collects a list of
    // assets using given wildcard pattern
    // TODO: variant accepting std::regex instead of wildcard, and replace uses where convenient
    void         FindAssets(std::vector<String> &assets, const String &wildcard,
                                   const String &filter = "") const;
    // Open asset stream in the given work mode; returns null if asset is not found or cannot be opened
    // This method only searches in libraries that do not have any defined filters
    Stream      *OpenAsset(const String &asset_name) const;
    // Open asset stream, providing a single filter to search in matching libraries
    Stream      *OpenAsset(const String &asset_name, const String &filter) const;
    inline Stream *OpenAsset(const AssetPath &apath) const { return OpenAsset(apath.Name, apath.Filter); }

private:
    // AssetLibEx combines library info with extended internal data required for the manager
    struct AssetLibEx : AssetLibInfo
    {
        std::vector<String> Filters; // asset filters this library is matching to
        std::vector<String> RealLibFiles; // fixed up library filenames

        bool TestFilter(const String &filter) const;
    };

    // Loads library and registers its contents into the cache
    AssetError  RegisterAssetLib(const String &path, AssetLibEx *&lib);

    // Tries to find asset in the given location, and then opens a stream for reading
    Stream     *OpenAssetFromLib(const AssetLibEx *lib, const String &asset_name) const;
    Stream     *OpenAssetFromDir(const AssetLibEx *lib, const String &asset_name) const;

    std::vector<std::unique_ptr<AssetLibEx>> _libs;
    std::vector<AssetLibEx*> _activeLibs;
    AssetSearchPriority _libsPriority = kAssetPriorityDir;
    // Sorting function, depends on priority setting
    std::function<bool(const AssetLibInfo*, const AssetLibInfo*)> _libsSorter;
};


String GetAssetErrorText(AssetError err);

//
// Global AssetManager instance.
// TODO: Move elsewhere, e.g. make a member of global object list or Engine instance at some point
extern std::unique_ptr<AssetManager> AssetMgr;

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_CORE__ASSETMANAGER_H
