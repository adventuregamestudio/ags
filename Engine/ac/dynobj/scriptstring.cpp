//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/scriptstring.h"
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
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
    char *text_ptr = reinterpret_cast<char*>(buf + MemHeaderSz);
    in->Read(text_ptr, len + 1); // it was writing trailing 0 for some reason
    text_ptr[len] = 0; // for safety
    Header &hdr = reinterpret_cast<Header&>(*buf);
    hdr.Length = len;
    hdr.ULength = ustrlen(text_ptr);
    hdr.LastCharIdx = 0u;
    hdr.LastCharOff = 0u;
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

ScriptString::Buffer ScriptString::CreateBuffer(size_t len, size_t ulen)
{
    assert(ulen <= len);
    std::unique_ptr<uint8_t[]> buf(new uint8_t[len + 1 + MemHeaderSz]);
    auto *header = reinterpret_cast<Header*>(buf.get());
    header->Length = len;
    header->ULength = ulen;
    header->LastCharIdx = 0;
    header->LastCharOff = 0;
    return Buffer(std::move(buf), len + 1 + MemHeaderSz);
}

DynObjectRef ScriptString::Create(const char *text)
{
    int len, ulen;
    ustrlen2(text, &len, &ulen);
    auto buf = CreateBuffer(len, ulen);
    memcpy(buf.Get(), text, len + 1);
    return CreateObject(buf._buf.release());
}

DynObjectRef ScriptString::Create(Buffer &&strbuf)
{
    uint8_t *buf = strbuf._buf.release();
    auto *header = reinterpret_cast<Header*>(buf);
    char *text_ptr = reinterpret_cast<char*>(buf + MemHeaderSz);
    if ((header->Length > 0) && (header->ULength == 0u))
        header->ULength = ustrlen(text_ptr);
    text_ptr[header->Length] = 0; // for safety
    return CreateObject(buf);
}
