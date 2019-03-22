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

#include <stdlib.h>
#include <string.h>
#include "script/cc_error.h"
#include "script/cc_script.h"
#include "script/script_common.h"
#include "util/stream.h"

using AGS::Common::Stream;

// currently executed line
int currentline;
// script file format signature
const char scfilesig[5] = "SCOM";

// [IKM] I reckon this function is almost identical to fgetstring in string_utils
void freadstring(char **strptr, Stream *in)
{
    static char ibuffer[300];
    int idxx = 0;

    while ((ibuffer[idxx] = in->ReadInt8()) != 0)
        idxx++;

    if (ibuffer[0] == 0) {
        strptr[0] = NULL;
        return;
    }

    strptr[0] = (char *)malloc(strlen(ibuffer) + 1);
    strcpy(strptr[0], ibuffer);
}

ccScript *ccScript::CreateFromStream(Stream *in)
{
    ccScript *scri = new ccScript();
    if (!scri->Read(in))
    {
        delete scri;
        return NULL;
    }
    return scri;
}

ccScript::ccScript()
{
    globaldata          = NULL;
    globaldatasize      = 0;
    code                = NULL;
    codesize            = 0;
    strings             = NULL;
    stringssize         = 0;
    fixuptypes          = NULL;
    fixups              = NULL;
    numfixups           = 0;
    importsCapacity     = 0;
    imports             = NULL;
    numimports          = 0;
    exportsCapacity     = 0;
    exports             = NULL;
    export_addr         = NULL;
    numexports          = 0;
    instances           = 0;
    sectionNames        = NULL;
    sectionOffsets      = NULL;
    numSections         = 0;
    capacitySections    = 0;
}

ccScript::ccScript(const ccScript &src)
{
    globaldatasize = src.globaldatasize;
    if (globaldatasize > 0)
    {
        globaldata = (char*)malloc(globaldatasize);
        memcpy(globaldata, src.globaldata, globaldatasize);
    }
    else
    {
        globaldata = NULL;
    }

    codesize = src.codesize;
    if (codesize > 0)
    {
        code = (int32_t*)malloc(codesize * sizeof(int32_t));
        memcpy(code, src.code, sizeof(int32_t) * codesize);
    }
    else
    {
        code = NULL;
    }

    stringssize = src.stringssize;
    if (stringssize > 0)
    {
        strings = (char*)malloc(stringssize);
        memcpy(strings, src.strings, stringssize);
    }
    else
    {
        strings = NULL;
    }

    numfixups = src.numfixups;
    if (numfixups > 0)
    {
        fixuptypes = (char*)malloc(numfixups);
        fixups = (int32_t*)malloc(numfixups * sizeof(int32_t));
        memcpy(fixuptypes, src.fixuptypes, numfixups);
        memcpy(fixups, src.fixups, numfixups * sizeof(int32_t));
    }
    else
    {
        fixups = NULL;
        fixuptypes = NULL;
    }

    importsCapacity = src.numimports;
    numimports = src.numimports;
    if (numimports > 0)
    {
        imports = (char**)malloc(sizeof(char*) * numimports);
        for (int i = 0; i < numimports; ++i)
            imports[i] = strdup(src.imports[i]);
    }
    else
    {
        imports = NULL;
    }

    exportsCapacity = src.numexports;
    numexports = src.numexports;
    if (numexports > 0)
    {
        exports = (char**)malloc(sizeof(char*) * numexports);
        export_addr = (int32_t*)malloc(sizeof(int32_t) * numexports);
        for (int i = 0; i < numexports; ++i)
        {
            exports[i] = strdup(src.exports[i]);
            export_addr[i] = src.export_addr[i];
        }
    }
    else
    {
        exports = NULL;
        export_addr = NULL;
    }

    capacitySections = src.numSections;
    numSections = src.numSections;
    if (numSections > 0)
    {
        sectionNames = (char**)malloc(numSections * sizeof(char*));
        sectionOffsets = (int32_t*)malloc(numSections * sizeof(int32_t));
        for (int i = 0; i < numSections; ++i)
        {
            sectionNames[i] = strdup(src.sectionNames[i]);
            sectionOffsets[i] = src.sectionOffsets[i];
        }
    }
    else
    {
        numSections = 0;
        sectionNames = NULL;
        sectionOffsets = NULL;
    }

    instances = 0;
}

ccScript::~ccScript()
{
    Free();
}

void ccScript::Write(Stream *out) {
    int n;
    out->Write(scfilesig,4);
    out->WriteInt32(SCOM_VERSION);
    out->WriteInt32(globaldatasize);
    out->WriteInt32(codesize);
    out->WriteInt32(stringssize);
    if (globaldatasize > 0)
        out->WriteArray(globaldata,globaldatasize,1);
    if (codesize > 0)
        out->WriteArrayOfInt32(code,codesize);
    if (stringssize > 0)
        out->WriteArray(strings,stringssize,1);
    out->WriteInt32(numfixups);
    if (numfixups > 0) {
        out->WriteArray(fixuptypes,numfixups,1);
        out->WriteArrayOfInt32(fixups,numfixups);
    }
    out->WriteInt32(numimports);
    for (n=0;n<numimports;n++)
        out->WriteArray(imports[n],strlen(imports[n])+1,1);
    out->WriteInt32(numexports);
    for (n=0;n<numexports;n++) {
        out->WriteArray(exports[n],strlen(exports[n])+1,1);
        out->WriteInt32(export_addr[n]);
    }
    out->WriteInt32(numSections);
    for (n = 0; n < numSections; n++) {
        out->WriteArray(sectionNames[n], strlen(sectionNames[n]) + 1, 1);
        out->WriteInt32(sectionOffsets[n]);
    }
    out->WriteInt32(ENDFILESIG);
}

bool ccScript::Read(Stream *in)
{
  instances = 0;
  int n;
  char gotsig[5];
  currentline = -1;
  // MACPORT FIX: swap 'size' and 'nmemb'
  in->Read(gotsig, 4);
  gotsig[4] = 0;

  int fileVer = in->ReadInt32();

  if ((strcmp(gotsig, scfilesig) != 0) || (fileVer > SCOM_VERSION)) {
    cc_error("file was not written by ccScript::Write or seek position is incorrect");
    return false;
  }

  globaldatasize = in->ReadInt32();
  codesize = in->ReadInt32();
  stringssize = in->ReadInt32();

  if (globaldatasize > 0) {
    globaldata = (char *)malloc(globaldatasize);
    // MACPORT FIX: swap
    in->Read(globaldata, globaldatasize);
  }
  else
    globaldata = NULL;

  if (codesize > 0) {
    code = (int32_t *)malloc(codesize * sizeof(int32_t));
    // MACPORT FIX: swap

    // 64 bit: Read code into 8 byte array, necessary for being able to perform
    // relocations on the references.
    in->ReadArrayOfInt32(code, codesize);
  }
  else
    code = NULL;

  if (stringssize > 0) {
    strings = (char *)malloc(stringssize);
    // MACPORT FIX: swap
    in->Read(strings, stringssize);
  } 
  else
    strings = NULL;

  numfixups = in->ReadInt32();
  if (numfixups > 0) {
    fixuptypes = (char *)malloc(numfixups);
    fixups = (int32_t *)malloc(numfixups * sizeof(int32_t));
    // MACPORT FIX: swap 'size' and 'nmemb'
    in->Read(fixuptypes, numfixups);
    in->ReadArrayOfInt32(fixups, numfixups);
  }
  else {
    fixups = NULL;
    fixuptypes = NULL;
  }

  numimports = in->ReadInt32();

  imports = (char**)malloc(sizeof(char*) * numimports);
  for (n = 0; n < numimports; n++)
    freadstring(&imports[n], in);

  numexports = in->ReadInt32();
  exports = (char**)malloc(sizeof(char*) * numexports);
  export_addr = (int32_t*)malloc(sizeof(int32_t) * numexports);
  for (n = 0; n < numexports; n++) {
    freadstring(&exports[n], in);
    export_addr[n] = in->ReadInt32();
  }

  if (fileVer >= 83) {
    // read in the Sections
    numSections = in->ReadInt32();
    sectionNames = (char**)malloc(numSections * sizeof(char*));
    sectionOffsets = (int32_t*)malloc(numSections * sizeof(int32_t));
    for (n = 0; n < numSections; n++) {
      freadstring(&sectionNames[n], in);
      sectionOffsets[n] = in->ReadInt32();
    }
  }
  else
  {
    numSections = 0;
    sectionNames = NULL;
    sectionOffsets = NULL;
  }

  if (in->ReadInt32() != ENDFILESIG) {
    cc_error("internal error rebuilding script");
    return false;
  }
  return true;
}

void ccScript::Free()
{
    if (globaldata != NULL)
        free(globaldata);

    if (code != NULL)
        free(code);

    if (strings != NULL)
        free(strings);

    if (fixups != NULL && numfixups > 0)
        free(fixups);

    if (fixuptypes != NULL && numfixups > 0)
        free(fixuptypes);

    globaldata = NULL;
    code = NULL;
    strings = NULL;
    fixups = NULL;
    fixuptypes = NULL;

    int aa;
    for (aa = 0; aa < numimports; aa++) {
        if (imports[aa] != NULL)
            free(imports[aa]);
    }

    for (aa = 0; aa < numexports; aa++)
        free(exports[aa]);

    for (aa = 0; aa < numSections; aa++)
        free(sectionNames[aa]);

    if (sectionNames != NULL)
    {
        free(sectionNames);
        free(sectionOffsets);
        sectionNames = NULL;
        sectionOffsets = NULL;
    }

    if (imports != NULL)
    {
        free(imports);
        free(exports);
        free(export_addr);
        imports = NULL;
        exports = NULL;
        export_addr = NULL;
    }
    numimports = 0;
    numexports = 0;
    numSections = 0;
}

const char* ccScript::GetSectionName(int32_t offs) {

    int i;
    for (i = 0; i < numSections; i++) {
        if (sectionOffsets[i] < offs)
            continue;
        break;
    }

    // if no sections in script, return unknown
    if (i == 0)
        return "(unknown section)";

    return sectionNames[i - 1];
}
