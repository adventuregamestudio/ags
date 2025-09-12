//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/asset_helper.h"
#include "ac/audiocliptype.h"
#include "ac/file.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/path_helper.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "platform/base/agsplatformdriver.h"
#include "util/stream.h"
#include "core/assetmanager.h"
#include "core/asset.h"
#include "main/engine.h"
#include "main/game_file.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern AGSPlatformDriver *platform;

// object-based File routines

int32_t FileOpen(const char *fnmm, FileOpenMode open_mode, StreamMode work_mode)
{
    debug_script_print(kDbgMsg_Debug, "FileOpen: request: %s, mode: %s",
                       fnmm, File::GetCMode(open_mode, work_mode).GetCStr());
    std::unique_ptr<Stream> s(ResolveScriptPathAndOpen(fnmm, open_mode, work_mode));
    if (!s)
        return 0;

    String res_path = s->GetPath();
    int32_t handle = add_file_stream(std::move(s), "FileOpen");
    debug_script_print(kDbgMsg_Info, "FileOpen: success, handle %d, path: %s", handle, res_path.GetCStr());
    return handle;
}

void FileClose(int32_t handle) {
    close_file_stream(handle, "FileClose");
}

int File_Exists(const char *fnmm) {
  const auto rp = ResolveScriptPathAndFindFile(fnmm, true);
  if (!rp)
    return 0;

  if (rp.AssetMgr)
    return AssetMgr->DoesAssetExist(rp.FullPath, "*") ? 1 : 0;
  return 1; // was found in fs
}

ScriptDateTime* File_GetFileTime(const char *fnmm) {
  const auto rp = ResolveScriptPathAndFindFile(fnmm, true);
  if (!rp)
    return nullptr;

  time_t ft;
  if (rp.AssetMgr)
  {
    if (!AssetMgr->GetAssetTime(rp.FullPath, ft, "*"))
      return nullptr;
  }
  else
  {
    ft = File::GetFileTime(rp.FullPath);
  }

  ScriptDateTime *sdt = ScriptDateTime::FromStdTime(ft);
  ccRegisterManagedObject(sdt, sdt);
  return sdt;
}

int File_Delete(const char *fnmm) {
  const auto rp = ResolveScriptPathAndFindFile(fnmm, false);
  if (!rp)
    return 0;

  return File::DeleteFile(rp.FullPath) ? 1 : 0;
}

int File_Copy(const char *old_name, const char *new_name) {
  // first path is readonly, second must be writeable and create dirs
  const auto old_rp = ResolveScriptPathAndFindFile(old_name, true);
  if (!old_rp)
    return 0;
  const auto new_rp = ResolveWritePathAndCreateDirs(new_name);
  if (!new_rp)
    return 0;

  if (Path::ComparePaths(old_rp.FullPath, new_rp.FullPath) == 0)
    return 0; // cannot copy into itself

  if (!old_rp.AssetMgr)
    return File::CopyFile(old_rp.FullPath, new_rp.FullPath, true) ? 1 : 0;

  auto in = AssetMgr->OpenAsset(old_rp.FullPath, "*");
  auto out = File::CreateFile(new_rp.FullPath);
  if (!in || !out)
    return 0;

  return (CopyStream(in.get(), out.get(), in->GetLength()) == in->GetLength()) ? 1 : 0;
}

int File_Rename(const char *old_name, const char *new_name) {
  // both paths must be writeable, but should also create dirs for the second
  const auto old_rp = ResolveScriptPathAndFindFile(old_name, false);
  if (!old_rp)
    return 0;
  const auto new_rp = ResolveWritePathAndCreateDirs(new_name);
  if (!new_rp)
    return 0;

  if (Path::ComparePaths(old_rp.FullPath, new_rp.FullPath) == 0)
    return 0; // cannot rename into itself

  return File::RenameFile(old_rp.FullPath, new_rp.FullPath) ? 1 : 0;
}

static void FillDirList(std::vector<FileEntry> &files, const FSLocation &loc, const String &pattern)
{
    // Do ci search for the location, as parts of the path may have case mismatch
    String path = File::FindFileCI(loc.BaseDir, loc.SubDir, true);
    if (path.IsEmpty())
        return;
    Directory::GetFiles(path, files, pattern);
}

void FillDirList(std::vector<String> &files, const String &pattern, ScriptFileSortStyle file_sort, ScriptSortDirection sort_dir)
{
    ResolvedPath rp = ResolveScriptPath(pattern, true);
    if (!rp)
        return;

    std::vector<FileEntry> fileents;
    if (rp.AssetMgr)
    {
        AssetMgr->FindAssets(fileents, rp.FullPath, "*");
    }
    else
    {
        FillDirList(fileents, rp.Loc, Path::GetFilename(rp.FullPath));
    }

    const bool ascending = (sort_dir != kScSortDescending) || (file_sort == kScFileSort_None);
    switch (file_sort)
    {
    case kScFileSort_Name:
        if (ascending)
            std::sort(fileents.begin(), fileents.end(), FileEntryCmpByNameCI());
        else
            std::sort(fileents.rbegin(), fileents.rend(), FileEntryCmpByNameCI());
        break;
    case kScFileSort_Time:
        if (ascending)
            std::sort(fileents.begin(), fileents.end(), FileEntryCmpByTime());
        else
            std::sort(fileents.rbegin(), fileents.rend(), FileEntryCmpByTime());
        break;
    default: break;
    }

    for (const auto &fe : fileents)
    {
        files.push_back(fe.Name);
    }
}

void *File_GetFiles(const char *filemask, int file_sort, int sort_dir)
{
    file_sort = ValidateFileSort("ListBox.FillDirList", file_sort);
    sort_dir = ValidateSortDirection("ListBox.FillDirList", sort_dir);

    std::vector<String> files;
    FillDirList(files, filemask, (ScriptFileSortStyle)file_sort, (ScriptSortDirection)sort_dir);

    DynObjectRef arr = DynamicArrayHelpers::CreateStringArray(files);
    return arr.Obj;
}

void *sc_OpenFile(const char *fnmm, int mode) {
  if ((mode < scFileRead) || (mode > scFileAppend))
    quit("!OpenFile: invalid file mode");

  sc_File *scf = new sc_File();
  if (scf->OpenFile(fnmm, mode) == 0) {
    delete scf;
    return nullptr;
  }
  ccRegisterManagedObject(scf, scf);
  return scf;
}

const char *File_ResolvePath(const char *fnmm)
{
    ResolvedPath rp = ResolveScriptPathAndFindFile(fnmm, true, true);
    // Make path pretty -
    String path = Path::MakeAbsolutePath(rp.FullPath);
    return CreateNewScriptString(path.GetCStr());
}

void File_Close(sc_File *fil) {
  fil->Close();
}

void FileWrite(int32_t handle, const char *towrite) {
    Stream *out = get_file_stream(handle, "FileWrite");
    size_t len = strlen(towrite);
    out->WriteInt32(len + 1); // write with null-terminator
    out->Write(towrite, len + 1);
}

void File_WriteString(sc_File *fil, const char *towrite) {
  FileWrite(fil->handle, towrite);
}

void FileWriteInt(int32_t handle, int into) {
    Stream *out = get_file_stream(handle, "FileWriteInt");
    out->WriteInt8('I');
    out->WriteInt32(into);
}

void File_WriteInt(sc_File *fil, int towrite) {
  FileWriteInt(fil->handle, towrite);
}

void File_WriteFloat(sc_File *fil, float towrite) {
  Stream *out = get_file_stream(fil->handle, "File.WriteFloat");
  out->WriteInt8('F');
  out->WriteFloat32(towrite);
}

void FileWriteRawChar(int32_t handle, int chartoWrite) {
    Stream *out = get_file_stream(handle, "FileWriteRawChar");
    if ((chartoWrite < 0) || (chartoWrite > 255))
        debug_script_warn("FileWriteRawChar: can only write values 0-255");

    out->WriteByte(static_cast<uint8_t>(chartoWrite));
}

void File_WriteRawChar(sc_File *fil, int towrite) {
  FileWriteRawChar(fil->handle, towrite);
}

void File_WriteRawFloat(sc_File *fil, float towrite) {
  Stream *out = get_file_stream(fil->handle, "FileWriteRawFloat");
  out->WriteFloat32(towrite);
}

void File_WriteRawInt(sc_File *fil, int towrite) {
  Stream *out = get_file_stream(fil->handle, "FileWriteRawInt");
  out->WriteInt32(towrite);
}

void FileWriteRawLine(int32_t handle, const char *towrite) {
    Stream *out = get_file_stream(handle, "FileWriteRawLine");
    out->Write(towrite, strlen(towrite));
    out->WriteInt8('\r');
    out->WriteInt8('\n');
}

void File_WriteRawLine(sc_File *fil, const char *towrite) {
  FileWriteRawLine(fil->handle, towrite);
}

// Reads line of chars until linebreak is met or buffer is filled;
// returns whether reached the end of line (false in case not enough buffer);
// guarantees null-terminator in the buffer.
static bool File_ReadRawLineImpl(sc_File *fil, char* buffer, size_t buf_len) {
    if (buf_len == 0) return false;
    Stream *in = get_file_stream(fil->handle, "File.ReadRawLine");
    for (size_t i = 0; i < buf_len - 1; ++i)
    {
        int c = in->ReadByte();
        if (c < 0 || c == '\n') // EOF or LF
        {
            buffer[i] = 0;
            return true;
        }
        if (c == '\r') // CR or CRLF
        {
            c = in->ReadByte();
            // Look for '\n', but it may be missing, which is also a valid case
            if (c >= 0 && c != '\n') in->Seek(-1, kSeekCurrent);
            buffer[i] = 0;
            return true;
        }
        buffer[i] = c;
    }
    buffer[buf_len - 1] = 0;
    return false; // not enough buffer
}

void File_ReadRawLine(sc_File *fil, char* buffer) {
  File_ReadRawLineImpl(fil, buffer, MAX_MAXSTRLEN);
}

const char* File_ReadRawLineBack(sc_File *fil) {
  char readbuffer[MAX_MAXSTRLEN];
  if (File_ReadRawLineImpl(fil, readbuffer, MAX_MAXSTRLEN))
    return CreateNewScriptString(readbuffer);
  String sbuf = readbuffer;
  bool done = false;
  while (!done)
  {
    done = File_ReadRawLineImpl(fil, readbuffer, MAX_MAXSTRLEN);
    sbuf.Append(readbuffer);
  };
  return CreateNewScriptString(sbuf.GetCStr());
}

void FileRead(int32_t handle, char *toread) {
    VALIDATE_STRING(toread);
    Stream *in = get_file_stream(handle, "FileRead");
    if (in->EOS()) {
        toread[0] = 0;
        return;
    }
    size_t lle = (uint32_t)in->ReadInt32();
    // This tests for the legacy string (limited by 200 chars)
    if ((lle >= 200) | (lle < 1))
    {
        debug_script_warn("FileRead: file was not written by FileWrite");
        return;
    }
    in->Read(toread, lle);
}

void File_ReadString(sc_File *fil, char *toread) {
  FileRead(fil->handle, toread);
}

const char* File_ReadStringBack(sc_File *fil) {
  Stream *in = get_file_stream(fil->handle, "File.ReadStringBack");
  if (in->EOS()) {
    return CreateNewScriptString("");
  }

  size_t data_sz = (uint32_t)in->ReadInt32();
  if (data_sz == 0)
  {
    debug_script_warn("File.ReadStringBack: file was not written by WriteString");
    return CreateNewScriptString("");;
  }

  // NOTE: support both deserialized with and without null terminator for varied use cases
  auto buf = ScriptString::CreateBuffer(data_sz);
  in->Read(buf.Get(), data_sz);
  return CreateNewScriptString(std::move(buf));
}

int FileReadInt(int32_t handle) {
    Stream *in = get_file_stream(handle, "FileReadInt");
    if (in->EOS())
        return -1;
    if (in->ReadInt8() != 'I')
    {
        debug_script_warn("FileReadInt: File read back in wrong order");
        return -1;
    }
    return in->ReadInt32();
}

int File_ReadInt(sc_File *fil) {
  return FileReadInt(fil->handle);
}

float File_ReadFloat(sc_File *fil) {
  Stream *in = get_file_stream(fil->handle, "File.ReadFloat");
  if (in->EOS())
    return -1;
  if (in->ReadInt8() != 'F')
  {
    debug_script_warn("File.ReadFloat: File read back in wrong order");
    return -1;
  }
  return in->ReadFloat32();
}

char FileReadRawChar(int32_t handle) {
    Stream *in = get_file_stream(handle, "FileReadRawChar");
    return static_cast<uint8_t>(in->ReadByte());
    // NOTE: this function has incorrect return value for historical reasons;
    // we keep this strictly for backwards compatibility with old scripts
}

int File_ReadRawChar(sc_File *fil) {
  return FileReadRawChar(fil->handle);
}

int FileReadRawInt(int32_t handle) {
    Stream *in = get_file_stream(handle, "FileReadRawInt");
    if (in->EOS())
        return -1;
    return in->ReadInt32();
}


int File_ReadRawInt(sc_File *fil) {
  return FileReadRawInt(fil->handle);
}

float File_ReadRawFloat(sc_File *fil) {
  Stream *out = get_file_stream(fil->handle, "File.ReadRawFloat");
  return out->ReadFloat32();
}

int File_ReadRawBytes(sc_File *fil, void *arr_obj, int index, int count)
{
    Stream *in = get_file_stream(fil->handle, "File.ReadRawBytes");
    const auto &hdr = CCDynamicArray::GetHeader(arr_obj);
    if (hdr.GetElemCount() == 0)
    {
        debug_script_warn("File.ReadRawBytes: dynamic array has zero length");
        return 0;
    }
    if (index < 0 || static_cast<uint32_t>(index) >= hdr.GetElemCount())
    {
        debug_script_warn("File.ReadRawBytes: starting index out of bounds: %d, range is %u..%u", index, 0u, hdr.GetElemCount() - 1);
        return 0;
    }
    if (count < 0 || static_cast<uint32_t>(index) > hdr.GetElemCount() - index)
    {
        debug_script_warn("File.ReadRawBytes: invalid count: %d, valid range is %u..%u", count, 0u, hdr.GetElemCount() - index);
        return 0;
    }

    uint32_t elem_size = hdr.TotalSize / hdr.GetElemCount();
    uint32_t read_at = index * elem_size;
    uint32_t read_count = std::min(static_cast<uint32_t>(count), hdr.TotalSize - read_at);
    return static_cast<int>(in->Read(static_cast<uint8_t*>(arr_obj) + read_at, read_count));
}

int File_WriteRawBytes(sc_File *fil, void *arr_obj, int index, int count)
{
    Stream *out = get_file_stream(fil->handle, "File.WriteRawBytes");
    const auto &hdr = CCDynamicArray::GetHeader(arr_obj);
    if (hdr.GetElemCount() == 0)
    {
        debug_script_warn("File.WriteRawBytes: dynamic array has zero length");
        return 0;
    }
    if (index < 0 || static_cast<uint32_t>(index) >= hdr.GetElemCount())
    {
        debug_script_warn("File.WriteRawBytes: starting index out of bounds: %d, range is %u..%u", index, 0u, hdr.GetElemCount() - 1);
        return 0;
    }
    if (count < 0 || static_cast<uint32_t>(index) > hdr.GetElemCount() - index)
    {
        debug_script_warn("File.WriteRawBytes: invalid count: %d, valid range is %u..%u", count, 0u, hdr.GetElemCount() - index);
        return 0;
    }

    uint32_t elem_size = hdr.TotalSize / hdr.GetElemCount();
    uint32_t write_at = index * elem_size;
    uint32_t write_count = std::min(static_cast<uint32_t>(count), hdr.TotalSize - write_at);
    return static_cast<int>(out->Write(static_cast<uint8_t*>(arr_obj) + write_at, write_count));
}

int File_Seek(sc_File *fil, int offset, int origin)
{
    Stream *in = get_file_stream(fil->handle, "File.Seek");
    return in->Seek(offset, (StreamSeek)origin);
}

int FileIsEOF(int32_t handle) {
    Stream *stream = get_file_stream(handle, "FileIsEOF");
    if (stream->EOS())
        return 1;

    // TODO: stream errors
    if (stream->GetError())
        return 1;

    if (stream->GetPosition() >= stream->GetLength())
        return 1;
    return 0;
}

int File_GetEOF(sc_File *fil) {
  if (fil->handle <= 0)
    return 1;
  return FileIsEOF(fil->handle);
}

int FileIsError(int32_t handle) {
    Stream *stream = get_file_stream(handle, "FileIsError");

    // TODO: stream errors
    if (stream->GetError())
        return 1;

    return 0;
}

int File_GetError(sc_File *fil) {
  if (fil->handle <= 0)
    return 1;
  return FileIsError(fil->handle);
}

int File_GetPosition(sc_File *fil)
{
    if (fil->handle <= 0)
        return -1;
    Stream *stream = get_file_stream(fil->handle, "File.Position");
    // TODO: a problem is that AGS script does not support unsigned or long int
    return (int)stream->GetPosition();
}

const char *File_GetPath(sc_File *fil)
{
    if (fil->handle <= 0)
        return nullptr;
    Stream *stream = get_file_stream(fil->handle, "File.Path");
    return CreateNewScriptString(stream->GetPath());
}

//=============================================================================


const String GameInstallRootToken    = "$INSTALLDIR$";
const String UserSavedgamesRootToken = "$MYDOCS$";
const String GameSavedgamesDirToken  = "$SAVEGAMEDIR$";
const String GameDataDirToken        = "$APPDATADIR$";
const String GameAssetToken          = "$DATA$";
const String UserConfigFileToken     = "$CONFIGFILE$";

void FixupFilename(char *filename)
{
    const char *illegal = platform->GetIllegalFileChars();
    for (char *name_ptr = filename; *name_ptr; ++name_ptr)
    {
        if (*name_ptr < ' ')
        {
            *name_ptr = '_';
        }
        else
        {
            for (const char *ch_ptr = illegal; *ch_ptr; ++ch_ptr)
                if (*name_ptr == *ch_ptr)
                    *name_ptr = '_';
        }
    }
}

String PathFromInstallDir(const String &path)
{
    if (Path::IsRelativePath(path))
        return Path::ConcatPaths(ResPaths.DataDir, path);
    return path;
}

FSLocation PathFromInstallDir(const FSLocation &fsloc)
{
    if (Path::IsRelativePath(fsloc.FullDir))
        return FSLocation(ResPaths.DataDir).Concat(fsloc.FullDir);
    return fsloc;
}

String PreparePathForWriting(const FSLocation& fsloc, const String &filename)
{
    if (Directory::CreateAllDirectories(fsloc.BaseDir, fsloc.SubDir))
        return Path::ConcatPaths(fsloc.FullDir, filename);
    return "";
}

FSLocation GetGlobalUserConfigDir()
{
    FSLocation dir = platform->GetUserGlobalConfigDirectory();
    if (Path::IsRelativePath(dir.FullDir)) // relative dir is resolved relative to the game data dir
        return FSLocation(ResPaths.DataDir).Concat(dir.FullDir);
    return dir;
}

FSLocation GetGameUserConfigDir()
{
    FSLocation dir = platform->GetUserConfigDirectory();
    if (!usetup.UserConfDir.IsEmpty()) // directive to use custom userconf location
        return FSLocation(usetup.UserConfDir);
    else if (Path::IsRelativePath(dir.FullDir)) // relative dir is resolved relative to the game data dir
        return FSLocation(ResPaths.DataDir).Concat(dir.FullDir);
    // For absolute dir, we assume it's a special directory prepared for AGS engine
    // and therefore append a game's own subdir
    return dir.Concat(game.saveGameFolderName);
}

// Constructs data dir using rules for default system location
inline FSLocation MakeDefaultDataDir(const FSLocation &def_dir)
{
    // Relative dir is resolved relative to the game data dir
    if (Path::IsRelativePath(def_dir.FullDir))
        return FSLocation(ResPaths.DataDir).Concat(def_dir.FullDir);
    // For absolute dir, we assume it's a special directory prepared for AGS engine
    // and therefore amend it with a game's own subdir (to separate files from different games)
    return def_dir.Concat(game.saveGameFolderName);
}

// Constructs data dir using rules for the user-specified location
inline FSLocation MakeUserDataDir(const String &user_dir)
{
    // If user-set location is inside game dir, then form a relative path
    if (Path::IsRelativePath(user_dir))
        return FSLocation(ResPaths.DataDir).Concat(user_dir);
    // Otherwise treat it as an absolute path
    return FSLocation(Path::MakeAbsolutePath(user_dir));
}

FSLocation GetGameAppDataDir()
{
    if (usetup.AppDataDir.IsEmpty())
        return MakeDefaultDataDir(platform->GetAllUsersDataDirectory());
    return MakeUserDataDir(usetup.AppDataDir);
}

FSLocation GetGameUserDataDir()
{
    if (usetup.UserSaveDir.IsEmpty())
        return MakeDefaultDataDir(platform->GetUserSavedgamesDirectory());
    return MakeUserDataDir(usetup.UserSaveDir);
}

bool CreateFSDirs(const FSLocation &fs)
{
    return Directory::CreateAllDirectories(fs.BaseDir, fs.SubDir);
}

ResolvedPath ResolveScriptPath(const String &orig_sc_path, bool read_only)
{
    // Make sure that the script path has a system-portable form;
    String sc_path = orig_sc_path;
    sc_path.Replace('\\', '/');
    sc_path.MergeSequences('/');

    // TODO: much of the following may be refactored into having a map (or list of pairs)
    // where key is a token to find in sc_path, and value is FSLocation.
    // Probably have a kind of a virtual game filesystem which provides such map,
    // and lets configure actual locations.

    // File tokens (they must be the only thing in script path)
    if (sc_path.Compare(UserConfigFileToken) == 0)
    {
        auto loc = GetGameUserConfigDir();
        return ResolvedPath(loc, DefaultConfigFileName);
    }

    // Test absolute paths
    if (!Path::IsRelativePath(sc_path))
    {
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to absolute path)", sc_path.GetCStr());
            return {};
        }
        return ResolvedPath(sc_path);
    }

    // Resolve location tokens
    if (sc_path.CompareLeft(GameAssetToken) == 0)
    {
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game assets)", sc_path.GetCStr());
            return {};
        }
        return ResolvedPath(sc_path.Mid(GameAssetToken.GetLength() + 1), true);
    }

    FSLocation parent_dir;
    String child_path;
    if (sc_path.CompareLeft(GameInstallRootToken) == 0)
    {
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game installation directory)",
                sc_path.GetCStr());
            return {};
        }
        parent_dir = FSLocation(ResPaths.DataDir);
        child_path = sc_path.Mid(GameInstallRootToken.GetLength());
    }
    else if (sc_path.CompareLeft(GameSavedgamesDirToken) == 0)
    {
        parent_dir = FSLocation(get_save_game_directory()); // FIXME: get FSLocation of save dir 
        child_path = sc_path.Mid(GameSavedgamesDirToken.GetLength());
    }
    else if (sc_path.CompareLeft(GameDataDirToken) == 0)
    {
        parent_dir = GetGameAppDataDir();
        child_path = sc_path.Mid(GameDataDirToken.GetLength());
    }
    // FIXME: invalid token case; parse token prefix between $$ first
    else
    {
        // If no token found -- assume game installation directory
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game installation directory)",
                sc_path.GetCStr());
            return {};
        }
        parent_dir = FSLocation(ResPaths.DataDir);
        child_path = sc_path;
    }

    child_path.TrimLeft('/'); // remove any preceding slash, or this will be abs path
    // Create a proper ResolvedPath with FSLocation separating base location
    // (which the engine is not allowed to create) and sub-dirs (created by the engine).
    // FIXME: following 2 lines may be redundant, maybe may just use ResolvedPath ctor
    parent_dir = parent_dir.Concat(Path::GetParent(child_path));
    child_path = Path::GetFilename(child_path);
    ResolvedPath test_rp = ResolvedPath(parent_dir, child_path);
    // don't allow write operations for relative paths outside game dir
    if (!read_only)
    {
        if (!Path::IsSameOrSubDir(test_rp.Loc.FullDir, test_rp.FullPath))
        {
            debug_script_warn("Attempt to access file '%s' denied (outside of game directory)", sc_path.GetCStr());
            return {};
        }
    }
    return test_rp;
}

ResolvedPath ResolveScriptPathAndFindFile(const String &sc_path, bool read_only, bool ignore_find_result)
{
    ResolvedPath rp = ResolveScriptPath(sc_path, read_only);
    if (!rp)
    {
        debug_script_warn("ResolveScriptPath: failed to resolve path: %s", sc_path.GetCStr());
        return {}; // cannot be resolved
    }

    if (rp.AssetMgr)
        return rp; // we don't test AssetMgr here

    ResolvedPath final_rp = rp;
    String most_found, missing_path;
    String found_file = File::FindFileCI(rp.Loc.BaseDir, rp.SubPath, false, &most_found, &missing_path);
    if (found_file.IsEmpty())
    {
        if (ignore_find_result)
        {
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
            return final_rp; // if we want a case-precise result, need to adjust FindFileCI, see comment inside
#else
            return ResolvedPath(most_found, missing_path);
#endif
        }

        debug_script_warn("ResolveScriptPath: failed to find a match for: %s\n\ttried: %s",
            sc_path.GetCStr(), rp.FullPath.GetCStr());
        return {}; // nothing matching found
    }
    return ResolvedPath(found_file);
}

ResolvedPath ResolveWritePathAndCreateDirs(const String &sc_path)
{
    ResolvedPath rp = ResolveScriptPath(sc_path, false);
    if (!rp)
        return {}; // cannot be resolved

    String most_found, missing_path, res_path;
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
    most_found = rp.Loc.BaseDir;
    missing_path = rp.Loc.SubDir;
    res_path = rp.FullPath;
#else
    // First do case-insensitive search to find an existing part of the SubDir:
    // because some portion may exist but mismatch case, and CreateAllDirectories
    // won't detect that.
    String found_file = File::FindFileCI(rp.Loc.BaseDir, rp.SubPath, false, &most_found, &missing_path);
    if (!found_file.IsEmpty())
    {
        // the file already exists, overwrite it
        return ResolvedPath(found_file);
    }
    res_path = Path::ConcatPaths(most_found, missing_path);
    missing_path = Path::GetParent(missing_path);
#endif

    if (!Directory::CreateAllDirectories(most_found, missing_path))
    {
        debug_script_warn("ResolveScriptPath: failed to create all subdirectories: %s", rp.FullPath.GetCStr());
        return {}; // fail
    }
    return ResolvedPath(res_path);
}

std::unique_ptr<Stream> ResolveScriptPathAndOpen(const String &sc_path,
    FileOpenMode open_mode, StreamMode work_mode)
{
    ResolvedPath rp;
    if (open_mode == kFile_Open && work_mode == kStream_Read)
        rp = ResolveScriptPathAndFindFile(sc_path, true);
    else
        rp = ResolveWritePathAndCreateDirs(sc_path);

    if (!rp)
        return nullptr;
    auto s = rp.AssetMgr ?
        AssetMgr->OpenAsset(rp.FullPath, "*") :
        File::OpenFile(rp.FullPath, open_mode, work_mode);
    if (!s)
        debug_script_warn("FileOpen: failed to open: %s", rp.FullPath.GetCStr());
    return s;
}

//
// AGS custom PACKFILE callbacks, that use our own Stream object
//
static int ags_pf_fclose(void *userdata)
{
    delete (AGS_PACKFILE_OBJ*)userdata;
    return 0;
}

static int ags_pf_getc(void *userdata)
{
    AGS_PACKFILE_OBJ* obj = (AGS_PACKFILE_OBJ*)userdata;
    if (obj->remains > 0)
    {
        obj->remains--;
        return obj->stream->ReadByte();
    }
    return -1;
}

static int ags_pf_ungetc(int /*c*/, void* /*userdata*/)
{
    return -1; // we do not want to support this
}

static long ags_pf_fread(void *p, long n, void *userdata)
{
    AGS_PACKFILE_OBJ* obj = (AGS_PACKFILE_OBJ*)userdata;
    if (obj->remains > 0)
    {
        size_t read = std::min(obj->remains, (size_t)n);
        obj->remains -= read;
        return obj->stream->Read(p, read);
    }
    return -1;
}

static int ags_pf_putc(int /*c*/, void* /*userdata*/)
{
    return -1;  // don't support write
}

static long ags_pf_fwrite(AL_CONST void* /*p*/, long /*n*/, void* /*userdata*/)
{
    return -1; // don't support write
}

static int ags_pf_fseek(void *userdata, int offset)
{
    AGS_PACKFILE_OBJ* obj = (AGS_PACKFILE_OBJ*)userdata;
    obj->stream->Seek(offset, kSeekCurrent);
    return 0;
}

static int ags_pf_feof(void *userdata)
{
    return ((AGS_PACKFILE_OBJ*)userdata)->remains == 0;
}

static int ags_pf_ferror(void *userdata)
{
    return ((AGS_PACKFILE_OBJ*)userdata)->stream->GetError() ? 1 : 0;
}

// Custom PACKFILE callback table
static PACKFILE_VTABLE ags_packfile_vtable = {
    ags_pf_fclose,
    ags_pf_getc,
    ags_pf_ungetc,
    ags_pf_fread,
    ags_pf_putc,
    ags_pf_fwrite,
    ags_pf_fseek,
    ags_pf_feof,
    ags_pf_ferror
};
//

PACKFILE *PackfileFromStream(std::unique_ptr<Stream> stream)
{
    const size_t asset_size = stream->GetLength();
    if (asset_size == 0) return nullptr;
    AGS_PACKFILE_OBJ* obj = new AGS_PACKFILE_OBJ;
    obj->stream = std::move(stream);
    obj->asset_size = asset_size;
    obj->remains = asset_size;
    return pack_fopen_vtable(&ags_packfile_vtable, obj);
}

String find_assetlib(const String &filename)
{
    String libname = File::FindFileCI(ResPaths.DataDir, filename);
    if (!libname.IsEmpty() && AssetManager::IsDataFile(libname))
        return libname;

    // Look up in the alternative locations;
    // Test only locations that include "general data" filter (= empty filter)
    for (const auto &opt_dir : ResPaths.OptDataDirs)
    {
        if (opt_dir.second.FindSection("", ',') != String::NoIndex)
        {
            libname = File::FindFileCI(opt_dir.first, filename);
            if (!libname.IsEmpty() && AssetManager::IsDataFile(libname))
                return libname;
        }
    }
    return "";
}

AssetPath get_audio_clip_assetpath(int /*bundling_type*/, const String &filename)
{ // NOTE: bundling_type is ignored now
    return AssetPath(filename, "audio");
}

AssetPath get_voice_over_assetpath(const String &filename)
{
    return AssetPath(filename, "voice");
}

//=============================================================================

// ScriptFileHandle is a wrapper over a Stream object, prepared for script.
class ScriptFileHandle
{
public:
    ScriptFileHandle() = default;
    ScriptFileHandle(std::unique_ptr<Stream> &&s, int32_t handle)
        : _s(std::move(s)), _handle(handle) {}

    Stream *GetStream() const { return _s.get(); }
    int32_t GetHandle() const { return _handle; }

    // Releases Stream ownership; used in case of temporary stream wrap
    Stream *ReleaseStream() { return _s.release(); }

private:
    std::unique_ptr<Stream> _s;
    int32_t _handle = 0;
};

std::vector<std::unique_ptr<ScriptFileHandle>> file_streams;

int32_t add_file_stream(std::unique_ptr<Stream> &&stream, const char * /*operation_name*/)
{
    uint32_t handle = 1;
    for (; handle < file_streams.size() && file_streams[handle]; ++handle) {}
    if (handle >= file_streams.size())
        file_streams.resize(handle + 1);
    file_streams[handle].reset(new ScriptFileHandle(std::move(stream), handle));
    return static_cast<int32_t>(handle);
}

static ScriptFileHandle *check_file_stream(int32_t fhandle, const char *operation_name)
{
    if (fhandle <= 0 || static_cast<uint32_t>(fhandle) >= file_streams.size()
        || !file_streams[fhandle])
    {
        quitprintf("!%s: invalid file handle; file not previously opened or has been closed", operation_name);
        return nullptr;
    }
    return file_streams[fhandle].get();
}

void close_file_stream(int32_t fhandle, const char *operation_name)
{
    if (fhandle <= 0 || static_cast<uint32_t>(fhandle) >= file_streams.size()
        || !file_streams[fhandle])
    {
        quitprintf("!%s: invalid file handle; file not previously opened or has been closed", operation_name);
    }
    else
    {
        file_streams[fhandle] = nullptr;
    }
}

Stream *get_file_stream(int32_t fhandle, const char *operation_name)
{
    ScriptFileHandle *fh = check_file_stream(fhandle, operation_name);
    return fh ? fh->GetStream() : nullptr;
}

IStreamBase *get_file_stream_iface(int32_t fhandle, const char *operation_name)
{
    ScriptFileHandle *fh = check_file_stream(fhandle, operation_name);
    return fh ? fh->GetStream()->GetStreamBase() : nullptr;
}

void close_all_file_streams()
{
    file_streams.clear();
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/scriptstring.h"


RuntimeScriptValue Sc_File_Copy(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ2(File_Copy, const char, const char);
}

// int (const char *fnmm)
RuntimeScriptValue Sc_File_Delete(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(File_Delete, const char);
}

// int (const char *fnmm)
RuntimeScriptValue Sc_File_Exists(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(File_Exists, const char);
}

RuntimeScriptValue Sc_File_GetFileTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptDateTime, File_GetFileTime, const char);
}

RuntimeScriptValue Sc_File_Rename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ2(File_Rename, const char, const char);
}

RuntimeScriptValue Sc_File_GetFiles(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ_PINT2(void, globalDynamicArray, File_GetFiles, const char);
}

// void *(const char *fnmm, int mode)
RuntimeScriptValue Sc_sc_OpenFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT(sc_File, sc_OpenFile, const char);
}

RuntimeScriptValue Sc_File_ResolvePath(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, File_ResolvePath, const char);
}

// void (sc_File *fil)
RuntimeScriptValue Sc_File_Close(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(sc_File, File_Close);
}

// int (sc_File *fil)
RuntimeScriptValue Sc_File_ReadInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadInt);
}

RuntimeScriptValue Sc_File_ReadFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(sc_File, File_ReadFloat);
}

// int (sc_File *fil)
RuntimeScriptValue Sc_File_ReadRawChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadRawChar);
}

RuntimeScriptValue Sc_File_ReadRawFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(sc_File, File_ReadRawFloat);
}

// int (sc_File *fil)
RuntimeScriptValue Sc_File_ReadRawInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadRawInt);
}

// void (sc_File *fil, char* buffer)
RuntimeScriptValue Sc_File_ReadRawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_ReadRawLine, char);
}

// const char* (sc_File *fil)
RuntimeScriptValue Sc_File_ReadRawLineBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(sc_File, const char, myScriptStringImpl, File_ReadRawLineBack);
}

// void (sc_File *fil, char *toread)
RuntimeScriptValue Sc_File_ReadString(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_ReadString, char);
}

// const char* (sc_File *fil)
RuntimeScriptValue Sc_File_ReadStringBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(sc_File, const char, myScriptStringImpl, File_ReadStringBack);
}

// void (sc_File *fil, int towrite)
RuntimeScriptValue Sc_File_WriteInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteInt);
}

RuntimeScriptValue Sc_File_WriteFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(sc_File, File_WriteFloat);
}

// void (sc_File *fil, int towrite)
RuntimeScriptValue Sc_File_WriteRawChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteRawChar);
}

RuntimeScriptValue Sc_File_WriteRawFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(sc_File, File_WriteRawFloat);
}

RuntimeScriptValue Sc_File_WriteRawInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteRawInt);
}

// void (sc_File *fil, const char *towrite)
RuntimeScriptValue Sc_File_WriteRawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_WriteRawLine, const char);
}

// void (sc_File *fil, const char *towrite)
RuntimeScriptValue Sc_File_WriteString(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_WriteString, const char);
}

RuntimeScriptValue Sc_File_ReadRawBytes(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PINT2(sc_File, File_ReadRawBytes, void);
}

RuntimeScriptValue Sc_File_WriteRawBytes(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PINT2(sc_File, File_WriteRawBytes, void);
}

RuntimeScriptValue Sc_File_Seek(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(sc_File, File_Seek);
}

// int (sc_File *fil)
RuntimeScriptValue Sc_File_GetEOF(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_GetEOF);
}

// int (sc_File *fil)
RuntimeScriptValue Sc_File_GetError(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_GetError);
}

RuntimeScriptValue Sc_File_GetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_GetPosition);
}

RuntimeScriptValue Sc_File_GetPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(sc_File, const char, myScriptStringImpl, File_GetPath);
}


void RegisterFileAPI()
{
    ScFnRegister file_api[] = {
        { "File::Copy^2",             API_FN_PAIR(File_Copy) },
        { "File::Delete^1",           API_FN_PAIR(File_Delete) },
        { "File::Exists^1",           API_FN_PAIR(File_Exists) },
        { "File::GetFileTime^1",      API_FN_PAIR(File_GetFileTime) },
        { "File::GetFiles^3",         API_FN_PAIR(File_GetFiles) },
        { "File::Rename^2",           API_FN_PAIR(File_Rename) },
        { "File::Open^2",             API_FN_PAIR(sc_OpenFile) },
        { "File::ResolvePath^1",      API_FN_PAIR(File_ResolvePath) },

        { "File::Close^0",            API_FN_PAIR(File_Close) },
        { "File::ReadInt^0",          API_FN_PAIR(File_ReadInt) },
        { "File::ReadFloat^0",        API_FN_PAIR(File_ReadFloat) },
        { "File::ReadRawChar^0",      API_FN_PAIR(File_ReadRawChar) },
        { "File::ReadRawInt^0",       API_FN_PAIR(File_ReadRawInt) },
        { "File::ReadRawFloat^0",     API_FN_PAIR(File_ReadRawFloat) },
        { "File::ReadRawLine^1",      API_FN_PAIR(File_ReadRawLine) },
        { "File::ReadRawLineBack^0",  API_FN_PAIR(File_ReadRawLineBack) },
        { "File::ReadString^1",       API_FN_PAIR(File_ReadString) },
        { "File::ReadStringBack^0",   API_FN_PAIR(File_ReadStringBack) },
        { "File::WriteInt^1",         API_FN_PAIR(File_WriteInt) },
        { "File::WriteFloat^1",       API_FN_PAIR(File_WriteFloat) },
        { "File::WriteRawChar^1",     API_FN_PAIR(File_WriteRawChar) },
        { "File::WriteRawFloat^1",    API_FN_PAIR(File_WriteRawFloat) },
        { "File::WriteRawInt^1",      API_FN_PAIR(File_WriteRawInt) },
        { "File::WriteRawLine^1",     API_FN_PAIR(File_WriteRawLine) },
        { "File::WriteString^1",      API_FN_PAIR(File_WriteString) },
        { "File::ReadRawBytes^3",     API_FN_PAIR(File_ReadRawBytes) },
        { "File::WriteRawBytes^3",    API_FN_PAIR(File_WriteRawBytes) },
        { "File::Seek^2",             API_FN_PAIR(File_Seek) },
        { "File::get_EOF",            API_FN_PAIR(File_GetEOF) },
        { "File::get_Error",          API_FN_PAIR(File_GetError) },
        { "File::get_Position",       API_FN_PAIR(File_GetPosition) },
        { "File::get_Path",           API_FN_PAIR(File_GetPath) },
    };

    ccAddExternalFunctions(file_api);
}
