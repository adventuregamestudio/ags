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
//
// Runtime script value struct
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H
#define __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H

#include "script/script_api.h"

struct ICCStaticObject;
struct StaticArray;
struct ICCDynamicObject;

enum ScriptValueType
{
    kScValUndefined,    // to detect errors
    kScValInteger,      // as strictly 32-bit integer (for integer math)
    kScValFloat,        // as float (for floating point math), 32-bit
    kScValStackPtr,     // as a pointer to stack entry
    kScValData,         // as a container for randomly sized data (usually array)
    kScValGlobalVar,    // as a pointer to script variable; used only for global vars,
                        // as pointer to local vars must have StackPtr type so that the
                        // stack allocation could work
    kScValStringLiteral,// as a pointer to literal string (array of chars)
    kScValStaticObject, // as a pointer to static global script object
    kScValStaticArray,  // as a pointer to static global array (of static or dynamic objects)
    kScValDynamicObject,// as a pointer to managed script object
    kScValStaticFunction,// as a pointer to static function
    kScValObjectFunction,// as a pointer to object member function, gets object pointer as
                        // first parameter
    kScValCodePtr,      // as a pointer to element in byte-code array
};

struct RuntimeScriptValue
{
public:
    RuntimeScriptValue()
    {
        Type        = kScValUndefined;
        IValue		= 0;
        Ptr         = NULL;
        MgrPtr      = NULL;
        Size        = 0;
    }

private:
    ScriptValueType Type;
    // The 32-bit value used for integer/float math and for storing
    // variable/element offset relative to object (and array) address
    union
    {
        int32_t     IValue; // access Value as int32 type
        float	    FValue;	// access Value as float type
    };
    // Pointer is used for storing... pointers - to objects, arrays,
    // functions and stack entries (other RSV)
    union
    {
        char                *Ptr;   // generic data pointer
        RuntimeScriptValue  *RValue;// access ptr as a pointer to Runtime Value
        ScriptAPIFunction   *SPfn;  // access ptr as a pointer to Script API Static Function
        ScriptAPIObjectFunction *ObjPfn; // access ptr as a pointer to Script API Object Function
    };
    // TODO: separation to Ptr and MgrPtr is only needed so far as there's
    // a separation between Script*, Dynamic* and game entity classes.
    // Once those classes are merged, it will no longer be needed.
    union
    {
        void                *MgrPtr;// generic object manager pointer
        ICCStaticObject     *StcMgr;// static object manager
        StaticArray         *StcArr;// static array manager
        ICCDynamicObject    *DynMgr;// dynamic object manager
    };
    // The "real" size of data, either one stored in I/FValue,
    // or the one referenced by Ptr. Used for calculating stack
    // offsets.
    // Original AGS scripts always assumed pointer is 32-bit.
    // Therefore for stored pointers Size is always 4 both for x32
    // and x64 builds, so that the script is interpreted correctly.
    int             Size;

public:
    inline bool IsValid() const
    {
        return Type != kScValUndefined;
    }
    inline bool IsNull() const
    {
        return Ptr == 0 && IValue == 0;
    }
    inline ScriptValueType GetType() const
    {
        return Type;
    }

    inline int32_t GetInt32() const
    {
        return IValue;
    }
    inline float GetFloat() const
    {
        return FValue;
    }
    inline bool GetAsBool() const
    {
        return !IsNull();
    }
    inline char *GetPtr() const
    {
        return Ptr;
    }
    inline char* GetPtrWithOffset() const
    {
        return Ptr + IValue;
    }
    inline RuntimeScriptValue *GetStackEntry() const
    {
        return RValue;
    }
    inline RuntimeScriptValue *GetGlobalVar() const
    {
        return RValue;
    }
    inline RuntimeScriptValue GetRValueWithOffset() const
    {
        RuntimeScriptValue rval = RValue ? *RValue : RuntimeScriptValue();
        rval += IValue;
        return rval;
    }
    inline ScriptAPIFunction *GetStaticFuncPtr() const
    {
        return SPfn;
    }
    inline ScriptAPIObjectFunction *GetObjectFunctionPtr() const
    {
        return ObjPfn;
    }
    inline ICCStaticObject *GetStaticManager() const
    {
        return StcMgr;
    }
    inline StaticArray *GetStaticArray() const
    {
        return StcArr;
    }
    inline ICCDynamicObject *GetDynamicManager() const
    {
        return DynMgr;
    }
    inline int GetSize() const
    {
        return Size;
    }

    inline RuntimeScriptValue &Invalidate()
    {
        Type    = kScValUndefined;
        IValue   = 0;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 0;
        return *this;
    }
    inline RuntimeScriptValue &SetInt8(int8_t val)
    {
        Type    = kScValInteger;
        IValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 1;
        return *this;
    }
    inline RuntimeScriptValue &SetInt16(int16_t val)
    {
        Type    = kScValInteger;
        IValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 2;
        return *this;
    }
    inline RuntimeScriptValue &SetInt32(int32_t val)
    {
        Type    = kScValInteger;
        IValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetFloat(float val)
    {
        Type    = kScValFloat;
        FValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetInt32AsBool(bool val)
    {
        return SetInt32(val ? 1 : 0);
    }
    inline RuntimeScriptValue &SetFloatAsBool(bool val)
    {
        return SetFloat(val ? 1.0F : 0.0F);
    }
    inline RuntimeScriptValue &SetStackPtr(RuntimeScriptValue *stack_entry)
    {
        Type    = kScValStackPtr;
        IValue  = 0;
        RValue  = stack_entry;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetData(char *data, int size)
    {
        Type    = kScValData;
        IValue  = 0;
        Ptr     = data;
        MgrPtr  = NULL;
        Size    = size;
        return *this;
    }
    inline RuntimeScriptValue &SetGlobalVar(RuntimeScriptValue *glvar_value)
    {
        Type    = kScValGlobalVar;
        IValue  = 0;
        RValue  = glvar_value;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    // TODO: size?
    inline RuntimeScriptValue &SetStringLiteral(char *str)
    {
        Type    = kScValStringLiteral;
        IValue  = 0;
        Ptr     = str;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetStaticObject(void *object, ICCStaticObject *manager)
    {
        Type    = kScValStaticObject;
        IValue  = 0;
        Ptr     = (char*)object;
        StcMgr  = manager;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetStaticArray(void *object, StaticArray *manager)
    {
        Type    = kScValStaticArray;
        IValue  = 0;
        Ptr     = (char*)object;
        StcArr  = manager;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetDynamicObject(void *object, ICCDynamicObject *manager)
    {
        Type    = kScValDynamicObject;
        IValue  = 0;
        Ptr     = (char*)object;
        DynMgr  = manager;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetStaticFunction(ScriptAPIFunction *pfn)
    {
        Type    = kScValStaticFunction;
        IValue  = 0;
        SPfn    = pfn;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetObjectFunction(ScriptAPIObjectFunction *pfn)
    {
        Type    = kScValObjectFunction;
        IValue  = 0;
        ObjPfn  = pfn;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetCodePtr(char *ptr)
    {
        Type    = kScValCodePtr;
        IValue  = 0;
        Ptr     = ptr;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }

    inline RuntimeScriptValue operator !() const
    {
        return RuntimeScriptValue().SetInt32AsBool(!GetAsBool());
    }

    inline RuntimeScriptValue &operator +=(const RuntimeScriptValue &rval)
    {
        IValue += rval.IValue;
        return *this;
    }
    inline RuntimeScriptValue &operator -=(const RuntimeScriptValue &rval)
    {
        IValue -= rval.IValue;
        return *this;
    }
    inline RuntimeScriptValue &operator *=(const RuntimeScriptValue &rval)
    {
        IValue *= rval.IValue;
        return *this;
    }
    inline RuntimeScriptValue &operator /=(const RuntimeScriptValue &rval)
    {
        IValue /= rval.IValue;
        return *this;
    }

    inline RuntimeScriptValue &operator +=(intptr_t val)
    {
        IValue += val;
        return *this;
    }
    inline RuntimeScriptValue &operator -=(intptr_t val)
    {
        IValue -= val;
        return *this;
    }
    inline RuntimeScriptValue &operator *=(intptr_t val)
    {
        IValue *= val;
        return *this;
    }
    inline RuntimeScriptValue &operator /=(intptr_t val)
    {
        IValue /= val;
        return *this;
    }

    inline bool operator ==(const RuntimeScriptValue &rval)
    {
        return ((intptr_t)Ptr + (intptr_t)IValue) == ((intptr_t)rval.Ptr + (intptr_t)rval.IValue);
    }
    inline bool operator !=(const RuntimeScriptValue &rval)
    {
        return !(*this == rval);
    }

    // Helper functions for reading or writing values from/to
    // object, referenced by this Runtime Value.
    // Copy implementation depends on value type.
    uint8_t     ReadByte();
    int16_t     ReadInt16();
    int32_t     ReadInt32();
    RuntimeScriptValue ReadValue();
    bool        WriteByte(uint8_t val);
    bool        WriteInt16(int16_t val);
    bool        WriteInt32(int32_t val);
    bool        WriteValue(const RuntimeScriptValue &rval);
};

#endif // __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H
