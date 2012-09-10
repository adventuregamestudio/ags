
//=============================================================================
//
// Debug assertion tools
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__ASSERT_H
#define __AGS_CN_DEBUG__ASSERT_H

#ifdef _DEBUG

// TODO: revise this later (add platform-specific output maybe?)
inline void assert(bool expr)
{
    if (!expr) {
        _asm {
            int 3
        }
    }
}

inline void assert(bool expr, const char *err_msg)
{
    if (!expr) {
        _asm {
            int 3
        }
    }
}

#else // !_DEBUG

#define assert(a)
#define assert(a,b)

#endif // !_DEBUG

#endif // __AGS_CN_DEBUG__ASSERT_H
