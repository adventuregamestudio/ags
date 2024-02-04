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

#include <string.h>
#include "ac/global_file.h"
#include "ac/common.h"
#include "ac/file.h"
#include "ac/path_helper.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/stream.h"

using namespace AGS::Common;

int32_t FileOpenCMode(const char*fnmm, const char* cmode)
{
  Common::FileOpenMode open_mode;
  Common::StreamMode work_mode;
  // NOTE: here we ignore the text-mode flag. AGS 2.62 did not let
  // game devs to open files in text mode. The file reading and
  // writing logic in AGS makes extra control characters added for
  // security reasons, and FileWriteRawLine adds CR/LF to the end
  // of string on its own.
  if (!Common::File::GetFileModesFromCMode(cmode, open_mode, work_mode))
  {
      return 0;
  }
  return FileOpen(fnmm, open_mode, work_mode);
}

int32_t FileOpen(const char *fnmm, Common::FileOpenMode open_mode, Common::StreamMode work_mode)
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
void FileWrite(int32_t handle, const char *towrite) {
  Stream *out = get_file_stream(handle, "FileWrite");
  size_t len = strlen(towrite);
  out->WriteInt32(len + 1); // write with null-terminator
  out->Write(towrite, len + 1);
  }
void FileWriteRawLine(int32_t handle, const char*towrite) {
  Stream *out = get_file_stream(handle, "FileWriteRawLine");
  out->Write(towrite,strlen(towrite));
  out->WriteInt8('\r');
  out->WriteInt8('\n');
  }
void FileRead(int32_t handle,char*toread) {
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
  in->Read(toread,lle);
  }
int FileIsEOF (int32_t handle) {
  Stream *stream = get_file_stream(handle, "FileIsEOF");
  if (stream->EOS())
    return 1;

  // TODO: stream errors
  if (stream->GetError())
    return 1;

  if (stream->GetPosition () >= stream->GetLength())
    return 1;
  return 0;
}
int FileIsError(int32_t handle) {
  Stream *stream = get_file_stream(handle, "FileIsError");

  // TODO: stream errors
  if (stream->GetError())
    return 1;

  return 0;
}
void FileWriteInt(int32_t handle,int into) {
  Stream *out = get_file_stream(handle, "FileWriteInt");
  out->WriteInt8('I');
  out->WriteInt32(into);
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
char FileReadRawChar(int32_t handle) {
  Stream *in = get_file_stream(handle, "FileReadRawChar");
  return static_cast<uint8_t>(in->ReadByte());
  // NOTE: this function has incorrect return value for historical reasons;
  // we keep this strictly for backwards compatibility with old scripts
  }
int FileReadRawInt(int32_t handle) {
  Stream *in = get_file_stream(handle, "FileReadRawInt");
  if (in->EOS())
    return -1;
  return in->ReadInt32();
}
void FileWriteRawChar(int32_t handle, int chartoWrite) {
  Stream *out = get_file_stream(handle, "FileWriteRawChar");
  if ((chartoWrite < 0) || (chartoWrite > 255))
    debug_script_warn("FileWriteRawChar: can only write values 0-255");

  out->WriteByte(static_cast<uint8_t>(chartoWrite));
}
