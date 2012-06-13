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

//=============================================================================
//#include "ac/acroom_func_core.h"
//=============================================================================

//extern int cunpackbitl(unsigned char *, int size, FILE *infile);

/*long cloadcompfile(FILE*outpt,block tobesaved,color*pal,long poot=0);
  }*/


// ** SCHEMA LOAD/SAVE FUNCTIONS
//void CustomPropertySchema::Serialize (FILE *outto);
//int CustomPropertySchema::UnSerialize (FILE *infrom);


// ** OBJECT PROPERTIES LOAD/SAVE FUNCTIONS
//void CustomProperties::Serialize (FILE *outto);
//int CustomProperties::UnSerialize (FILE *infrom);

//=============================================================================
//#include "ac/acroom_func_action.h"
//=============================================================================
//#include "ac/acroom_func_interaction.h"
//=============================================================================




//=============================================================================
//#include "ac/acroom_func_compress.h"
//=============================================================================
//#include "ac/acroom_func_script.h"
//=============================================================================
//#include "ac/acroom_func_dictionary.h"
//=============================================================================

//
// Those defines are not used anywhere!

#ifdef UNUSED_CODE

#define HS_STANDON    0
#define HS_LOOKAT     1
#define HS_INTERACT   2
#define HS_USEINV     3
#define HS_TALKTO     4
#define OBJ_LOOKAT    0
#define OBJ_INTERACT  1
#define OBJ_TALKTO    2
#define OBJ_USEINV    3

#endif // UNUSED_CODE
//
//

//=============================================================================
//#include "ac/acroom_func_room.h"
//=============================================================================


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


//=============================================================================
//#include "ac/acroom_func_gamestruct.h"
//=============================================================================


#endif // __CROOM_FUNC_H