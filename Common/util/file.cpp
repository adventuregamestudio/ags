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
#include "util/file.h"
#include <errno.h>
#include <stdexcept>
#include "core/platform.h"
#include "util/bufferedstream.h"
#include "util/filestream.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"
#if defined (AGS_CASE_SENSITIVE_FILESYSTEM)
#include "util/directory.h"
#endif
#if AGS_PLATFORM_OS_ANDROID
#include "util/aasset_stream.h"
#include "util/android_file.h"
#endif

namespace AGS
{
namespace Common
{

bool File::IsDirectory(const String &filename)
{
    // stat() does not like trailing slashes, remove them
    String fixed_path = Path::MakePathNoSlash(filename);
    return ags_directory_exists(fixed_path.GetCStr()) != 0;
}

bool File::IsFile(const String &filename)
{
    bool res = ags_file_exists(filename.GetCStr()) != 0;
#if AGS_PLATFORM_OS_ANDROID
    if (!res)
        res = GetAAssetExists(filename);
#endif // AGS_PLATFORM_OS_ANDROID
    return res;
}

bool File::IsFileOrDir(const String &filename)
{
    // stat() does not like trailing slashes, remove them
    String fixed_path = Path::MakePathNoSlash(filename);
    bool res = ags_path_exists(fixed_path.GetCStr()) != 0;
#if AGS_PLATFORM_OS_ANDROID
    if (!res)
        res = GetAAssetExists(filename);
#endif // AGS_PLATFORM_OS_ANDROID
    return res;
}

soff_t File::GetFileSize(const String &filename)
{
    soff_t size = ags_file_size(filename.GetCStr());
#if AGS_PLATFORM_OS_ANDROID
    if (size < 0)
        size = GetAAssetSize(filename);
#endif
    return size;
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
        ags_file_remove(filename.GetCStr());
        return true;
    }
    return false;
}

bool File::DeleteFile(const String &filename)
{
    if (ags_file_remove(filename.GetCStr()) != 0)
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
    return ags_file_rename(old_name.GetCStr(), new_name.GetCStr()) == 0;
}

bool File::CopyFile(const String &src_path, const String &dst_path, bool overwrite)
{
    return ags_file_copy(src_path.GetCStr(), dst_path.GetCStr(), overwrite) == 0;
}

bool File::GetFileModesFromCMode(const String &cmode, FileOpenMode &open_mode, StreamMode &work_mode)
{
    // We do not test for 'b' and 't' here, because text mode reading/writing should be done with
    // the use of ITextReader and ITextWriter implementations.
    // The number of supported variants here is quite limited due the restrictions AGS makes on them.
    bool read_base_mode = false;
    // Default mode is open/read for safety reasons
    open_mode = kFile_Open;
    work_mode = kStream_Read;
    for (size_t c = 0; c < cmode.GetLength(); ++c)
    {
        if (read_base_mode)
        {
            if (cmode[c] == '+')
            {
                work_mode = kStream_ReadWrite;
            }
            break;
        }
        else
        {
            if (cmode[c] == 'r')
            {
                open_mode = kFile_Open;
                work_mode = kStream_Read;
                read_base_mode = true;
            }
            else if (cmode[c] == 'a')
            {
                open_mode = kFile_Create;
                work_mode = kStream_Write;
                read_base_mode = true;
            }
            else if (cmode[c] == 'w')
            {
                open_mode = kFile_CreateAlways;
                work_mode = kStream_Write;
                read_base_mode = true;
            }
        }
    }
    return read_base_mode;
}

String File::GetCMode(FileOpenMode open_mode, StreamMode work_mode)
{
    String mode;
    // filter out only read/write flags
    work_mode = static_cast<StreamMode>(work_mode & kStream_ReadWrite);
    if (open_mode == kFile_Open)
    {
        if (work_mode == kStream_Read)
            mode.AppendChar('r');
        else if (work_mode == kStream_Write || work_mode == kStream_ReadWrite)
            mode.Append("r+");
    }
    else if (open_mode == kFile_Create)
    {
        if (work_mode == kStream_Write)
            mode.AppendChar('a');
        else if (work_mode == kStream_Read || work_mode == kStream_ReadWrite)
            mode.Append("a+");
    }
    else if (open_mode == kFile_CreateAlways)
    {
        if (work_mode == kStream_Write)
            mode.AppendChar('w');
        else if (work_mode == kStream_Read || work_mode == kStream_ReadWrite)
            mode.Append("w+");
    }
    mode.AppendChar('b');
    return mode;
}

Stream *File::OpenFile(const String &filename, FileOpenMode open_mode, StreamMode work_mode)
{
    Stream *fs = nullptr;
    try {
        fs = new BufferedStream(filename, open_mode, work_mode);
        if (fs != nullptr && !fs->IsValid()) {
            delete fs;
            fs = nullptr;
        }
    } catch(std::runtime_error) {
        fs = nullptr;
#if AGS_PLATFORM_OS_ANDROID
        // strictly for read-only streams: look into Android Assets too
        try {
            if ((work_mode & kStream_Write) == 0)
                fs = new AAssetStream(filename, AASSET_MODE_RANDOM);
            if (fs != nullptr && !fs->IsValid()) {
                delete fs;
                fs = nullptr;
            }
        } catch(std::runtime_error) {
            fs = nullptr;
        }
#endif
    }
    return fs;
}


Stream *File::OpenStdin()
{
    return FileStream::WrapHandle(stdin, kStream_Read, kDefaultSystemEndianess);
}

Stream *File::OpenStdout()
{
    return FileStream::WrapHandle(stdout, kStream_Write, kDefaultSystemEndianess);
}

Stream *File::OpenStderr()
{
    return FileStream::WrapHandle(stderr, kStream_Write, kDefaultSystemEndianess);
}

String File::FindFileCI(const String &base_dir, const String &file_name,
    bool is_dir, String *most_found, String *not_found)
{
    // Case insensitive file find - on case sensitive filesystems
    if (most_found)
        *most_found = base_dir;
    if (not_found)
        *not_found = "";

    if (file_name.IsEmpty())
        return {}; // fail, no filename provided

    String directory;
    String filename;
    if (!base_dir.IsEmpty())
    {
        directory = base_dir;
        Path::FixupPath(directory);
    }
    filename = file_name;
    Path::FixupPath(filename);

    String test;
    // FIXME: handle these conditions in ConcatPaths too?
    const bool is_relative = Path::IsRelativePath(filename);
    if (is_relative && directory.IsEmpty())
        test = Path::ConcatPaths(".", filename);
    else if (is_relative)
        test = Path::ConcatPaths(directory, filename);
    else if (!is_relative && directory.IsEmpty())
        test = filename; // absolute filepath
    else
        return {}; // fail, cannot be handled

    // First try exact match
    if ((is_dir && File::IsDirectory(test.GetCStr())) ||
        (!is_dir && File::IsFile(test.GetCStr())))
        return test; // success

#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
    // TODO: most_found & not_found are never used in practice with CI fs;
    // but if becomes necessary, we may reuse most of the CS code below
    // for this case too, and uncommenting the following condition -
    /* if (!most_found && !not_found) */
    return {}; // fail
#else
    // Begin splitting filename into path sections,
    // for each section open previous dir and search for any case-insensitive
    // match for the next section.
    if (directory.IsEmpty())
    {
        if (is_relative)
        {
            directory = "./";
        }
        else
        {
            // NOTE: we intentionally don't support ci-parsing full absolute path
            directory = Path::GetParent(filename);
            filename = Path::GetFilename(filename);
        }
    }
    
    String path = directory;
    size_t begin = 0u;
    for (size_t end = filename.FindChar('/', 0u); // TODO: string iterators
        end > begin; begin = end + 1, end = filename.FindChar('/', end + 1))
    {
        end = std::min(end, filename.GetLength()); // FIXME: iterators? string view?
        test.SetString(filename.GetCStr() + begin, end - begin);
        if (test.Compare(".") == 0)
            continue; // let them have random "/./" in the middle of the path

        auto di = DirectoryIterator::Open(path);
        if (!di)
        {
            fprintf(stderr, "FindFileCI: cannot open directory: %s\n", path.GetCStr());
            break; // failed
        }

        bool found = false;
        for (; !di.AtEnd(); di.Next())
        {
            if (test.CompareNoCase(di.Current()) != 0)
                continue;

            Path::AppendPath(path, di.Current()); // append exact entry's name
            // We succeed when we are at the end of the searched path,
            // and this is a matching file / dir, as requested
            if (end == filename.GetLength() &&
                ((is_dir && di.GetEntry().IsDir) || (!is_dir && di.GetEntry().IsFile)))
            {
            #if AGS_PLATFORM_DEBUG
                fprintf(stderr, "FindFileCI: Looked for %s in rough %s, found diamond %s.\n",
                    test.GetCStr(), directory.GetCStr(), path.GetCStr());
            #endif // AGS_PLATFORM_DEBUG
                return path;
            }

            found = true;
            break; // found matching subdir
        }

        if (!found)
            break; // failed
    }

    // On failure: fill most_found but return empty string
    if (most_found)
        *most_found = path;
    if (not_found)
        *not_found = filename.GetCStr() + begin;
    return {};
#endif
}

Stream *File::OpenFileCI(const String &base_dir, const String &file_name, FileOpenMode open_mode, StreamMode work_mode)
{
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
    return File::OpenFile(Path::ConcatPaths(base_dir, file_name), open_mode, work_mode);
#else
    String fullpath = FindFileCI(base_dir, file_name);
    if (!fullpath.IsEmpty())
        return File::OpenFile(fullpath, open_mode, work_mode);
    // If the file was not found, and it's Create mode, then open new file
    if (open_mode != kFile_Open)
        return File::OpenFile(Path::ConcatPaths(base_dir, file_name), open_mode, work_mode);
    return nullptr;
#endif
}

Stream *File::OpenFile(const String &filename, soff_t start_off, soff_t end_off)
{
    try {
        Stream *fs = new BufferedSectionStream(filename, start_off, end_off, kFile_Open, kStream_Read);
        if (fs != nullptr && !fs->IsValid()) {
            delete fs;
            return nullptr;
        }
        return fs;
    }
    catch (std::runtime_error) {
        Stream* fs = nullptr;
#if AGS_PLATFORM_OS_ANDROID
        try {
            fs = new AAssetStream(filename, AASSET_MODE_RANDOM, start_off, end_off);
            if (fs != nullptr && !fs->IsValid()) {
                delete fs;
                fs = nullptr;
            }
        } catch(std::runtime_error) {
            fs = nullptr;
        }
#endif
        return fs;
    }
}

} // namespace Common
} // namespace AGS
