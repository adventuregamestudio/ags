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
// Class AlignedStream
// A simple wrapper around stream that controls data padding.
// 
// Originally, a number of objects in AGS were read and written directly
// as a data struct in a whole. In order to support backwards compatibility
// with games made by older versions of AGS, some of the game objects must
// be read having automatic data alignment in mind.
//-----------------------------------------------------------------------------
//
// AlignedStream uses the underlying stream, it overrides the reading and
// writing, and inserts extra data padding when needed.
//
// Aligned stream works either in read or write mode, it cannot be opened in
// combined mode.
//
// AlignedStream does not support seek, hence moving stream pointer to random
// position will break padding count logic.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__ALIGNEDSTREAM_H
#define __AGS_CN_UTIL__ALIGNEDSTREAM_H

#include "util/proxystream.h"

namespace AGS
{
    namespace Common
    {

        enum AlignedStreamMode
        {
            kAligned_Read,
            kAligned_Write
        };

        class AlignedStream : public ProxyStream
        {
        public:
            AlignedStream(Stream *stream, AlignedStreamMode mode,
                ObjectOwnershipPolicy stream_ownership_policy = kReleaseAfterUse,
                size_t base_alignment = sizeof(int16_t));
            virtual ~AlignedStream();

            // Read/Write cumulated padding and reset block counter
            void            Reset();

            virtual void    Close();

            virtual bool    CanRead() const;
            virtual bool    CanWrite() const;
            virtual bool    CanSeek() const;

            virtual size_t  Read(void *buffer, size_t size);
            virtual int32_t ReadByte();
            virtual int16_t ReadInt16();
            virtual int32_t ReadInt32();
            virtual int64_t ReadInt64();
            virtual size_t  ReadArray(void *buffer, size_t elem_size, size_t count);
            virtual size_t  ReadArrayOfInt16(int16_t *buffer, size_t count);
            virtual size_t  ReadArrayOfInt32(int32_t *buffer, size_t count);
            virtual size_t  ReadArrayOfInt64(int64_t *buffer, size_t count);

            virtual size_t  Write(const void *buffer, size_t size);
            virtual int32_t WriteByte(uint8_t b);
            virtual size_t  WriteInt16(int16_t val);
            virtual size_t  WriteInt32(int32_t val);
            virtual size_t  WriteInt64(int64_t val);
            virtual size_t  WriteArray(const void *buffer, size_t elem_size, size_t count);
            virtual size_t  WriteArrayOfInt16(const int16_t *buffer, size_t count);
            virtual size_t  WriteArrayOfInt32(const int32_t *buffer, size_t count);
            virtual size_t  WriteArrayOfInt64(const int64_t *buffer, size_t count);

            virtual size_t  Seek(StreamSeek seek, int pos);

        protected:
            void            ReadPadding(size_t next_type);
            void            WritePadding(size_t next_type);
            void            FinalizeBlock();

        private:
            static const size_t LargestPossibleType = sizeof(int64_t);

            AlignedStreamMode   _mode;
            size_t              _baseAlignment;
            size_t              _maxAlignment;
            int64_t             _block;
            int8_t              _paddingBuffer[sizeof(int64_t)];
        };

    } // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__ALIGNEDSTREAM_H
