
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

namespace out
{
    class CConsoleOutputTarget : public AGS::Common::out::IOutputTarget
    {
    public:
        CConsoleOutputTarget();
        virtual ~CConsoleOutputTarget();

        virtual void out(const char *sz_fullmsg);

    protected:

    private:
    };

}   // namespace out

}   // namespace Engine
}   // namespace AGS

#endif // __AGS_EE_DEBUG__CONSOLEOUTPUTTARGET_H
