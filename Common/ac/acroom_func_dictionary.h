#ifndef __CROOM_FUNC_DICTIONARY_H
#define __CROOM_FUNC_DICTIONARY_H

extern void decrypt_text(char*toenc);
extern void read_string_decrypt(FILE *ooo, char *sss);
extern void read_dictionary (WordsDictionary *dict, FILE *writeto);
extern void freadmissout(short *pptr, FILE *opty);

//void WordsDictionary::sort ();
//int WordsDictionary::find_index (const char*wrem);

#endif