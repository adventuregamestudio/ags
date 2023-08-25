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
// PNG compression.
//
//=============================================================================
#include "util/png.h"
#include <stdlib.h>
#include "util/bbop.h"
#include "util/stream.h"
#include <algorithm>
#include <zlib.h>
#include <iostream>

using namespace AGS::Common;

bool pngcompress(Stream* input, Stream* output) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    int ret = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        std::cerr << "Error initializing compression" << std::endl;
        return false;
    }

    stream.data_type = Z_BINARY;

    char inbuf[1024];
    char outbuf[1024];

    stream.avail_in = input->Read(inbuf, sizeof(inbuf));
    while (stream.avail_in > 0) {
        stream.next_in = (Bytef*)inbuf;
        int flush = input->EOS() ? Z_FINISH : Z_NO_FLUSH;

        do {
            stream.next_out = (Bytef*)outbuf;
            stream.avail_out = sizeof(outbuf);
            ret = deflate(&stream, flush);
            assert(ret != Z_STREAM_ERROR);
            int have = sizeof(outbuf) - stream.avail_out;
            output->Write(outbuf, have);
        } while (stream.avail_out == 0);

        stream.avail_in = input->Read(inbuf, sizeof(inbuf));
    }

    (void)deflateEnd(&stream);

    return true;
}

bool pngexpand(const uint8_t* src, size_t src_sz, uint8_t* dst, size_t dst_sz) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    int ret = inflateInit(&stream);
    if (ret != Z_OK) {
        std::cerr << "Error initializing decompression" << std::endl;
        return false;
    }

    stream.next_in = (Bytef*)src;
    stream.avail_in = src_sz;
    stream.next_out = dst;
    stream.avail_out = dst_sz;

    do {
        ret = inflate(&stream, Z_FINISH);
        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
        case Z_BUF_ERROR:
            std::cerr << "Error decompressing data" << std::endl;
            (void)inflateEnd(&stream);
            return false;
        default:
            stream.next_out = dst + (dst_sz - stream.avail_out);
            break;
        }
    } while (stream.avail_out > 0);

    (void)inflateEnd(&stream);

    return true;
}