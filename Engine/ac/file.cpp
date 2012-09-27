#define USE_CLIB
#include "util/wgt2allg.h"
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
#include "util/datastream.h"
#include "core/assetmanager.h"

using AGS::Common::DataStream;

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

extern char* game_file_name;
extern int MAXSTRLEN;

// object-based File routines

int File_Exists(const char *fnmm) {
  char fileToCheck[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToCheck, false))
    return 0;

  if (!Common::File::TestReadFile(fileToCheck))
    return 0;

  return 1;
}

int File_Delete(const char *fnmm) {

  char fileToDelete[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToDelete, true))
    return 0;

  unlink(fileToDelete);

  return 1;
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
  check_valid_file_handle(fil->handle, "File.ReadRawLine");
  check_strlen(buffer);
  int i = 0;
  while (i < MAXSTRLEN - 1) {
    buffer[i] = fil->handle->ReadInt8();
    if (buffer[i] == 13) {
      // CR -- skip LF and abort
      fil->handle->ReadInt8();
      break;
    }
    if (buffer[i] == 10)  // LF only -- abort
      break;
    if (fil->handle->EOS())  // EOF -- abort
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
  check_valid_file_handle(fil->handle, "File.ReadStringBack");
  if (fil->handle->EOS()) {
    return CreateNewScriptString("");
  }

  int lle = fil->handle->ReadInt32();
  if ((lle >= 20000) || (lle < 1))
    quit("!File.ReadStringBack: file was not written by WriteString");

  char *retVal = (char*)malloc(lle);
  fil->handle->Read(retVal, lle);

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

int File_GetEOF(sc_File *fil) {
  if (fil->handle == NULL)
    return 1;
  return FileIsEOF(fil->handle);
}

int File_GetError(sc_File *fil) {
  if (fil->handle == NULL)
    return 1;
  return FileIsError(fil->handle);
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

DataStream *valid_handles[MAX_OPEN_SCRIPT_FILES+1];
int num_open_script_files = 0;
int check_valid_file_handle(DataStream *hann, char*msg) {
  int aa;
  if (hann != NULL) {
    for (aa=0; aa < num_open_script_files; aa++) {
      if (hann == valid_handles[aa])
        return aa;
    }
  }
  char exmsg[100];
  sprintf(exmsg,"!%s: invalid file handle; file not previously opened or has been closed",msg);
  quit(exmsg);
  return -1;
  }

bool validate_user_file_path(const char *fnmm, char *output, bool currentDirOnly)
{
  if (strncmp(fnmm, "$SAVEGAMEDIR$", 13) == 0) 
  {
    fnmm += 14;
    sprintf(output, "%s%s", saveGameDirectory, fnmm);
  }
  else if (strncmp(fnmm, "$APPDATADIR$", 12) == 0) 
  {
    fnmm += 13;
    const char *appDataDir = platform->GetAllUsersDataDirectory();
    if (appDataDir == NULL) appDataDir = ".";
    if (game.saveGameFolderName[0] != 0)
    {
      sprintf(output, "%s/%s", appDataDir, game.saveGameFolderName);
      fix_filename_slashes(output);
      mkdir(output
#if !defined (WINDOWS_VERSION)
                  , 0755
#endif
      );
    }
    else 
    {
      strcpy(output, appDataDir);
    }
    put_backslash(output);
    strcat(output, fnmm);
  }
  else
  {
    get_current_dir_path(output, fnmm);
  }

  // don't allow access to files outside current dir
  if (!currentDirOnly) { }
  else if ((strchr (fnmm, '/') != NULL) || (strchr(fnmm, '\\') != NULL) ||
    (strstr(fnmm, "..") != NULL) || (strchr(fnmm, ':') != NULL)) {
    debug_log("Attempt to access file '%s' denied (not current directory)", fnmm);
    return false;
  }

  return true;
}
