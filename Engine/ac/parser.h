
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__PARSER_H
#define __AGS_EE_AC__PARSER_H

int Parser_FindWordID(const char *wordToFind);
const char* Parser_SaidUnknownWord();
void ParseText (char*text);
int Said (char*checkwords);

//=============================================================================

int find_word_in_dictionary (char *lookfor);
int is_valid_word_char(char theChar);
int FindMatchingMultiWordWord(char *thisword, char **text);
int parse_sentence (char*text, int *numwords, short*wordarray, short*compareto, int comparetonum);

#endif // __AGS_EE_AC__PARSER_H
