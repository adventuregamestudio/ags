
#include <string.h>
#include "debug/rawfileoutputtarget.h"
#include "debug/out.h"

using namespace AGS;
using namespace Engine;
//
// TODO:
// Use advanced utility classes instead of C-style strings and arrays.      
//

//
// TODO: filepath parameter here may be actually used as a pattern
// or prefix, while the actual filename could be made by combining
// this prefix with current date, game name, and similar additional
// useful information. Whether this is to be determined here or on
// high-level side remains a question.
//
out::CRawFileOutputTarget::CRawFileOutputTarget(const char *sz_filepath)
{
    File = NULL;

    int len = strlen(sz_filepath);
    FilePath = new char[len + 1];
    strcpy(FilePath, sz_filepath);

    DidWriteOnce = false;
}

out::CRawFileOutputTarget::~CRawFileOutputTarget()
{
    CloseFile();
    delete [] FilePath;
}

void out::CRawFileOutputTarget::out(const char *sz_fullmsg)
{
    if (!OpenFile()) {
        return;
    }

    fprintf(File, "%s\n", sz_fullmsg);
    CloseFile();
}

bool out::CRawFileOutputTarget::OpenFile()
{
    char *open_mode;
    if (DidWriteOnce) {
        open_mode = "at";
    }
    else {
        open_mode = "wt";
    }

    File = fopen(FilePath, open_mode);
    if (!File) {
        // TODO: make emergency call
        return false;
    }

    DidWriteOnce = true;
    return true;
}

void out::CRawFileOutputTarget::CloseFile()
{
    if (!File) {
        return;
    }

    fclose(File);
    File = NULL;
}
