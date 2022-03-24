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
#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

#include "util/compress.h"
#include <stdlib.h>
#include <stdio.h>
#include "ac/common.h"	// quit, update_polled_stuff
#include "gfx/bitmap.h"
#include "util/lzw.h"
#include "util/memorystream.h"
#if AGS_PLATFORM_ENDIAN_BIG
#include "util/bbop.h"
#endif

using namespace AGS::Common;

//-----------------------------------------------------------------------------
// RLE
//-----------------------------------------------------------------------------

static void cpackbitl(const uint8_t *line, size_t size, Stream *out)
{
  size_t cnt = 0;               // bytes encoded

  while (cnt < size) {
    // IMPORTANT: the algorithm below requires signed operations
    int i = static_cast<int32_t>(cnt);
    int j = i + 1;
    int jmax = i + 126;
    if (static_cast<uint32_t>(jmax) >= size)
      jmax = size - 1;

    if (static_cast<uint32_t>(i) == size - 1) { //......last byte alone
      out->WriteInt8(0);
      out->WriteInt8(line[i]);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      out->WriteInt8(i - j);
      out->WriteInt8(line[i]);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      out->WriteInt8(j - i);
      out->Write(line + i, j - i + 1);
      cnt += j - i + 1;

    }
  } // end while
}

static void cpackbitl16(const uint16_t *line, size_t size, Stream *out)
{
  size_t cnt = 0;               // bytes encoded

  while (cnt < size) {
    // IMPORTANT: the algorithm below requires signed operations
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (static_cast<uint32_t>(jmax) >= size)
      jmax = size - 1;

    if (static_cast<uint32_t>(i) == size - 1) { //......last byte alone
      out->WriteInt8(0);
      out->WriteInt16(line[i]);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      out->WriteInt8(i - j);
      out->WriteInt16(line[i]);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      out->WriteInt8(j - i);
      out->WriteArray(line + i, j - i + 1, 2);
      cnt += j - i + 1;

    }
  } // end while
}

static void cpackbitl32(const uint32_t *line, size_t size, Stream *out)
{
  size_t cnt = 0;               // bytes encoded

  while (cnt < size) {
    // IMPORTANT: the algorithm below requires signed operations
    int i = cnt;
    int j = i + 1;
    int jmax = i + 126;
    if (static_cast<uint32_t>(jmax) >= size)
      jmax = size - 1;

    if (static_cast<uint32_t>(i) == size - 1) { //......last byte alone
      out->WriteInt8(0);
      out->WriteInt32(line[i]);
      cnt++;

    } else if (line[i] == line[j]) {    //....run
      while ((j < jmax) && (line[j] == line[j + 1]))
        j++;
     
      out->WriteInt8(i - j);
      out->WriteInt32(line[i]);
      cnt += j - i + 1;

    } else {                    //.............................sequence
      while ((j < jmax) && (line[j] != line[j + 1]))
        j++;

      out->WriteInt8(j - i);
      out->WriteArray(line + i, j - i + 1, 4);
      cnt += j - i + 1;

    }
  } // end while
}

static int cunpackbitl(uint8_t *line, size_t size, Stream *in)
{
  size_t n = 0;                  // number of bytes decoded

  while (n < size) {
    int ix = in->ReadByte();     // get index byte
    if (in->HasErrors())
      break;

    signed char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      char ch = in->ReadInt8();
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = ch;
      }
    } else {                     //.....................seq
      int i = cx + 1;
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = in->ReadByte();
      }
    }
  }

  return in->HasErrors() ? -1 : 0;
}

static int cunpackbitl16(uint16_t *line, size_t size, Stream *in)
{
  size_t n = 0;                  // number of bytes decoded

  while (n < size) {
    int ix = in->ReadByte();     // get index byte
    if (in->HasErrors())
      break;

    signed char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      unsigned short ch = in->ReadInt16();
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = ch;
      }
    } else {                     //.....................seq
      int i = cx + 1;
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = in->ReadInt16();
      }
    }
  }

  return in->HasErrors() ? -1 : 0;
}

static int cunpackbitl32(uint32_t *line, size_t size, Stream *in)
{
  size_t n = 0;                  // number of bytes decoded

  while (n < size) {
    int ix = in->ReadByte();     // get index byte
    if (in->HasErrors())
      break;

    signed char cx = ix;
    if (cx == -128)
      cx = 0;

    if (cx < 0) {                //.............run
      int i = 1 - cx;
      unsigned int ch = in->ReadInt32();
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = ch;
      }
    } else {                     //.....................seq
      int i = cx + 1;
      while (i--) {
        // test for buffer overflow
        if (n >= size)
          return -1;

        line[n++] = (unsigned int)in->ReadInt32();
      }
    }
  }

  return in->HasErrors() ? -1 : 0;
}

void rle_compress(const uint8_t *data, size_t data_sz, int image_bpp, Stream *out)
{
    switch (image_bpp)
    {
    case 1: cpackbitl(data, data_sz, out); break;
    case 2: cpackbitl16(reinterpret_cast<const uint16_t*>(data), data_sz / sizeof(uint16_t), out); break;
    case 4: cpackbitl32(reinterpret_cast<const uint32_t*>(data), data_sz / sizeof(uint32_t), out); break;
    default: assert(0); break;
    }
}

void rle_decompress(uint8_t *data, size_t data_sz, int image_bpp, Stream *in)
{
    switch (image_bpp)
    {
    case 1: cunpackbitl(data, data_sz, in); break;
    case 2: cunpackbitl16(reinterpret_cast<uint16_t*>(data), data_sz / sizeof(uint16_t), in); break;
    case 4: cunpackbitl32(reinterpret_cast<uint32_t*>(data), data_sz / sizeof(uint32_t), in); break;
    default: assert(0); break;
    }
}

void save_rle_bitmap8(Stream *out, const Bitmap *bmp, const RGB (*pal)[256])
{
    assert(bmp->GetBPP() == 1);
    out->WriteInt16(static_cast<uint16_t>(bmp->GetWidth()));
    out->WriteInt16(static_cast<uint16_t>(bmp->GetHeight()));
    // Pack the pixels
    cpackbitl(bmp->GetData(), bmp->GetWidth() * bmp->GetHeight(), out);
    // Save palette
    if (!pal)
    { // if no pal, write dummy palette, because we have to
        out->WriteByteCount(0, 256 * 3);
        return;
    }
    const RGB *ppal = *pal;
    for (int i = 0; i < 256; ++i)
    {
        out->WriteInt8(ppal[i].r);
        out->WriteInt8(ppal[i].g);
        out->WriteInt8(ppal[i].b);
    }
}

std::unique_ptr<Bitmap> load_rle_bitmap8(Stream *in, RGB (*pal)[256])
{
    int w = in->ReadInt16();
    int h = in->ReadInt16();
    std::unique_ptr<Bitmap> bmp(BitmapHelper::CreateBitmap(w, h, 8));
    if (!bmp) return nullptr;
    // Unpack the pixels
    cunpackbitl(bmp->GetDataForWriting(), w * h, in);
    // Load or skip the palette
    if (!pal)
    {
        in->Seek(3 * 256);
        return bmp;
    }
    RGB *ppal = *pal;
    for (int i = 0; i < 256; ++i)
    {
        ppal[i].r = in->ReadInt8();
        ppal[i].g = in->ReadInt8();
        ppal[i].b = in->ReadInt8();
    }
    return bmp;
}

void skip_rle_bitmap8(Common::Stream *in)
{
    int w = in->ReadInt16();
    int h = in->ReadInt16();
    // Skip 8-bit pixel data + RGB palette
    in->Seek((w * h) + (3 * 256));
}


//-----------------------------------------------------------------------------
// LZW
//-----------------------------------------------------------------------------

void lzw_compress(const uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *out)
{
    // LZW algorithm that we use fails on sequence less than 16 bytes.
    if (data_sz < 16)
    {
        out->Write(data, data_sz);
        return;
    }
    MemoryStream mem_in(data, data_sz);
    lzwcompress(&mem_in, out);
}

void lzw_decompress(uint8_t *data, size_t data_sz, int image_bpp, Common::Stream *in)
{
    // LZW algorithm that we use fails on sequence less than 16 bytes.
    if (data_sz < 16)
    {
        in->Read(data, data_sz);
        return;
    }
    MemoryStream ms(data, data_sz, kStream_Write);
    lzwexpand(in, &ms, data_sz);
}

void save_lzw(Stream *out, const Bitmap *bmpp, const RGB (*pal)[256])
{
  // First write original bitmap's info and data into the memory buffer
  // NOTE: we must do this purely for backward compatibility with old room formats:
  // because they also included bmp width and height into compressed data!
  std::vector<uint8_t> membuf;
  {
    VectorStream memws(membuf, kStream_Write);
    int w = bmpp->GetWidth(), h = bmpp->GetHeight(), bpp = bmpp->GetBPP();
    memws.WriteInt32(w * bpp); // stride
    memws.WriteInt32(h);
    switch (bpp)
    {
    case 1: memws.Write(bmpp->GetData(), w * h * bpp); break;
    case 2: memws.WriteArrayOfInt16(reinterpret_cast<const int16_t*>(bmpp->GetData()), w * h); break;
    case 4: memws.WriteArrayOfInt32(reinterpret_cast<const int32_t*>(bmpp->GetData()), w * h); break;
    default: assert(0); break;
    }
  }

  // Open same buffer for reading, and begin writing compressed data into the output
  VectorStream mem_in(membuf);
  // NOTE: old format saves full RGB struct here (4 bytes, including the filler)
  if (pal)
    out->WriteArray(*pal, sizeof(RGB), 256);
  else
    out->WriteByteCount(0, sizeof(RGB) * 256);
  out->WriteInt32((uint32_t)mem_in.GetLength());

  // reserve space for compressed size
  soff_t cmpsz_at = out->GetPosition();
  out->WriteInt32(0);
  lzwcompress(&mem_in, out);
  soff_t toret = out->GetPosition();
  out->Seek(cmpsz_at, kSeekBegin);
  soff_t compressed_sz = (toret - cmpsz_at) - sizeof(uint32_t);
  out->WriteInt32(compressed_sz); // write compressed size
  // seek back to the end of the output stream
  out->Seek(toret, kSeekBegin);
}

std::unique_ptr<Bitmap> load_lzw(Stream *in, int dst_bpp, RGB (*pal)[256])
{
  // NOTE: old format saves full RGB struct here (4 bytes, including the filler)
  if (pal)
    in->Read(*pal, sizeof(RGB) * 256);
  else
    in->Seek(sizeof(RGB) * 256);
  const size_t uncomp_sz = in->ReadInt32();
  const size_t comp_sz = in->ReadInt32();
  const soff_t end_pos = in->GetPosition() + comp_sz;

  // First decompress data into the memory buffer
  std::vector<uint8_t> membuf;
  {
    VectorStream memws(membuf, kStream_Write);
    lzwexpand(in, &memws, uncomp_sz);
  }

  // Open same buffer for reading and get params and pixels
  VectorStream mem_in(membuf);
  int stride = mem_in.ReadInt32(); // width * bpp
  int height = mem_in.ReadInt32();
  std::unique_ptr<Bitmap> bmm(BitmapHelper::CreateBitmap((stride / dst_bpp), height, dst_bpp * 8));
  if (!bmm) return nullptr; // out of mem?

  size_t num_pixels = stride * height / dst_bpp;
  uint8_t *bmp_data = bmm->GetDataForWriting();
  switch (dst_bpp)
  {
  case 1: mem_in.Read(bmp_data, num_pixels); break;
  case 2: mem_in.ReadArrayOfInt16(reinterpret_cast<int16_t*>(bmp_data), num_pixels); break;
  case 4: mem_in.ReadArrayOfInt32(reinterpret_cast<int32_t*>(bmp_data), num_pixels); break;
  default: assert(0); break;
  }

  if (in->GetPosition() != end_pos)
    in->Seek(end_pos, kSeekBegin);

  return bmm;
}
