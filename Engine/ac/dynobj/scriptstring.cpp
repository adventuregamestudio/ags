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
#include "ac/dynobj/scriptstring.h"
#include <stdlib.h>
#include <string.h>
#include "ac/string.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

ScriptString myScriptStringImpl;

const char *ScriptString::GetType()
{
    return "String";
}

int ScriptString::Dispose(void *address, bool /*force*/)
{
    delete[] (static_cast<uint8_t*>(address) - MemHeaderSz);
    return 1;
}

size_t ScriptString::CalcSerializeSize(const void *address)
{
    const Header &hdr = GetHeader(address);
    return hdr.Length + 1 + FileHeaderSz;
}

void ScriptString::Serialize(const void* address, Stream *out)
{
    const Header &hdr = GetHeader(address);
    out->WriteInt32(hdr.Length);
    out->Write(address, hdr.Length + 1); // it was writing trailing 0 for some reason
}

void ScriptString::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    size_t len = in->ReadInt32();
    uint8_t *buf = new uint8_t[len + 1 + MemHeaderSz];
    Header &hdr = reinterpret_cast<Header&>(*buf);
    hdr.Length = len;
    char *text_ptr = reinterpret_cast<char*>(buf + MemHeaderSz);
    in->Read(text_ptr, len + 1); // it was writing trailing 0 for some reason
    text_ptr[len] = 0; // for safety
    ccRegisterUnserializedObject(index, text_ptr, this);
}

DynObjectRef ScriptString::CreateObject(uint8_t *buf)
{
    char *text_ptr = reinterpret_cast<char*>(buf + MemHeaderSz);
    int32_t handle = ccRegisterManagedObject(text_ptr, &myScriptStringImpl);
    if (handle == 0)
    {
        delete[] buf;
        return DynObjectRef();
    }
    return DynObjectRef(handle, text_ptr, &myScriptStringImpl);
}

ScriptString::Buffer ScriptString::CreateBuffer(size_t data_sz)
{
    std::unique_ptr<uint8_t[]> buf(new uint8_t[data_sz + MemHeaderSz]);
    return Buffer(std::move(buf), data_sz + MemHeaderSz);
}

DynObjectRef ScriptString::Create(const char *text)
{
    size_t len = strlen(text);
    uint8_t *buf = new uint8_t[len + 1 + MemHeaderSz];
    char *text_ptr = reinterpret_cast<char*>(buf + MemHeaderSz);
    memcpy(buf, text, len + 1);
    Header &hdr = reinterpret_cast<Header&>(*buf);
    hdr.Length = len;
    return CreateObject(buf);
}

DynObjectRef ScriptString::Create(Buffer &&strbuf)
{
    uint8_t *buf = strbuf._buf.release();
    Header &hdr = reinterpret_cast<Header&>(*buf);
    hdr.Length = strbuf._sz - 1;
    strbuf._sz = 0u;
    return CreateObject(buf);
}
