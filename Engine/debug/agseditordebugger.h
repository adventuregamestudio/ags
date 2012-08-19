
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DEBUG__AGSEDITORDEBUGGER_H
#define __AGS_EE_DEBUG__AGSEDITORDEBUGGER_H

#include "util/string.h"
using namespace AGS; // FIXME later

#define DBG_NOIFACE       1
#define DBG_NODRAWSPRITES 2
#define DBG_NOOBJECTS     4
#define DBG_NOUPDATE      8
#define DBG_NOSFX      0x10
#define DBG_NOMUSIC    0x20
#define DBG_NOSCRIPT   0x40
#define DBG_DBGSCRIPT  0x80
#define DBG_DEBUGMODE 0x100
#define DBG_REGONLY   0x200
#define DBG_NOVIDEO   0x400

struct IAGSEditorDebugger
{
public:
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool SendMessageToEditor(const Common::CString &message) = 0;
    virtual bool IsMessageAvailable() = 0;
    // Message will be allocated on heap with malloc
    virtual Common::CString GetNextMessage() = 0;
};

#endif // __AGS_EE_DEBUG__AGSEDITORDEBUGGER_H
