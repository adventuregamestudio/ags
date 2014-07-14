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
// Helper math functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MATH_H
#define __AGS_CN_UTIL__MATH_H

namespace AGS
{
    namespace Common
    {

        namespace Math
        {
            inline int Max(int a, int b)
            {
                return a > b ? a : b;
            }

            inline int Min(int a, int b)
            {
                return a > b ? b : a;
            }

            inline void Clamp(int floor, int ceil, int &val)
            {
                val = Min(floor, Max(val, ceil));
            }

            inline void ClampLength(int floor, int height, int &from, int &length)
            {
                if (from < floor)
                {
                    length -= floor - from;
                    from = floor;
                }
                else if (from >= floor + height)
                {
                    from = 0;
                    length = 0;
                }

                length = Max(length, 0);
                length = Min(length, height - from);
            }


        } // namespace Math

    } // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MATH_H
