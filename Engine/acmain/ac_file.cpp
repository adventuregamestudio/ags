
#include "acmain/ac_maindefines.h"


#define MAX_OPEN_SCRIPT_FILES 10
FILE*valid_handles[MAX_OPEN_SCRIPT_FILES+1];
int num_open_script_files = 0;
int check_valid_file_handle(FILE*hann, char*msg) {
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
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
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

FILE* FileOpen(const char*fnmm, const char* mode) {
  int useindx = 0;
  char fileToOpen[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToOpen, strcmp(mode, "rb") != 0))
    return NULL;

  // find a free file handle to use
  for (useindx = 0; useindx < num_open_script_files; useindx++) 
  {
    if (valid_handles[useindx] == NULL)
      break;
  }

  valid_handles[useindx] = fopen(fileToOpen, mode);

  if (valid_handles[useindx] == NULL)
    return NULL;

  if (useindx >= num_open_script_files) 
  {
    if (num_open_script_files >= MAX_OPEN_SCRIPT_FILES)
      quit("!FileOpen: tried to open more than 10 files simultaneously - close some first");
    num_open_script_files++;
  }
  return valid_handles[useindx];
}

void FileClose(FILE*hha) {
  valid_handles[check_valid_file_handle(hha,"FileClose")] = NULL;
  fclose(hha);
  }
void FileWrite(FILE*haa, const char *towrite) {
  check_valid_file_handle(haa,"FileWrite");
  putw(strlen(towrite)+1,haa);
  fwrite(towrite,strlen(towrite)+1,1,haa);
  }
void FileWriteRawLine(FILE*haa, const char*towrite) {
  check_valid_file_handle(haa,"FileWriteRawLine");
  fwrite(towrite,strlen(towrite),1,haa);
  fputc (13, haa);
  fputc (10, haa);
  }
void FileRead(FILE*haa,char*toread) {
  VALIDATE_STRING(toread);
  check_valid_file_handle(haa,"FileRead");
  if (feof(haa)) {
    toread[0] = 0;
    return;
  }
  int lle=getw(haa);
  if ((lle>=200) | (lle<1)) quit("!FileRead: file was not written by FileWrite");
  fread(toread,lle,1,haa);
  }
int FileIsEOF (FILE *haa) {
  check_valid_file_handle(haa,"FileIsEOF");
  if (feof(haa))
    return 1;
  if (ferror (haa))
    return 1;
  if (ftell (haa) >= filelength (fileno(haa)))
    return 1;
  return 0;
}
int FileIsError(FILE *haa) {
  check_valid_file_handle(haa,"FileIsError");
  if (ferror(haa))
    return 1;
  return 0;
}
void FileWriteInt(FILE*haa,int into) {
  check_valid_file_handle(haa,"FileWriteInt");
  fputc('I',haa);
  putw(into,haa);
  }
int FileReadInt(FILE*haa) {
  check_valid_file_handle(haa,"FileReadInt");
  if (feof(haa))
    return -1;
  if (fgetc(haa)!='I')
    quit("!FileReadInt: File read back in wrong order");
  return getw(haa);
  }
char FileReadRawChar(FILE*haa) {
  check_valid_file_handle(haa,"FileReadRawChar");
  if (feof(haa))
    return -1;
  return fgetc(haa);
  }
int FileReadRawInt(FILE*haa) {
  check_valid_file_handle(haa,"FileReadRawInt");
  if (feof(haa))
    return -1;
  return getw(haa);
}
void FileWriteRawChar(FILE *haa, int chartoWrite) {
  check_valid_file_handle(haa,"FileWriteRawChar");
  if ((chartoWrite < 0) || (chartoWrite > 255))
    quit("!FileWriteRawChar: can only write values 0-255");

  fputc(chartoWrite, haa);
}

// object-based File routines

const char *fopenModes[] = {NULL, "rb", "wb", "ab"};

int sc_File::OpenFile(const char *filename, int mode) {
  handle = FileOpen(filename, fopenModes[mode]);
  if (handle == NULL)
    return 0;
  return 1;
}

void sc_File::Close() {
  if (handle) {
    FileClose(handle);
    handle = NULL;
  }
}

int File_Exists(const char *fnmm) {
  char fileToCheck[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToCheck, false))
    return 0;

  FILE *iii = fopen(fileToCheck, "rb");
  if (iii == NULL)
    return 0;

  fclose(iii);
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
    buffer[i] = fgetc(fil->handle);
    if (buffer[i] == 13) {
      // CR -- skip LF and abort
      fgetc(fil->handle);
      break;
    }
    if (buffer[i] == 10)  // LF only -- abort
      break;
    if (feof(fil->handle))  // EOF -- abort
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
  if (feof(fil->handle)) {
    return CreateNewScriptString("");
  }

  int lle = getw(fil->handle);
  if ((lle >= 20000) || (lle < 1))
    quit("!File.ReadStringBack: file was not written by WriteString");

  char *retVal = (char*)malloc(lle);
  fread(retVal, lle, 1, fil->handle);

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
