
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_script.h"
#include "script/script_common.h"      // SCOM_VERSION, scfilesig
#include "cc_error.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

// [IKM] I reckon this function is almost identical to fgetstring in string_utils
void freadstring(char **strptr, CDataStream *in)
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

void fwrite_script(ccScript*scri, CDataStream *out) {
    int n;
    out->Write(scfilesig,4);
    out->WriteInt32(SCOM_VERSION);
    out->WriteInt32(scri->globaldatasize);
    out->WriteInt32(scri->codesize);
    out->WriteInt32(scri->stringssize);
    if (scri->globaldatasize > 0)
        out->WriteArray(scri->globaldata,scri->globaldatasize,1);
    if (scri->codesize > 0)
        out->WriteArrayOfIntPtr32((intptr_var_t*)scri->code,scri->codesize);
    if (scri->stringssize > 0)
        out->WriteArray(scri->strings,scri->stringssize,1);
    out->WriteInt32(scri->numfixups);
    if (scri->numfixups > 0) {
        out->WriteArray(scri->fixuptypes,scri->numfixups,1);
        out->WriteArrayOfIntPtr32((intptr_var_t*)scri->fixups,scri->numfixups);
    }
    out->WriteInt32(scri->numimports);
    for (n=0;n<scri->numimports;n++)
        out->WriteArray(scri->imports[n],strlen(scri->imports[n])+1,1);
    out->WriteInt32(scri->numexports);
    for (n=0;n<scri->numexports;n++) {
        out->WriteArray(scri->exports[n],strlen(scri->exports[n])+1,1);
        out->WriteInt32(scri->export_addr[n]);
    }
    out->WriteInt32(scri->numSections);
    for (n = 0; n < scri->numSections; n++) {
        out->WriteArray(scri->sectionNames[n], strlen(scri->sectionNames[n]) + 1, 1);
        out->WriteInt32(scri->sectionOffsets[n]);
    }
    out->WriteInt32(ENDFILESIG);
}

ccScript *fread_script(CDataStream *in)
{
  ccScript *scri = (ccScript *) malloc(sizeof(ccScript));
  scri->instances = 0;
  int n;
  char gotsig[5];
  currentline = -1;
  // MACPORT FIX: swap 'size' and 'nmemb'
  in->Read(gotsig, 4);
  gotsig[4] = 0;

  int fileVer = in->ReadInt32();

  if ((strcmp(gotsig, scfilesig) != 0) || (fileVer > SCOM_VERSION)) {
    cc_error("file was not written by fwrite_script or seek position is incorrect");
    free(scri);
    return NULL;
  }

  scri->globaldatasize = in->ReadInt32();
  scri->codesize = in->ReadInt32();
  scri->stringssize = in->ReadInt32();

  if (scri->globaldatasize > 0) {
    scri->globaldata = (char *)malloc(scri->globaldatasize);
    // MACPORT FIX: swap
    in->Read(scri->globaldata, scri->globaldatasize);
  }
  else
    scri->globaldata = NULL;

  if (scri->codesize > 0) {
    scri->code = (long *)malloc(scri->codesize * sizeof(long));
    // MACPORT FIX: swap

    // 64 bit: Read code into 8 byte array, necessary for being able to perform
    // relocations on the references.
    in->ReadArrayOfIntPtr32((intptr_var_t*)scri->code, scri->codesize);
    //int i;
    //for (i = 0; i < scri->codesize; i++)
    //  scri->code[i] = in->ReadInt32();
  }
  else
    scri->code = NULL;

  if (scri->stringssize > 0) {
    scri->strings = (char *)malloc(scri->stringssize);
    // MACPORT FIX: swap
    in->Read(scri->strings, scri->stringssize);
  } 
  else
    scri->strings = NULL;

  scri->numfixups = in->ReadInt32();
  if (scri->numfixups > 0) {
    scri->fixuptypes = (char *)malloc(scri->numfixups);
    scri->fixups = (long *)malloc(scri->numfixups * sizeof(long));
    // MACPORT FIX: swap 'size' and 'nmemb'
    in->Read(scri->fixuptypes, scri->numfixups);

    // 64 bit: Read fixups into 8 byte array too
    in->ReadArrayOfIntPtr32((intptr_var_t*)scri->fixups, scri->numfixups);
    //int i;
    //for (i = 0; i < scri->numfixups; i++)
    //  scri->fixups[i] = in->ReadInt32();
  }
  else {
    scri->fixups = NULL;
    scri->fixuptypes = NULL;
  }

  scri->numimports = in->ReadInt32();

  scri->imports = (char**)malloc(sizeof(char*) * scri->numimports);
  for (n = 0; n < scri->numimports; n++)
    freadstring(&scri->imports[n], in);

  scri->numexports = in->ReadInt32();
  scri->exports = (char**)malloc(sizeof(char*) * scri->numexports);
  scri->export_addr = (long*)malloc(sizeof(long) * scri->numexports);
  for (n = 0; n < scri->numexports; n++) {
    freadstring(&scri->exports[n], in);
    scri->export_addr[n] = in->ReadInt32();
  }

  if (fileVer >= 83) {
    // read in the Sections
    scri->numSections = in->ReadInt32();
    scri->sectionNames = (char**)malloc(scri->numSections * sizeof(char*));
    scri->sectionOffsets = (long*)malloc(scri->numSections * sizeof(long));
    for (n = 0; n < scri->numSections; n++) {
      freadstring(&scri->sectionNames[n], in);
      scri->sectionOffsets[n] = in->ReadInt32();
    }
  }
  else
  {
    scri->numSections = 0;
    scri->sectionNames = NULL;
    scri->sectionOffsets = NULL;
  }

  if (in->ReadInt32() != ENDFILESIG) {
    cc_error("internal error rebuilding script");
    free(scri);
    return NULL;
  }
  return scri;
}

void ccFreeScript(ccScript * ccs)
{
    if (ccs->globaldata != NULL)
        free(ccs->globaldata);

    if (ccs->code != NULL)
        free(ccs->code);

    if (ccs->strings != NULL)
        free(ccs->strings);

    if (ccs->fixups != NULL && ccs->numfixups > 0)
        free(ccs->fixups);

    if (ccs->fixuptypes != NULL && ccs->numfixups > 0)
        free(ccs->fixuptypes);

    ccs->globaldata = NULL;
    ccs->code = NULL;
    ccs->strings = NULL;
    ccs->fixups = NULL;
    ccs->fixuptypes = NULL;

    int aa;
    for (aa = 0; aa < ccs->numimports; aa++) {
        if (ccs->imports[aa] != NULL)
            free(ccs->imports[aa]);
    }

    for (aa = 0; aa < ccs->numexports; aa++)
        free(ccs->exports[aa]);

    for (aa = 0; aa < ccs->numSections; aa++)
        free(ccs->sectionNames[aa]);

    if (ccs->sectionNames != NULL)
    {
        free(ccs->sectionNames);
        free(ccs->sectionOffsets);
        ccs->sectionNames = NULL;
        ccs->sectionOffsets = NULL;
    }


    if (ccs->imports != NULL)
    {
        free(ccs->imports);
        free(ccs->exports);
        free(ccs->export_addr);
        ccs->imports = NULL;
        ccs->exports = NULL;
        ccs->export_addr = NULL;
    }
    ccs->numimports = 0;
    ccs->numexports = 0;
    ccs->numSections = 0;
}
