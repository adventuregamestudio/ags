#ifndef __AC_WORDSDICTIONARY_H
#define __AC_WORDSDICTIONARY_H

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

#define MAX_PARSER_WORD_LENGTH 30
#define ANYWORD     29999
#define RESTOFLINE  30000

struct WordsDictionary {
    int   num_words;
    char**word;
    short*wordnum;

    void allocate_memory(int wordCount);
    void free_memory();
    void  sort();
    int   find_index (const char *);
};

extern char *passwencstring;

extern void decrypt_text(char*toenc);
extern void read_string_decrypt(Common::CDataStream *in, char *sss);
extern void read_dictionary (WordsDictionary *dict, Common::CDataStream *in);
extern void freadmissout(short *pptr, Common::CDataStream *in);

extern void encrypt_text(char *toenc);
extern void write_string_encrypt(Common::CDataStream *out, char *sss);
extern void write_dictionary (WordsDictionary *dict, Common::CDataStream *out);

#endif // __AC_WORDSDICTIONARY_H