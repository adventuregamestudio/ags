/*
** ACROOM - AGS main header file
** Copyright (C) 1995-2003, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
** INTERNAL WORKING COPY: This file should NEVER leave my computer - if
** you have this file then you are in breach of the license agreement
** and must delete it at once.
*/

//=============================================================================
//
// This is originally a part of acroom.h, that was put under CROOM_NOFUNCTIONS
// macro control and enabled when CROOM_NOFUNCTIONS was *NOT* set.
// This should be included INSTEAD of acroom.h in the source files that
// previously included acroom.h with *NO* CROOM_NOFUNCTIONS define.
// There's no need to include both acroom.h and acroom_func.h, since latter
// includes acroom.h on its own.
// CROOM_NOFUNCTIONS macro is being removed since no longer needed.
//
//=============================================================================

#ifndef __CROOM_FUNC_H
#define __CROOM_FUNC_H

#include "acroom.h"
#include "compress.h"

extern char *croom_h_copyright;
extern char *game_file_sig;
#define GAME_FILE_VERSION 42
extern block backups[5];


extern int cunpackbitl(unsigned char *, int size, FILE *infile);

/*long cloadcompfile(FILE*outpt,block tobesaved,color*pal,long poot=0);
  }*/


// ** SCHEMA LOAD/SAVE FUNCTIONS
//void CustomPropertySchema::Serialize (FILE *outto);
//int CustomPropertySchema::UnSerialize (FILE *infrom);


// ** OBJECT PROPERTIES LOAD/SAVE FUNCTIONS
//void CustomProperties::Serialize (FILE *outto);
//int CustomProperties::UnSerialize (FILE *infrom);


extern int in_interaction_editor;


//void WordsDictionary::sort ();
//int WordsDictionary::find_index (const char*wrem);

extern ActionTypes actions[NUM_ACTION_TYPES];
extern InteractionVariable globalvars[MAX_GLOBAL_VARIABLES];
extern int numGlobalVars;

void serialize_command_list (NewInteractionCommandList *nicl, FILE*ooo);
void serialize_new_interaction (NewInteraction *nint, FILE*ooo);
NewInteractionCommandList *deserialize_command_list (FILE *ooo);

extern NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (FILE *ooo);

//void NewInteractionCommandList::reset ();



extern int ff;
/*void roomstruct::allocall();
*/
//void roomstruct::freemessage();
/*void roomstruct::freeall();
}*/
/*void roomstruct::freeall();*/

struct room_file_header {
  short version PCKD;
  
#ifdef ALLEGRO_BIG_ENDIAN
  void ReadFromFile(FILE *fp)
  {
    version = __getshort__bigendian(fp);
  }
#endif
};

extern int _acroom_bpp;  // bytes per pixel of currently loading room

// returns bytes per pixel for bitmap's color depth
extern int bmp_bpp(BITMAP*bmpt);


extern void update_polled_stuff_if_runtime();
//#ifdef LOADROOM_DO_POLL
//extern void update_polled_stuff();
//#else
//static void update_polled_stuff() { }
//#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION) || defined(DJGPP) || defined(_MSC_VER)
extern void lzwcompress(FILE *,FILE *);
extern void lzwexpand(FILE *,FILE *);
extern unsigned char *lzwexpand_to_mem(FILE *);
extern long maxsize, outbytes, putbytes;
extern char *lztempfnm;

extern long save_lzw(char *fnn, BITMAP *bmpp, color *pall, long offe);
extern BITMAP *recalced;
/*long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff);*/
extern long load_lzw(FILE *iii, BITMAP *bmm, color *pall);
extern long savecompressed_allegro(char *fnn, BITMAP *bmpp, color *pall, long ooo);
extern long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo);
#endif // LINUX_VERSION || MAC_VERSION || DJGPP || _MSC_VER

#define BLOCKTYPE_MAIN        1
#define BLOCKTYPE_SCRIPT      2
#define BLOCKTYPE_COMPSCRIPT  3
#define BLOCKTYPE_COMPSCRIPT2 4
#define BLOCKTYPE_OBJECTNAMES 5
#define BLOCKTYPE_ANIMBKGRND  6
#define BLOCKTYPE_COMPSCRIPT3 7     // new CSCOMP script instead of SeeR
#define BLOCKTYPE_PROPERTIES  8
#define BLOCKTYPE_OBJECTSCRIPTNAMES 9
#define BLOCKTYPE_EOF         0xff

extern void load_script_configuration(FILE *);
extern void save_script_configuration(FILE *);
extern void load_graphical_scripts(FILE *, roomstruct *);
extern void save_graphical_scripts(FILE *, roomstruct *);
extern char *passwencstring;;

extern void decrypt_text(char*toenc);
extern void read_string_decrypt(FILE *ooo, char *sss);
extern void read_dictionary (WordsDictionary *dict, FILE *writeto);
extern void freadmissout(short *pptr, FILE *opty);

#define HS_STANDON    0
#define HS_LOOKAT     1
#define HS_INTERACT   2
#define HS_USEINV     3
#define HS_TALKTO     4
#define OBJ_LOOKAT    0
#define OBJ_INTERACT  1
#define OBJ_TALKTO    2
#define OBJ_USEINV    3

extern void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr);
extern int usesmisccond;

void deserialize_interaction_scripts(FILE *iii, InteractionScripts *scripts);

extern void load_main_block(roomstruct *rstruc, char *files, FILE *opty, room_file_header rfh);
extern void load_room(char *files, roomstruct *rstruc, bool gameIsHighRes);
//void ViewStruct::Initialize(int loopCount);
//void ViewStruct::Dispose();
//void ViewStruct::WriteToFile(FILE *ooo);
//void ViewStruct::ReadFromFile(FILE *iii);
//void ViewLoopNew::Initialize(int frameCount);
//void ViewLoopNew::Dispose();
//void ViewLoopNew::WriteToFile(FILE *ooo);
//void ViewLoopNew::ReadFromFile(FILE *iii);

//int CharacterInfo::get_effective_y();
//int CharacterInfo::get_baseline();
//int CharacterInfo::get_blocking_top();
//int CharacterInfo::get_blocking_bottom();


#define COPY_CHAR_VAR(name) ci->name = oci->name
void ConvertOldCharacterToNew (OldCharacterInfo *oci, CharacterInfo *ci);
void ConvertOldGameStruct (OldGameSetupStruct *ogss, GameSetupStruct *gss);
void Convert272ViewsToNew (int numof, ViewStruct272 *oldv, ViewStruct *newv);


#endif // __CROOM_FUNC_H