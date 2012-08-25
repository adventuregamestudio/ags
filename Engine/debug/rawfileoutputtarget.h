
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

namespace out
{
    class CRawFileOutputTarget : public AGS::Common::out::IOutputTarget
    {
    public:
        CRawFileOutputTarget(const char *sz_filepath);
        virtual ~CRawFileOutputTarget();

        virtual void out(const char *sz_fullmsg);

    protected:
        bool OpenFile();
        void CloseFile();

    private:
        FILE *File;
        char *FilePath;
        bool DidWriteOnce;
    };

}   // namespace out

}   // namespace Engine
}   // namespace AGS

#endif // __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H
