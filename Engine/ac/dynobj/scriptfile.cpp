
#include "ac/dynobj/scriptfile.h"
#include "ac/global_file.h"

// CHECKME: actually NULLs here will be equal to kFile_Open & kFile_Read
const Common::FileOpenMode sc_File::fopenModes[] = 
    {Common::kFile_Open/*CHECKME, was undefined*/, Common::kFile_Open, Common::kFile_CreateAlways, Common::kFile_Create};
const Common::FileWorkMode sc_File::fworkModes[] = 
    {Common::kFile_Read/*CHECKME, was undefined*/, Common::kFile_Read, Common::kFile_Write, Common::kFile_Write};

int sc_File::Dispose(const char *address, bool force) {
    Close();
    delete this;
    return 1;
}

const char *sc_File::GetType() {
    return "File";
}

int sc_File::Serialize(const char *address, char *buffer, int bufsize) {
    // we cannot serialize an open file, so it will get closed
    return 0;
}

int sc_File::OpenFile(const char *filename, int mode) {
  handle = FileOpen(filename, fopenModes[mode], fworkModes[mode]);
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

sc_File::sc_File() {
    handle = NULL;
}
