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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_script.h"
#include "script/script_common.h"      // SCOM_VERSION, scfilesig
#include "cc_error.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

// [IKM] I reckon this function is almost identical to fgetstring in string_utils
void freadstring(char **strptr, DataStream *in)
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

ccScript *ccScript::CreateFromStream(Common::DataStream *in)
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

ccScript::~ccScript()
{
    Free();
}

void ccScript::Write(DataStream *out) {
    int n;
    out->Write(scfilesig,4);
    out->WriteInt32(SCOM_VERSION);
    out->WriteInt32(globaldatasize);
    out->WriteInt32(codesize);
    out->WriteInt32(stringssize);
    if (globaldatasize > 0)
        out->WriteArray(globaldata,globaldatasize,1);
    if (codesize > 0)
        out->WriteArrayOfIntPtr32((intptr_t*)code,codesize);
    if (stringssize > 0)
        out->WriteArray(strings,stringssize,1);
    out->WriteInt32(numfixups);
    if (numfixups > 0) {
        out->WriteArray(fixuptypes,numfixups,1);
        out->WriteArrayOfIntPtr32((intptr_t*)fixups,numfixups);
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

bool ccScript::Read(DataStream *in)
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
    code = (long *)malloc(codesize * sizeof(long));
    // MACPORT FIX: swap

    // 64 bit: Read code into 8 byte array, necessary for being able to perform
    // relocations on the references.
    in->ReadArrayOfIntPtr32((intptr_t*)code, codesize);
    //int i;
    //for (i = 0; i < codesize; i++)
    //  code[i] = in->ReadInt32();
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
    fixups = (long *)malloc(numfixups * sizeof(long));
    // MACPORT FIX: swap 'size' and 'nmemb'
    in->Read(fixuptypes, numfixups);

    // 64 bit: Read fixups into 8 byte array too
    in->ReadArrayOfIntPtr32((intptr_t*)fixups, numfixups);
    //int i;
    //for (i = 0; i < numfixups; i++)
    //  fixups[i] = in->ReadInt32();
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
  export_addr = (long*)malloc(sizeof(long) * numexports);
  for (n = 0; n < numexports; n++) {
    freadstring(&exports[n], in);
    export_addr[n] = in->ReadInt32();
  }

  if (fileVer >= 83) {
    // read in the Sections
    numSections = in->ReadInt32();
    sectionNames = (char**)malloc(numSections * sizeof(char*));
    sectionOffsets = (long*)malloc(numSections * sizeof(long));
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

const char* ccScript::GetSectionName(long offs) {

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
