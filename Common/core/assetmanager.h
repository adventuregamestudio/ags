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
// TODO: because Stream is now just a wrapper over IStreamBase,
// there are two *alternate* changes that we might consider for the future:
// 1. Variant 1 - return not a unique_ptr<Stream>, but a plain moveable
//    Stream object instead. This would require altering alot  of code
//    throughout the engine though, where Stream is accessed as a ptr.
// 2. Variant 2 - return unique_ptr<IStreamBase>. This means getting a more
//    primitive object, which may be wrapped into Stream only where necessary.
//    This would require minor code adjustments, but may be less convenient
//    in general use (?).
// Same changes could be done for File helpers (OpenFile etc).
//
//=============================================================================
#ifndef __AGS_CN_CORE__ASSETMANAGER_H
#define __AGS_CN_CORE__ASSETMANAGER_H

#include <functional>
#include <memory>
#include <unordered_map>
#include "core/asset.h"
#include "util/directory.h"
#include "util/stream.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

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

    operator bool() const { return !Name.IsEmpty(); }
};

// AssetLibEntry describes AssetLibrary registered in the AssetManager,
// and the filters applied to that library
struct AssetLibEntry
{
    bool IsDirectory = false;
    String Path; // path to the asset library (either dir or head library file)
    std::vector<String> LibFiles; // registered library filenames
    String Filters; // filter string this library is matching

    AssetLibEntry() = default;
    AssetLibEntry(bool is_dir, const String &path, const std::vector<String> &files, const String filters)
        : IsDirectory(is_dir), Path(path), LibFiles(files), Filters(filters) {}
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

    // Add library location to the list of asset locations
    AssetError   AddLibrary(const String &path, const AssetLibInfo **lib = nullptr);
    // Add library location, specifying comma-separated list of filters;
    // if library was already added before, this method will overwrite the filters only
    AssetError   AddLibrary(const String &path, const String &filters, const AssetLibInfo **lib = nullptr);
    // Remove library location from the list of asset locations
    void         RemoveLibrary(const String &path);
    // Removes all libraries
    void         RemoveAllLibraries();

    // Tells the number of registered Asset libraries
    size_t       GetLibraryCount() const;
    // Gets a basic info of a registered AssetLibrary, paired with its filters
    AssetLibEntry GetLibraryEntry(size_t index) const;
    // Gets full description of a AssetLibrary (including asset TOC)
    const AssetLibInfo *GetLibraryInfo(size_t index) const;
    // Tells whether asset exists in any of the registered search locations
    bool         DoesAssetExist(const String &asset_name, const String &filter = "") const;
    inline bool  DoesAssetExist(const AssetPath &apath) const { return DoesAssetExist(apath.Name, apath.Filter); }
    // Tries to get asset's "file time" (last modification time).
    // Note that for the assets packed within a CLIB format this will return library's time instead.
    bool         GetAssetTime(const String &asset_name, time_t &ft, const String &filter = "") const;
    // Searches in all the registered locations and collects a list of
    // assets using given wildcard pattern
    // TODO: variant accepting std::regex instead of wildcard, and replace uses where convenient
    void         FindAssets(std::vector<String> &assets, const String &wildcard,
                                   const String &filter = "") const;
    // Searches in all the registered locations and collects a list of
    // FileEntry objects corresponding to assets, using given wildcard pattern.
    // NOTE: lib file assets will have their time property equal to lib's time.
    void         FindAssets(std::vector<FileEntry> &assets, const String &wildcard,
                                   const String &filter = "") const;
    // Open asset stream in the given work mode; returns null if asset is not found or cannot be opened
    // This method only searches in libraries that do not have any defined filters
    std::unique_ptr<Stream> OpenAsset(const String &asset_name) const;
    // Open asset stream, providing a single filter to search in matching libraries
    std::unique_ptr<Stream> OpenAsset(const String &asset_name, const String &filter) const;
    inline std::unique_ptr<Stream> OpenAsset(const AssetPath &apath) const
        { return OpenAsset(apath.Name, apath.Filter); }

private:
    // AssetLibEx combines library info with extended internal data required for the manager
    struct AssetLibEx : AssetLibInfo
    {
        String FilterString; // filter string, as received on input (for diagnostic purposes)
        std::vector<String> Filters; // asset filters this library is matching to
        std::vector<String> RealLibFiles; // fixed up library filenames
        std::unordered_map<String, size_t, HashStrNoCase, StrEqNoCase> Lookup; // name to index asset lookup

        bool TestFilter(const String &filter) const;
    };

    // Loads library and registers its contents into the cache
    AssetError  RegisterAssetLib(const String &path, AssetLibEx *&lib);

    // Tries to find asset in the given location, and then opens a stream for reading
    std::unique_ptr<Stream> OpenAssetFromLib(const AssetLibEx *lib, const String &asset_name) const;
    std::unique_ptr<Stream> OpenAssetFromDir(const AssetLibEx *lib, const String &asset_name) const;

    std::vector<std::unique_ptr<AssetLibEx>> _libs;
    std::vector<AssetLibEx*> _activeLibs;
    AssetSearchPriority _libsPriority = kAssetPriorityDir;
    // Sorting function, depends on priority setting
    std::function<bool(const AssetLibInfo*, const AssetLibInfo*)> _libsSorter;
};


String GetAssetErrorText(AssetError err);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_CORE__ASSETMANAGER_H
