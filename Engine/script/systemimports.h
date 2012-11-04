//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#ifndef __CC_SYSTEMIMPORTS_H
#define __CC_SYSTEMIMPORTS_H

#include "script/cc_instance.h"    // ccInstance
#include "script/cc_treemap.h"     // ccTreeMap

struct ICCDynamicObject;
struct ICCStaticObject;

enum ScriptImportType
{
    kScImportUndefined,         // to detect errors
    //kScImportData,            // for direct engine memory access (TODO: unsupport later!)
    kScImportStaticFunction,    // a static function
    kScImportStaticObject,      // static script object
    kScImportDynamicObject,     // dynamic script object
    kScImportObjectFunction,    // script object member function, gets 'this' pointer
    kScImportScriptData,        // script function or variable
};

struct ScriptImport
{
    ScriptImport()
    {
        Type        = kScImportUndefined;
        Name        = NULL;
        Ptr         = NULL;
        DynMgr      = NULL;
        InstancePtr = NULL;
    }

    ScriptImportType    Type;
    const char          *Name;          // import's uid
    void                *Ptr;           // object or function pointer
    // TODO: separation to Ptr and MgrPtr is only needed so far as there's
    // a separation between Script*, Dynamic* and game entity classes.
    union
    {
        void                *MgrPtr;        // generic object manager pointer
        ICCDynamicObject    *DynMgr;        // dynamic object manager
        ICCStaticObject     *StcMgr;        // static object manager
    };
    ccInstance          *InstancePtr;   // script instance
};

struct SystemImports
{
private:
    ScriptImport *imports;
    int numimports;
    int bufferSize;
    ccTreeMap btree;

public:
    int  add(ScriptImportType type, const char *name, void *ptr, void *manager, ccInstance *inst);
    void remove(const char *name);
    const ScriptImport *getByName(const char *name);
    int  get_index_of(const char *name);
    const ScriptImport *getByIndex(int index);
    //void *get_addr_of(const char *name);
    //ccInstance* is_script_import(const char *name);
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
    }
};

extern SystemImports simp;

#endif  // __CC_SYSTEMIMPORTS_H