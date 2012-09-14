
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
  char data_filenames[MAXMULTIFILES][20]; // filename for each clib part
  // current clib contents
  int num_files;                        // total number of files
  char filenames[MAX_FILES][25];        // existing file names
  int offset[MAX_FILES];                // file offsets
  int length[MAX_FILES];                // file lengths
  char file_datafile[MAX_FILES];        // index of datafile for each file in split clib
};

struct MultiFileLibNew
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
    delete &_clib32Info;
    delete &_mflib;
}

/* static */ bool AssetManager::SetSearchPriority(AssetsSearchPriority priority)
{
    assert(_theAssetManager != NULL);
    if (_theAssetManager)
    {
        _theAssetManager->_searchPriority = priority;
        switch (_theAssetManager->_searchPriority)
        {
        case kAssetPriority_Data:
            _theAssetManager->_clib32Info.cfopenpriority = PR_DATAFIRST;
            break;
        case kAssetPriority_File:
            _theAssetManager->_clib32Info.cfopenpriority = PR_FILEFIRST;
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
        _theAssetManager->csetlib(data_file, "") : -1; // NOTE: passwords should be kept in AssetManager
}

/* static */ String AssetManager::GetAssetFilePath(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager->clibgetdatafile(asset_file);
}

/* static */ long AssetManager::GetAssetOffset(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->cliboffset(asset_file) : 0;
}

/* static */ long AssetManager::GetAssetSize(const String &asset_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->clibfilesize(asset_file) : 0;
}

/* static */ long AssetManager::GetLastAssetSize()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_clib32Info.last_opened_size : 0;
}

/* static */ int AssetManager::GetAssetCount()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->clibGetNumFiles() : 0;
}

/* static */ String AssetManager::GetAssetFileByIndex(int index)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->clibGetFileName(index) : "";
}

/* static */ String AssetManager::GetOriginalDataFile()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->clibgetoriginalfilename() : "";
}

/* static */ void AssetManager::InitPseudoRand(int seed)
{
    assert(_theAssetManager != NULL);
    if (_theAssetManager)
    {
        _theAssetManager->init_pseudo_rand_gen(seed);
    }
}

/* static */ int AssetManager::GetNextPseudoRand()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->get_pseudo_rand() : 0;
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
    if (_theAssetManager->_currentDataFile.Compare(data_file) != 0)
    {
        if (AssetManager::SetDataFile(data_file) != 0)
        {
            return NULL;
        }
        _theAssetManager->_currentDataFile = data_file;
    }
    return _theAssetManager->clibfopen(asset_file, open_mode, work_mode);
}

AssetManager::AssetManager()
    : _clib32Info(*new Clib32Info())
    , _mflib(*new MultiFileLibNew())
{
    strcpy(_clib32Info.lib_file_name, " ");
    strcpy(_clib32Info.base_path, ".");
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

// this seems to be identical to generic fgetstring_limit
void AssetManager::fgetnulltermstring(char *sss, DataStream *ci_s, int bufsize) {
    int b = -1;
    do {
        if (b < bufsize - 1)
            b++;
        sss[b] = ci_s->ReadInt8();
        if (ci_s->EOS())
            return;
    } while (sss[b] != 0);
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
// read_new_new_enc_format_clib:
//
// reading clib info for versions 20+
// note: everything is decrypted here using pseudo-rand numbers
//-----------------------------------------------------------------------------
int AssetManager::read_new_new_enc_format_clib(MultiFileLibNew * mfl, DataStream *ci_s, int libver)
{
    int aa;
    // init randomizer
    int randSeed = ci_s->ReadInt32();
    init_pseudo_rand_gen(randSeed + _clib32Info.RAND_SEED_SALT);
    // number of clib parts
    mfl->num_data_files = getw_enc(ci_s);
    // filenames for all clib parts
    for (aa = 0; aa < mfl->num_data_files; aa++)
    {
        fgetstring_enc(mfl->data_filenames[aa], ci_s, 50);
    }
    // number of files in clib
    mfl->num_files = getw_enc(ci_s);

    if (mfl->num_files > MAX_FILES)
        return -1; // too many files in clib, return error code

    // read information on clib contents
    for (aa = 0; aa < mfl->num_files; aa++)
    {
        fgetstring_enc(mfl->filenames[aa], ci_s, 100);
    }

    int i;
    for (i = 0; i < mfl->num_files; i++)
    {
        mfl->offset[i] = getw_enc(ci_s);
    }

    for (i = 0; i < mfl->num_files; i++)
    {
        mfl->length[i] = getw_enc(ci_s);
    }

    fread_data_enc(&mfl->file_datafile[0], 1, mfl->num_files, ci_s);

    return 0;
}

//-----------------------------------------------------------------------------
// read_new_new_enc_format_clib:
//
// reading clib info for version 20
//-----------------------------------------------------------------------------
int AssetManager::read_new_new_format_clib(MultiFileLibNew * mfl, DataStream *ci_s, int libver)
{
    int aa;
    // number of clib parts
    mfl->num_data_files = ci_s->ReadInt32();
    // filenames for all clib parts
    for (aa = 0; aa < mfl->num_data_files; aa++)
    {
        fgetnulltermstring(mfl->data_filenames[aa], ci_s, 50);
    }
    // number of files in clib
    mfl->num_files = ci_s->ReadInt32();

    if (mfl->num_files > MAX_FILES)
        return -1; // too many files in clib, return error code

    // read information on clib contents
    for (aa = 0; aa < mfl->num_files; aa++)
    {
        short nameLength;
        nameLength = ci_s->ReadInt16();
        nameLength /= 5;
        ci_s->ReadArray(mfl->filenames[aa], nameLength, 1);
        // decrypt filenames
        clib_decrypt_text(mfl->filenames[aa]);
    }
    ci_s->ReadArrayOfInt32(&mfl->offset[0], mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->ReadArrayOfInt8((int8_t*)&mfl->file_datafile[0], mfl->num_files);
    return 0;
}

//-----------------------------------------------------------------------------
// read_new_new_enc_format_clib:
//
// reading clib info for versions 10 to 19
//-----------------------------------------------------------------------------
int AssetManager::read_new_format_clib(MultiFileLib * mfl, DataStream *ci_s, int libver)
{
    // number of clib parts
    mfl->num_data_files = ci_s->ReadInt32();
    // filenames for all clib parts
    ci_s->ReadArray(&mfl->data_filenames[0][0], 20, mfl->num_data_files);
    // number of files in clib
    mfl->num_files = ci_s->ReadInt32();

    if (mfl->num_files > MAX_FILES)
        return -1; // too many files in clib, return error code

    // read information on clib contents
    ci_s->ReadArray(&mfl->filenames[0][0], 25, mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->offset[0], mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->ReadArrayOfInt8((int8_t*)&mfl->file_datafile[0], mfl->num_files);

    if (libver >= 11)
    {
        // decrypt filenames
        int aa;
        for (aa = 0; aa < mfl->num_files; aa++)
            clib_decrypt_text(mfl->filenames[aa]);
    }
    return 0;
}

//-----------------------------------------------------------------------------
// csetlib: set active multifile library, read info about its contents
//-----------------------------------------------------------------------------
int AssetManager::csetlib(const char *namm, const char *passw)
{
    // reset original base filename
    _clib32Info.original_base_filename[0] = 0;

    // if no lib name, reset current lib name and return
    if (namm == NULL) {
        _clib32Info.lib_file_name[0] = ' ';
        _clib32Info.lib_file_name[1] = 0;
        return 0;
    }
    // base path is current directory
    strcpy(_clib32Info.base_path, ".");

    int passwmodifier = 0, cc, aa;
    // open data library
    DataStream *ci_s = ci_fopen(namm, Common::kFile_Open, Common::kFile_Read);
    if (ci_s == NULL)
        return -1; // can't be opened, return error code

    long absoffs = 0; // library offset in this file
    char clbuff[20];
    // check multifile lib signature at the beginning of file
    ci_s->ReadArray(&clbuff[0], 5, 1);
    if (strncmp(clbuff, "CLIB", 4) != 0) {

        // signature not found, check signature at the end of file
        ci_s->Seek(Common::kSeekEnd, -12);
        ci_s->ReadArray(&clbuff[0], 12, 1);

        // signature not found, return error code
        if (strncmp(clbuff, _clib32Info.clibendfilesig, 12) != 0)
            return -2;

        ci_s->Seek(Common::kSeekEnd, -16);  // it's an appended-to-end-of-exe thing
        int debug_pos2 = ci_s->GetPosition();
        // read multifile lib offset value
        absoffs = ci_s->ReadInt32();
        ci_s->Seek(Common::kSeekBegin, absoffs + 5);
    }

    // read library header
    int lib_version = ci_s->ReadInt8();
    if ((lib_version != 6) && (lib_version != 10) &&
        (lib_version != 11) && (lib_version != 15) &&
        (lib_version != 20) && (lib_version != 21))
        return -3;  // unsupported version

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

    if (lib_version >= 10) {
        // read newer clib versions (versions 10+)

        if (ci_s->ReadInt8() != 0)
            return -4;  // not first datafile in chain

        if (lib_version >= 21)
        {
            // read new clib format with encoding support (versions 21+)
            if (read_new_new_enc_format_clib(&_mflib, ci_s, lib_version))
                return -5;
        }
        else if (lib_version == 20)
        {
            // read new clib format without encoding support (version 20)
            if (read_new_new_format_clib(&_mflib, ci_s, lib_version))
                return -5;
        }
        else 
        {
            // read older clib format (versions 10 to 19), the ones with shorter filenames
            // PSP: Allocate struct on the heap to avoid overflowing the stack.
            MultiFileLib* mflibOld = (MultiFileLib*)malloc(sizeof(MultiFileLib));

            if (read_new_format_clib(mflibOld, ci_s, lib_version))
                return -5;
            // convert to newer format
            _mflib.num_files = mflibOld->num_files;
            _mflib.num_data_files = mflibOld->num_data_files;
            memcpy(&_mflib.offset[0], &mflibOld->offset[0], sizeof(int) * _mflib.num_files);
            memcpy(&_mflib.length[0], &mflibOld->length[0], sizeof(int) * _mflib.num_files);
            memcpy(&_mflib.file_datafile[0], &mflibOld->file_datafile[0], sizeof(char) * _mflib.num_files);
            for (aa = 0; aa < _mflib.num_data_files; aa++)
                strcpy(_mflib.data_filenames[aa], mflibOld->data_filenames[aa]);
            for (aa = 0; aa < _mflib.num_files; aa++)
                strcpy(_mflib.filenames[aa], mflibOld->filenames[aa]);

            free(mflibOld);
        }

        // clib loading completed
        delete ci_s;

        // set library filename
        strcpy(_clib32Info.lib_file_name, namm);

        // make a backup of the original file name
        strcpy(_clib32Info.original_base_filename, _mflib.data_filenames[0]);
        strlwr(_clib32Info.original_base_filename);

        strcpy(_mflib.data_filenames[0], namm);
        for (aa = 0; aa < _mflib.num_files; aa++) {
            // correct offsets for EXE file
            if (_mflib.file_datafile[aa] == 0)
                _mflib.offset[aa] += absoffs;
        }
        // return success
        return 0;
    }

    // read oldest clib versions (versions 1 to 9), the ones without split clib

    passwmodifier = ci_s->ReadInt8();
    ci_s->ReadInt8(); // unused byte
    _mflib.num_data_files = 1;
    strcpy(_mflib.data_filenames[0], namm);

    short tempshort;
    tempshort = ci_s->ReadInt16();
    _mflib.num_files = tempshort;

    if (_mflib.num_files > MAX_FILES)
        return -4; // too many files in clib, return error code

    ci_s->ReadArray(clbuff, 13, 1);  // skip password dooberry
    // read information on contents
    for (aa = 0; aa < _mflib.num_files; aa++) {
        ci_s->ReadArray(&_mflib.filenames[aa][0], 13, 1);
        for (cc = 0; cc < (int)strlen(_mflib.filenames[aa]); cc++)
            _mflib.filenames[aa][cc] -= passwmodifier;
    }
    ci_s->ReadArrayOfInt32(&_mflib.length[0], _mflib.num_files);
    ci_s->Seek(Common::kSeekCurrent, 2 * _mflib.num_files);  // skip flags & ratio

    _mflib.offset[0] = ci_s->GetPosition();
    strcpy(_clib32Info.lib_file_name, namm);
    // loading clib completed
    delete ci_s;

    // fixup offsets
    for (aa = 1; aa < _mflib.num_files; aa++) {
        _mflib.offset[aa] = _mflib.offset[aa - 1] + _mflib.length[aa - 1];
        _mflib.file_datafile[aa] = 0;
    }
    _mflib.file_datafile[0] = 0;
    // return success
    return 0;
}

//-----------------------------------------------------------------------------
// clibGetNumFiles: get number of files in clib
//-----------------------------------------------------------------------------
int AssetManager::clibGetNumFiles()
{
    if (_clib32Info.lib_file_name[0] == ' ')
        return 0;
    return _mflib.num_files;
}

//-----------------------------------------------------------------------------
// clibGetFileName: get filename by index
//-----------------------------------------------------------------------------
const char *AssetManager::clibGetFileName(int index)
{
    if (_clib32Info.lib_file_name[0] == ' ')
        return NULL;

    if ((index < 0) || (index >= _mflib.num_files))
        return NULL;

    return &_mflib.filenames[index][0];
}

//-----------------------------------------------------------------------------
// clibfindindex: get file's index by filename
//-----------------------------------------------------------------------------
int AssetManager::clibfindindex(const char *fill)
{
    if (_clib32Info.lib_file_name[0] == ' ')
        return -1;

    int bb;
    for (bb = 0; bb < _mflib.num_files; bb++) {
        if (stricmp(_mflib.filenames[bb], fill) == 0)
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
        return _mflib.length[idxx];
    return -1;
}

//-----------------------------------------------------------------------------
// cliboffset: get file offset in its data file
//-----------------------------------------------------------------------------
long AssetManager::cliboffset(const char *fill)
{
    int idxx = clibfindindex(fill);
    if (idxx >= 0)
        return _mflib.offset[idxx];
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
        sprintf(_clib32Info.actfilename, "%s/%s", _clib32Info.base_path, _mflib.data_filenames[_mflib.file_datafile[idxx]]);
#else
        sprintf(_clib32Info.actfilename, "%s\\%s", _clib32Info.base_path, _mflib.data_filenames[_mflib.file_datafile[idxx]]);
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
    for (bb = 0; bb < _mflib.num_files; bb++) {
        if (stricmp(_mflib.filenames[bb], filly) == 0) {
            // file found: set a clib's part filename containing it
#if defined(ANDROID_VERSION)
            sprintf(_clib32Info.actfilename, "%s/%s", _clib32Info.base_path, _mflib.data_filenames[_mflib.file_datafile[bb]]);
#else
            sprintf(_clib32Info.actfilename, "%s\\%s", _clib32Info.base_path, _mflib.data_filenames[_mflib.file_datafile[bb]]);
#endif
            // open clib's datafile
            DataStream *tfil = ci_fopen(_clib32Info.actfilename, open_mode, work_mode);
            if (tfil == NULL)
                return NULL; // failed to open
            // set stream ptr at the beginning of wanted section
            tfil->Seek(Common::kSeekBegin, _mflib.offset[bb]);
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
