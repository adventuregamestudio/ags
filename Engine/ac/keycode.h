
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__KEYCODE_H
#define __AGS_EE_AC__KEYCODE_H

#if defined(MAC_VERSION) && !defined(IOS_VERSION)
#define EXTENDED_KEY_CODE 0x3f
#else
#define EXTENDED_KEY_CODE 0
#endif

#define AGS_KEYCODE_INSERT 382
#define AGS_KEYCODE_DELETE 383
#define AGS_KEYCODE_ALT_TAB 399
#define READKEY_CODE_ALT_TAB 0x4000

#endif // __AGS_EE_AC__KEYCODE_H
