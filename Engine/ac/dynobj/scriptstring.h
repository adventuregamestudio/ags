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
#ifndef __AC_SCRIPTSTRING_H
#define __AC_SCRIPTSTRING_H

#include <memory>
#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptString final : AGSCCDynamicObject
{
public:
    struct Header
    {
        uint32_t Length = 0u;  // string length in bytes (not counting 0)
        uint32_t ULength = 0u; // Unicode compatible length in characters
        // Saved last requested character index and buffer offset;
        // significantly speeds up Unicode string iteration, but adds 4 bytes
        // per ScriptString object. Replace with a proper str iterator later!
        // NOTE: intentionally limited to 64k chars/bytes to save bit of mem.
        uint16_t LastCharIdx = 0u;
        uint16_t LastCharOff = 0u;
    };

    struct Buffer
    {
        friend ScriptString;
    public:
        Buffer() = default;
        ~Buffer() = default;
        Buffer(Buffer &&buf) = default;
        // Returns a pointer to the beginning of a text buffer
        char *Get() { return reinterpret_cast<char*>(_buf.get() + MemHeaderSz); }
        // Returns size allocated for a text content (includes null pointer)
        size_t GetSize() const { return _sz - MemHeaderSz; }

    private:
        Buffer(std::unique_ptr<uint8_t[]> &&buf, size_t buf_sz)
            : _buf(std::move(buf)), _sz(buf_sz) {}

        std::unique_ptr<uint8_t[]> _buf;
        size_t _sz;
    };


    ScriptString() = default;
    ~ScriptString() = default;

    inline static const Header &GetHeader(const void *address)
    {
        return reinterpret_cast<const Header&>(*(static_cast<const uint8_t*>(address) - MemHeaderSz));
    }

    inline static Header &GetHeader(void *address)
    {
        return reinterpret_cast<Header&>(*(static_cast<uint8_t*>(address) - MemHeaderSz));
    }

    // Allocates a ScriptString-compatible buffer large enough to accomodate
    // given length in bytes (len). This buffer may be filled by the caller
    // and then passed into Create(). If ulen is left eq 0, then it will be
    // recalculated on script string's creation.
    static Buffer CreateBuffer(size_t len, size_t ulen = 0u);
    // Create a new script string by copying the given text
    static DynObjectRef Create(const char *text);
    // Create a new script string by taking ownership over the given buffer;
    // passed buffer variable becomes invalid after this call.
    static DynObjectRef Create(Buffer &&strbuf);

    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    friend ScriptString::Buffer;
    // The size of the array's header in memory, prepended to the element data
    static const size_t MemHeaderSz = sizeof(Header);
    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t);

    static DynObjectRef CreateObject(uint8_t *buf);

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

extern ScriptString myScriptStringImpl;

#endif // __AC_SCRIPTSTRING_H
