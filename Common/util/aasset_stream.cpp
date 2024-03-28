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
        AAssetStream::AAssetStream(const String &asset_name, int asset_mode)
            : StreamBase()
        {
            AAsset *asset = OpenAAsset(asset_name, asset_mode);
            Open(asset, true, asset_name, asset_mode);
        }

        AAssetStream::AAssetStream(AAsset *aasset, bool own, int asset_mode)
            : StreamBase()
        {
            Open(aasset, own, "" /* none provided */, asset_mode);
        }

        AAssetStream::~AAssetStream()
        {
            if (_ownHandle)
                Close();
        }

        void AAssetStream::Close()
        {
            if (_aAsset)
            {
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
            return (_end - _cur_offset) <= 0;
        }

        soff_t AAssetStream::GetLength() const
        {
            return _end - _start;
        }

        soff_t AAssetStream::GetPosition() const
        {
            return _cur_offset - _start; // convert to formal offset range
        }

        size_t AAssetStream::Read(void *buffer, size_t size)
        {
            assert(_aAsset);
            size = std::min<size_t>(size, _end - _cur_offset);
            if (size == 0)
                return 0;
            auto read_size = AAsset_read(_aAsset, buffer, size);
            if (read_size < 0)
                return 0; // error
            _cur_offset += read_size;
            return read_size;
        }

        int32_t AAssetStream::ReadByte()
        {
            uint8_t ch;
            auto read_size = Read(&ch, 1);
            return (read_size == 1) ? ch : EOF;
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
            assert(_aAsset);
            int aa_origin;
            switch (origin)
            {
                case kSeekBegin:    aa_origin = SEEK_SET; break;
                case kSeekCurrent:  aa_origin = SEEK_CUR; break;
                case kSeekEnd:      aa_origin = SEEK_END; break;
                default: return -1;
            }

            off64_t new_off = AAsset_seek64(_aAsset, static_cast<off64_t>(offset), aa_origin);
            // if seek returns error (< 0), then the position must remain
            if (new_off < 0)
                return -1;
            _cur_offset = new_off;
            return _cur_offset - _start; // convert to formal offset range
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

    } // namespace Common
} // namespace AGS

#endif
