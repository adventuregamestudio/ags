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

// [IKM] 2012-10-14:
// This is probably obsolete now, with the fixes made to script interpreter
#ifdef AGS_BIG_ENDIAN
#include <list>
#include <algorithm>

class Span
{
public:
    Span(char *low, unsigned int len) : mLo(low), mHi(low+len), mRC(1) {}
    ~Span() {}

    char *mLo;
    char *mHi;
    unsigned int mRC;

    bool operator < (Span const &rhs) const { return mHi <= rhs.mLo; }
    bool operator == (Span const &rhs) const { return mHi > rhs.mLo && mLo < rhs.mHi; }
    //bool operator > (Span const &rhs) const { return mLo >= rhs.mHi; }
};

class Spans
{
public:
    Spans() {}
    ~Spans() {}

    void AddSpan(Span const &span);
    void RemoveSpan(Span const &span);
    bool const IsInSpan(char *value) const;

    std::list<Span> mSpans;
};

void Spans::AddSpan(Span const &span)
{
    // lower bound is basically binary find insertion position
    std::list<Span>::iterator it = std::lower_bound(mSpans.begin(), mSpans.end(), span);
    if (*it == span)
    {
        (*it).mRC++;
    }
    else
    {
        mSpans.insert(it, span);
    }
}

void Spans::RemoveSpan(Span const &span)
{
    std::list<Span>::iterator it = std::lower_bound(mSpans.begin(), mSpans.end(), span);
    if (*it == span)
    {
        if (--((*it).mRC) <= 0)
        {
            mSpans.erase(it);
        }
    }
}

bool const Spans::IsInSpan(char *value) const
{
    Span const span = Span(value, 1);
    std::list<Span>::const_iterator it = std::lower_bound(mSpans.begin(), mSpans.end(), span);
    return (*it == span);
}

#endif // AGS_BIG_ENDIAN
