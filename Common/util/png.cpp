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
#include "zlib1213/zlib.h"

using namespace AGS::Common;

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

bool pngcompress(Stream* input, Stream* output) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    int ret = deflateInit(&stream, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        std::cerr << "Error initializing compression" << std::endl;
        return false;
    }

    char inbuf[1024];
    char outbuf[1024];

    input.Open();
    output.Open();

    stream.avail_in = input.Read(inbuf, sizeof(inbuf));

    while (stream.avail_in > 0) {
        stream.next_in = (Bytef*)inbuf;
        stream.avail_out = sizeof(outbuf);
        stream.next_out = (Bytef*)outbuf;

        ret = deflate(&stream, Z_NO_FLUSH);
        if (ret != Z_OK) {
            std::cerr << "Error compressing data" << std::endl;
            break;
        }

        int have = sizeof(outbuf) - stream.avail_out;
        output.Write(outbuf, have);

        stream.avail_in = input.Read(inbuf, sizeof(inbuf));
    }

    (void)deflateEnd(&stream);

    input.Close();
    output.Close();

    return true;
}

bool pngexpand(const uint8_t* src, size_t src_sz, uint8_t* dst, size_t dst_sz) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    int ret = deflateInit(&stream, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        std::cerr << "Error initializing compression" << std::endl;
        return false;
    }

    stream.next_in = src;
    stream.avail_in = src_sz;
    stream.next_out = dst;
    stream.avail_out = dst_sz;

    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        std::cerr << "Error compressing data" << std::endl;
        (void)deflateEnd(&stream);
        return false;
    }

    (void)deflateEnd(&stream);

    return true;
}