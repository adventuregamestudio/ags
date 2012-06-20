
//=============================================================================
//
// Internal definitions for ex_* units
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__EX_DEFINES_H
#define __AGS_EE_SCRIPT__EX_DEFINES_H

// defined in cs_runtime
extern void ccAddExternalSymbol(char *namof, void *addrof);

#define scAdd_External_Symbol ccAddExternalSymbol

#endif // __AGS_EE_SCRIPT__EX_DEFINES_H
