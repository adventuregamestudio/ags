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
#ifndef __AGS_CN_DEBUG__OUTPUTTARGET_H
#define __AGS_CN_DEBUG__OUTPUTTARGET_H

namespace AGS
{
namespace Common
{

namespace Out
{
    class IOutputTarget
    {
    public:
        virtual ~IOutputTarget() {}

        virtual void Out(const char *sz_fullmsg) = 0;
    };

}   // namespace out

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__OUTPUTTARGET_H
