
int find_word_in_dictionary (char *lookfor);
int SaidUnknownWord (char*buffer);
const char* Parser_SaidUnknownWord();
int is_valid_word_char(char theChar);
int FindMatchingMultiWordWord(char *thisword, char **text);

int parse_sentence (char*text, int *numwords, short*wordarray, short*compareto, int comparetonum);
void ParseText (char*text);
int Parser_FindWordID(const char *wordToFind);

int Said (char*checkwords);
