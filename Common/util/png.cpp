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
// LZW compression.
//
//=============================================================================
#include "util/png.h"
#include <stdlib.h>
#include "util/bbop.h"
#include "util/stream.h"
#include <algorithm>
#include <zlib.h>

using namespace AGS::Common;

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

int insert(int, int);
void _delete(int);

#define N 4096
#define F 16
#define THRESHOLD 3

#define dad (node+1)
#define lson (node+1+N)
#define rson (node+1+N+N)
#define root (node+1+N+N+N)
#define NIL -1

uint8_t* lzbuffer;
int* node;
int pos;
size_t outbytes = 0;

int insert(int i, int run)
{
    int c, j, k, l, n, match;
    int* p;

    c = NIL;

    k = l = 1;
    match = THRESHOLD - 1;
    p = &root[lzbuffer[i]];
    lson[i] = rson[i] = NIL;
    while ((j = *p) != NIL) {
        for (n = std::min(k, l); n < run && (c = (lzbuffer[j + n] - lzbuffer[i + n])) == 0; n++);

        if (n > match) {
            match = n;
            pos = j;
        }

        if (c < 0) {
            p = &lson[j];
            k = n;
        }
        else if (c > 0) {
            p = &rson[j];
            l = n;
        }
        else {
            dad[j] = NIL;
            dad[lson[j]] = lson + i - node;
            dad[rson[j]] = rson + i - node;
            lson[i] = lson[j];
            rson[i] = rson[j];
            break;
        }
    }

    dad[i] = p - node;
    *p = i;
    return match;
}

void _delete(int z)
{
    int j;

    if (dad[z] != NIL) {
        if (rson[z] == NIL)
            j = lson[z];
        else if (lson[z] == NIL)
            j = rson[z];
        else {
            j = lson[z];
            if (rson[j] != NIL) {
                do {
                    j = rson[j];
                } while (rson[j] != NIL);

                node[dad[j]] = lson[j];
                dad[lson[j]] = dad[j];
                lson[j] = lson[z];
                dad[lson[z]] = lson + j - node;
            }

            rson[j] = rson[z];
            dad[rson[z]] = rson + j - node;
        }

        dad[j] = dad[z];
        node[dad[z]] = j;
        dad[z] = NIL;
    }
}

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