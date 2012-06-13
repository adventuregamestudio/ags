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

#ifndef __CROOM_H
#define __CROOM_H

#if !defined __CROOM_FUNC_H && !defined __CROOM_NOFUNC_H
#error Do not include acroom.h directly, include either acroom_func.h or acroom_nofunc.h instead.
#endif

#if defined CROOM_NOFUNCTIONS
#error CROOM_NOFUNCTIONS macro is obsolete and should not be defined anymore.
#endif

#if defined NO_SAVE_FUNCTIONS
#error NO_SAVE_FUNCTIONS macro is obsolete and should not be defined anymore.
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "cscomp.h"

// MACPORT FIX: endian support
#include "bigend.h"

#error Do not include acroom.h for now
/*
//=============================================================================
#include "ac/ac_defines.h"
//=============================================================================
#include "ac/ac_common.h"
//=============================================================================
#include "ac/ac_object.h"
//=============================================================================

//=============================================================================
#include "ac/ac_interaction.h"
//=============================================================================
#include "ac/ac_customproperties.h"
//=============================================================================
#include "ac/ac_room.h"
//=============================================================================

//=============================================================================
#include "ac/ac_script.h"
//=============================================================================
#include "ac/ac_view.h"
//=============================================================================
#include "ac/ac_dialog.h"
//=============================================================================
#include "ac/ac_audioclip.h"
//=============================================================================
#include "ac/ac_gamesetupstruct.h"
//=============================================================================

//=============================================================================
#include "ac/ac_actiontype.h"
//=============================================================================
#include "ac/ac_lipsync.h"
//=============================================================================
#include "ac/ac_move.h"
//=============================================================================
*/

#endif  // __CROOM_H
