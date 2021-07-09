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
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "util/misc.h"
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

extern GameSetup usetup;
extern GameSetupStruct game;
extern AGSPlatformDriver *platform;

extern int MAXSTRLEN;

// object-based File routines

int File_Exists(const char *fnmm) {

  ResolvedPath rp;
  if (!ResolveScriptPath(fnmm, true, rp))
    return 0;

  return (File::TestReadFile(rp.FullPath) || File::TestReadFile(rp.AltPath)) ? 1 : 0;
}

int File_Delete(const char *fnmm) {

  ResolvedPath rp;
  if (!ResolveScriptPath(fnmm, false, rp))
    return 0;

  if (File::DeleteFile(rp.FullPath))
      return 1;
  if (errno == ENOENT && !rp.AltPath.IsEmpty() && rp.AltPath.Compare(rp.FullPath) != 0)
      return File::DeleteFile(rp.AltPath) ? 1 : 0;
  return 0;
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

void File_WriteRawLine(sc_File *fil, const char *towrite) {
  FileWriteRawLine(fil->handle, towrite);
}

void File_ReadRawLine(sc_File *fil, char* buffer) {
  Stream *in = get_valid_file_stream_from_handle(fil->handle, "File.ReadRawLine");
  check_strlen(buffer);
  int i = 0;
  while (i < MAXSTRLEN - 1) {
    buffer[i] = in->ReadInt8();
    if (buffer[i] == 13) {
      // CR -- skip LF and abort
      in->ReadInt8();
      break;
    }
    if (buffer[i] == 10)  // LF only -- abort
      break;
    if (in->EOS())  // EOF -- abort
      break;
    i++;
  }
  buffer[i] = 0;
}

const char* File_ReadRawLineBack(sc_File *fil) {
  char readbuffer[MAX_MAXSTRLEN + 1];
  File_ReadRawLine(fil, readbuffer);
  return CreateNewScriptString(readbuffer);
}

void File_ReadString(sc_File *fil, char *toread) {
  FileRead(fil->handle, toread);
}

const char* File_ReadStringBack(sc_File *fil) {
  Stream *in = get_valid_file_stream_from_handle(fil->handle, "File.ReadStringBack");
  if (in->EOS()) {
    return CreateNewScriptString("");
  }

  int lle = in->ReadInt32();
  if ((lle >= 20000) || (lle < 1))
    quit("!File.ReadStringBack: file was not written by WriteString");

  char *retVal = (char*)malloc(lle);
  in->Read(retVal, lle);

  return CreateNewScriptString(retVal, false);
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
    Stream *in = get_valid_file_stream_from_handle(fil->handle, "File.Seek");
    if (!in->Seek(offset, (StreamSeek)origin)) { return -1; }
    return in->GetPosition();
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
    Stream *stream = get_valid_file_stream_from_handle(fil->handle, "File.Position");
    // TODO: a problem is that AGS script does not support unsigned or long int
    return (int)stream->GetPosition();
}

//=============================================================================


const String GameInstallRootToken    = "$INSTALLDIR$";
const String UserSavedgamesRootToken = "$MYDOCS$";
const String GameSavedgamesDirToken  = "$SAVEGAMEDIR$";
const String GameDataDirToken        = "$APPDATADIR$";
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

String PreparePathForWriting(const FSLocation& fsloc, const String &filename)
{
    if (Directory::CreateAllDirectories(fsloc.BaseDir, fsloc.SubDir))
        return Path::ConcatPaths(fsloc.FullDir, filename);
    return "";
}

FSLocation GetGlobalUserConfigDir()
{
    String dir = platform->GetUserGlobalConfigDirectory();
    if (Path::IsRelativePath(dir)) // relative dir is resolved relative to the game data dir
        return FSLocation(ResPaths.DataDir, dir);
    return FSLocation(dir);
}

FSLocation GetGameUserConfigDir()
{
    String dir = platform->GetUserConfigDirectory();
    if (Path::IsRelativePath(dir)) // relative dir is resolved relative to the game data dir
        return FSLocation(ResPaths.DataDir, dir);
    else if (usetup.local_user_conf) // directive to use game dir location
        return FSLocation(ResPaths.DataDir);
    // For absolute dir, we assume it's a special directory prepared for AGS engine
    // and therefore amend it with a game own subdir
    return FSLocation(dir, game.saveGameFolderName);
}

// A helper function that deduces a data directory either using default system location,
// or user option from config. In case of a default location a path is appended with
// game's "save folder" name, which is meant to separate files from different games.
static FSLocation MakeGameDataDir(const String &default_dir, const String &user_option)
{
    if (user_option.IsEmpty())
    {
        String dir = default_dir;
        if (Path::IsRelativePath(dir)) // relative dir is resolved relative to the game data dir
            return FSLocation(ResPaths.DataDir, dir);
        // For absolute dir, we assume it's a special directory prepared for AGS engine
        // and therefore amend it with a game own subdir
        return FSLocation(dir, game.saveGameFolderName);
    }
    // If this location is set up by user config, then use it as is (resolving relative path if necessary)
    String dir = user_option;
    if (Path::IsSameOrSubDir(ResPaths.DataDir, dir)) // check if it's inside game dir
        return FSLocation(ResPaths.DataDir, Path::MakeRelativePath(ResPaths.DataDir, dir));
    dir = Path::MakeAbsolutePath(dir);
    return FSLocation(dir);
}

FSLocation GetGameAppDataDir()
{
    return MakeGameDataDir(platform->GetAllUsersDataDirectory(), usetup.shared_data_dir);
}

FSLocation GetGameUserDataDir()
{
    return MakeGameDataDir(platform->GetUserSavedgamesDirectory(), usetup.user_data_dir);
}

bool ResolveScriptPath(const String &orig_sc_path, bool read_only, ResolvedPath &rp)
{
    rp = ResolvedPath();

    // File tokens (they must be the only thing in script path)
    if (orig_sc_path.Compare(UserConfigFileToken) == 0)
    {
        auto loc = GetGameUserConfigDir();
        rp = ResolvedPath(loc, DefaultConfigFileName);
        return true;
    }

    // Test absolute paths
    bool is_absolute = !Path::IsRelativePath(orig_sc_path);
    if (is_absolute && !read_only)
    {
        debug_script_warn("Attempt to access file '%s' denied (cannot write to absolute path)", orig_sc_path.GetCStr());
        return false;
    }

    if (is_absolute)
    {
        rp = ResolvedPath(orig_sc_path);
        return true;
    }

    // Resolve location tokens
    // IMPORTANT: for compatibility reasons we support both cases:
    // when token is followed by the path separator and when it is not, in which case it's assumed.
    String sc_path = orig_sc_path;
    FSLocation parent_dir;
    String child_path;
    String alt_path;
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
            alt_path = Path::ConcatPaths(ResPaths.DataDir, sc_path);

        // For games made in the safe-path-aware versions of AGS, report a warning
        // if the unsafe path is used for write operation
        if (!read_only && game.options[OPT_SAFEFILEPATHS])
        {
            debug_script_warn("Attempt to access file '%s' denied (cannot write to game installation directory);\nPath will be remapped to the app data directory: '%s'",
                sc_path.GetCStr(), parent_dir.FullDir.GetCStr());
        }
    }

    // don't allow write operations for relative paths outside game dir
    ResolvedPath test_rp = ResolvedPath(parent_dir, child_path, alt_path);
    if (!read_only)
    {
        if (!Path::IsSameOrSubDir(test_rp.Loc.FullDir, test_rp.FullPath))
        {
            debug_script_warn("Attempt to access file '%s' denied (outside of game directory)", sc_path.GetCStr());
            return false;
        }
    }
    rp = test_rp;
    return true;
}

bool ResolveWritePathAndCreateDirs(const String &sc_path, ResolvedPath &rp)
{
    if (!ResolveScriptPath(sc_path, false, rp))
        return false;

    if (!rp.Loc.SubDir.IsEmpty() &&
        !Directory::CreateAllDirectories(rp.Loc.BaseDir, rp.Loc.SubDir))
    {
        debug_script_warn("ResolveScriptPath: failed to create all subdirectories: %s", rp.FullPath.GetCStr());
        return false;
    }
    return true;
}

Stream *LocateAsset(const AssetPath &path, size_t &asset_size)
{
    String assetname = path.Name;
    String filter = path.Filter;
    soff_t asset_sz = 0;
    Stream *asset_stream = AssetMgr->OpenAsset(assetname, filter, &asset_sz);
    asset_size = asset_sz;
    return asset_stream;
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

static int ags_pf_ungetc(int c, void *userdata)
{
    return -1; // we do not want to support this
}

static long ags_pf_fread(void *p, long n, void *userdata)
{
    AGS_PACKFILE_OBJ* obj = (AGS_PACKFILE_OBJ*)userdata;
    if (obj->remains > 0)
    {
        size_t read = Math::Min(obj->remains, (size_t)n);
        obj->remains -= read;
        return obj->stream->Read(p, read);
    }
    return -1;
}

static int ags_pf_putc(int c, void *userdata)
{
    return -1;  // don't support write
}

static long ags_pf_fwrite(AL_CONST void *p, long n, void *userdata)
{
    return -1; // don't support write
}

static int ags_pf_fseek(void *userdata, int offset)
{
    return -1; // don't support seek
}

static int ags_pf_feof(void *userdata)
{
    return ((AGS_PACKFILE_OBJ*)userdata)->remains == 0;
}

static int ags_pf_ferror(void *userdata)
{
    return ((AGS_PACKFILE_OBJ*)userdata)->stream->HasErrors() ? 1 : 0;
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

PACKFILE *PackfileFromAsset(const AssetPath &path, size_t &asset_size)
{
    Stream *asset_stream = LocateAsset(path, asset_size);
    if (asset_stream && asset_size > 0)
    {
        AGS_PACKFILE_OBJ* obj = new AGS_PACKFILE_OBJ;
        obj->stream.reset(asset_stream);
        obj->asset_size = asset_size;
        obj->remains = asset_size;
        return pack_fopen_vtable(&ags_packfile_vtable, obj);
    }
    return nullptr;
}

bool DoesAssetExistInLib(const AssetPath &path)
{
    String assetname = path.Name;
    String filter = path.Filter;
    return AssetMgr->DoesAssetExist(assetname, filter);
}

String find_assetlib(const String &filename)
{
    String libname = ci_find_file(ResPaths.DataDir, filename);
    if (AssetManager::IsDataFile(libname))
        return libname;
    if (Path::ComparePaths(ResPaths.DataDir, ResPaths.DataDir2) != 0)
    {
      // Hack for running in Debugger
      libname = ci_find_file(ResPaths.DataDir2, filename);
      if (AssetManager::IsDataFile(libname))
        return libname;
    }
    return "";
}

AssetPath get_audio_clip_assetpath(int bundling_type, const String &filename)
{
    return AssetPath(filename, "audio");
}

AssetPath get_voice_over_assetpath(const String &filename)
{
    return AssetPath(filename, "voice");
}

ScriptFileHandle valid_handles[MAX_OPEN_SCRIPT_FILES + 1];
// [IKM] NOTE: this is not precisely the number of files opened at this moment,
// but rather maximal number of handles that were used simultaneously during game run
int num_open_script_files = 0;
ScriptFileHandle *check_valid_file_handle_ptr(Stream *stream_ptr, const char *operation_name)
{
  if (stream_ptr)
  {
      for (int i = 0; i < num_open_script_files; ++i)
      {
          if (stream_ptr == valid_handles[i].stream)
          {
              return &valid_handles[i];
          }
      }
  }

  String exmsg = String::FromFormat("!%s: invalid file handle; file not previously opened or has been closed", operation_name);
  quit(exmsg);
  return nullptr;
}

ScriptFileHandle *check_valid_file_handle_int32(int32_t handle, const char *operation_name)
{
  if (handle > 0)
  {
    for (int i = 0; i < num_open_script_files; ++i)
    {
        if (handle == valid_handles[i].handle)
        {
            return &valid_handles[i];
        }
    }
  }

  String exmsg = String::FromFormat("!%s: invalid file handle; file not previously opened or has been closed", operation_name);
  quit(exmsg);
  return nullptr;
}

Stream *get_valid_file_stream_from_handle(int32_t handle, const char *operation_name)
{
    ScriptFileHandle *sc_handle = check_valid_file_handle_int32(handle, operation_name);
    return sc_handle ? sc_handle->stream : nullptr;
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

extern ScriptString myScriptStringImpl;

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


void RegisterFileAPI()
{
    ccAddExternalStaticFunction("File::Delete^1",           Sc_File_Delete);
    ccAddExternalStaticFunction("File::Exists^1",           Sc_File_Exists);
    ccAddExternalStaticFunction("File::Open^2",             Sc_sc_OpenFile);
    ccAddExternalObjectFunction("File::Close^0",            Sc_File_Close);
    ccAddExternalObjectFunction("File::ReadInt^0",          Sc_File_ReadInt);
    ccAddExternalObjectFunction("File::ReadRawChar^0",      Sc_File_ReadRawChar);
    ccAddExternalObjectFunction("File::ReadRawInt^0",       Sc_File_ReadRawInt);
    ccAddExternalObjectFunction("File::ReadRawLine^1",      Sc_File_ReadRawLine);
    ccAddExternalObjectFunction("File::ReadRawLineBack^0",  Sc_File_ReadRawLineBack);
    ccAddExternalObjectFunction("File::ReadString^1",       Sc_File_ReadString);
    ccAddExternalObjectFunction("File::ReadStringBack^0",   Sc_File_ReadStringBack);
    ccAddExternalObjectFunction("File::WriteInt^1",         Sc_File_WriteInt);
    ccAddExternalObjectFunction("File::WriteRawChar^1",     Sc_File_WriteRawChar);
    ccAddExternalObjectFunction("File::WriteRawLine^1",     Sc_File_WriteRawLine);
    ccAddExternalObjectFunction("File::WriteString^1",      Sc_File_WriteString);
    ccAddExternalObjectFunction("File::Seek^2",             Sc_File_Seek);
    ccAddExternalObjectFunction("File::get_EOF",            Sc_File_GetEOF);
    ccAddExternalObjectFunction("File::get_Error",          Sc_File_GetError);
    ccAddExternalObjectFunction("File::get_Position",       Sc_File_GetPosition);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("File::Delete^1",           (void*)File_Delete);
    ccAddExternalFunctionForPlugin("File::Exists^1",           (void*)File_Exists);
    ccAddExternalFunctionForPlugin("File::Open^2",             (void*)sc_OpenFile);
    ccAddExternalFunctionForPlugin("File::Close^0",            (void*)File_Close);
    ccAddExternalFunctionForPlugin("File::ReadInt^0",          (void*)File_ReadInt);
    ccAddExternalFunctionForPlugin("File::ReadRawChar^0",      (void*)File_ReadRawChar);
    ccAddExternalFunctionForPlugin("File::ReadRawInt^0",       (void*)File_ReadRawInt);
    ccAddExternalFunctionForPlugin("File::ReadRawLine^1",      (void*)File_ReadRawLine);
    ccAddExternalFunctionForPlugin("File::ReadRawLineBack^0",  (void*)File_ReadRawLineBack);
    ccAddExternalFunctionForPlugin("File::ReadString^1",       (void*)File_ReadString);
    ccAddExternalFunctionForPlugin("File::ReadStringBack^0",   (void*)File_ReadStringBack);
    ccAddExternalFunctionForPlugin("File::WriteInt^1",         (void*)File_WriteInt);
    ccAddExternalFunctionForPlugin("File::WriteRawChar^1",     (void*)File_WriteRawChar);
    ccAddExternalFunctionForPlugin("File::WriteRawLine^1",     (void*)File_WriteRawLine);
    ccAddExternalFunctionForPlugin("File::WriteString^1",      (void*)File_WriteString);
    ccAddExternalFunctionForPlugin("File::get_EOF",            (void*)File_GetEOF);
    ccAddExternalFunctionForPlugin("File::get_Error",          (void*)File_GetError);
}
