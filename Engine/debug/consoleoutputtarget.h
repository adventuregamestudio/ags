
//=============================================================================
//
// AGS logging system
//
//=============================================================================
#ifndef __AGS_EE_DEBUG__CONSOLEOUTPUTTARGET_H
#define __AGS_EE_DEBUG__CONSOLEOUTPUTTARGET_H

#include "debug/outputtarget.h"

namespace AGS
{
namespace Engine
{

namespace Out
{
    class CConsoleOutputTarget : public AGS::Common::Out::IOutputTarget
    {
    public:
        CConsoleOutputTarget();
        virtual ~CConsoleOutputTarget();

        virtual void Out(const char *sz_fullmsg);

    protected:

    private:
    };

}   // namespace Out

}   // namespace Engine
}   // namespace AGS

#endif // __AGS_EE_DEBUG__CONSOLEOUTPUTTARGET_H
