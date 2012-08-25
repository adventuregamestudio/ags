
//=============================================================================
//
// AGS logging system
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__OUTPUTTARGET_H
#define __AGS_CN_DEBUG__OUTPUTTARGET_H

namespace AGS
{
namespace Common
{

namespace out
{
    class IOutputTarget
    {
    public:
        virtual ~IOutputTarget() {}

        virtual void out(const char *sz_fullmsg) = 0;
    };

}   // namespace out

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__OUTPUTTARGET_H
