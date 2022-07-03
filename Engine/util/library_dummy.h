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
#ifndef __AGS_EE_UTIL__LIBRARY_DUMMY_H
#define __AGS_EE_UTIL__LIBRARY_DUMMY_H

namespace AGS
{
namespace Engine
{

using AGS::Common::String;

class DummyLibrary : public BaseLibrary
{
public:
    DummyLibrary() = default;
    ~DummyLibrary() override { /* do nothing */ };

    String GetFilenameForLib(const String &libname) override
    {
        return ""; // not supported
    }

    bool Load(const String &libname) override
    {
        return false; // always fail
    }

    void Unload() override { /* do nothing */ }

    bool IsLoaded() const override { return false; /* always fail */ }

    void *GetFunctionAddress(const String &fn_name) override
    {
        return nullptr; // not supported
    }
};


typedef DummyLibrary Library;


} // namespace Engine
} // namespace AGS



#endif // __AGS_EE_UTIL__LIBRARY_DUMMY_H
