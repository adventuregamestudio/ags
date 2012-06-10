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

#include "cscomp.h"

// MACPORT FIX: endian support
#include "bigend.h"

//=============================================================================
#include "acroom/acroom_defines.h"
//=============================================================================
#include "acroom/acroom_core.h"
//=============================================================================
#include "acroom/acroom_object.h"
//=============================================================================

//=============================================================================
#include "acroom/acroom_interaction.h"
//=============================================================================
#include "acroom/acroom_properties.h"
//=============================================================================
#include "acroom/acroom_room.h"
//=============================================================================

//=============================================================================
#include "acroom/acroom_script.h"
//=============================================================================
#include "acroom/acroom_view.h"
//=============================================================================
#include "acroom/acroom_dialog.h"
//=============================================================================
#include "acroom/acroom_audio.h"
//=============================================================================
#include "acroom/acroom_gamestruct.h"
//=============================================================================

//=============================================================================
#include "acroom/acroom_action.h"
//=============================================================================
#include "acroom/acroom_lipsync.h"
//=============================================================================
#include "acroom/acroom_move.h"
//=============================================================================

#endif  // __CROOM_H
