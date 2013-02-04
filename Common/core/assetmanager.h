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
//
// CLIB32 - DJGPP implemention of the CLIB reader.
//
// 22/12/02 - Shawn's Linux changes approved and integrated - CJ
//
// v1.2 (Apr'01)  added support for new multi-file CLIB version 10 files
// v1.1 (Jul'99)  added support for appended-to-exe data files
//
//=============================================================================

#ifndef __AGS_CN_CORE__ASSETMANAGER_H
#define __AGS_CN_CORE__ASSETMANAGER_H

#include "util/file.h"

namespace AGS
{
namespace Common
{

class Stream;
struct MultiFileLib;
struct AssetLibInfo;
struct AssetInfo;

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
    kAssetErrNoLibSig       = -2, // library signature does not match
    kAssetErrLibVersion     = -3, // library version unsupported
    kAssetErrNoLibBase      = -4, // file is not library base (head)
    kAssetErrLibAssetCount  = -5, // too many assets in library
    kAssetErrNoManager      = -6, // asset manager not initialized
};

class AssetManager
{
public:
    static bool     CreateInstance();
    static void     DestroyInstance();
    ~AssetManager();

    static bool     SetSearchPriority(AssetSearchPriority priority);
    static AssetSearchPriority GetSearchPriority();

    static bool         IsDataFile(const String &data_file);

    // NOTE: this group of methods are only temporarily public
    static AssetError   SetDataFile(const String &data_file);
    static String       GetLibraryBaseFile();
    static int          GetAssetCount();
    static String       GetLibraryForAsset(const String &asset_name);
    static String       GetAssetFileByIndex(int index);
    static long         GetAssetOffset(const String &asset_name);
    static long         GetAssetSize(const String &asset_name);
    static long         GetLastAssetSize();    
    
    static void         InitPseudoRand(int seed);
    static int          GetNextPseudoRand();


    static bool         DoesAssetExist(const String &asset_name);
    static Stream       *OpenAsset(const String &asset_name,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read);
    static Stream       *OpenAsset(const String &data_file, const String &asset_name,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read);

private:
    AssetManager();

    bool        _SetSearchPriority(AssetSearchPriority priority);
    AssetSearchPriority _GetSearchPriority();
    bool        _IsDataFile(const String &data_file);
    AssetError  _SetDataFile(const String &data_file);
    String      _GetLibraryBaseFile();
    int         _GetAssetCount();    
    String      _GetLibraryForAsset(const String &asset_name);
    String      _GetAssetFileByIndex(int index);
    long        _GetAssetOffset(const String &asset_name);
    long        _GetAssetSize(const String &asset_name);
    long        _GetLastAssetSize();
    
    void        _InitPseudoRand(int seed);
    int         _GetNextPseudoRand();

    AssetError  RegisterAssetLib(const String &data_file, const String &password);

    AssetError  ReadSingleFileAssetLib(MultiFileLib * mfl, Stream *ci_s, int lib_version);
    AssetError  ReadMultiFileAssetLib(MultiFileLib * mfl, Stream *ci_s, int lib_version);
    AssetError  ReadAssetLibV10(MultiFileLib * mfl, Stream *ci_s, int lib_version);
    AssetError  ReadAssetLibV20(MultiFileLib * mfl, Stream *ci_s, int lib_version);
    AssetError  ReadAssetLibV21(MultiFileLib * mfl, Stream *ci_s, int lib_version);

    bool        _DoesAssetExist(const String &asset_name);
    Stream      *_OpenAsset(const String &asset_name,
        FileOpenMode open_mode = kFile_Open,
        FileWorkMode work_mode = kFile_Read);

    AssetInfo   *FindAssetByFileName(const String &asset_name);
    String      MakeLibraryFileNameForAsset(const AssetInfo *asset);
    Stream      *OpenAssetFromLib(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
    Stream      *OpenAssetFromDir(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
    Stream      *OpenAssetByPriority(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);

    // Decryption routines
    void        ReadEncArray(void *data, int dataSize, int dataCount, Stream *ci_s);
    int32_t     ReadEncInt32(Stream *ci_s);
    void        ReadEncString(char *buffer, int maxLength, Stream *ci_s);
    void        DecryptText(char *text, int length);

    static AssetManager     *_theAssetManager;
    AssetSearchPriority     _searchPriority;

    AssetLibInfo            &_assetLib;
    long                    _lastAssetSize;     // size of asset that was opened last time

    static const String     _libHeadSig;
    static const String     _libTailSig;
    static const int        _encryptionRandSeed;
    static const String     _encryptionString;
    int                     _lastRandValue;     // to remember last used random number during decryption
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_CORE__ASSETMANAGER_H
