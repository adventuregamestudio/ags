//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Utility helpers around STL containers.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STLUTILS_H
#define __AGS_CN_UTIL__STLUTILS_H

#include <queue>

namespace AGS
{
namespace Common
{

namespace StlUtil
{
    // Move elements from one queue to another
    template <typename T>
    void MoveQueue(std::queue<T> &dst, std::queue<T> &src)
    {
        for (; !src.empty(); src.pop())
            dst.push(src.front());
    }

    // A dirty solution for removing an element of queue from the middle;
    // to be used as a quick hack or temporary fix only.
    template <typename T>
    void EraseFromQueue(std::queue<T> &q, const T &elem)
    {
        std::queue<T> t;
        for (; !q.empty(); q.pop())
        {
            if (q.front() != elem)
                t.push(q.front());
        }
        MoveQueue(q, t);
    }

} // namespace STLUtil

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STLUTILS_H
