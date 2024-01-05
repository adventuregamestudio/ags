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
#include "ac/dynobj/scriptfile.h"
#include "ac/global_file.h"

// CHECKME: actually NULLs here will be equal to kFile_Open & kStream_Read
const Common::FileOpenMode sc_File::fopenModes[] = 
    {Common::kFile_Open/*CHECKME, was undefined*/, Common::kFile_Open, Common::kFile_CreateAlways, Common::kFile_Create};
const Common::StreamMode sc_File::fworkModes[] = 
    {Common::kStream_Read/*CHECKME, was undefined*/, Common::kStream_Read, Common::kStream_Write, Common::kStream_Write};

int sc_File::Dispose(void* /*address*/, bool /*force*/) {
    Close();
    delete this;
    return 1;
}

const char *sc_File::GetType() {
    return "File";
}

int sc_File::Serialize(void* /*address*/, uint8_t* /*buffer*/, int /*bufsize*/) {
    // we cannot serialize an open file, so it will get closed
    return 0;
}

int sc_File::OpenFile(const char *filename, int mode) {
  handle = FileOpen(filename, fopenModes[mode], fworkModes[mode]);
  if (handle <= 0)
      return 0;
  return 1;
}

void sc_File::Close() {
  if (handle > 0) {
    FileClose(handle);
    handle = 0;
  }
}

sc_File::sc_File() {
    handle = 0;
}
