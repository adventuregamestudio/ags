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
#include "ac/asset_helper.h"
#include "ac/audiocliptype.h"
#include "ac/file.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_file.h"
#include "ac/path_helper.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
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

int File_Exists(const char *fnmm) {
  const auto rp = ResolveScriptPathAndFindFile(fnmm, true);
  if (!rp)
    return 0;

  if (rp.AssetMgr)
    return AssetMgr->DoesAssetExist(rp.FullPath, "*") ? 1 : 0;
  return 1; // was found in fs
}

int File_Delete(const char *fnmm) {
  const auto rp = ResolveScriptPathAndFindFile(fnmm, false);
  if (!rp)
    return 0;

  return File::DeleteFile(rp.FullPath) ? 1 : 0;
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

void File_WriteString(sc_File *fil, const char *towrite) {
  FileWrite(fil->handle, towrite);
}

void File_WriteInt(sc_File *fil, int towrite) {
  FileWriteInt(fil->handle, towrite);
}

void File_WriteRawChar(sc_File *fil, int towrite) {
  FileWriteRawChar(fil->handle, towrite);
}

void File_WriteRawInt(sc_File *fil, int towrite) {
  Stream *out = get_file_stream(fil->handle, "FileWriteRawInt");
  out->WriteInt32(towrite);
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
  size_t buflen = check_scstrcapacity(buffer);
  File_ReadRawLineImpl(fil, buffer, buflen);
  commit_scstr_update(buffer);
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

  auto buf = ScriptString::CreateBuffer(data_sz - 1); // stored len + 1
  in->Read(buf.Get(), data_sz);
  return CreateNewScriptString(std::move(buf));
}

int File_ReadInt(sc_File *fil) {
  return FileReadInt(fil->handle);
}

int File_ReadRawChar(sc_File *fil) {
  return FileReadRawChar(fil->handle);
}

int File_ReadRawInt(sc_File *fil) {
  return FileReadRawInt(fil->handle);
}

int File_Seek(sc_File *fil, int offset, int origin)
{
    Stream *in = get_file_stream(fil->handle, "File.Seek");
    return in->Seek(offset, (StreamSeek)origin);
}

int File_GetEOF(sc_File *fil) {
  if (fil->handle <= 0)
    return 1;
  return FileIsEOF(fil->handle);
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
    if (!usetup.user_conf_dir.IsEmpty()) // directive to use custom userconf location
        return FSLocation(usetup.user_conf_dir);
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
    if (usetup.shared_data_dir.IsEmpty())
        return MakeDefaultDataDir(platform->GetAllUsersDataDirectory());
    return MakeUserDataDir(usetup.shared_data_dir);
}

FSLocation GetGameUserDataDir()
{
    if (usetup.user_data_dir.IsEmpty())
        return MakeDefaultDataDir(platform->GetUserSavedgamesDirectory());
    return MakeUserDataDir(usetup.user_data_dir);
}

bool CreateFSDirs(const FSLocation &fs)
{
    return Directory::CreateAllDirectories(fs.BaseDir, fs.SubDir);
}

bool ResolveScriptPath(const String &orig_sc_path, bool read_only, ResolvedPath &rp, ResolvedPath &alt_rp)
{
    rp = ResolvedPath();

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
        rp = ResolvedPath(loc, DefaultConfigFileName);
        return true;
    }

    // Test absolute paths
    if (!Path::IsRelativePath(sc_path))
    {
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to absolute path)", sc_path.GetCStr());
            return false;
        }
        rp = ResolvedPath(sc_path);
        return true;
    }

    // Resolve location tokens
    if (sc_path.CompareLeft(GameAssetToken, GameAssetToken.GetLength()) == 0)
    {
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game assets)", sc_path.GetCStr());
            return false;
        }
        rp = ResolvedPath(sc_path.Mid(GameAssetToken.GetLength() + 1), true);
        return true;
    }

    FSLocation parent_dir;
    String child_path;
    FSLocation alt_parent_dir;
    String alt_path;
    // IMPORTANT: for compatibility reasons we support both cases:
    // when token is followed by the path separator and when it is not, in which case it's assumed.
    if (sc_path.CompareLeft(GameInstallRootToken) == 0)
    {
        if (!read_only)
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game installation directory)",
                sc_path.GetCStr());
            return false;
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
    else
    {
        child_path = sc_path;

        // For games which were made without having safe paths in mind,
        // provide two paths: a path to the local directory and a path to
        // AppData directory.
        // This is done in case game writes a file by local path, and would
        // like to read it back later. Since AppData path has higher priority,
        // game will first check the AppData location and find a previously
        // written file.
        // If no file was written yet, but game is trying to read a pre-created
        // file in the installation directory, then such file will be found
        // following the 'alt_path'.
        parent_dir = GetGameAppDataDir();
        // Set alternate non-remapped "unsafe" path for read-only operations
        if (read_only)
        {
            alt_parent_dir = FSLocation(ResPaths.DataDir);
            alt_path = sc_path;
        }

        // For games made in the safe-path-aware versions of AGS, report a warning
        // if the unsafe path is used for write operation
        if (!read_only && game.options[OPT_SAFEFILEPATHS])
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game installation directory);\nPath will be remapped to the app data directory: '%s'",
                sc_path.GetCStr(), parent_dir.FullDir.GetCStr());
        }
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
            return false;
        }
    }
    rp = test_rp;
    if (!alt_parent_dir.FullDir.IsEmpty() && alt_parent_dir.FullDir.Compare(rp.Loc.FullDir) != 0)
        alt_rp = ResolvedPath(alt_parent_dir, alt_path);
    return true;
}

ResolvedPath ResolveScriptPathAndFindFile(const String &sc_path, bool read_only, bool ignore_find_result)
{
    ResolvedPath rp, alt_rp;
    if (!ResolveScriptPath(sc_path, read_only, rp, alt_rp))
    {
        debug_script_warn("ResolveScriptPath: failed to resolve path: %s", sc_path.GetCStr());
        return {}; // cannot be resolved
    }

    if (rp.AssetMgr)
        return rp; // we don't test AssetMgr here

    ResolvedPath final_rp = rp;
    String most_found, missing_path;
    String found_file = File::FindFileCI(rp.Loc.BaseDir, rp.SubPath, false, &most_found, &missing_path);
    if (found_file.IsEmpty() && alt_rp)
    {
        final_rp = alt_rp;
        found_file = File::FindFileCI(alt_rp.Loc.BaseDir, alt_rp.SubPath, false, &most_found, &missing_path);
    }
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

        debug_script_warn("ResolveScriptPath: failed to find a match for: %s\n\ttried: %s\n\talt try: %s",
            sc_path.GetCStr(), rp.FullPath.GetCStr(), alt_rp.FullPath.GetCStr());
        return {}; // nothing matching found
    }
    return ResolvedPath(found_file);
}

ResolvedPath ResolveWritePathAndCreateDirs(const String &sc_path)
{
    ResolvedPath rp, alt_rp;
    if (!ResolveScriptPath(sc_path, false, rp, alt_rp))
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

Stream *ResolveScriptPathAndOpen(const String &sc_path,
    FileOpenMode open_mode, StreamMode work_mode)
{
    ResolvedPath rp;
    if (open_mode == kFile_Open && work_mode == kStream_Read)
        rp = ResolveScriptPathAndFindFile(sc_path, true);
    else
        rp = ResolveWritePathAndCreateDirs(sc_path);

    if (!rp)
        return nullptr;
    Stream *s = rp.AssetMgr ?
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

PACKFILE *PackfileFromAsset(const AssetPath &path)
{
    Stream *asset_stream = AssetMgr->OpenAsset(path);
    if (!asset_stream) return nullptr;
    const size_t asset_size = asset_stream->GetLength();
    if (asset_size == 0) return nullptr;
    AGS_PACKFILE_OBJ* obj = new AGS_PACKFILE_OBJ;
    obj->stream.reset(asset_stream);
    obj->asset_size = asset_size;
    obj->remains = asset_size;
    return pack_fopen_vtable(&ags_packfile_vtable, obj);
}

String find_assetlib(const String &filename)
{
    String libname = File::FindFileCI(ResPaths.DataDir, filename);
    if (!libname.IsEmpty() && AssetManager::IsDataFile(libname))
        return libname;
    if (!ResPaths.DataDir2.IsEmpty() &&
        Path::ComparePaths(ResPaths.DataDir, ResPaths.DataDir2) != 0)
    {
        // Hack for running in Debugger
        libname = File::FindFileCI(ResPaths.DataDir2, filename);
        if (!libname.IsEmpty() && AssetManager::IsDataFile(libname))
            return libname;
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

Stream *release_file_stream(int32_t fhandle, const char *operation_name)
{
    ScriptFileHandle *fh = check_file_stream(fhandle, operation_name);
    if (!fh)
        return nullptr;
    Stream *s = fh->ReleaseStream();
    close_file_stream(fhandle, operation_name);
    return s;
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

// int (sc_File *fil)
RuntimeScriptValue Sc_File_ReadRawChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadRawChar);
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

// void (sc_File *fil, int towrite)
RuntimeScriptValue Sc_File_WriteRawChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteRawChar);
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
        { "File::Delete^1",           API_FN_PAIR(File_Delete) },
        { "File::Exists^1",           API_FN_PAIR(File_Exists) },
        { "File::Open^2",             API_FN_PAIR(sc_OpenFile) },
        { "File::ResolvePath^1",      API_FN_PAIR(File_ResolvePath) },

        { "File::Close^0",            API_FN_PAIR(File_Close) },
        { "File::ReadInt^0",          API_FN_PAIR(File_ReadInt) },
        { "File::ReadRawChar^0",      API_FN_PAIR(File_ReadRawChar) },
        { "File::ReadRawInt^0",       API_FN_PAIR(File_ReadRawInt) },
        { "File::ReadRawLine^1",      API_FN_PAIR(File_ReadRawLine) },
        { "File::ReadRawLineBack^0",  API_FN_PAIR(File_ReadRawLineBack) },
        { "File::ReadString^1",       API_FN_PAIR(File_ReadString) },
        { "File::ReadStringBack^0",   API_FN_PAIR(File_ReadStringBack) },
        { "File::WriteInt^1",         API_FN_PAIR(File_WriteInt) },
        { "File::WriteRawChar^1",     API_FN_PAIR(File_WriteRawChar) },
        { "File::WriteRawInt^1",      API_FN_PAIR(File_WriteRawInt) },
        { "File::WriteRawLine^1",     API_FN_PAIR(File_WriteRawLine) },
        { "File::WriteString^1",      API_FN_PAIR(File_WriteString) },
        { "File::Seek^2",             API_FN_PAIR(File_Seek) },
        { "File::get_EOF",            API_FN_PAIR(File_GetEOF) },
        { "File::get_Error",          API_FN_PAIR(File_GetError) },
        { "File::get_Position",       API_FN_PAIR(File_GetPosition) },
        { "File::get_Path",           API_FN_PAIR(File_GetPath) },
    };

    ccAddExternalFunctions(file_api);
}
