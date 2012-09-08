
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_script.h"
#include "script/script_common.h"      // SCOM_VERSION, scfilesig
#include "cc_error.h"

void fput_long(long loo,FILE*ooo) {
    fwrite(&loo,4,1,ooo);
}

// 64 bit: This is supposed to read a 32 bit value
int fget_long(FILE * iii)
{
  int tmpp;
  fread(&tmpp, 4, 1, iii);
  return tmpp;
}

void freadstring(char **strptr, FILE * iii)
{
    static char ibuffer[300];
    int idxx = 0;

    while ((ibuffer[idxx] = fgetc(iii)) != 0)
        idxx++;

    if (ibuffer[0] == 0) {
        strptr[0] = NULL;
        return;
    }

    strptr[0] = (char *)malloc(strlen(ibuffer) + 1);
    strcpy(strptr[0], ibuffer);
}

void fwrite_script(ccScript*scri,FILE*ooo) {
    int n;
    fwrite(scfilesig,4,1,ooo);
    fput_long(SCOM_VERSION,ooo);
    fput_long(scri->globaldatasize,ooo);
    fput_long(scri->codesize,ooo);
    fput_long(scri->stringssize,ooo);
    if (scri->globaldatasize > 0)
        fwrite(scri->globaldata,scri->globaldatasize,1,ooo);
    if (scri->codesize > 0)
        fwrite(scri->code,scri->codesize,sizeof(long),ooo);
    if (scri->stringssize > 0)
        fwrite(scri->strings,scri->stringssize,1,ooo);
    fput_long(scri->numfixups,ooo);
    if (scri->numfixups > 0) {
        fwrite(scri->fixuptypes,scri->numfixups,1,ooo);
        fwrite(scri->fixups,scri->numfixups,sizeof(long),ooo);
    }
    fput_long(scri->numimports,ooo);
    for (n=0;n<scri->numimports;n++)
        fwrite(scri->imports[n],strlen(scri->imports[n])+1,1,ooo);
    fput_long(scri->numexports,ooo);
    for (n=0;n<scri->numexports;n++) {
        fwrite(scri->exports[n],strlen(scri->exports[n])+1,1,ooo);
        fput_long(scri->export_addr[n],ooo);
    }
    fput_long(scri->numSections, ooo);
    for (n = 0; n < scri->numSections; n++) {
        fwrite(scri->sectionNames[n], strlen(scri->sectionNames[n]) + 1, 1, ooo);
        fput_long(scri->sectionOffsets[n], ooo);
    }
    fput_long(ENDFILESIG,ooo);
}

ccScript *fread_script(FILE * ooo)
{
  ccScript *scri = (ccScript *) malloc(sizeof(ccScript));
  scri->instances = 0;
  int n;
  char gotsig[5];
  currentline = -1;
  // MACPORT FIX: swap 'size' and 'nmemb'
  fread(gotsig, 1, 4, ooo);
  gotsig[4] = 0;

  int fileVer = fget_long(ooo);

  if ((strcmp(gotsig, scfilesig) != 0) || (fileVer > SCOM_VERSION)) {
    cc_error("file was not written by fwrite_script or seek position is incorrect");
    free(scri);
    return NULL;
  }

  scri->globaldatasize = fget_long(ooo);
  scri->codesize = fget_long(ooo);
  scri->stringssize = fget_long(ooo);

  if (scri->globaldatasize > 0) {
    scri->globaldata = (char *)malloc(scri->globaldatasize);
    // MACPORT FIX: swap
    fread(scri->globaldata, sizeof(char), scri->globaldatasize, ooo);
  }
  else
    scri->globaldata = NULL;

  if (scri->codesize > 0) {
    scri->code = (long *)malloc(scri->codesize * sizeof(long));
    // MACPORT FIX: swap

    // 64 bit: Read code into 8 byte array, necessary for being able to perform
    // relocations on the references.
    int i;
    for (i = 0; i < scri->codesize; i++)
      scri->code[i] = fget_long(ooo);
  }
  else
    scri->code = NULL;

  if (scri->stringssize > 0) {
    scri->strings = (char *)malloc(scri->stringssize);
    // MACPORT FIX: swap
    fread(scri->strings, sizeof(char), scri->stringssize, ooo);
  } 
  else
    scri->strings = NULL;

  scri->numfixups = fget_long(ooo);
  if (scri->numfixups > 0) {
    scri->fixuptypes = (char *)malloc(scri->numfixups);
    scri->fixups = (long *)malloc(scri->numfixups * sizeof(long));
    // MACPORT FIX: swap 'size' and 'nmemb'
    fread(scri->fixuptypes, sizeof(char), scri->numfixups, ooo);

    // 64 bit: Read fixups into 8 byte array too
    int i;
    for (i = 0; i < scri->numfixups; i++)
      scri->fixups[i] = fget_long(ooo);
  }
  else {
    scri->fixups = NULL;
    scri->fixuptypes = NULL;
  }

  scri->numimports = fget_long(ooo);

  scri->imports = (char**)malloc(sizeof(char*) * scri->numimports);
  for (n = 0; n < scri->numimports; n++)
    freadstring(&scri->imports[n], ooo);

  scri->numexports = fget_long(ooo);
  scri->exports = (char**)malloc(sizeof(char*) * scri->numexports);
  scri->export_addr = (long*)malloc(sizeof(long) * scri->numexports);
  for (n = 0; n < scri->numexports; n++) {
    freadstring(&scri->exports[n], ooo);
    scri->export_addr[n] = fget_long(ooo);
  }

  if (fileVer >= 83) {
    // read in the Sections
    scri->numSections = fget_long(ooo);
    scri->sectionNames = (char**)malloc(scri->numSections * sizeof(char*));
    scri->sectionOffsets = (long*)malloc(scri->numSections * sizeof(long));
    for (n = 0; n < scri->numSections; n++) {
      freadstring(&scri->sectionNames[n], ooo);
      scri->sectionOffsets[n] = fget_long(ooo);
    }
  }
  else
  {
    scri->numSections = 0;
    scri->sectionNames = NULL;
    scri->sectionOffsets = NULL;
  }

  if (fget_long(ooo) != ENDFILESIG) {
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
