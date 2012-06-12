#ifndef __CC_SYSTEMIMPORTS_H
#define __CC_SYSTEMIMPORTS_H

#include "cc_instance.h"    // ccInstance
#include "cc_treemap.h"     // ccTreeMap

struct SystemImports
{
private:
    char **name;
    char **addr;
    ccInstance **isScriptImp;
    int numimports;
    int bufferSize;
    ccTreeMap btree;

public:
    int  add(char *, char *, ccInstance*);
    void remove(char *);
    char *get_addr_of(char *);
    int  get_index_of(char *);
    ccInstance* is_script_import(char *);
    void remove_range(char *, unsigned long);
    void clear() {
        numimports = 0;
        btree.clear();
    }
    //  void remove_all_script_exports();

    SystemImports()
    {
        numimports = 0;
        bufferSize = 0;
        name = NULL;
        addr = NULL;
        isScriptImp = NULL;
    }
};

extern SystemImports simp;

#endif  // __CC_SYSTEMIMPORTS_H