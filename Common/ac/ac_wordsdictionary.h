#ifndef __AC_WORDSDICTIONARY_H
#define __AC_WORDSDICTIONARY_H

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


#endif // __AC_WORDSDICTIONARY_H