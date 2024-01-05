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
//
// LZW compression.
//
//=============================================================================
#include "util/lzw.h"
#include <stdlib.h>
#include "util/bbop.h"
#include "util/stream.h"

using namespace AGS::Common;

int insert(int, int);
void _delete(int);

#define N 4096
#define F 16
#define THRESHOLD 3
#define min(xx,yy) ((yy<xx) ? yy : xx)

#define dad (node+1)
#define lson (node+1+N)
#define rson (node+1+N+N)
#define root (node+1+N+N+N)
#define NIL -1

uint8_t *lzbuffer;
int *node;
int pos;
size_t outbytes = 0;

int insert(int i, int run)
{
  int c, j, k, l, n, match;
  int *p;

  c = NIL;

  k = l = 1;
  match = THRESHOLD - 1;
  p = &root[lzbuffer[i]];
  lson[i] = rson[i] = NIL;
  while ((j = *p) != NIL) {
    for (n = min(k, l); n < run && (c = (lzbuffer[j + n] - lzbuffer[i + n])) == 0; n++) ;

    if (n > match) {
      match = n;
      pos = j;
    }

    if (c < 0) {
      p = &lson[j];
      k = n;
    } else if (c > 0) {
      p = &rson[j];
      l = n;
    } else {
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

bool lzwcompress(Stream *lzw_in, Stream *out)
{
  int ch, i, run, len, match, size, mask;
  uint8_t buf[17];

  lzbuffer = (uint8_t *)malloc(N + F + (N + 1 + N + N + 256) * sizeof(int));       // 28.5 k !
  if (lzbuffer == nullptr) {
    return false;
  }

  node = (int *)(lzbuffer + N + F);
  for (i = 0; i < 256; i++)
    root[i] = NIL;

  for (i = NIL; i < N; i++)
    dad[i] = NIL;

  size = mask = 1;
  buf[0] = 0;
  i = N - F - F;

  for (len = 0; len < F && (ch = lzw_in->ReadByte()) != -1; len++) {
    lzbuffer[i + F] = static_cast<uint8_t>(ch);
    i = (i + 1) & (N - 1);
  }

  run = len;

  do {
    ch = lzw_in->ReadByte();
    if (i >= N - F) {
      _delete(i + F - N);
      lzbuffer[i + F] = lzbuffer[i + F - N] = static_cast<uint8_t>(ch);
    } else {
      _delete(i + F);
      lzbuffer[i + F] = static_cast<uint8_t>(ch);
    }

    match = insert(i, run);
    if (ch == -1) {
      run--;
      len--;
    }

    if (len++ >= run) {
      if (match >= THRESHOLD) {
        buf[0] |= mask;
        // possible fix: change int* to short* ??
        *(short *)(buf + size) = static_cast<short>(((match - 3) << 12) | ((i - pos - 1) & (N - 1)));
        size += 2;
        len -= match;
      } else {
        buf[size++] = lzbuffer[i];
        len--;
      }

      if (!((mask += mask) & 0xFF)) {
        out->Write(buf, size);
        outbytes += size;
        size = mask = 1;
        buf[0] = 0;
      }
    }
    i = (i + 1) & (N - 1);
  } while (len > 0);

  if (size > 1) {
    out->Write(buf, size);
    outbytes += size;
  }

  free(lzbuffer);
  return true;
}

bool lzwexpand(const uint8_t *src, size_t src_sz, uint8_t *dst, size_t dst_sz)
{
  int bits, ch, i, j, len, mask;
  uint8_t *dst_ptr = dst;
  const uint8_t *src_ptr = src;

  if (dst_sz == 0)
    return false; // nowhere to expand to

  lzbuffer = (uint8_t *)malloc(N);
  if (lzbuffer == nullptr) {
    return false; // not enough memory
  }
  i = N - F;

  // Read from the src and expand, until either src or dst runs out of space
  while ((static_cast<size_t>(src_ptr - src) < src_sz) &&
         (static_cast<size_t>(dst_ptr - dst) < dst_sz)) {
    bits = *(src_ptr++);
    for (mask = 0x01; mask & 0xFF; mask <<= 1) {
      if (bits & mask) {
        if (static_cast<size_t>(src_ptr - src) > (src_sz - sizeof(int16_t)))
          break;

        short jshort = 0;
        jshort = BBOp::Int16FromLE(*(reinterpret_cast<const int16_t*>(src_ptr)));
        src_ptr += sizeof(int16_t);
        j = jshort;

        len = ((j >> 12) & 15) + 3;
        j = (i - j - 1) & (N - 1);

        if (static_cast<size_t>(dst_ptr - dst) > (dst_sz - len))
          break; // not enough dest buffer

        while (len--) {
          *(dst_ptr++) = (lzbuffer[i] = lzbuffer[j]);
          j = (j + 1) & (N - 1);
          i = (i + 1) & (N - 1);
        }
      } else {
        ch = *(src_ptr++);
        *(dst_ptr++) = (lzbuffer[i] = static_cast<uint8_t>(ch));
        i = (i + 1) & (N - 1);
      }

      if ((static_cast<size_t>(dst_ptr - dst) >= dst_sz) ||
          (static_cast<size_t>(src_ptr - src) >= src_sz)) {
        break; // not enough dest buffer for the next pass
      }
    } // end for mask
  }

  free(lzbuffer);
  return (src_ptr - src) == src_sz;
}
