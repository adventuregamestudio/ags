
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
struct MultiFileLibNew;

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
    void fgetnulltermstring(char *sss, DataStream *ci_s, int bufsize);
    void fread_data_enc(void *data, int dataSize, int dataCount, DataStream *ci_s);
    void fgetstring_enc(char *sss, DataStream *ci_s, int maxLength);
    int getw_enc(DataStream *ci_s);
    int read_new_new_enc_format_clib(MultiFileLibNew * mfl, DataStream *ci_s, int libver);
    int read_new_new_format_clib(MultiFileLibNew * mfl, DataStream *ci_s, int libver);
    int read_new_format_clib(MultiFileLib * mfl, DataStream *ci_s, int libver);
    int csetlib(const char *namm, const char *passw);
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
    AssetsSearchPriority    _searchPriority;
    String                  _currentDataFile;

    Clib32Info              &_clib32Info;
    MultiFileLibNew         &_mflib;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_EE_GAME__ASSETMANAGER_H
