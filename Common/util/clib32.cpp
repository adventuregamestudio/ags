/* CLIB32 - DJGPP implemention of the CLIB reader.
  (c) 1998-99 Chris Jones
  
  22/12/02 - Shawn's Linux changes approved and integrated - CJ

  v1.2 (Apr'01)  added support for new multi-file CLIB version 10 files
  v1.1 (Jul'99)  added support for appended-to-exe data files

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/file.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

#if defined(ANDROID_VERSION) || defined(IOS_VERSION)
#include <sys/stat.h>
#endif

#if defined(LINUX_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define NATIVESTATIC

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
// don't allow these fields to be in the assembly manifest
#undef NATIVESTATIC
#define NATIVESTATIC static
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <io.h>
#else
//#include "djcompat.h"
#include "allegro.h"
#endif
#include "util/misc.h"

#ifdef MAC_VERSION
#include "macport.h"
#include <sys/stat.h>
#endif

#include "util/string_utils.h"

#define CLIB_IS_INSTALLED
char clib32copyright[] = "CLIB32 v1.21 (c) 1995,1996,1998,2001,2007 Chris Jones";
char lib_file_name[255] = " ";
char base_path[255] = ".";
char original_base_filename[255];
char clbuff[20];
const int RAND_SEED_SALT = 9338638;  // must update editor agsnative.cpp if this changes
#define MAX_FILES 10000
#define MAXMULTIFILES 25

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
static off_t filelength(int fd) {
  struct stat st;
  fstat(fd, &st);
  return st.st_size;
}
#endif

struct MultiFileLib
{
  char data_filenames[MAXMULTIFILES][20];
  int num_data_files;
  char filenames[MAX_FILES][25];
  int offset[MAX_FILES];
  int length[MAX_FILES];
  char file_datafile[MAX_FILES];        // number of datafile
  int num_files;
};

struct MultiFileLibNew
{
  char data_filenames[MAXMULTIFILES][50];
  int num_data_files;
  char filenames[MAX_FILES][100];
  int offset[MAX_FILES];
  int length[MAX_FILES];
  char file_datafile[MAX_FILES];        // number of datafile
  int num_files;
};

MultiFileLibNew mflib;
NATIVESTATIC char *clibendfilesig = "CLIB\x1\x2\x3\x4SIGE";
NATIVESTATIC char *clibpasswencstring = "My\x1\xde\x4Jibzle";
NATIVESTATIC int _last_rand;

void init_pseudo_rand_gen(int seed)
{
  _last_rand = seed;
}

int get_pseudo_rand()
{
  return( ((_last_rand = _last_rand * 214013L
      + 2531011L) >> 16) & 0x7fff );
}

void clib_decrypt_text(char *toenc)
{
  int adx = 0;

  while (1) {
    toenc[0] -= clibpasswencstring[adx];
    if (toenc[0] == 0)
      break;

    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void fgetnulltermstring(char *sss, CDataStream *ci_s, int bufsize) {
  int b = -1;
  do {
    if (b < bufsize - 1)
      b++;
    sss[b] = ci_s->ReadInt8();
    if (ci_s->EOS())
      return;
  } while (sss[b] != 0);
}

extern "C"
{
  long last_opened_size;

  void fread_data_enc(void *data, int dataSize, int dataCount, CDataStream *ci_s)
  {
    ci_s->ReadArray(data, dataSize, dataCount);
    unsigned char *dataChar = (unsigned char*)data;
    for (int i = 0; i < dataSize * dataCount; i++)
    {
      dataChar[i] -= get_pseudo_rand();
    }
  }

  void fgetstring_enc(char *sss, CDataStream *ci_s, int maxLength) 
  {
    int i = 0;
    while ((i == 0) || (sss[i - 1] != 0))
    {
      sss[i] = ci_s->ReadInt8() - get_pseudo_rand();

      if (i < maxLength - 1)
        i++;
    }
  }

  int getw_enc(CDataStream *ci_s)
  {
    int numberRead;
    fread_data_enc(&numberRead, 4, 1, ci_s);
    return numberRead;
  }

  int read_new_new_enc_format_clib(MultiFileLibNew * mfl, CDataStream *ci_s, int libver)
  {
    int aa;
    int randSeed = ci_s->ReadInt32();
    init_pseudo_rand_gen(randSeed + RAND_SEED_SALT);
    mfl->num_data_files = getw_enc(ci_s);
    for (aa = 0; aa < mfl->num_data_files; aa++)
    {
      fgetstring_enc(mfl->data_filenames[aa], ci_s, 50);
    }
    mfl->num_files = getw_enc(ci_s);

    if (mfl->num_files > MAX_FILES)
      return -1;

    for (aa = 0; aa < mfl->num_files; aa++)
    {
      fgetstring_enc(mfl->filenames[aa], ci_s, 100);
    }
    fread_data_enc(&mfl->offset[0], sizeof(int), mfl->num_files, ci_s);
    fread_data_enc(&mfl->length[0], sizeof(int), mfl->num_files, ci_s);
    fread_data_enc(&mfl->file_datafile[0], 1, mfl->num_files, ci_s);
    return 0;
  }

  int read_new_new_format_clib(MultiFileLibNew * mfl, CDataStream *ci_s, int libver)
  {
    int aa;
    mfl->num_data_files = ci_s->ReadInt32();
    for (aa = 0; aa < mfl->num_data_files; aa++)
    {
      fgetnulltermstring(mfl->data_filenames[aa], ci_s, 50);
    }
    mfl->num_files = ci_s->ReadInt32();

    if (mfl->num_files > MAX_FILES)
      return -1;

    for (aa = 0; aa < mfl->num_files; aa++)
    {
      short nameLength;
      ci_s->ReadArray(&nameLength, 2, 1);
      nameLength /= 5;
      ci_s->ReadArray(mfl->filenames[aa], nameLength, 1);
      clib_decrypt_text(mfl->filenames[aa]);
    }
    ci_s->ReadArray(&mfl->offset[0], sizeof(int), mfl->num_files);
    ci_s->ReadArray(&mfl->length[0], sizeof(int), mfl->num_files);
    ci_s->ReadArray(&mfl->file_datafile[0], 1, mfl->num_files);
    return 0;
  }

  int read_new_format_clib(MultiFileLib * mfl, CDataStream *ci_s, int libver)
  {
    mfl->num_data_files = ci_s->ReadInt32();
    ci_s->ReadArray(&mfl->data_filenames[0][0], 20, mfl->num_data_files);
    mfl->num_files = ci_s->ReadInt32();

    if (mfl->num_files > MAX_FILES)
      return -1;

    ci_s->ReadArray(&mfl->filenames[0][0], 25, mfl->num_files);
    ci_s->ReadArray(&mfl->offset[0], sizeof(int), mfl->num_files);
    ci_s->ReadArray(&mfl->length[0], sizeof(int), mfl->num_files);
    ci_s->ReadArray(&mfl->file_datafile[0], 1, mfl->num_files);

    if (libver >= 11)
    {
      int aa;
      for (aa = 0; aa < mfl->num_files; aa++)
          clib_decrypt_text(mfl->filenames[aa]);
    }
    return 0;
  }

  int csetlib(char *namm, char *passw)
  {
    original_base_filename[0] = 0;

    if (namm == NULL) {
      lib_file_name[0] = ' ';
      lib_file_name[1] = 0;
      return 0;
    }
    strcpy(base_path, ".");

    int passwmodifier = 0, cc, aa;
    CDataStream *ci_s = ci_fopen(namm, Common::kFile_Open, Common::kFile_Read);
    if (ci_s == NULL)
      return -1;

    long absoffs = 0;
    ci_s->ReadArray(&clbuff[0], 5, 1);

    if (strncmp(clbuff, "CLIB", 4) != 0) {
        ci_s->Seek(Common::kSeekEnd, -12);
      ci_s->ReadArray(&clbuff[0], 12, 1);

      if (strncmp(clbuff, clibendfilesig, 12) != 0)
        return -2;

      ci_s->Seek(Common::kSeekEnd, -16);  // it's an appended-to-end-of-exe thing
      int debug_pos2 = ci_s->GetPosition();
      absoffs = ci_s->ReadInt32();
      ci_s->Seek(Common::kSeekBegin, absoffs + 5);
    }

    int lib_version = ci_s->ReadInt8();
    if ((lib_version != 6) && (lib_version != 10) &&
        (lib_version != 11) && (lib_version != 15) &&
        (lib_version != 20) && (lib_version != 21))
      return -3;  // unsupported version

    char *nammwas = namm;
    // remove slashes so that the lib name fits in the buffer
    while ((strchr(namm, '\\') != NULL) || (strchr(namm, '/') != NULL))
      namm++;

    if (namm != nammwas) {
      // store complete path
      strcpy(base_path, nammwas);
      base_path[namm - nammwas] = 0;
      if ((base_path[strlen(base_path) - 1] == '\\') || (base_path[strlen(base_path) - 1] == '/'))
        base_path[strlen(base_path) - 1] = 0;
    }

    if (lib_version >= 10) {
      if (ci_s->ReadInt8() != 0)
        return -4;  // not first datafile in chain

      if (lib_version >= 21)
      {
        if (read_new_new_enc_format_clib(&mflib, ci_s, lib_version))
          return -5;
      }
      else if (lib_version == 20)
      {
        if (read_new_new_format_clib(&mflib, ci_s, lib_version))
          return -5;
      }
      else 
      {
        // PSP: Allocate struct on the heap to avoid overflowing the stack.
        MultiFileLib* mflibOld = (MultiFileLib*)malloc(sizeof(MultiFileLib));

        if (read_new_format_clib(mflibOld, ci_s, lib_version))
          return -5;
        // convert to newer format
        mflib.num_files = mflibOld->num_files;
        mflib.num_data_files = mflibOld->num_data_files;
        memcpy(&mflib.offset[0], &mflibOld->offset[0], sizeof(int) * mflib.num_files);
        memcpy(&mflib.length[0], &mflibOld->length[0], sizeof(int) * mflib.num_files);
        memcpy(&mflib.file_datafile[0], &mflibOld->file_datafile[0], sizeof(char) * mflib.num_files);
        for (aa = 0; aa < mflib.num_data_files; aa++)
          strcpy(mflib.data_filenames[aa], mflibOld->data_filenames[aa]);
        for (aa = 0; aa < mflib.num_files; aa++)
          strcpy(mflib.filenames[aa], mflibOld->filenames[aa]);

        free(mflibOld);
      }

      delete ci_s;
      strcpy(lib_file_name, namm);

      // make a backup of the original file name
      strcpy(original_base_filename, mflib.data_filenames[0]);
      strlwr(original_base_filename);

      strcpy(mflib.data_filenames[0], namm);
      for (aa = 0; aa < mflib.num_files; aa++) {
        // correct offsetes for EXE file
        if (mflib.file_datafile[aa] == 0)
          mflib.offset[aa] += absoffs;
      }
      return 0;
    }

    passwmodifier = ci_s->ReadInt8();
    ci_s->ReadInt8(); // unused byte
    mflib.num_data_files = 1;
    strcpy(mflib.data_filenames[0], namm);

    short tempshort;
    ci_s->ReadArray(&tempshort, 2, 1);
    mflib.num_files = tempshort;

    if (mflib.num_files > MAX_FILES)
      return -4;

    ci_s->ReadArray(clbuff, 13, 1);  // skip password dooberry
    for (aa = 0; aa < mflib.num_files; aa++) {
      ci_s->ReadArray(&mflib.filenames[aa][0], 13, 1);
      for (cc = 0; cc < (int)strlen(mflib.filenames[aa]); cc++)
        mflib.filenames[aa][cc] -= passwmodifier;
    }
    ci_s->ReadArray(&mflib.length[0], 4, mflib.num_files);
    ci_s->Seek(Common::kSeekCurrent, 2 * mflib.num_files);  // skip flags & ratio

    mflib.offset[0] = ci_s->GetPosition();
    strcpy(lib_file_name, namm);
    delete ci_s;

    for (aa = 1; aa < mflib.num_files; aa++) {
      mflib.offset[aa] = mflib.offset[aa - 1] + mflib.length[aa - 1];
      mflib.file_datafile[aa] = 0;
    }
    mflib.file_datafile[0] = 0;
    return 0;
  }

  int clibGetNumFiles()
  {
    if (lib_file_name[0] == ' ')
      return 0;
    return mflib.num_files;
  }

  const char *clibGetFileName(int index)
  {
    if (lib_file_name[0] == ' ')
      return NULL;

    if ((index < 0) || (index >= mflib.num_files))
      return NULL;

    return &mflib.filenames[index][0];
  }

  int clibfindindex(const char *fill)
  {
    if (lib_file_name[0] == ' ')
      return -1;

    int bb;
    for (bb = 0; bb < mflib.num_files; bb++) {
      if (stricmp(mflib.filenames[bb], fill) == 0)
        return bb;
    }
    return -1;
  }

  long clibfilesize(const char *fill)
  {
    int idxx = clibfindindex(fill);
    if (idxx >= 0)
      return mflib.length[idxx];
    return -1;
  }

  long cliboffset(const char *fill)
  {
    int idxx = clibfindindex(fill);
    if (idxx >= 0)
      return mflib.offset[idxx];
    return -1;
  }

  const char *clibgetoriginalfilename() {
    return original_base_filename;
  }

  char actfilename[250];
  char *clibgetdatafile(char *fill)
  {
    int idxx = clibfindindex(fill);
    if (idxx >= 0) {
#if defined(LINUX_VERSION) || defined(MAC_VERSION) 
      sprintf(actfilename, "%s/%s", base_path, mflib.data_filenames[mflib.file_datafile[idxx]]);
#else
      sprintf(actfilename, "%s\\%s", base_path, mflib.data_filenames[mflib.file_datafile[idxx]]);
#endif
      return &actfilename[0];
    }
    return NULL;
  }

  CDataStream *tfil;
  CDataStream *clibopenfile(const char *filly, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
  {
    int bb;
    for (bb = 0; bb < mflib.num_files; bb++) {
      if (stricmp(mflib.filenames[bb], filly) == 0) {
        char actfilename[250];
#if defined(ANDROID_VERSION)
        sprintf(actfilename, "%s/%s", base_path, mflib.data_filenames[mflib.file_datafile[bb]]);
#else
        sprintf(actfilename, "%s\\%s", base_path, mflib.data_filenames[mflib.file_datafile[bb]]);
#endif
        tfil = ci_fopen(actfilename, open_mode, work_mode);
        if (tfil == NULL)
          return NULL;
        tfil->Seek(Common::kSeekBegin, mflib.offset[bb]);
        return tfil;
      }
    }
    return ci_fopen(filly, open_mode, work_mode);
  }

#define PR_DATAFIRST 1
#define PR_FILEFIRST 2
  int cfopenpriority = PR_DATAFIRST;

  CDataStream *clibfopen(const char *filnamm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
  {
    last_opened_size = -1;
    if (cfopenpriority == PR_FILEFIRST) {
      // check for file, otherwise use datafile
      if (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read) {
        tfil = ci_fopen(filnamm, open_mode, work_mode);
      } else {
        tfil = ci_fopen(filnamm, open_mode, work_mode);

        if ((tfil == NULL) && (lib_file_name[0] != ' ')) {
          tfil = clibopenfile(filnamm, open_mode, work_mode);
          last_opened_size = clibfilesize(filnamm);
        }
      }

    } else {
      // check datafile first, then scan directory
        if ((cliboffset(filnamm) < 1) ||
            (open_mode != Common::kFile_Open || work_mode != Common::kFile_Read))
        tfil = ci_fopen(filnamm, open_mode, work_mode);
      else {
        tfil = clibopenfile(filnamm, open_mode, work_mode);
        last_opened_size = clibfilesize(filnamm);
      }

    }

    if ((last_opened_size < 0) && (tfil != NULL))
      last_opened_size = tfil->GetLength();

    return tfil;
  }
} // extern "C"
