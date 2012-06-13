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
// This is originally a part of acroom.h, that was put under NO_SAVE_FUNCTIONS
// macro control and enabled when NO_SAVE_FUNCTIONS was *NOT* set.
// This should be included ALONG with acroom_func.h in the source files that
// previously included acroom.h with *NO* NO_SAVE_FUNCTIONS define.
// NO_SAVE_FUNCTIONS macro is being removed since no longer needed.
//
//=============================================================================

#if defined NO_SAVE_FUNCTIONS
#error NO_SAVE_FUNCTIONS macro is obsolete. Simply do not include this header if you do not want save functions.
#endif

/*
extern void encrypt_text(char *toenc);
extern void write_string_encrypt(FILE *ooo, char *sss);
extern void write_dictionary (WordsDictionary *dict, FILE *writeto);
*/
