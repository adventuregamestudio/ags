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
//  Android Asset Stream to read from NDK AAssetManager in AGS
//
//=============================================================================
#ifndef __AGS_CN_UTIL__AASSET_STREAM_H
#define __AGS_CN_UTIL__AASSET_STREAM_H

#include "core/platform.h"

#if AGS_PLATFORM_OS_ANDROID
#include <android/asset_manager.h>
#include "util/bufferedstream.h"
#include "util/memory_compat.h"


namespace AGS
{
    namespace Common
    {

        class AAssetStream : public StreamBase
        {
        public:
            // Represents an open android asset object from asset manager
            // The constructor may raise std::runtime_error if
            // - there is an issue opening the asset
            AAssetStream(const String &asset_name, int asset_mode);

            // Constructs an asset stream over an open AAsset handle;
            // Take an ownership over it and will close upon disposal
            static std::unique_ptr<AAssetStream> OwnHandle(AAsset * aasset, int asset_mode)
            { return std::unique_ptr<AAssetStream>(new AAssetStream(aasset, true, asset_mode)); }
            // Constructs a asset stream over an open AAsset handle;
            // does NOT take an ownership over it
            static std::unique_ptr<AAssetStream> WrapHandle(AAsset * aasset, int asset_mode)
            { return std::unique_ptr<AAssetStream>(new AAssetStream(aasset, false, asset_mode)); }
            ~AAssetStream() override;

            // Tells the AASSET_MODE this stream is working in.
            int     GetAssetMode() const { return _assetMode; }
            // Tells which mode the stream is working in, which defines
            // supported io operations, such as reading, writing, seeking, etc.
            // Invalid streams return kStream_None to indicate that they are not functional.
            StreamMode GetMode() const override;
            // Is end of stream
            bool    EOS() const override;
            // Total length of stream (if known)
            soff_t  GetLength() const override;
            // Current position (if known)
            soff_t  GetPosition() const override;

            size_t  Read(void *buffer, size_t size) override;
            int32_t ReadByte() override;
            size_t  Write(const void *buffer, size_t size) override;
            int32_t WriteByte(uint8_t b) override;

            soff_t  Seek(soff_t offset, StreamSeek origin) override;

            bool    Flush() override;
            void    Close() override;

        private:
            AAssetStream(AAsset *aasset, bool own, int asset_mode);

            static AAsset *OpenAAsset(const String &asset_name, int asset_mode);
            void    Open(AAsset *asset, bool own, const String &asset_name, int asset_mode);

            bool    _ownHandle = false;
            AAsset *_aAsset = nullptr;
            int     _assetMode = AASSET_MODE_UNKNOWN;
            StreamMode _mode = kStream_None;
            // Remember range of offsets and current pos, reduce API calls
            soff_t  _start = 0;
            soff_t  _end = 0;
            soff_t  _cur_offset = 0;
        };


        // A helper class that creates a buffered stream over a AAssetStream object
        class BufferedAAssetStream : public BufferedStream
        {
        public:
            BufferedAAssetStream(const String &asset_name, int asset_mode)
                : BufferedStream(std::make_unique<AAssetStream>(asset_name, asset_mode)) {}
            BufferedAAssetStream(const String &asset_name, soff_t start_pos, soff_t end_pos,
                                 int asset_mode)
                : BufferedStream(std::make_unique<AAssetStream>(asset_name, asset_mode), start_pos, end_pos) {}
        };

    } // namespace Common
} // namespace AGS

#endif // AGS_PLATFORM_OS_ANDROID

#endif // __AGS_CN_UTIL__AASSET_STREAM_H
