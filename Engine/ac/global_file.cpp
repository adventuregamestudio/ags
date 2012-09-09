
#include "util/wgt2allg.h"
#include "ac/global_file.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "util/filestream.h"

using AGS::Common::CDataStream;

#ifdef WINDOWS_VERSION
//#include <crtdbg.h>
//#include "winalleg.h"
//#include <shlwapi.h>

#elif (defined(LINUX_VERSION) || defined(MAC_VERSION)) && !defined(PSP_VERSION)
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
extern CDataStream *valid_handles[MAX_OPEN_SCRIPT_FILES+1];


CDataStream *FileOpen(const char*fnmm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode) {
  int useindx = 0;
  char fileToOpen[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToOpen,
      (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read)))
    return NULL;

  // find a free file handle to use
  for (useindx = 0; useindx < num_open_script_files; useindx++) 
  {
    if (valid_handles[useindx] == NULL)
      break;
  }

  valid_handles[useindx] = Common::File::OpenFile(fileToOpen, open_mode, work_mode);

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

void FileClose(CDataStream *hha) {
  valid_handles[check_valid_file_handle(hha,"FileClose")] = NULL;
  delete hha;
  }
void FileWrite(CDataStream *haa, const char *towrite) {
  check_valid_file_handle(haa,"FileWrite");
  haa->WriteInt32(strlen(towrite)+1);
  haa->Write(towrite,strlen(towrite)+1);
  }
void FileWriteRawLine(CDataStream *haa, const char*towrite) {
  check_valid_file_handle(haa,"FileWriteRawLine");
  haa->Write(towrite,strlen(towrite));
  haa->WriteInt8 (13);
  haa->WriteInt8 (10);
  }
void FileRead(CDataStream *haa,char*toread) {
  VALIDATE_STRING(toread);
  check_valid_file_handle(haa,"FileRead");
  if (haa->EOS()) {
    toread[0] = 0;
    return;
  }
  int lle=haa->ReadInt32();
  if ((lle>=200) | (lle<1)) quit("!FileRead: file was not written by FileWrite");
  haa->Read(toread,lle);
  }
int FileIsEOF (CDataStream *haa) {
  check_valid_file_handle(haa,"FileIsEOF");
  if (haa->EOS())
    return 1;

  // TODO: stream errors
  //if (ferror (haa))
  //  return 1;

  if (haa->GetPosition () >= haa->GetLength())
    return 1;
  return 0;
}
int FileIsError(CDataStream *haa) {
  check_valid_file_handle(haa,"FileIsError");

  // TODO: stream errors
  //if (ferror(haa))
  //  return 1;

  return 0;
}
void FileWriteInt(CDataStream *haa,int into) {
  check_valid_file_handle(haa,"FileWriteInt");
  haa->WriteInt8('I');
  haa->WriteInt32(into);
  }
int FileReadInt(CDataStream *haa) {
  check_valid_file_handle(haa,"FileReadInt");
  if (haa->EOS())
    return -1;
  if (haa->ReadInt8()!='I')
    quit("!FileReadInt: File read back in wrong order");
  return haa->ReadInt32();
  }
char FileReadRawChar(CDataStream *haa) {
  check_valid_file_handle(haa,"FileReadRawChar");
  if (haa->EOS())
    return -1;
  return haa->ReadInt8();
  }
int FileReadRawInt(CDataStream *haa) {
  check_valid_file_handle(haa,"FileReadRawInt");
  if (haa->EOS())
    return -1;
  return haa->ReadInt32();
}
void FileWriteRawChar(CDataStream *haa, int chartoWrite) {
  check_valid_file_handle(haa,"FileWriteRawChar");
  if ((chartoWrite < 0) || (chartoWrite > 255))
    quit("!FileWriteRawChar: can only write values 0-255");

  haa->WriteInt8(chartoWrite);
}
