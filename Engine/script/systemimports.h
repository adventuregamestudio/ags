#ifndef __CC_SYSTEMIMPORTS_H
#define __CC_SYSTEMIMPORTS_H

#include "script/cc_instance.h"    // ccInstance
#include "script/cc_treemap.h"     // ccTreeMap

enum ScriptImportType
{
    kScImportUndefined,         // to detect errors
    kScImportData,              // for direct engine memory access (TODO: unsupport later!)
    kScImportStaticFunction,    // a static function
    kScImportObject,            // script object
    kScImportObjectFunction,    // script object member function, gets 'this' pointer
    kScImportScriptData,        // script function or variable
};

struct ScriptImport
{
    ScriptImport()
    {
        Type    = kScImportUndefined;
        Name    = NULL;
        Ptr     = NULL;
    }

    ScriptImportType    Type;
    const char          *Name;          // import's uid
    void                *Ptr;           // object or function pointer
    ccInstance          *InstancePtr;   // script instance
};

struct SystemImports
{
private:
    //char **name;
    //char **addr;
    //ccInstance **isScriptImp;
    ScriptImport *imports;
    int numimports;
    int bufferSize;
    ccTreeMap btree;

public:
    int  add(ScriptImportType type, const char *name, void *ptr, ccInstance *inst);
    void remove(const char *name);
    void *get_addr_of(const char *name);
    int  get_index_of(const char *name);
    ccInstance* is_script_import(const char *name);
    void remove_range(void *from_ptr, unsigned long dist);
    void clear() {
        numimports = 0;
        btree.clear();
    }
    //  void remove_all_script_exports();

    SystemImports()
    {
        numimports  = 0;
        bufferSize  = 0;
        imports     = NULL;
        //name = NULL;
        //addr = NULL;
        //isScriptImp = NULL;
    }
};

extern SystemImports simp;

#endif  // __CC_SYSTEMIMPORTS_H