
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

        length = Min(length, height);
    }


} // namespace Math

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MATH_H
