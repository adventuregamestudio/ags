//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Platform-independent File functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILE_H
#define __AGS_CN_UTIL__FILE_H

#include "core/platform.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

// Forward declarations
class Stream;

enum FileOpenMode
{
    kFile_None = 0,
    kFile_Open,         // Open existing file
    kFile_Create,       // Create new file, or open existing one
    kFile_CreateAlways  // Always create a new file, replacing any existing one
};

namespace File
{
    // Tells if the given path is a directory
    bool        IsDirectory(const String &directory);
    // Tells if the given path is a file
    bool        IsFile(const String &filename);
    // Tells if the given path is file or directory;
    // may be used to check if it's valid to use
    bool        IsFileOrDir(const String &filename);
    // Returns size of a file, or -1 if no such file found
    soff_t      GetFileSize(const String &filename);
    // Tests if file could be opened for reading
    bool        TestReadFile(const String &filename);
    // Opens a file for writing or creates new one if it does not exist; deletes file if it was created during test
    bool        TestWriteFile(const String &filename);
    // Create new empty file and deletes it; returns TRUE if was able to create file
    bool        TestCreateFile(const String &filename);
    // Deletes existing file; returns TRUE if was able to delete one
    bool        DeleteFile(const String &filename);
    // Renames existing file to the new name; returns TRUE on success
    bool        RenameFile(const String &old_name, const String &new_name);
    // Copies a file from src_path to dst_path; returns TRUE on success
    bool        CopyFile(const String &src_path, const String &dst_path, bool overwrite);

    // Sets FileOpenMode and FileWorkMode values corresponding to C-style file open mode string
    bool        GetFileModesFromCMode(const String &cmode, FileOpenMode &open_mode, StreamMode &work_mode);
    // Gets C-style file mode from FileOpenMode and FileWorkMode
    String      GetCMode(FileOpenMode open_mode, StreamMode work_mode);

    // Opens file in the given mode
    Stream      *OpenFile(const String &filename, FileOpenMode open_mode, StreamMode work_mode);
    // Opens file for reading restricted to the arbitrary offset range
    Stream      *OpenFile(const String &filename, soff_t start_off, soff_t end_off);
    // Convenience helpers
    // Create a totally new file, overwrite existing one
    inline Stream *CreateFile(const String &filename)
    {
        return OpenFile(filename, kFile_CreateAlways, kStream_Write);
    }
    // Open existing file for reading
    inline Stream *OpenFileRead(const String &filename)
    {
        return OpenFile(filename, kFile_Open, kStream_Read);
    }
    // Open existing file for writing (append) or create if it does not exist
    inline Stream *OpenFileWrite(const String &filename)
    {
        return OpenFile(filename, kFile_Create, kStream_Write);
    }
    // Opens stdin stream for reading
    Stream      *OpenStdin();
    // Opens stdout stream for writing
    Stream      *OpenStdout();
    // Opens stderr stream for writing
    Stream      *OpenStderr();

    // TODO: FindFileCI is used a lot when opening game assets;
    // this may result in lots and lots of repeating same work.
    // consider generating a hashset that stores results of FindCI
    // (hashset with case-insensitive hash fn, we don't need a map here).
    // But then we might require a file-watcher, as dir contents may change.

    // Case insensitive find file: looks up for a file_name in base_dir
    // and finds out whether such path exists and valid. On case-sensitive
    // filesystems scans dir and does a case-insensitive search.
    // base_dir defines the base dir which is assumed to be correct already,
    // and file_name is considered to be relative to base dir;
    // base_dir may be empty, in which case relative filename is treated as
    // relative to the current working dir;
    // file_name is allowed to contain subdirs, all of which should be treated
    // as case-insensitive.
    // On success returns a full found path, on failure returns an empty string.
    // On failure optionally fills most_found and not_found strings with the
    // successful and failed part of the searched path respectively.
    String FindFileCI(const String &base_dir, const String &file_name,
        bool is_dir = false, String *most_found = nullptr, String *not_found = nullptr);
    // Case insensitive file open: looks up for the file using FindFileCI
    Stream *OpenFileCI(const String &base_dir, const String &file_name,
        FileOpenMode open_mode = kFile_Open,
        StreamMode work_mode = kStream_Read);
    inline Stream *OpenFileCI(const String &file_name,
        FileOpenMode open_mode = kFile_Open,
        StreamMode work_mode = kStream_Read)
    {
        return OpenFileCI("", file_name, open_mode, work_mode);
    }
} // namespace File

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__FILE_H
