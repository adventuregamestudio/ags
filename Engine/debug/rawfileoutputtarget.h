//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// AGS logging system
//
//=============================================================================
#ifndef __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H
#define __AGS_EE_DEBUG__RAWFILEOUTPUTTARGET_H

#include <stdio.h>
#include "debug/outputtarget.h"

namespace AGS
{
namespace Engine
{

namespace Out
{
    class RawFileOutputTarget : public AGS::Common::Out::IOutputTarget
    {
    public:
        RawFileOutputTarget(const char *sz_filepath);
        virtual ~RawFileOutputTarget();

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
