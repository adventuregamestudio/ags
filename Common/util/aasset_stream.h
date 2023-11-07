//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//  Android Asset Stream to read from NDK AAssetManager in AGS
//
//=============================================================================
#ifndef __AGS_CN_UTIL__AASSET_STREAM_H
#define __AGS_CN_UTIL__AASSET_STREAM_H

#include "core/platform.h"

#if AGS_PLATFORM_OS_ANDROID
#include <android/asset_manager.h>

#include "util/datastream.h"
#include "util/file.h" // TODO: extract filestream mode constants

namespace AGS
{
    namespace Common
    {

        class AAssetStream : public DataStream
        {
        public:

            // Represents an open android asset object from asset manager
            // The constructor may raise std::runtime_error if
            // - there is an issue opening the asset
            AAssetStream(const String &asset_name, int asset_mode,
                       DataEndianess stream_endianess = kLittleEndian);

            // section of Android Asset Stream
            // The constructor may raise std::runtime_error if
            // - there is an issue opening or seeking the asset
            AAssetStream(const String &asset_name, int asset_mode,
                                soff_t start_pos, soff_t end_pos, DataEndianess stream_endianess = kLittleEndian);

            // Constructs an asset stream over an open AAsset handle;
            // Take an ownership over it and will close upon disposal
            static AAssetStream *OwnHandle(AAsset * aasset, int asset_mode, DataEndianess stream_end = kLittleEndian)
            { return new AAssetStream(aasset, true, asset_mode, stream_end); }
            // Constructs a asset stream over an open AAsset handle;
            // does NOT take an ownership over it
            static AAssetStream *WrapHandle(AAsset * aasset, int asset_mode, DataEndianess stream_end = kLittleEndian)
            { return new AAssetStream(aasset, false, asset_mode, stream_end); }
            ~AAssetStream() override;

            bool    HasErrors() const override;
            void    Close() override;
            bool    Flush() override;

            // Is stream valid (underlying data initialized properly)
            bool    IsValid() const override;
            // Is end of stream
            bool    EOS() const override;
            // Total length of stream (if known)
            soff_t  GetLength() const override;
            // Current position (if known)
            soff_t  GetPosition() const override;
            bool    CanRead() const override;
            bool    CanWrite() const override;
            bool    CanSeek() const override;

            size_t  Read(void *buffer, size_t size) override;
            int32_t ReadByte() override;
            size_t  Write(const void *buffer, size_t size) override;
            int32_t WriteByte(uint8_t b) override;

            bool    Seek(soff_t offset, StreamSeek origin) override;

        protected:
            soff_t _start = 0;
            soff_t _end = 0;
            soff_t _cur_offset = 0;


        private:
            AAssetStream(AAsset *aasset, bool own, int asset_mode, DataEndianess stream_end);
            void    Open(const String &asset_name, int asset_mode);
            void    OpenSection(const String &asset_name, int asset_mode, soff_t start_pos, soff_t end_pos);

            bool                 _ownHandle;
            AAsset                *_aAsset = nullptr;
        };

    } // namespace Common
} // namespace AGS

#endif // AGS_PLATFORM_OS_ANDROID

#endif // __AGS_CN_UTIL__AASSET_STREAM_H
