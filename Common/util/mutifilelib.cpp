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

#include "util/bbop.h"
#include "util/multifilelib.h"
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

namespace MFLUtil
{
    const String HeadSig = "CLIB\x1a";
    const String TailSig = "CLIB\x1\x2\x3\x4SIGE";

    static const size_t SingleFilePswLen = 13;

    static const size_t MaxAssets        = 10000;
    static const size_t MaxMultifiles    = 25;
    static const size_t MaxAssetFileLen  = 100;
    static const size_t MaxDataFileLen   = 50;
    static const size_t V10LibFileLen    = 20;
    static const size_t V10AssetFileLen  = 25;

    static const String EncryptionString = "My\x1\xde\x4Jibzle";

    MFLError ReadSigsAndVersion(Stream *in, int *p_lib_version, long *p_abs_offset);
    MFLError ReadSingleFileLib(AssetLibInfo &lib, Stream *in, int lib_version);
    MFLError ReadMultiFileLib(AssetLibInfo &lib, Stream *in, int lib_version);
    MFLError ReadV10(AssetLibInfo &lib, Stream *in, int lib_version);
    MFLError ReadV20(AssetLibInfo &lib, Stream *in);
    MFLError ReadV21(AssetLibInfo &lib, Stream *in);

    // Decryption routines
    void     DecryptText(char *text);
    void     ReadEncArray(void *data, size_t size, size_t count, Stream *in, int &rand_val);
    int8_t   ReadEncInt8(Stream *in, int &rand_val);
    int32_t  ReadEncInt32(Stream *in, int &rand_val);
    void     ReadEncString(char *buffer, size_t max_len, Stream *in, int &rand_val);
};


MFLUtil::MFLError MFLUtil::TestIsMFL(Stream *in, bool test_is_main)
{
    int lib_version;
    MFLError err = ReadSigsAndVersion(in, &lib_version, NULL);
    if (err == kMFLNoError)
    {
        if (lib_version >= 10 && test_is_main)
        {
            // this version supports multiple data files, check if it is the first one
            if (in->ReadByte() != 0)
                return kMFLErrNoLibBase; // not first datafile in chain
        }
    }
    return err;
}

MFLUtil::MFLError MFLUtil::ReadHeader(AssetLibInfo &lib, Stream *in)
{
    int lib_version;
    long abs_offset;
    MFLError err = ReadSigsAndVersion(in, &lib_version, &abs_offset);
    if (err != kMFLNoError)
        return err;

    if (lib_version >= 10)
    {
        // read newer clib versions (versions 10+)
        err = ReadMultiFileLib(lib, in, lib_version);
    }
    else
    {
        // read older clib versions (versions 1 to 9)
        err = ReadSingleFileLib(lib, in, lib_version);
    }

    // apply absolute offset for the assets contained in base data file
    // (since only base data file may be EXE file, other clib parts are always on their own)
    if (abs_offset > 0)
    {
        for (AssetVec::iterator it = lib.AssetInfos.begin();
             it != lib.AssetInfos.end(); ++it)
        {
            if (it->LibUid == 0)
                it->Offset += abs_offset;
        }
    }
    return err;
}

MFLUtil::MFLError MFLUtil::ReadSigsAndVersion(Stream *in, int *p_lib_version, long *p_abs_offset)
{
    long abs_offset = 0; // library offset in this file
    String sig;
    // check multifile lib signature at the beginning of file
    sig.ReadCount(in, HeadSig.GetLength());
    if (HeadSig.Compare(sig) != 0)
    {
        // signature not found, check signature at the end of file
        in->Seek(-TailSig.GetLength(), kSeekEnd);
        sig.ReadCount(in, TailSig.GetLength());
        // signature not found, return error code
        if (TailSig.Compare(sig) != 0)
            return kMFLErrNoLibSig;

        // it's an appended-to-end-of-exe thing
        in->Seek(-TailSig.GetLength() - sizeof(int32_t), kSeekEnd);
        // read multifile lib offset value
        abs_offset = in->ReadInt32();
        in->Seek(abs_offset + HeadSig.GetLength(), kSeekBegin);
    }

    // read library header
    int lib_version = in->ReadByte();
    if ((lib_version != 6) && (lib_version != 10) &&
        (lib_version != 11) && (lib_version != 15) &&
        (lib_version != 20) && (lib_version != 21))
        return kMFLErrLibVersion; // unsupported version

    if (p_lib_version)
        *p_lib_version = lib_version;
    if (p_abs_offset)
        *p_abs_offset = abs_offset;
    return kMFLNoError;
}

MFLUtil::MFLError MFLUtil::ReadSingleFileLib(AssetLibInfo &lib, Stream *in, int lib_version)
{
    int passwmodifier = in->ReadByte();
    in->ReadInt8(); // unused byte
    lib.LibFileNames.resize(1); // only one library part
    size_t asset_count = in->ReadInt16();
    if (asset_count > MaxAssets)
        return kMFLErrLibAssetCount; // too many files in clib, return error code
    lib.AssetInfos.resize(asset_count);

    in->Seek(SingleFilePswLen, kSeekCurrent); // skip password dooberry
    char fn_buf[SingleFilePswLen + 1];
    // read information on contents
    for (size_t i = 0; i < asset_count; ++i)
    {
        in->Read(fn_buf, SingleFilePswLen);
        fn_buf[SingleFilePswLen] = 0;
        for (char *c = fn_buf; *c; ++c)
            *c -= passwmodifier;
        lib.AssetInfos[i].FileName = fn_buf;
        lib.AssetInfos[i].LibUid = 0;
    }
    for (size_t i = 0; i < asset_count; ++i)
    {
        lib.AssetInfos[i].Size = in->ReadInt32();
    }
    in->Seek(2 * asset_count, kSeekCurrent); // skip flags & ratio
    lib.AssetInfos[0].Offset = in->GetPosition();
    // set offsets (assets are positioned in sequence)
    for (size_t i = 1; i < asset_count; ++i)
    {
        lib.AssetInfos[i].Offset = lib.AssetInfos[i - 1].Offset + lib.AssetInfos[i - 1].Size;
    }
    // return success
    return kMFLNoError;
}

MFLUtil::MFLError MFLUtil::ReadMultiFileLib(AssetLibInfo &lib, Stream *in, int lib_version)
{
    if (in->ReadByte() != 0)
        return kMFLErrNoLibBase; // not first datafile in chain

    if (lib_version >= 21)
    {
        // read new clib format with encoding support (versions 21+)
        return ReadV21(lib, in);
    }
    else if (lib_version == 20)
    {
        // read new clib format without encoding support (version 20)
        return ReadV20(lib, in);
    }
    // read older clib format (versions 10 to 19), the ones with shorter filenames
    return ReadV10(lib, in, lib_version);
}

MFLUtil::MFLError MFLUtil::ReadV10(AssetLibInfo &lib, Stream *in, int lib_version)
{
    // number of clib parts
    size_t mf_count = in->ReadInt32();
    lib.LibFileNames.resize(mf_count);
    // filenames for all clib parts; filenames are only 20 chars long in this format version
    for (size_t i = 0; i < mf_count; ++i)
    {
        lib.LibFileNames[i].ReadCount(in, V10LibFileLen);
    }

    // number of files in clib
    size_t asset_count = in->ReadInt32();
    if (asset_count > MaxAssets)
        return kMFLErrLibAssetCount; // too many files in clib, return error code
    // read information on clib contents
    lib.AssetInfos.resize(asset_count);
    // filename array is only 25 chars long in this format version
    char fn_buf[V10AssetFileLen];
    for (size_t i = 0; i < asset_count; ++i)
    {
        in->Read(fn_buf, V10AssetFileLen);
        if (lib_version >= 11)
            DecryptText(fn_buf);
        lib.AssetInfos[i].FileName = fn_buf;
    }
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].Offset = in->ReadInt32();
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].Size = in->ReadInt32();
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].LibUid = in->ReadInt8();
    return kMFLNoError;
}

MFLUtil::MFLError MFLUtil::ReadV20(AssetLibInfo &lib, Stream *in)
{
    // number of clib parts
    size_t mf_count = in->ReadInt32();
    lib.LibFileNames.resize(mf_count);
    // filenames for all clib parts
    for (size_t i = 0; i < mf_count; ++i)
    {
        lib.LibFileNames[i].Read(in, MaxDataFileLen);
    }

    // number of files in clib
    size_t asset_count = in->ReadInt32();
    if (asset_count > MaxAssets)
        return kMFLErrLibAssetCount; // too many files in clib, return error code
    // read information on clib contents
    lib.AssetInfos.resize(asset_count);
    char fn_buf[MaxAssetFileLen];
    for (size_t i = 0; i < asset_count; ++i)
    {
        short len = in->ReadInt16();
        len /= 5; // CHECKME: why 5?
        in->Read(fn_buf, len);
        // decrypt filenames
        DecryptText(fn_buf);
        lib.AssetInfos[i].FileName = fn_buf;
    }
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].Offset = in->ReadInt32();
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].Size = in->ReadInt32();
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].LibUid = in->ReadInt8();
    return kMFLNoError;
}

MFLUtil::MFLError MFLUtil::ReadV21(AssetLibInfo &lib, Stream *in)
{
    // init randomizer
    int rand_val = in->ReadInt32() + EncryptionRandSeed;
    // number of clib parts
    size_t mf_count = ReadEncInt32(in, rand_val);
    lib.LibFileNames.resize(mf_count);
    // filenames for all clib parts
    char fn_buf[MaxDataFileLen];
    for (size_t i = 0; i < mf_count; ++i)
    {
        ReadEncString(fn_buf, MaxDataFileLen, in, rand_val);
        lib.LibFileNames[i] = fn_buf;
    }

    // number of files in clib
    size_t asset_count = ReadEncInt32(in, rand_val);
    if (asset_count > MaxAssets)
        return kMFLErrLibAssetCount; // too many files in clib, return error code
    // read information on clib contents
    lib.AssetInfos.resize(asset_count);
    for (size_t i = 0; i < asset_count; ++i)
    {
        ReadEncString(fn_buf, MaxAssetFileLen, in, rand_val);
        lib.AssetInfos[i].FileName = fn_buf;
    }
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].Offset = ReadEncInt32(in, rand_val);
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].Size = ReadEncInt32(in, rand_val);
    for (size_t i = 0; i < asset_count; ++i)
        lib.AssetInfos[i].LibUid = ReadEncInt8(in, rand_val);
    return kMFLNoError;
}

void MFLUtil::DecryptText(char *text)
{
    int adx = 0;
    while (true)
    {
        text[0] -= EncryptionString[adx];
        if (text[0] == 0)
            break;

        adx++;
        text++;

        if (adx > 10) // CHECKME: why 10?
            adx = 0;
    }
}

int MFLUtil::GetNextPseudoRand(int &rand_val)
{
    return( ((rand_val = rand_val * 214013L
        + 2531011L) >> 16) & 0x7fff );
}

void MFLUtil::ReadEncArray(void *data, size_t size, size_t count, Stream *in, int &rand_val)
{
    in->ReadArray(data, size, count);
    uint8_t *ch = (uint8_t*)data;
    const size_t len = size * count;
    for (size_t i = 0; i < len; ++i)
    {
        ch[i] -= GetNextPseudoRand(rand_val);
    }
}

int8_t MFLUtil::ReadEncInt8(Stream *in, int &rand_val)
{
    return in->ReadByte() - GetNextPseudoRand(rand_val);
}

int32_t MFLUtil::ReadEncInt32(Stream *in, int &rand_val)
{
    int val;
    ReadEncArray(&val, sizeof(int32_t), 1, in, rand_val);
#if defined(AGS_BIG_ENDIAN)
    AGS::Common::BitByteOperations::SwapBytesInt32(val);
#endif
    return val;
}

void MFLUtil::ReadEncString(char *buffer, size_t max_len, Stream *in, int &rand_val)
{
    size_t i = 0;
    while ((i == 0) || (buffer[i - 1] != 0))
    {
        buffer[i] = in->ReadByte() - GetNextPseudoRand(rand_val);
        if (i < max_len - 1)
            i++;
        else
            break; // avoid an endless loop
    }
}

} // namespace AGS
} // namespace Common
