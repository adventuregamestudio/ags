#ifndef __CC_MACROTABLE_H
#define __CC_MACROTABLE_H

#define MAX_LINE_LENGTH 500
#define MAXDEFINES 1500
struct MacroTable {
    int num;

    char *name[MAXDEFINES];
    char *macro[MAXDEFINES];
    void init();

    void shutdown();
    int  find_name(char *);
    void add(char *,char *);
    void remove(int index);
    void merge(MacroTable *);

    MacroTable() {
        init();
    }
};


extern MacroTable macros;

#endif // __CC_MACROTABLE_H
