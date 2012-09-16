
#include <string.h>
#include <stdlib.h>
#include "core/assetmanager.h"
#include "debug/assert.h"
#include "util/datastream.h"

#if defined(ANDROID_VERSION) || defined(IOS_VERSION)
#include <sys/stat.h>
#endif

#if defined(LINUX_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <io.h>
#else
//#include "djcompat.h"
#include "allegro.h"
#endif
#include "util/misc.h"

#ifdef MAC_VERSION
#include "macport.h"
#include <sys/stat.h>
#endif

#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

#define MAX_FILES 10000
#define MAXMULTIFILES 25

#define PR_DATAFIRST 1
#define PR_FILEFIRST 2

// Information on single asset
struct AssetInfo
{
    // A pair of filename and libuid is assumed to be unique in game scope
    String      FileName;   // filename associated with asset
    int32_t     LibUid;     // uid of library, containing this asset
    int         Offset;     // asset's position in library file (in bytes)
    int         Size;       // asset's size (in bytes)
};

// Information on multifile asset library
struct AssetLibInfo
{
    int         PartCount;  // number of parts this library is split to
    String      LibFileNames[MAXMULTIFILES]; // filename for each library part

    // Library contents
    int         AssetCount; // total number of assets in library
    AssetInfo   AssetInfos[MAX_FILES]; // information on contained assets

    void AssignFromMFL(const MultiFileLib &mflib);
};

struct Clib32Info
{
    static const char *clib32copyright;
    static const char *clibendfilesig;
    static const char *clibpasswencstring;
    static const int RAND_SEED_SALT;

    char lib_file_name[255]; // clib filename
    char base_path[255]; // clib's path
    char original_base_filename[255]; // backup of clib's base filename
    char actfilename[250]; // stores clib's part filename which contains required file

    int _last_rand; // to remember last used random number during decryption
    long last_opened_size; // size of file that was opened last time
    int cfopenpriority; // file search priority (either clib, or directory)
};

const char *Clib32Info::clib32copyright = "CLIB32 v1.21 (c) 1995,1996,1998,2001,2007 Chris Jones";
const int Clib32Info::RAND_SEED_SALT = 9338638;  // must update editor agsnative.cpp if this changes
const char *Clib32Info::clibendfilesig = "CLIB\x1\x2\x3\x4SIGE";
const char *Clib32Info::clibpasswencstring = "My\x1\xde\x4Jibzle";

struct MultiFileLib
{
  int num_data_files;                   // number of clib parts
  char data_filenames[MAXMULTIFILES][50]; // filename for each clib part
  // current clib contents
  int num_files;                        // total number of files
  char filenames[MAX_FILES][100];       // existing file names
  int offset[MAX_FILES];                // file offsets
  int length[MAX_FILES];                // file lengths
  char file_datafile[MAX_FILES];        // index of datafile for each file in split clib
};

void AssetLibInfo::AssignFromMFL(const MultiFileLib &mflib)
{
    PartCount = mflib.num_data_files;
    for (int i = 0; i < PartCount; ++i)
    {
        LibFileNames[i] = mflib.data_filenames[i];
    }
    AssetCount = mflib.num_files;
    for (int i = 0; i < AssetCount; ++i)
    {
        AssetInfos[i].FileName = mflib.filenames[i];
        AssetInfos[i].LibUid = mflib.file_datafile[i];
        AssetInfos[i].Offset = mflib.offset[i];
        AssetInfos[i].Size = mflib.length[i];
    }
}

AssetManager *AssetManager::_theAssetManager = NULL;

/* static */ bool AssetManager::CreateInstance()
{
    // Issue a warning - recreating asset manager is not a normal behavior
    assert(_theAssetManager == NULL);
    delete _theAssetManager;
    _theAssetManager = new AssetManager();
    _theAssetManager->SetSearchPriority(kAssetPriorityFile);
    return _theAssetManager != NULL; // well, we should return _something_
}

/* static */ void AssetManager::DestroyInstance()
{
    delete _theAssetManager;
    _theAssetManager = NULL;
}

AssetManager::~AssetManager()
{
    delete &_clib32Info;
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

/* static */ AssetError AssetManager::SetDataFile(const String &data_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_SetDataFile(data_file) : kAssetErrNoManager;
}

/* static */ String AssetManager::GetAssetFilePath(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetFilePath(asset_file) : "";
}

/* static */ long AssetManager::GetAssetOffset(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetOffset(asset_file) : 0;
}

/* static */ long AssetManager::GetAssetSize(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetSize(asset_file) : 0;
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

/* static */ String AssetManager::GetOriginalDataFile()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetOriginalDataFile() : "";
}

/* static */ void AssetManager::InitPseudoRand(int seed)
{
    assert(_theAssetManager != NULL);
    if (_theAssetManager)
    {
        _theAssetManager->_InitPseudoRand(seed);
    }
}

/* static */ int AssetManager::GetNextPseudoRand()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetNextPseudoRand() : 0;
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
    return _theAssetManager->clibfopen(asset_file, open_mode, work_mode);
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
    if (_theAssetManager->_SetDataFile(data_file) != 0)
    {
        return NULL;
    }
    return _theAssetManager->_OpenAsset(asset_file, open_mode, work_mode);
}

AssetManager::AssetManager()
    : _clib32Info(*new Clib32Info())
    , _assetLib(*new AssetLibInfo())
{
    strcpy(_clib32Info.lib_file_name, " ");
    strcpy(_clib32Info.base_path, ".");
}

bool AssetManager::_SetSearchPriority(AssetSearchPriority priority)
{
    _searchPriority = priority;
    switch (_searchPriority)
    {
    case kAssetPriorityData:
        _clib32Info.cfopenpriority = PR_DATAFIRST;
        break;
    case kAssetPriorityFile:
        _clib32Info.cfopenpriority = PR_FILEFIRST;
        break;
    default:
        return false;
    }
    return true;
}

AssetSearchPriority AssetManager::_GetSearchPriority()
{
    return _searchPriority;
}

AssetError AssetManager::_SetDataFile(const String &data_file)
{
    if (_currentDataFile.Compare(data_file) == 0)
    {
        return kAssetNoError;
    }
    AssetError err = csetlib(data_file, "");
    if (err == kAssetNoError)
    {
        _currentDataFile = data_file;
    }
    return err;
}

String AssetManager::_GetAssetFilePath(const String &asset_file)
{
    return clibgetdatafile(asset_file);
}

long AssetManager::_GetAssetOffset(const String &asset_file)
{
    return cliboffset(asset_file);
}

long AssetManager::_GetAssetSize(const String &asset_file)
{
    return clibfilesize(asset_file);
}

long AssetManager::_GetLastAssetSize()
{
    return _clib32Info.last_opened_size;
}

int AssetManager::_GetAssetCount()
{
    return clibGetNumFiles();
}

String AssetManager::_GetAssetFileByIndex(int index)
{
    return clibGetFileName(index);
}

String AssetManager::_GetOriginalDataFile()
{
    return clibgetoriginalfilename();
}

void AssetManager::_InitPseudoRand(int seed)
{
    init_pseudo_rand_gen(seed);
}

int AssetManager::_GetNextPseudoRand()
{
    return get_pseudo_rand();
}

DataStream *AssetManager::_OpenAsset(const String &asset_file,
                       FileOpenMode open_mode,
                       FileWorkMode work_mode)
{
    return clibfopen(asset_file, open_mode, work_mode);
}

//=============================================================================

/* CLIB32 - DJGPP implemention of the CLIB reader.
(c) 1998-99 Chris Jones

22/12/02 - Shawn's Linux changes approved and integrated - CJ

v1.2 (Apr'01)  added support for new multi-file CLIB version 10 files
v1.1 (Jul'99)  added support for appended-to-exe data files

This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

void AssetManager::init_pseudo_rand_gen(int seed)
{
    _clib32Info._last_rand = seed;
}

int AssetManager::get_pseudo_rand()
{
    return( ((_clib32Info._last_rand = _clib32Info._last_rand * 214013L
        + 2531011L) >> 16) & 0x7fff );
}

//-----------------------------------------------------------------------------
// read & decrypt procedures
//-----------------------------------------------------------------------------
void AssetManager::clib_decrypt_text(char *toenc)
{
    int adx = 0;

    while (1) {
        toenc[0] -= _clib32Info.clibpasswencstring[adx];
        if (toenc[0] == 0)
            break;

        adx++;
        toenc++;

        if (adx > 10)
            adx = 0;
    }
}

void AssetManager::fread_data_enc(void *data, int dataSize, int dataCount, DataStream *ci_s)
{
    ci_s->ReadArray(data, dataSize, dataCount);
    unsigned char *dataChar = (unsigned char*)data;
    for (int i = 0; i < dataSize * dataCount; i++)
    {
        dataChar[i] -= get_pseudo_rand();
    }
}

void AssetManager::fgetstring_enc(char *sss, DataStream *ci_s, int maxLength) 
{
    int i = 0;
    while ((i == 0) || (sss[i - 1] != 0))
    {
        sss[i] = ci_s->ReadInt8() - get_pseudo_rand();

        if (i < maxLength - 1)
            i++;
        else
        {
            // Avoid an endless loop
            break;
        }
    }
}

int AssetManager::getw_enc(DataStream *ci_s)
{
    int numberRead;
    fread_data_enc(&numberRead, 4, 1, ci_s);

#if defined(AGS_BIGENDIAN)
    AGS::Common::BitByteOperations::SwapBytesInt32(numberRead);
#endif

    return numberRead;
}

//-----------------------------------------------------------------------------
// reading clib info for versions 21+
// note: everything is decrypted here using pseudo-rand numbers
//-----------------------------------------------------------------------------
AssetError AssetManager::read_ver21plus_enc_format_clib(MultiFileLib * mfl, DataStream *ci_s, int libver)
{
    int i;

    // init randomizer
    int randSeed = ci_s->ReadInt32();
    init_pseudo_rand_gen(randSeed + _clib32Info.RAND_SEED_SALT);
    // number of clib parts
    mfl->num_data_files = getw_enc(ci_s);
    // filenames for all clib parts
    for (i = 0; i < mfl->num_data_files; i++)
    {
        fgetstring_enc(mfl->data_filenames[i], ci_s, 50);
    }
    // number of files in clib
    mfl->num_files = getw_enc(ci_s);

    if (mfl->num_files > MAX_FILES)
        return kAssetErrLibAssetCount; // too many files in clib, return error code

    // read information on clib contents
    for (i = 0; i < mfl->num_files; i++)
    {
        fgetstring_enc(mfl->filenames[i], ci_s, 100);
    }

    for (i = 0; i < mfl->num_files; i++)
    {
        mfl->offset[i] = getw_enc(ci_s);
    }

    for (i = 0; i < mfl->num_files; i++)
    {
        mfl->length[i] = getw_enc(ci_s);
    }

    fread_data_enc(&mfl->file_datafile[0], 1, mfl->num_files, ci_s);

    return kAssetNoError;
}

//-----------------------------------------------------------------------------
// reading clib info for version 20
//-----------------------------------------------------------------------------
AssetError AssetManager::read_ver20_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version)
{
    int i;

    // number of clib parts
    mfl->num_data_files = ci_s->ReadInt32();
    // filenames for all clib parts
    for (i = 0; i < mfl->num_data_files; i++)
    {
        fgetstring_limit(mfl->data_filenames[i], ci_s, 50);
    }
    // number of files in clib
    mfl->num_files = ci_s->ReadInt32();

    if (mfl->num_files > MAX_FILES)
        return kAssetErrLibAssetCount; // too many files in clib, return error code

    // read information on clib contents
    for (i = 0; i < mfl->num_files; i++)
    {
        short nameLength;
        nameLength = ci_s->ReadInt16();
        nameLength /= 5;
        ci_s->Read(mfl->filenames[i], nameLength);
        // decrypt filenames
        clib_decrypt_text(mfl->filenames[i]);
    }
    ci_s->ReadArrayOfInt32(&mfl->offset[0], mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->ReadArrayOfInt8((int8_t*)&mfl->file_datafile[0], mfl->num_files);
    return kAssetNoError;
}

//-----------------------------------------------------------------------------
// reading clib info for versions 10 to 19
//-----------------------------------------------------------------------------
AssetError AssetManager::read_ver10to19_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version)
{
    // number of clib parts
    mfl->num_data_files = ci_s->ReadInt32();
    // filenames for all clib parts
    ci_s->ReadArray(&mfl->data_filenames[0][0], 20, mfl->num_data_files);
    // number of files in clib
    mfl->num_files = ci_s->ReadInt32();

    if (mfl->num_files > MAX_FILES)
        return kAssetErrLibAssetCount; // too many files in clib, return error code

    // read information on clib contents
    ci_s->ReadArray(&mfl->filenames[0][0], 25, mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->offset[0], mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->ReadArrayOfInt8((int8_t*)&mfl->file_datafile[0], mfl->num_files);

    if (lib_version >= 11)
    {
        // decrypt filenames
        for (int i = 0; i < mfl->num_files; i++)
            clib_decrypt_text(mfl->filenames[i]);
    }
    return kAssetNoError;
}

//-----------------------------------------------------------------------------
// reading clib info for versions 10+ with support for multifile clib
//-----------------------------------------------------------------------------
AssetError AssetManager::read_ver10plus_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version)
{
    if (ci_s->ReadByte() != 0)
        return kAssetErrNoLibBase;  // not first datafile in chain

    if (lib_version >= 21)
    {
        // read new clib format with encoding support (versions 21+)
        AssetError err = read_ver21plus_enc_format_clib(mfl, ci_s, lib_version);
        if (err != kAssetNoError)
            return err;
    }
    else if (lib_version == 20)
    {
        // read new clib format without encoding support (version 20)
        AssetError err = read_ver20_format_clib(mfl, ci_s, lib_version);
        if (err != kAssetNoError)
            return err;
    }
    else 
    {
        // read older clib format (versions 10 to 19), the ones with shorter filenames
        AssetError err = read_ver10to19_format_clib(mfl, ci_s, lib_version);
        if (err != kAssetNoError)
            return err;
    }
    // return success
    return kAssetNoError;
}

//-----------------------------------------------------------------------------
// reading clib info for versions 1 to 9 (single file clib)
//-----------------------------------------------------------------------------
AssetError AssetManager::read_ver1to9_format_clib(MultiFileLib * mfl, DataStream *ci_s, int lib_version)
{
    int i;

    int passwmodifier = ci_s->ReadByte();
    ci_s->ReadInt8(); // unused byte
    mfl->num_data_files = 1;

    mfl->num_files = ci_s->ReadInt16();
    if (mfl->num_files > MAX_FILES)
        return kAssetErrLibAssetCount; // too many files in clib, return error code

    char clbuff[20];
    ci_s->Read(clbuff, 13);  // skip password dooberry
    // read information on contents
    for (i = 0; i < mfl->num_files; i++) {
        ci_s->Read(&mfl->filenames[i][0], 13); // CHECKME, is it really 13?
        for (int j = 0; j < (int)strlen(mfl->filenames[i]); j++)
            mfl->filenames[i][j] -= passwmodifier;
    }
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->Seek(Common::kSeekCurrent, 2 * mfl->num_files);  // skip flags & ratio

    mfl->offset[0] = ci_s->GetPosition();
    // set offsets (assets are positioned in sequence)
    for (i = 1; i < mfl->num_files; i++) {
        mfl->offset[i] = mfl->offset[i - 1] + mfl->length[i - 1];
        mfl->file_datafile[i] = 0;
    }
    mfl->file_datafile[0] = 0;

    // return success
    return kAssetNoError;
}

//-----------------------------------------------------------------------------
// csetlib: set active multifile library, read info about its contents
//-----------------------------------------------------------------------------
AssetError AssetManager::csetlib(const char *namm, const char *passw)
{
    // reset original base filename
    _clib32Info.original_base_filename[0] = 0;

    // if no lib name, reset current lib name and return
    if (namm == NULL) {
        _clib32Info.lib_file_name[0] = ' ';
        _clib32Info.lib_file_name[1] = 0;
        return kAssetNoError;
    }
    // base path is current directory
    strcpy(_clib32Info.base_path, ".");

    // open data library
    DataStream *ci_s = ci_fopen(namm, Common::kFile_Open, Common::kFile_Read);
    if (ci_s == NULL)
        return kAssetErrNoLibFile; // can't be opened, return error code

    long abs_offset = 0; // library offset in this file
    char clbuff[20];
    // check multifile lib signature at the beginning of file
    ci_s->Read(&clbuff[0], 5);
    if (strncmp(clbuff, "CLIB", 4) != 0) {

        // signature not found, check signature at the end of file
        ci_s->Seek(Common::kSeekEnd, -12);
        ci_s->Read(&clbuff[0], 12);

        // signature not found, return error code
        if (strncmp(clbuff, _clib32Info.clibendfilesig, 12) != 0)
            return kAssetErrNoLibSig;

        ci_s->Seek(Common::kSeekEnd, -16);  // it's an appended-to-end-of-exe thing
        int debug_pos2 = ci_s->GetPosition();
        // read multifile lib offset value
        abs_offset = ci_s->ReadInt32();
        ci_s->Seek(Common::kSeekBegin, abs_offset + 5);
    }

    // read library header
    int lib_version = ci_s->ReadByte();
    if ((lib_version != 6) && (lib_version != 10) &&
        (lib_version != 11) && (lib_version != 15) &&
        (lib_version != 20) && (lib_version != 21))
        return kAssetErrLibVersion;  // unsupported version

    const char *nammwas = namm;
    // remove slashes so that the lib name fits in the buffer
    while ((strchr(namm, '\\') != NULL) || (strchr(namm, '/') != NULL))
        namm++;

    if (namm != nammwas) {
        // store complete path
        strcpy(_clib32Info.base_path, nammwas);
        _clib32Info.base_path[namm - nammwas] = 0;
        if ((_clib32Info.base_path[strlen(_clib32Info.base_path) - 1] == '\\') || (_clib32Info.base_path[strlen(_clib32Info.base_path) - 1] == '/'))
            _clib32Info.base_path[strlen(_clib32Info.base_path) - 1] = 0;
    }

    AssetError err;
    MultiFileLib *mflib = new MultiFileLib();
    if (lib_version >= 10)
    {
        // read newer clib versions (versions 10+)
        err = read_ver10plus_format_clib(mflib, ci_s, lib_version);
    }
    else
    {
        // read older clib versions (versions 1 to 9)
        err = read_ver1to9_format_clib(mflib, ci_s, lib_version);
    }
    _assetLib.AssignFromMFL(*mflib);
    delete mflib;
    // Finished reading clib
    delete ci_s;

    // set library filename
    strcpy(_clib32Info.lib_file_name, namm);
    _assetLib.LibFileNames[0] = namm;

    // make a backup of the original file name
    strcpy(_clib32Info.original_base_filename, _assetLib.LibFileNames[0]);
    strlwr(_clib32Info.original_base_filename);
    
    // apply absolute offset for the assets contained in base data file
    // (since only base data file may be EXE file, other clib parts are always on their own)
    if (abs_offset > 0)
    {
        for (int i = 0; i < _assetLib.AssetCount; i++) {
            // correct offsets for EXE file
            if (_assetLib.AssetInfos[i].LibUid == 0)
                _assetLib.AssetInfos[i].Offset += abs_offset;
        }
    }

    return err;
}

//-----------------------------------------------------------------------------
// clibGetNumFiles: get number of files in clib
//-----------------------------------------------------------------------------
int AssetManager::clibGetNumFiles()
{
    if (_clib32Info.lib_file_name[0] == ' ')
        return 0;
    return _assetLib.AssetCount;
}

//-----------------------------------------------------------------------------
// clibGetFileName: get filename by index
//-----------------------------------------------------------------------------
const char *AssetManager::clibGetFileName(int index)
{
    if (_clib32Info.lib_file_name[0] == ' ')
        return NULL;

    if ((index < 0) || (index >= _assetLib.AssetCount))
        return NULL;

    return _assetLib.AssetInfos[index].FileName;
}

//-----------------------------------------------------------------------------
// clibfindindex: get file's index by filename
//-----------------------------------------------------------------------------
int AssetManager::clibfindindex(const char *fill)
{
    if (_clib32Info.lib_file_name[0] == ' ')
        return -1;

    int bb;
    for (bb = 0; bb < _assetLib.AssetCount; bb++) {
        if (stricmp(_assetLib.AssetInfos[bb].FileName, fill) == 0)
            return bb;
    }
    return -1;
}

//-----------------------------------------------------------------------------
// clibfilesize: get file size
//-----------------------------------------------------------------------------
long AssetManager::clibfilesize(const char *fill)
{
    int idxx = clibfindindex(fill);
    if (idxx >= 0)
        return _assetLib.AssetInfos[idxx].Size;
    return -1;
}

//-----------------------------------------------------------------------------
// cliboffset: get file offset in its data file
//-----------------------------------------------------------------------------
long AssetManager::cliboffset(const char *fill)
{
    int idxx = clibfindindex(fill);
    if (idxx >= 0)
        return _assetLib.AssetInfos[idxx].Offset;
    return -1;
}

//-----------------------------------------------------------------------------
// clibgetoriginalfilename: get clib's base filename
//-----------------------------------------------------------------------------
const char *AssetManager::clibgetoriginalfilename() {
    return _clib32Info.original_base_filename;
}

//-----------------------------------------------------------------------------
// clibgetdatafile: get filename for clib's part that contains required file
//-----------------------------------------------------------------------------
char *AssetManager::clibgetdatafile(const char *fill)
{
    int idxx = clibfindindex(fill);
    if (idxx >= 0) {
#if defined(LINUX_VERSION) || defined(MAC_VERSION) 
        sprintf(_clib32Info.actfilename, "%s/%s", _clib32Info.base_path, _assetLib.LibFileNames[_assetLib.AssetInfos[idxx].LibUid]);
#else
        sprintf(_clib32Info.actfilename, "%s\\%s", _clib32Info.base_path, _assetLib.LibFileNames[_assetLib.AssetInfos[idxx].LibUid]);
#endif
        return &_clib32Info.actfilename[0];
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// clibopenfile: opens stream for reading certain file;
// this may normal file from disk or section from clib
//-----------------------------------------------------------------------------
DataStream *AssetManager::clibopenfile(const char *filly, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    int bb;
    // search for the filename in the registered list
    for (bb = 0; bb < _assetLib.AssetCount; bb++) {
        if (stricmp(_assetLib.AssetInfos[bb].FileName, filly) == 0) {
            // file found: set a clib's part filename containing it
#if defined(ANDROID_VERSION)
            sprintf(_clib32Info.actfilename, "%s/%s", _clib32Info.base_path, _assetLib.LibFileNames[_assetLib.AssetInfos[bb].LibUid]);
#else
            sprintf(_clib32Info.actfilename, "%s\\%s", _clib32Info.base_path, _assetLib.LibFileNames[_assetLib.AssetInfos[bb].LibUid]);
#endif
            // open clib's datafile
            DataStream *tfil = ci_fopen(_clib32Info.actfilename, open_mode, work_mode);
            if (tfil == NULL)
                return NULL; // failed to open
            // set stream ptr at the beginning of wanted section
            tfil->Seek(Common::kSeekBegin, _assetLib.AssetInfos[bb].Offset);
            return tfil;
        }
    }
    // file not known, try to open it on disk
    return ci_fopen(filly, open_mode, work_mode);
}

//-----------------------------------------------------------------------------
// clibfopen: opens stream for reading certain file when its location is unknown
//-----------------------------------------------------------------------------
DataStream *AssetManager::clibfopen(const char *filnamm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    _clib32Info.last_opened_size = -1;
    DataStream *tfil = NULL;
    if (_clib32Info.cfopenpriority == PR_FILEFIRST) {
        // check for file, otherwise use datafile
        // note: creating/writing is allowed only for common files on disk
        if (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read) {
            tfil = ci_fopen(filnamm, open_mode, work_mode);
        } else {
            tfil = ci_fopen(filnamm, open_mode, work_mode);

            // if real file not found on disk, try to find it in clib
            if ((tfil == NULL) && (_clib32Info.lib_file_name[0] != ' ')) {
                tfil = clibopenfile(filnamm, open_mode, work_mode);
                _clib32Info.last_opened_size = clibfilesize(filnamm);
            }
        }

    } else {
        // check datafile first, then scan directory
        // note: creating/writing is allowed only for common files on disk
        if ((cliboffset(filnamm) < 1) ||
            (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read))
            // not known, try to open on disk
            tfil = ci_fopen(filnamm, open_mode, work_mode);
        else {
            // file is known, find in clib
            tfil = clibopenfile(filnamm, open_mode, work_mode);
            _clib32Info.last_opened_size = clibfilesize(filnamm);
        }
    }

    // remember size of opened file
    if ((_clib32Info.last_opened_size < 0) && (tfil != NULL))
        _clib32Info.last_opened_size = tfil->GetLength();

    return tfil;
}

} // namespace AGS
} // namespace Common
