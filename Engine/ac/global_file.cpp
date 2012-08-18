
#include "ac/global_file.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"

#ifdef WINDOWS_VERSION
//#include <crtdbg.h>
//#include "winalleg.h"
//#include <shlwapi.h>

#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <dlfcn.h>
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "../PSP/launcher/pe.h"
*/

#else   // it's DOS (DJGPP)

//#include "sys/exceptn.h"

//int sys_getch() {
//    return getch();
//}

#endif  // WINDOWS_VERSION

extern int num_open_script_files;
extern FILE*valid_handles[MAX_OPEN_SCRIPT_FILES+1];


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
