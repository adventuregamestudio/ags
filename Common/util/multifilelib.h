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
// A packed data file header and functions for reading it from the stream.
//
//-----------------------------------------------------------------------------
//
// The code is based on CLIB32, by Chris Jones (1998-99), DJGPP implementation
// of the CLIB reader.
//
//=============================================================================

#ifndef __AGS_CN_UTIL__MULTIFILELIB_H
#define __AGS_CN_UTIL__MULTIFILELIB_H

#include "core/asset.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

//
// MultiFileLib utilities: (de)serialization of asset library in MFL format
//
namespace MFLUtil
{
    enum MFLError
    {
        kMFLNoError           =  0,
        kMFLErrNoLibSig       = -1, // library signature does not match
        kMFLErrLibVersion     = -2, // library version unsupported
        kMFLErrNoLibBase      = -3, // file is not library base (head)
        kMFLErrLibAssetCount  = -4, // too many assets in library
    };

    extern const String HeadSig;
    extern const String TailSig;

    MFLError TestIsMFL(Stream *in, bool test_is_main = false);
    MFLError ReadHeader(AssetLibInfo &lib, Stream *in);

    const int EncryptionRandSeed = 9338638;
    int      GetNextPseudoRand(int &rand_val);
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MULTIFILELIB_H
