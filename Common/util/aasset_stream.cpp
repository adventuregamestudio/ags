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
#include "core/platform.h"
#if AGS_PLATFORM_OS_ANDROID
#include "util/aasset_stream.h"
#include <algorithm>
#include <stdexcept>
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "util/android_file.h"
#include "util/path.h"
#include "util/string.h"

namespace AGS
{
    namespace Common
    {
        AAssetStream::AAssetStream(const String &asset_name, int asset_mode, DataEndianess stream_endianess)
            : DataStream(stream_endianess)
        {
            AAsset *asset = OpenAAsset(asset_name, asset_mode);
            Open(asset, true, asset_name, asset_mode);
        }

        AAssetStream::AAssetStream(const String &asset_name, int asset_mode,
                                   soff_t start_pos, soff_t end_pos, DataEndianess stream_endianess)
            : DataStream(stream_endianess)
        {
            AAsset *asset = OpenAAsset(asset_name, asset_mode);
            OpenSection(asset, true, asset_name, asset_mode, start_pos, end_pos);
        }

        AAssetStream::AAssetStream(AAsset *aasset, bool own, int asset_mode, DataEndianess stream_end)
            : DataStream(stream_end)
        {
            Open(aasset, own, "" /* none provided */, asset_mode);
        }

        AAssetStream::~AAssetStream()
        {
            if(_ownHandle) Close();
        }

        void AAssetStream::Close()
        {
            if(_aAsset) {
                AAsset_close(_aAsset);
                _aAsset = nullptr;
            }
            _mode = kStream_None;
        }

        bool AAssetStream::Flush()
        {
            return false;
        }

        StreamMode AAssetStream::GetMode() const
        {
            return _mode;
        }

        bool AAssetStream::EOS() const
        {
            return !IsValid() || _end - _cur_offset <= 0;
        }

        soff_t AAssetStream::GetLength() const
        {
            if(IsValid())
            {
                return _end - _start;
            }
            return 0;
        }

        soff_t AAssetStream::GetPosition() const
        {
            if(IsValid())
            {
                return _cur_offset - _start;
            }
            return -1;
        }

        size_t AAssetStream::Read(void *buffer, size_t size)
        {
            if(!IsValid()) return 0;

            size = std::min<size_t>(size, _end - _cur_offset);
            if(size == 0) return 0;
            auto read_size = AAsset_read(_aAsset,buffer,size);
            if(read_size > 0) {
                _cur_offset += read_size;
            }
            return read_size;
        }

        int32_t AAssetStream::ReadByte()
        {
            uint8_t ch;
            auto bytesRead = Read(&ch, 1);
            if (bytesRead != 1) { return EOF; }
            return ch;
        }

        size_t AAssetStream::Write(const void *buffer, size_t size)
        {
            return 0;
        }

        int32_t AAssetStream::WriteByte(uint8_t val)
        {
            return -1;
        }

        soff_t AAssetStream::Seek(soff_t offset, StreamSeek origin)
        {
            if (!IsValid())
            {
                return -1;
            }

            soff_t want_pos = -1;
            switch (origin)
            {
                case kSeekBegin:    want_pos = _start      + offset; break;
                case kSeekCurrent:  want_pos = _cur_offset + offset; break;
                case kSeekEnd:      want_pos = _end        + offset; break;
                default: return -1;
            }

            // clamp to a valid range
            want_pos = std::min(std::max(want_pos, _start), _end);
            off64_t new_off = AAsset_seek64(_aAsset, static_cast<off64_t>(want_pos), SEEK_SET);
            // if seek returns error (< 0), then the position must remain
            if (new_off >= 0)
                _cur_offset = new_off;
            return _cur_offset - _start; // convert to a stream section pos
        }

        AAsset *AAssetStream::OpenAAsset(const String &asset_name, int asset_mode)
        {
            AAssetManager* aAssetManager = GetAAssetManager();
            if(aAssetManager == nullptr)
                throw std::runtime_error("Couldn't get AAssetManager, SDL not initialized yet.");

            String a_asset_name = Path::GetPathInForeignAsset(asset_name);
            AAsset *asset = AAssetManager_open(aAssetManager, a_asset_name.GetCStr(), asset_mode);
            if (asset == nullptr)
                throw std::runtime_error("Error opening file.");
            return asset;
        }

        void AAssetStream::Open(AAsset *asset, bool own, const String &asset_name, int asset_mode)
        {
            _aAsset = asset;
            _ownHandle = own;
            _cur_offset = 0;
            _start = 0;
            _end = AAsset_getLength64(_aAsset);
            _assetMode = asset_mode;
            _mode = static_cast<StreamMode>(kStream_Read | kStream_Seek);
            _path = asset_name;
        }

        void AAssetStream::OpenSection(AAsset *asset, bool own, const String &asset_name, int asset_mode,
                                       soff_t start_pos, soff_t end_pos)
        {
            Open(asset, own, asset_name, asset_mode);

            // clamp requested range to the predetermined full stream range
            assert(start_pos <= end_pos);
            end_pos = std::max(_start, std::min(_end, end_pos));
            start_pos = std::max(_start, std::min(start_pos, end_pos));

            if (Seek(start_pos, kSeekBegin) < 0)
            {
                Close();
                throw std::runtime_error("Error setting stream section.");
            }

            _start = start_pos;
            _end = end_pos;
        }

    } // namespace Common
} // namespace AGS

#endif