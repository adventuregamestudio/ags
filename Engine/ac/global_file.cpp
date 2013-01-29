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

#include "util/wgt2allg.h"
#include "ac/global_file.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "util/filestream.h"

using AGS::Common::DataStream;

extern int num_open_script_files;
extern DataStream *valid_handles[MAX_OPEN_SCRIPT_FILES+1];


int32_t FileOpenCMode(const char*fnmm, const char* cmode)
{
  Common::FileOpenMode open_mode;
  Common::FileWorkMode work_mode;
  // NOTE: here we ignore the text-mode flag. AGS 2.62 did not let
  // game devs to open files in text mode. The file reading and
  // writing logic in AGS makes extra control characters added for
  // security reasons, and FileWriteRawLine adds CR/LF to the end
  // of string on its own.
  if (!Common::File::GetFileModesFromCMode(cmode, open_mode, work_mode))
  {
      return -1;
  }
  return FileOpen(fnmm, open_mode, work_mode);
}

int32_t FileOpen(const char*fnmm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
  int useindx = 0;
  char fileToOpen[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToOpen,
      (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read)))
    return -1;

  // find a free file handle to use
  for (useindx = 0; useindx < num_open_script_files; useindx++) 
  {
    if (valid_handles[useindx] == NULL)
      break;
  }

  valid_handles[useindx] = Common::File::OpenFile(fileToOpen, open_mode, work_mode);

  if (valid_handles[useindx] == NULL)
    return -1;

  if (useindx >= num_open_script_files) 
  {
    if (num_open_script_files >= MAX_OPEN_SCRIPT_FILES)
      quit("!FileOpen: tried to open more than 10 files simultaneously - close some first");
    num_open_script_files++;
  }
  return useindx;
}

void FileClose(int32_t handle) {
  DataStream *stream = check_valid_file_handle_int32(handle,"FileClose");
  if (stream)
  {
    valid_handles[handle] = NULL;
  }
  delete stream;
  }
void FileWrite(int32_t handle, const char *towrite) {
  DataStream *out = check_valid_file_handle_int32(handle,"FileWrite");
  out->WriteInt32(strlen(towrite)+1);
  out->Write(towrite,strlen(towrite)+1);
  }
void FileWriteRawLine(int32_t handle, const char*towrite) {
  DataStream *out = check_valid_file_handle_int32(handle,"FileWriteRawLine");
  out->Write(towrite,strlen(towrite));
  out->WriteInt8 (13);
  out->WriteInt8 (10);
  }
void FileRead(int32_t handle,char*toread) {
  VALIDATE_STRING(toread);
  DataStream *in = check_valid_file_handle_int32(handle,"FileRead");
  if (in->EOS()) {
    toread[0] = 0;
    return;
  }
  int lle=in->ReadInt32();
  if ((lle>=200) | (lle<1)) quit("!FileRead: file was not written by FileWrite");
  in->Read(toread,lle);
  }
int FileIsEOF (int32_t handle) {
  DataStream *stream = check_valid_file_handle_int32(handle,"FileIsEOF");
  if (stream->EOS())
    return 1;

  // TODO: stream errors
  if (ferror (((Common::FileStream*)stream)->GetHandle()))
    return 1;

  if (stream->GetPosition () >= stream->GetLength())
    return 1;
  return 0;
}
int FileIsError(int32_t handle) {
  DataStream *stream = check_valid_file_handle_int32(handle,"FileIsError");

  // TODO: stream errors
  if (ferror(((Common::FileStream*)stream)->GetHandle()))
    return 1;

  return 0;
}
void FileWriteInt(int32_t handle,int into) {
  DataStream *out = check_valid_file_handle_int32(handle,"FileWriteInt");
  out->WriteInt8('I');
  out->WriteInt32(into);
  }
int FileReadInt(int32_t handle) {
  DataStream *in = check_valid_file_handle_int32(handle,"FileReadInt");
  if (in->EOS())
    return -1;
  if (in->ReadInt8()!='I')
    quit("!FileReadInt: File read back in wrong order");
  return in->ReadInt32();
  }
char FileReadRawChar(int32_t handle) {
  DataStream *in = check_valid_file_handle_int32(handle,"FileReadRawChar");
  if (in->EOS())
    return -1;
  return in->ReadInt8();
  }
int FileReadRawInt(int32_t handle) {
  DataStream *in = check_valid_file_handle_int32(handle,"FileReadRawInt");
  if (in->EOS())
    return -1;
  return in->ReadInt32();
}
void FileWriteRawChar(int32_t handle, int chartoWrite) {
  DataStream *out = check_valid_file_handle_int32(handle,"FileWriteRawChar");
  if ((chartoWrite < 0) || (chartoWrite > 255))
    quit("!FileWriteRawChar: can only write values 0-255");

  out->WriteInt8(chartoWrite);
}

DataStream *GetValidFileStream(int32_t handle)
{
    if (handle >= 0 && handle < num_open_script_files)
    {
        return valid_handles[handle];
    }
    return NULL;
}
