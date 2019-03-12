
#include "cs_parser_common.h"

int is_whitespace(char cht) {
    // space, tab, EOF, VT (dunno, Chrille had this char appearing)
    if ((cht == ' ') || (cht == 9) || (cht == 26) || (cht == 11)) return 1;
    return 0;
}

void skip_whitespace(char **pttt) {
    char *mpt=pttt[0];
    while (is_whitespace(mpt[0])) mpt++;
    pttt[0]=mpt;
}

int is_digit(int chrac) {
    if ((chrac >= '0') && (chrac <= '9')) return 1;
    return 0;
}

int is_alphanum(int chrac) {
    if ((chrac>='A') & (chrac<='Z')) return 1;
    if ((chrac>='a') & (chrac<='z')) return 1;
    if ((chrac>='0') & (chrac<='9')) return 1;
    if (chrac == '_') return 1;
    if (chrac == '\"') return 1;
    if (chrac == '\'') return 1;
    return 0;
}
