
//=============================================================================
//
// AGS logging system
//
//=============================================================================
#ifndef __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H
#define __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H

#include "debug/outputtarget.h"
#include "util/file.h"

namespace AGS
{
namespace Engine
{

namespace Out
{
    class CRawFileOutputTarget : public AGS::Common::Out::IOutputTarget
    {
    public:
        CRawFileOutputTarget(const char *sz_filepath);
        virtual ~CRawFileOutputTarget();

        virtual void Out(const char *sz_fullmsg);

    protected:
        bool OpenFile();
        void CloseFile();

    private:
        FILE *File;
        char *FilePath;
        bool DidWriteOnce;
    };

}   // namespace Out

}   // namespace Engine
}   // namespace AGS

#endif // __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H
