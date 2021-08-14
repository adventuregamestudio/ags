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
#include "util/file.h"
#include <errno.h>
#include <stdexcept>
#if !AGS_PLATFORM_OS_WINDOWS
#include <sys/stat.h>
#include <dirent.h>
#include <string.h> // strcasecmp
#endif
#include "core/platform.h"
#include "util/bufferedstream.h"
#include "util/filestream.h"
#include "util/path.h"
#include "util/stdio_compat.h"

namespace AGS
{
namespace Common
{

soff_t File::GetFileSize(const String &filename)
{
    return ags_file_size(filename.GetCStr());
}

bool File::TestReadFile(const String &filename)
{
    FILE *test_file = ags_fopen(filename.GetCStr(), "rb");
    if (test_file)
    {
        fclose(test_file);
        return true;
    }
    return false;
}

bool File::TestWriteFile(const String &filename)
{
    FILE *test_file = ags_fopen(filename.GetCStr(), "r+");
    if (test_file)
    {
        fclose(test_file);
        return true;
    }
    return TestCreateFile(filename);
}

bool File::TestCreateFile(const String &filename)
{
    FILE *test_file = ags_fopen(filename.GetCStr(), "wb");
    if (test_file)
    {
        fclose(test_file);
        ags_remove(filename.GetCStr());
        return true;
    }
    return false;
}

bool File::DeleteFile(const String &filename)
{
    if (ags_remove(filename.GetCStr()) != 0)
    {
        int err;
#if AGS_PLATFORM_OS_WINDOWS
        _get_errno(&err);
#else
        err = errno;
#endif
        if (err == EACCES)
        {
            return false;
        }
    }
    return true;
}

bool File::RenameFile(const String &old_name, const String &new_name)
{
    return ags_rename(old_name.GetCStr(), new_name.GetCStr()) == 0;
}

bool File::GetFileModesFromCMode(const String &cmode, FileOpenMode &open_mode, FileWorkMode &work_mode)
{
    // We do not test for 'b' and 't' here, because text mode reading/writing should be done with
    // the use of ITextReader and ITextWriter implementations.
    // The number of supported variants here is quite limited due the restrictions AGS makes on them.
    bool read_base_mode = false;
    // Default mode is open/read for safety reasons
    open_mode = kFile_Open;
    work_mode = kFile_Read;
    for (size_t c = 0; c < cmode.GetLength(); ++c)
    {
        if (read_base_mode)
        {
            if (cmode[c] == '+')
            {
                work_mode = kFile_ReadWrite;
            }
            break;
        }
        else
        {
            if (cmode[c] == 'r')
            {
                open_mode = kFile_Open;
                work_mode = kFile_Read;
                read_base_mode = true;
            }
            else if (cmode[c] == 'a')
            {
                open_mode = kFile_Create;
                work_mode = kFile_Write;
                read_base_mode = true;
            }
            else if (cmode[c] == 'w')
            {
                open_mode = kFile_CreateAlways;
                work_mode = kFile_Write;
                read_base_mode = true;
            }
        }
    }
    return read_base_mode;
}

String File::GetCMode(FileOpenMode open_mode, FileWorkMode work_mode)
{
    String mode;
    if (open_mode == kFile_Open)
    {
        if (work_mode == kFile_Read)
            mode.AppendChar('r');
        else if (work_mode == kFile_Write || work_mode == kFile_ReadWrite)
            mode.Append("r+");
    }
    else if (open_mode == kFile_Create)
    {
        if (work_mode == kFile_Write)
            mode.AppendChar('a');
        else if (work_mode == kFile_Read || work_mode == kFile_ReadWrite)
            mode.Append("a+");
    }
    else if (open_mode == kFile_CreateAlways)
    {
        if (work_mode == kFile_Write)
            mode.AppendChar('w');
        else if (work_mode == kFile_Read || work_mode == kFile_ReadWrite)
            mode.Append("w+");
    }
    mode.AppendChar('b');
    return mode;
}

Stream *File::OpenFile(const String &filename, FileOpenMode open_mode, FileWorkMode work_mode)
{
    FileStream *fs = nullptr;
    try {
        if (work_mode == kFile_Read) // NOTE: BufferedStream does not work correctly in the write mode
            fs = new BufferedStream(filename, open_mode, work_mode);
        else
            fs = new FileStream(filename, open_mode, work_mode);
        if (fs != nullptr && !fs->IsValid()) {
            delete fs;
            fs = nullptr;
        }
    } catch(std::runtime_error) {
        fs = nullptr;
    }
    return fs;
}


Stream *File::OpenStdin()
{
    return FileStream::WrapHandle(stdin, kFile_Read, kDefaultSystemEndianess);
}

Stream *File::OpenStdout()
{
    return FileStream::WrapHandle(stdout, kFile_Write, kDefaultSystemEndianess);
}

Stream *File::OpenStderr()
{
    return FileStream::WrapHandle(stderr, kFile_Write, kDefaultSystemEndianess);
}

String File::FindFileCI(const String &dir_name, const String &file_name)
{
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
    // Simply concat dir and filename paths
    return Path::ConcatPaths(dir_name, file_name);
#else
    // Case insensitive file find - on case sensitive filesystems
    //
    // TODO: still not covered: a situation when the file_name contains
    // nested path -and- the case of at least one path parts does not match
    // (with all matching case the file will be found by an early check).
    //
    struct stat   statbuf;
    struct dirent *entry = nullptr;

    if (dir_name.IsEmpty() && file_name.IsEmpty())
        return nullptr;

    String directory;
    String filename;
    String buf;

    if (!dir_name.IsEmpty())
    {
        directory = dir_name;
        Path::FixupPath(directory);
    }
    if (!file_name.IsEmpty())
    {
        filename = file_name;
        Path::FixupPath(filename);
    }

    if (!filename.IsEmpty())
    {
        // TODO: move this case to ConcatPaths too?
        if (directory.IsEmpty() && filename[0] == '/')
            buf = filename;
        else
            buf = Path::ConcatPaths(directory.IsEmpty() ? "." : directory, filename);

        if (lstat(buf.GetCStr(), &statbuf) == 0 &&
            (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)))
        {
            return buf;
        }
    }

    if (directory.IsEmpty())
    {
        String match = Path::GetFilename(filename);
        if (match.IsEmpty())
            return nullptr;
        directory = Path::GetParent(filename);
        filename = match;
    }

    DIR *rough = nullptr;
    if ((rough = opendir(directory.GetCStr())) == nullptr)
    {
        fprintf(stderr, "ci_find_file: cannot open directory: %s\n", directory.GetCStr());
        return nullptr;
    }

    String diamond;
    while ((entry = readdir(rough)) != nullptr)
    {
        if (strcasecmp(filename.GetCStr(), entry->d_name) == 0)
        {
            if (lstat(entry->d_name, &statbuf) == 0 &&
                (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)))
            {
#if AGS_PLATFORM_DEBUG
                fprintf(stderr, "ci_find_file: Looked for %s in rough %s, found diamond %s.\n",
                    filename.GetCStr(), directory.GetCStr(), entry->d_name);
#endif // AGS_PLATFORM_DEBUG
                diamond = Path::ConcatPaths(directory, entry->d_name);
                break;
            }
        }
    }
    closedir(rough);
    return diamond;
#endif
}

Stream *File::OpenFileCI(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode)
{
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
    return File::OpenFile(file_name, open_mode, work_mode);
#else
    String fullpath = FindFileCI(nullptr, file_name);
    if (!fullpath.IsEmpty())
        return File::OpenFile(fullpath, open_mode, work_mode);
    // If the file was not found, and it's Create mode, then open new file
    if (open_mode != kFile_Open)
        return File::OpenFile(file_name, open_mode, work_mode);
    return nullptr;
#endif
}

Stream *File::OpenFile(const String &filename, soff_t start_off, soff_t end_off)
{
    try {
        FileStream *fs = new BufferedSectionStream(filename, start_off, end_off, kFile_Open, kFile_Read);
        if (fs != nullptr && !fs->IsValid()) {
            delete fs;
            return nullptr;
        }
        return fs;
    }
    catch (std::runtime_error) {
        return nullptr;
    }
}

} // namespace Common
} // namespace AGS
