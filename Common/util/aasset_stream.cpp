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
                , _aAsset(nullptr)
        {
            Open(asset_name, asset_mode);
        }

        AAssetStream::AAssetStream(const String &asset_name, int asset_mode,
                                   soff_t start_pos, soff_t end_pos, DataEndianess stream_endianess)
                : DataStream(stream_endianess)
                , _aAsset(nullptr)
        {
            OpenSection(asset_name, asset_mode, start_pos, end_pos);
        }

        AAssetStream::AAssetStream(AAsset * aasset, bool own, int asset_mode, DataEndianess stream_end)
                : DataStream(stream_end)
                , _aAsset(nullptr)
                , _ownHandle(own)
        {
        }

        AAssetStream::~AAssetStream()
        {
            if(_ownHandle) Close();
        }

        bool AAssetStream::HasErrors() const
        {
            return !IsValid();
        }

        void AAssetStream::Close()
        {
            if(_aAsset) {
                AAsset_close(_aAsset);
                _aAsset = nullptr;
            }
        }

        bool AAssetStream::Flush()
        {
            return false;
        }

        bool AAssetStream::IsValid() const
        {
            return _aAsset != nullptr;
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

        bool AAssetStream::CanRead() const
        {
            return IsValid();
        }

        bool AAssetStream::CanWrite() const
        {
            return false;
        }

        bool AAssetStream::CanSeek() const
        {
            return IsValid();
        }

        size_t AAssetStream::Read(void *buffer, size_t size)
        {
            if(!IsValid()) return -1;

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
            return -1;
        }

        int32_t AAssetStream::WriteByte(uint8_t val)
        {
            return -1;
        }

        bool AAssetStream::Seek(soff_t offset, StreamSeek origin)
        {
            if (!IsValid())
            {
                return false;
            }

            soff_t want_pos = -1;
            switch (origin)
            {
                case kSeekBegin:    want_pos = _start      + offset; break;
                case kSeekCurrent:  want_pos = _cur_offset + offset; break;
                case kSeekEnd:      want_pos = _end        + offset; break;
                default:
                    // TODO: warning to the log
                    return false;
            }

            _cur_offset = std::min(std::max(want_pos, (soff_t)_start), _end);
            off64_t off = AAsset_seek64(_aAsset, (off64_t)_cur_offset, SEEK_SET);
            return off != (off64_t) -1;
        }


        void AAssetStream::Open(const String &asset_name, int asset_mode)
        {
            AAssetManager* aAssetManager = GetAAssetManager();
            if(aAssetManager == nullptr)
                throw std::runtime_error("Couldn't get AAssetManager, SDL not initialized yet.");
            _ownHandle = true;

            String a_asset_name = Path::GetPathInForeignAsset(asset_name);

            _aAsset = AAssetManager_open(aAssetManager, a_asset_name.GetCStr(), asset_mode);

            if (_aAsset == nullptr)
                throw std::runtime_error("Error opening file.");

            _cur_offset = 0;
            _start = 0;
            _end = AAsset_getLength64(_aAsset);
        }


        void AAssetStream::OpenSection(const String &asset_name, int asset_mode,
                                                 soff_t start_pos, soff_t end_pos)
        {
            Open(asset_name, asset_mode);
            assert(start_pos <= end_pos);
            start_pos = std::min(start_pos, end_pos);

            if (!Seek(start_pos, kSeekBegin)) {
                Close();
                throw std::runtime_error("Error determining stream end.");
            }

            _start = std::min(start_pos, _end);
            _end = std::min(end_pos, _end);
        }

    } // namespace Common
} // namespace AGS

#endif