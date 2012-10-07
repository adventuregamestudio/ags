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
// Platform-independent FILE interface
//
// TODO: abstract interface wrapper around file handle
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILE_H
#define __AGS_CN_UTIL__FILE_H

#include <stdio.h>
// TODO: other platforms?

#if !defined (WINDOWS_VERSION)
long int filelength(int fhandle);
#endif

#if defined ALLEGRO_BIG_ENDIAN
#include "platform/bigend/bigend.h"
#else
// Two functions to match those defined by bigend version by McCrea
short int   getshort(FILE *);
void        putshort(short int, FILE *);
#endif // !ALLEGRO_BIG_ENDIAN

// Get required padding length when reading/writing a structure
// same way as if it was written as an object in whole;
// result could be from 0 to 3 inclusive.
inline size_t get_padding(int previous_data_length)
{
    return 4 - (previous_data_length % 4);
}

#include "util/string.h"
//#include "util/stream.h"

namespace AGS
{
namespace Common
{

// Forward declarations
class FileStream;

enum FileOpenMode
{
    kFile_Open,         // Open existing file
    kFile_Create,       // Create new file, or open existing one
    kFile_CreateAlways  // Always create a new file, replacing any existing one
};

enum FileWorkMode
{
    kFile_Read,
    kFile_Write,
    kFile_ReadWrite
};

namespace File
{
    // Tests if file could be opened for reading
    bool        TestReadFile(const String &filename);
    // Create new empty file and deletes it; returns TRUE if was able to create file
    bool        TestCreateFile(const String &filename);
    // Deletes existing file; returns TRUE if was able to delete one
    bool        DeleteFile(const String &filename);

    FileStream *OpenFile(const String &filename, FileOpenMode open_mode, FileWorkMode work_mode);
    // Convenience helpers
    // Create a totally new file, overwrite existing one
    inline FileStream *CreateFile(const String &filename)
    {
        return OpenFile(filename, kFile_CreateAlways, kFile_Write);
    }
    // Open existing file for reading
    inline FileStream *OpenFileRead(const String &filename)
    {
        return OpenFile(filename, kFile_Open, kFile_Read);
    }
    // Open existing file for writing (append) or create if it does not exist
    inline FileStream *OpenFileWrite(const String &filename)
    {
        return OpenFile(filename, kFile_Create, kFile_Write);
    }
}; // namespace File

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__FILE_H
