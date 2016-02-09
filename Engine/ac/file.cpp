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

#define USE_CLIB
#include "ac/file.h"
#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_file.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "util/misc.h"
#include "platform/base/agsplatformdriver.h"
#include "util/stream.h"
#include "core/assetmanager.h"
#include "main/game_file.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string.h"

using namespace AGS::Common;

#if defined (AGS_RUNTIME_PATCH_ALLEGRO)
#include <dlfcn.h>
#endif // AGS_RUNTIME_PATCH_ALLEGRO


// ***** EXTERNS ****

// override packfile functions to allow it to load from our
// custom CLIB datafiles
extern "C" {
	PACKFILE*_my_temppack;
#if ALLEGRO_DATE > 19991010
#define PFO_PARAM const char *
#else
#define PFO_PARAM char *
#endif
#if !defined(AGS_RUNTIME_PATCH_ALLEGRO)
	extern PACKFILE *__old_pack_fopen(PFO_PARAM,PFO_PARAM);
#endif
}

extern GameSetup usetup;
extern GameSetupStruct game;
extern char saveGameDirectory[260];
extern AGSPlatformDriver *platform;

extern int MAXSTRLEN;

// object-based File routines

int File_Exists(const char *fnmm) {

  String path, alt_path;
  if (!ResolveScriptPath(fnmm, true, path, alt_path))
    return 0;

  return (File::TestReadFile(path) || File::TestReadFile(alt_path)) ? 1 : 0;
}

int File_Delete(const char *fnmm) {

  String path, alt_path;
  if (!ResolveScriptPath(fnmm, false, path, alt_path))
    return 0;

  if (unlink(path) == 0)
      return 1;
  if (errno == ENOENT && !alt_path.IsEmpty() && alt_path.Compare(path) != 0)
      return unlink(alt_path) == 0 ? 1 : 0;
  return 0;
}

void *sc_OpenFile(const char *fnmm, int mode) {
  if ((mode < scFileRead) || (mode > scFileAppend))
    quit("!OpenFile: invalid file mode");

  sc_File *scf = new sc_File();
  if (scf->OpenFile(fnmm, mode) == 0) {
    delete scf;
    return 0;
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
    return (int)in->Seek(offset, (StreamSeek)origin);
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

// [IKM] NOTE: this function is used only by few media/audio units
// TODO: find a way to hide allegro behind some interface/wrapper function
#if ALLEGRO_DATE > 19991010
PACKFILE *pack_fopen(const char *filnam1, const char *modd1) {
#else
PACKFILE *pack_fopen(char *filnam1, char *modd1) {
#endif
  char  *filnam = (char *)filnam1;
  char  *modd = (char *)modd1;
  int   needsetback = 0;

  if (filnam[0] == '~') {
    // ~ signals load from specific data file, not the main default one
    char gfname[80];
    int ii = 0;
    
    filnam++;
    while (filnam[0]!='~') {
      gfname[ii] = filnam[0];
      filnam++;
      ii++;
    }
    filnam++;
    // MACPORT FIX 9/6/5: changed from NULL TO '\0'
    gfname[ii] = '\0';
/*    char useloc[250];
#ifdef LINUX_VERSION
    sprintf(useloc,"%s/%s",usetup.data_files_dir,gfname);
#else
    sprintf(useloc,"%s\\%s",usetup.data_files_dir,gfname);
#endif
    Common::AssetManager::SetDataFile(useloc);*/
    
    char *libname = ci_find_file(usetup.data_files_dir, gfname);
    if (Common::AssetManager::SetDataFile(libname) != Common::kAssetNoError)
    {
      // Hack for running in Debugger
      free(libname);
      libname = ci_find_file("Compiled", gfname);
      Common::AssetManager::SetDataFile(libname);
    }
    free(libname);
    
    needsetback = 1;
  }

  // if the file exists, override the internal file
  bool file_exists = Common::File::TestReadFile(filnam);

#if defined(AGS_RUNTIME_PATCH_ALLEGRO)
  static PACKFILE * (*__old_pack_fopen)(PFO_PARAM, PFO_PARAM) = NULL;
  if(!__old_pack_fopen) {
    __old_pack_fopen = (PACKFILE* (*)(PFO_PARAM, PFO_PARAM))dlsym(RTLD_NEXT, "pack_fopen");
    if(!__old_pack_fopen) {
      // Looks like we're linking statically to allegro...
      // Let's see if it has been patched
      __old_pack_fopen = (PACKFILE* (*)(PFO_PARAM, PFO_PARAM))dlsym(RTLD_DEFAULT, "__allegro_pack_fopen");
      if(!__old_pack_fopen) {
        fprintf(stderr, "If you're linking statically to allegro, you need to apply this patch to allegro:\n"
        "https://sourceforge.net/tracker/?func=detail&aid=3302567&group_id=5665&atid=355665\n");
        exit(1);
      }
    }
  }
#endif

  if ((Common::AssetManager::GetAssetOffset(filnam)<1) || (file_exists)) {
    if (needsetback) Common::AssetManager::SetDataFile(game_file_name);
    return __old_pack_fopen(filnam, modd);
  } 
  else {
    _my_temppack=__old_pack_fopen(Common::AssetManager::GetLibraryForAsset(filnam), modd);
    if (_my_temppack == NULL)
      quitprintf("pack_fopen: unable to change datafile: not found: %s", Common::AssetManager::GetLibraryForAsset(filnam).GetCStr());

    pack_fseek(_my_temppack,Common::AssetManager::GetAssetOffset(filnam));
    
#if ALLEGRO_DATE < 20050101
    _my_temppack->todo=Common::AssetManager::GetAssetSize(filnam);
#else
    _my_temppack->normal.todo = Common::AssetManager::GetAssetSize(filnam);
#endif

    if (needsetback)
      Common::AssetManager::SetDataFile(game_file_name);
    return _my_temppack;
  }
}

// end packfile functions


const String UserSavedgamesRootToken = "$MYDOCS$";
const String GameSavedgamesDirToken  = "$SAVEGAMEDIR$";
const String GameDataDirToken        = "$APPDATADIR$";

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

String MakeSpecialSubDir(const String &sp_dir)
{
    if (is_relative_filename(sp_dir))
        return sp_dir;
    String full_path = sp_dir;
    if (full_path.GetLast() != '/' && full_path.GetLast() != '\\')
        full_path.AppendChar('/');
    full_path.Append(game.saveGameFolderName);
    Directory::CreateDirectory(full_path);
    return full_path;
}

String MakeAppDataPath()
{
    String app_data_path;
    if (usetup.user_data_dir.IsEmpty())
        app_data_path = MakeSpecialSubDir(PathOrCurDir(platform->GetAllUsersDataDirectory()));
    else
        app_data_path.Format("%s/Data", usetup.user_data_dir.GetCStr());
    Directory::CreateDirectory(app_data_path);
    app_data_path.AppendChar('/');
    return app_data_path;
}

bool ResolveScriptPath(const String &sc_path, bool read_only, String &path, String &alt_path)
{
    path.Empty();
    alt_path.Empty();

    bool is_absolute = !is_relative_filename(sc_path);
    if (is_absolute && !read_only)
    {
        debug_log("Attempt to access file '%s' denied (cannot write to absolute path)", sc_path.GetCStr());
        return false;
    }

    String parent_dir;
    String child_path;

    if (is_absolute)
    {
        path = sc_path;
        return true;
    }
    
    if (sc_path.CompareLeft(GameSavedgamesDirToken, GameSavedgamesDirToken.GetLength()) == 0)
    {
        parent_dir = saveGameDirectory;
        child_path = sc_path.Mid(GameSavedgamesDirToken.GetLength());
    }
    else if (sc_path.CompareLeft(GameDataDirToken, GameDataDirToken.GetLength()) == 0)
    {
        parent_dir = MakeAppDataPath();
        child_path = sc_path.Mid(GameDataDirToken.GetLength());
    }
    else
    {
        // Only let non-ready-only access to local dir paths in backwards-compatible mode
        if (!read_only && game.options[OPT_SAFEFILEPATHS])
        {
            debug_log("Attempt to access file '%s' denied (cannot write to game installation directory)", sc_path.GetCStr());
            return false;
        }

        child_path = sc_path;
        if (!game.options[OPT_SAFEFILEPATHS])
        {
            // For old games, which were made without having safe paths in mind,
            // provide two paths: a path to the local directory and a path to
            // AppData directory.
            // This is done in case game writes a file by local path, and would
            // like to read it back later. Since AppData path has higher priority,
            // game will first check the AppData location and find a previously
            // written file.
            // If no file was written yet, but game is trying to read a pre-created
            // file in the installation directory, then such file will be found
            // following the 'alt_path'.
            parent_dir = MakeAppDataPath();
            // Set alternate non-remapped "unsafe" path for read-only operations
            if (read_only)
                alt_path = sc_path;
        }
    }

    if (child_path[0] == '\\' || child_path[0] == '/')
        child_path.ClipLeft(1);

    path = String::FromFormat("%s%s", parent_dir.GetCStr(), child_path.GetCStr());
    // don't allow write operations for relative paths outside game dir
    if (!read_only)
    {
        if (!Path::IsSameOrSubDir(parent_dir, path))
        {
            debug_log("Attempt to access file '%s' denied (outside of game directory)", sc_path.GetCStr());
            path = "";
            return false;
        }
    }
    return true;
}


String get_current_dir()
{
    if (use_compiled_folder_as_current_dir)
        return "Compiled/";
    return "./";
}

void get_current_dir_path(char* buffer, const char *fileName)
{
    if (use_compiled_folder_as_current_dir)
    {
        sprintf(buffer, "Compiled\\%s", fileName);
    }
    else
    {
        strcpy(buffer, fileName);
    }
}

ScriptFileHandle valid_handles[MAX_OPEN_SCRIPT_FILES + 1];
// [IKM] NOTE: this is not precisely the number of files opened at this moment,
// but rather maximal number of handles that were used simultaneously during game run
int num_open_script_files = 0;
ScriptFileHandle *check_valid_file_handle_ptr(Common::Stream *stream_ptr, const char *operation_name)
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
  return NULL;
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
  return NULL;
}

Stream *get_valid_file_stream_from_handle(int32_t handle, const char *operation_name)
{
    ScriptFileHandle *sc_handle = check_valid_file_handle_int32(handle, operation_name);
    return sc_handle ? sc_handle->stream : NULL;
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
