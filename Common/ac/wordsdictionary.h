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
#ifndef __AC_WORDSDICTIONARY_H
#define __AC_WORDSDICTIONARY_H

#include <vector>
#include "core/types.h"
#include "util/string.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MAX_PARSER_WORD_LENGTH 30
#define ANYWORD     29999
#define RESTOFLINE  30000

struct WordsDictionary {
    int   num_words;
    char**word;
    short*wordnum;

    WordsDictionary();
    ~WordsDictionary();
    void allocate_memory(int wordCount);
    void free_memory();
    void  sort();
    int   find_index (const char *);
};

extern const char *passwencstring;

// Decrypts text found in the given buffer, writes back to the same buffer
extern void decrypt_text(char *buf, size_t buf_sz);
// Reads an encrypted string from the stream and decrypts into the provided buffer
extern void read_string_decrypt(Common::Stream *in, char *buf, size_t buf_sz);
// Reads an encrypted string from the stream and returns as a string;
// uses provided vector as a temporary decryption buffer (avoid extra allocs)
extern Common::String read_string_decrypt(Common::Stream *in, std::vector<char> &dec_buf);
extern void read_dictionary(WordsDictionary *dict, Common::Stream *in);

#if defined (OBSOLETE)
// TODO: not a part of wordsdictionary, move to obsoletes
extern void freadmissout(short *pptr, Common::Stream *in);
#endif

extern void encrypt_text(char *toenc);
extern void write_string_encrypt(Common::Stream *out, const char *s);
extern void write_dictionary (WordsDictionary *dict, Common::Stream *out);

#endif // __AC_WORDSDICTIONARY_H