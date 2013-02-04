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

#include <string.h>
#include <stdlib.h>
#include "core/assetmanager.h"
#include "debug/assert.h"
#include "util/stream.h"
#include "util/file.h"

#if defined (WINDOWS_VERSION)
#include <io.h>
#else
//#include "djcompat.h"
#include "allegro.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "util/misc.h"


#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

#define MAX_FILES 10000
#define MAXMULTIFILES 25

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
    String      BaseFileName;                // library's base (head) filename
    String      BasePath;                    // library's parent path (directory)
    int         PartCount;                   // number of parts this library is split to
    String      LibFileNames[MAXMULTIFILES]; // filename for each library part

    // Library contents
    int         AssetCount; // total number of assets in library
    AssetInfo   AssetInfos[MAX_FILES]; // information on contained assets

    void AssignFromMFL(const MultiFileLib &mflib);
    void Unload();
};

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

void AssetLibInfo::Unload()
{
    BaseFileName    = "";
    BasePath        = "";
    PartCount       = 0;
    AssetCount      = 0;
}

AssetManager *AssetManager::_theAssetManager = NULL;

/* static */ bool AssetManager::CreateInstance()
{
    // Issue a warning - recreating asset manager is not a normal behavior
    assert(_theAssetManager == NULL);
    delete _theAssetManager;
    _theAssetManager = new AssetManager();
    _theAssetManager->SetSearchPriority(kAssetPriorityDir);
    return _theAssetManager != NULL; // well, we should return _something_
}

/* static */ void AssetManager::DestroyInstance()
{
    delete _theAssetManager;
    _theAssetManager = NULL;
}

AssetManager::~AssetManager()
{
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

/* static */ bool AssetManager::IsDataFile(const String &data_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_IsDataFile(data_file) : false;
}

/* static */ AssetError AssetManager::SetDataFile(const String &data_file)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_SetDataFile(data_file) : kAssetErrNoManager;
}

/* static */ String AssetManager::GetLibraryForAsset(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetLibraryForAsset(asset_name) : "";
}

/* static */ long AssetManager::GetAssetOffset(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetOffset(asset_name) : 0;
}

/* static */ long AssetManager::GetAssetSize(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetAssetSize(asset_name) : 0;
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

/* static */ String AssetManager::GetLibraryBaseFile()
{
    assert(_theAssetManager != NULL);
    return _theAssetManager ? _theAssetManager->_GetLibraryBaseFile() : "";
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

/* static */ bool AssetManager::DoesAssetExist(const String &asset_name)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    return _theAssetManager->_DoesAssetExist(asset_name);
}

/* static */ Stream *AssetManager::OpenAsset(const String &asset_name,
                                                  FileOpenMode open_mode,
                                                  FileWorkMode work_mode)
{
    assert(_theAssetManager != NULL);
    if (!_theAssetManager)
    {
        return NULL;
    }
    return _theAssetManager->_OpenAsset(asset_name, open_mode, work_mode);
}

/* static */ Stream *AssetManager::OpenAsset(const String &data_file,
                                                  const String &asset_name,
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
    return _theAssetManager->_OpenAsset(asset_name, open_mode, work_mode);
}

const String AssetManager::_libHeadSig = "CLIB";
const String AssetManager::_libTailSig = "CLIB\x1\x2\x3\x4SIGE";
const int AssetManager::_encryptionRandSeed = 9338638;
const String AssetManager::_encryptionString = "My\x1\xde\x4Jibzle";

AssetManager::AssetManager()
    : _assetLib(*new AssetLibInfo())
{
}

bool AssetManager::_SetSearchPriority(AssetSearchPriority priority)
{
    _searchPriority = priority;
    return true;
}

AssetSearchPriority AssetManager::_GetSearchPriority()
{
    return _searchPriority;
}

bool AssetManager::_IsDataFile(const String &data_file)
{
    if (data_file.IsEmpty())
    {
        return false;
    }

    // open data library
    Stream *ci_s = ci_fopen(data_file, Common::kFile_Open, Common::kFile_Read);
    if (ci_s == NULL)
    {
        return false;
    }

    long abs_offset = 0; // library offset in this file
    char clbuff[20];
    // check multifile lib signature at the beginning of file
    ci_s->Read(&clbuff[0], 5);
    if (strncmp(clbuff, _libHeadSig /*"CLIB"*/, 4) != 0)
    {
        // signature not found, check signature at the end of file
        ci_s->Seek(Common::kSeekEnd, -12);
        ci_s->Read(&clbuff[0], 12);
        // signature not found, return error code
        if (strncmp(clbuff, _libTailSig, 12) != 0)
        {
            return false;
        }
    }
    delete ci_s;
    return true;
}

AssetError AssetManager::_SetDataFile(const String &data_file)
{
    if (data_file.IsEmpty())
    {
        return kAssetErrNoLibFile;
    }
    if (_assetLib.BaseFileName.Compare(data_file) == 0)
    {
        return kAssetNoError;
    }
    AssetError err = RegisterAssetLib(data_file, "");
    return err;
}

String AssetManager::_GetLibraryForAsset(const String &asset_name)
{
    if (asset_name.IsEmpty())
    {
        return "";
    }
    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (!asset)
    {
        // asset not found
        return "";
    }

    return MakeLibraryFileNameForAsset(asset);
}

long AssetManager::_GetAssetOffset(const String &asset_name)
{
    if (asset_name.IsEmpty())
    {
        return -1;
    }
    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (asset)
    {
        return asset->Offset;
    }
    return -1;
}

long AssetManager::_GetAssetSize(const String &asset_name)
{
    if (asset_name.IsEmpty())
    {
        return -1;
    }
    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (asset)
    {
        return asset->Size;
    }
    return -1;
}

long AssetManager::_GetLastAssetSize()
{
    return _lastAssetSize;
}

int AssetManager::_GetAssetCount()
{
    return _assetLib.AssetCount;
}

String AssetManager::_GetAssetFileByIndex(int index)
{
    if ((index < 0) || (index >= _assetLib.AssetCount))
        return NULL;

    return _assetLib.AssetInfos[index].FileName;
}

String AssetManager::_GetLibraryBaseFile()
{
    return _assetLib.BaseFileName;
}

void AssetManager::_InitPseudoRand(int seed)
{
    _lastRandValue = seed;
}

int AssetManager::_GetNextPseudoRand()
{
    return( ((_lastRandValue = _lastRandValue * 214013L
        + 2531011L) >> 16) & 0x7fff );
}

bool AssetManager::_DoesAssetExist(const String &asset_name)
{
    return FindAssetByFileName(asset_name) != NULL ||
        File::TestReadFile(asset_name);
}

Stream *AssetManager::_OpenAsset(const String &asset_name,
                       FileOpenMode open_mode,
                       FileWorkMode work_mode)
{
    return OpenAssetByPriority(asset_name, open_mode, work_mode);
}

AssetError AssetManager::RegisterAssetLib(const String &data_file, const String &password)
{
    // base path is current directory
    _assetLib.BasePath = ".";

    // open data library
    Stream *ci_s = ci_fopen(data_file, Common::kFile_Open, Common::kFile_Read);
    if (ci_s == NULL)
    {
        return kAssetErrNoLibFile; // can't be opened, return error code
    }

    long abs_offset = 0; // library offset in this file
    char clbuff[20];
    // check multifile lib signature at the beginning of file
    ci_s->Read(&clbuff[0], 5);
    if (strncmp(clbuff, _libHeadSig /*"CLIB"*/, 4) != 0)
    {
        // signature not found, check signature at the end of file
        ci_s->Seek(Common::kSeekEnd, -12);
        ci_s->Read(&clbuff[0], 12);
        // signature not found, return error code
        if (strncmp(clbuff, _libTailSig, 12) != 0)
        {
            return kAssetErrNoLibSig;
        }
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

    String nammwas = data_file;
    String data_file_fixed = data_file;
    // TODO: this algorythm should be in path/string utils
    data_file_fixed.TruncateToRightSection('\\');
    data_file_fixed.TruncateToRightSection('/');
    if (data_file_fixed.Compare(nammwas) != 0)
    {
        // store complete path
        _assetLib.BasePath = nammwas;
        _assetLib.BasePath.TruncateToLeft(nammwas.GetLength() - data_file_fixed.GetLength());
        _assetLib.BasePath.TrimRight('\\');
        _assetLib.BasePath.TrimRight('/');
    }

    AssetError err;
    MultiFileLib *mflib = new MultiFileLib();
    if (lib_version >= 10)
    {
        // read newer clib versions (versions 10+)
        err = ReadMultiFileAssetLib(mflib, ci_s, lib_version);
    }
    else
    {
        // read older clib versions (versions 1 to 9)
        err = ReadSingleFileAssetLib(mflib, ci_s, lib_version);
    }
    _assetLib.AssignFromMFL(*mflib);
    delete mflib;
    // Finished reading clib
    delete ci_s;

    // set library filename
    _assetLib.LibFileNames[0] = data_file_fixed;
    // make a backup of the original file name
    _assetLib.BaseFileName = _assetLib.LibFileNames[0];
    _assetLib.BaseFileName.MakeLower();

    // apply absolute offset for the assets contained in base data file
    // (since only base data file may be EXE file, other clib parts are always on their own)
    if (abs_offset > 0)
    {
        for (int i = 0; i < _assetLib.AssetCount; i++)
        {
            // correct offsets for EXE file
            if (_assetLib.AssetInfos[i].LibUid == 0)
            {
                _assetLib.AssetInfos[i].Offset += abs_offset;
            }
        }
    }
    return err;
}

AssetError AssetManager::ReadSingleFileAssetLib(MultiFileLib * mfl, Stream *ci_s, int lib_version)
{
    int passwmodifier = ci_s->ReadByte();
    ci_s->ReadInt8(); // unused byte
    mfl->num_data_files = 1;
    mfl->num_files = ci_s->ReadInt16();
    if (mfl->num_files > MAX_FILES)
    {
        return kAssetErrLibAssetCount; // too many files in clib, return error code
    }

    char clbuff[20];
    ci_s->Read(clbuff, 13);  // skip password dooberry
    // read information on contents
    for (int i = 0; i < mfl->num_files; i++)
    {
        ci_s->Read(&mfl->filenames[i][0], 13); // CHECKME, is it really 13?
        for (int j = 0; j < (int)strlen(mfl->filenames[i]); j++)
            mfl->filenames[i][j] -= passwmodifier;
    }
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->Seek(Common::kSeekCurrent, 2 * mfl->num_files);  // skip flags & ratio
    mfl->offset[0] = ci_s->GetPosition();
    // set offsets (assets are positioned in sequence)
    for (int i = 1; i < mfl->num_files; i++)
    {
        mfl->offset[i] = mfl->offset[i - 1] + mfl->length[i - 1];
        mfl->file_datafile[i] = 0;
    }
    mfl->file_datafile[0] = 0;
    // return success
    return kAssetNoError;
}

AssetError AssetManager::ReadMultiFileAssetLib(MultiFileLib * mfl, Stream *ci_s, int lib_version)
{
    if (ci_s->ReadByte() != 0)
    {
        return kAssetErrNoLibBase;  // not first datafile in chain
    }

    if (lib_version >= 21)
    {
        // read new clib format with encoding support (versions 21+)
        AssetError err = ReadAssetLibV21(mfl, ci_s, lib_version);
        if (err != kAssetNoError)
        {
            return err;
        }
    }
    else if (lib_version == 20)
    {
        // read new clib format without encoding support (version 20)
        AssetError err = ReadAssetLibV20(mfl, ci_s, lib_version);
        if (err != kAssetNoError)
        {
            return err;
        }
    }
    else 
    {
        // read older clib format (versions 10 to 19), the ones with shorter filenames
        AssetError err = ReadAssetLibV10(mfl, ci_s, lib_version);
        if (err != kAssetNoError)
        {
            return err;
        }
    }
    // return success
    return kAssetNoError;
}

AssetError AssetManager::ReadAssetLibV10(MultiFileLib * mfl, Stream *ci_s, int lib_version)
{
    // number of clib parts
    mfl->num_data_files = ci_s->ReadInt32();
    // filenames for all clib parts; filename array is only 20 chars long in this format version
    for (int i = 0; i < mfl->num_data_files; i++)
      ci_s->ReadArray(&mfl->data_filenames[i][0], 20, 1);

    // number of files in clib
    mfl->num_files = ci_s->ReadInt32();
    if (mfl->num_files > MAX_FILES)
    {
        return kAssetErrLibAssetCount; // too many files in clib, return error code
    }

    // read information on clib contents

    // filename array is only 25 chars long in this format version
    for (int i = 0; i < mfl->num_files; i++)
      ci_s->ReadArray(&mfl->filenames[i][0], 25, 1);

    ci_s->ReadArrayOfInt32(&mfl->offset[0], mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->ReadArrayOfInt8((int8_t*)&mfl->file_datafile[0], mfl->num_files);

    if (lib_version >= 11)
    {
        // decrypt filenames
        for (int i = 0; i < mfl->num_files; i++)
        {
            DecryptText(mfl->filenames[i], strlen(mfl->filenames[i]));
        }
    }
    return kAssetNoError;
}

AssetError AssetManager::ReadAssetLibV20(MultiFileLib * mfl, Stream *ci_s, int lib_version)
{
    // number of clib parts
    mfl->num_data_files = ci_s->ReadInt32();
    // filenames for all clib parts
    for (int i = 0; i < mfl->num_data_files; i++)
    {
        fgetstring_limit(mfl->data_filenames[i], ci_s, 50);
    }
    // number of files in clib
    mfl->num_files = ci_s->ReadInt32();
    if (mfl->num_files > MAX_FILES)
    {
        return kAssetErrLibAssetCount; // too many files in clib, return error code
    }

    // read information on clib contents
    for (int i = 0; i < mfl->num_files; i++)
    {
        short nameLength;
        nameLength = ci_s->ReadInt16();
        nameLength /= 5;
        ci_s->Read(mfl->filenames[i], nameLength);
        // decrypt filenames
        DecryptText(mfl->filenames[i], strlen(mfl->filenames[i]));
    }
    ci_s->ReadArrayOfInt32(&mfl->offset[0], mfl->num_files);
    ci_s->ReadArrayOfInt32(&mfl->length[0], mfl->num_files);
    ci_s->ReadArrayOfInt8((int8_t*)&mfl->file_datafile[0], mfl->num_files);
    return kAssetNoError;
}

AssetError AssetManager::ReadAssetLibV21(MultiFileLib * mfl, Stream *ci_s, int libver)
{
    // init randomizer
    int randSeed = ci_s->ReadInt32();
    _InitPseudoRand(randSeed + _encryptionRandSeed);
    // number of clib parts
    mfl->num_data_files = ReadEncInt32(ci_s);
    // filenames for all clib parts
    for (int i = 0; i < mfl->num_data_files; i++)
    {
        ReadEncString(mfl->data_filenames[i], 50, ci_s);
    }
    // number of files in clib
    mfl->num_files = ReadEncInt32(ci_s);
    if (mfl->num_files > MAX_FILES)
    {
        return kAssetErrLibAssetCount; // too many files in clib, return error code
    }

    // read information on clib contents
    for (int i = 0; i < mfl->num_files; i++)
    {
        ReadEncString(mfl->filenames[i], 100, ci_s);
    }

    for (int i = 0; i < mfl->num_files; i++)
    {
        mfl->offset[i] = ReadEncInt32(ci_s);
    }

    for (int i = 0; i < mfl->num_files; i++)
    {
        mfl->length[i] = ReadEncInt32(ci_s);
    }

    ReadEncArray(&mfl->file_datafile[0], 1, mfl->num_files, ci_s);
    return kAssetNoError;
}

AssetInfo *AssetManager::FindAssetByFileName(const String &asset_name)
{
    for (int i = 0; i < _assetLib.AssetCount; ++i)
    {
        if (_assetLib.AssetInfos[i].FileName.CompareNoCase(asset_name) == 0)
        {
            return &_assetLib.AssetInfos[i];
        }
    }
    return NULL;
}

String AssetManager::MakeLibraryFileNameForAsset(const AssetInfo *asset)
{
    // deduce asset library file containing this asset
    String lib_filename;
#if defined (WINDOWS_VERSION)
    lib_filename.Format("%s\\%s",_assetLib.BasePath.GetCStr(), _assetLib.LibFileNames[asset->LibUid].GetCStr());
#else
    lib_filename.Format("%s/%s", _assetLib.BasePath.GetCStr(), _assetLib.LibFileNames[asset->LibUid].GetCStr());
#endif
    return lib_filename;
}

Stream *AssetManager::OpenAssetFromLib(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    // creating/writing is allowed only for common files on disk
    if (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read)
    {
        return NULL;
    }

    AssetInfo *asset = FindAssetByFileName(asset_name);
    if (!asset)
    {
        // asset not found
        return NULL;
    }

    String lib_filename = MakeLibraryFileNameForAsset(asset);
    // open library datafile
    Stream *lib_s = ci_fopen(lib_filename, open_mode, work_mode);
    if (lib_s)
    {
        // set stream ptr at the beginning of wanted section
        lib_s->Seek(Common::kSeekBegin, asset->Offset);
        // remember size of opened asset
        _lastAssetSize = asset->Size;
    }
    return lib_s;
}

Stream *AssetManager::OpenAssetFromDir(const String &file_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    Stream *asset_s = ci_fopen(file_name, open_mode, work_mode);
    if (asset_s)
    {
        // remember size of opened file
        _lastAssetSize = asset_s->GetLength();
    }
    return asset_s;
}

Stream *AssetManager::OpenAssetByPriority(const String &asset_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
    Stream *asset_s = NULL;
    if (_searchPriority == kAssetPriorityDir)
    {
        // check for disk, otherwise use datafile
        asset_s = OpenAssetFromDir(asset_name, open_mode, work_mode);
        if (!asset_s)
        {
            asset_s = OpenAssetFromLib(asset_name, open_mode, work_mode);
        }
    }
    else if (_searchPriority == kAssetPriorityLib)
    {
        // check datafile first, then scan directory
        asset_s = OpenAssetFromLib(asset_name, open_mode, work_mode);
        if (!asset_s)
        {
            asset_s = OpenAssetFromDir(asset_name, open_mode, work_mode);
        }
    }
    return asset_s;
}

void AssetManager::ReadEncArray(void *data, int dataSize, int dataCount, Stream *ci_s)
{
    ci_s->ReadArray(data, dataSize, dataCount);
    uint8_t *dataChar = (uint8_t*)data;
    for (int i = 0; i < dataSize * dataCount; i++)
    {
        dataChar[i] -= _GetNextPseudoRand();
    }
}

int32_t AssetManager::ReadEncInt32(Stream *ci_s)
{
    int numberRead;
    ReadEncArray(&numberRead, 4, 1, ci_s);
#if defined(AGS_BIG_ENDIAN)
    AGS::Common::BitByteOperations::SwapBytesInt32(numberRead);
#endif
    return numberRead;
}

void AssetManager::ReadEncString(char *buffer, int maxLength, Stream *ci_s)
{
    int i = 0;
    while ((i == 0) || (buffer[i - 1] != 0))
    {
        buffer[i] = ci_s->ReadByte() - _GetNextPseudoRand();
        if (i < maxLength - 1)
        {
            i++;
        }
        else
        {
            // Avoid an endless loop
            break;
        }
    }
}

void AssetManager::DecryptText(char *text, int length)
{
/*
    int adx = 0;
    for (int i = 0; i < length; ++i)
    {
        text[i] -= _encryptionString[adx];
        if (++adx > 10) // CHECKME: why 10?
        {
            adx = 0;
        }
    }
*/
  int adx = 0;

  while (1)
  {
    text[0] -= _encryptionString[adx];
    if (text[0] == 0)
    {
      break;
    }

    adx++;
    text++;

    if (adx > 10)
    {
      adx = 0;
    }
  }

}



} // namespace AGS
} // namespace Common
