
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
struct Clib32Info;
struct MultiFileLib;

enum AssetSearchPriority
{
    // TODO: rename this to something more obvious
    kAssetPriorityUndefined,
    kAssetPriorityData,
    kAssetPriorityFile
};

enum AssetError
{
    kAssetNoError           =  0,
    kAssetErrNoLibFile      = -1, // library file not found or can't be read
    kAssetErrNoLibSig       = -2, // library signature does not match
    kAssetErrLibVersion     = -3, // library version unsupported
    kAssetErrNoLibBase      = -4, // file is not library base (head)
    kAssetErrLibAssetCount  = -5, // too many assets in library
};

class AssetManager
{
public:
    static bool     CreateInstance();
    static void     DestroyInstance();
    ~AssetManager();

    static bool     SetSearchPriority(AssetSearchPriority priority);
    static AssetSearchPriority GetSearchPriority();

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
    static void     InitPseudoRand(int seed);
    static int      GetNextPseudoRand();


    static DataStream *OpenAsset(const String &asset_file,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read);
    static DataStream *OpenAsset(const String &data_file, const String &asset_file,
                                   FileOpenMode open_mode = kFile_Open,
                                   FileWorkMode work_mode = kFile_Read);

private:
    AssetManager();

    //=========================================================================
    // Former clib32 functions
    void init_pseudo_rand_gen(int seed);
    int get_pseudo_rand();
    void clib_decrypt_text(char *toenc);
    void fread_data_enc(void *data, int dataSize, int dataCount, DataStream *ci_s);
    void fgetstring_enc(char *sss, DataStream *ci_s, int maxLength);
    int getw_enc(DataStream *ci_s);
    AssetError read_ver21plus_enc_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version);
    AssetError read_ver20_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version);
    AssetError read_ver10to19_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version);
    AssetError read_ver10plus_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version);
    AssetError read_ver1to9_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version);
    AssetError csetlib(const char *namm, const char *passw);
    int clibGetNumFiles();
    const char *clibGetFileName(int index);
    int clibfindindex(const char *fill);
    long clibfilesize(const char *fill);
    long cliboffset(const char *fill);
    const char *clibgetoriginalfilename();
    char *clibgetdatafile(const char *fill);
    DataStream *clibopenfile(const char *filly, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
    DataStream *clibfopen(const char *filnamm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
    //=========================================================================

    static AssetManager     *_theAssetManager;
    AssetSearchPriority     _searchPriority;
    String                  _currentDataFile;

    Clib32Info              &_clib32Info;
    MultiFileLib            &_mflib;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_EE_GAME__ASSETMANAGER_H
