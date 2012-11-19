//=============================================================================
//
// Runtime script value struct
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H
#define __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H

#include "core/types.h"

struct ICCStaticObject;
struct StaticArray;
struct ICCDynamicObject;

enum ScriptValueType
{
    kScValUndefined,    // to detect errors
    // TODO: the 'generic' type should be eventually made obsolete, since
    // it practically means 'something we do not know what'
    kScValGeneric,      // as long (intptr_t)
    kScValInteger,      // as strictly 32-bit integer (for integer math)
    kScValFloat,        // as float (for floating point math), 32-bit
    kScValStackPtr,     // as a pointer to stack
    kScValDataPtr,      // as a pointer to randomly sized data (usually array)
    kScValGlobalData,   // a pointer to global data; at the moment serves only as
                        // a workaround for big endian builds (maybe temporary);
                        // works similarly to kScValDataPtr for the rest
    kScValStaticObject, // as a pointer to static global script object
    kScValStaticArray,  // as a pointer to static global array (of static or dynamic objects)
    kScValDynamicObject,// as a pointer to managed script object
    kScValStaticFunction,// as a pointer to static function
    kScValObjectFunction,// as a pointer to object member function, gets object pointer as
                        // first parameter
    kScValScriptData    // an import from another script, could be object or function ptr;
                        // at the moment used only for CALLOBJ arg type check
};

struct RuntimeScriptValue
{
public:
    RuntimeScriptValue()
    {
        Type        = kScValUndefined;
        Value		= 0;
        Ptr         = NULL;
        MgrPtr      = NULL;
        Size        = 0;
    }

private:
    ScriptValueType Type;
    union
    {
        intptr_t    Value;	// generic Value
        int32_t     IValue; // access Value as int32 type
        float	    FValue;	// access Value as float type
    };
    union
    {
        char                *Ptr;   // generic data pointer
        RuntimeScriptValue  *RValue;// access ptr as a pointer to Runtime Value
    };
    // TODO: separation to Ptr and MgrPtr is only needed so far as there's
    // a separation between Script*, Dynamic* and game entity classes.
    union
    {
        void                *MgrPtr;// generic object manager pointer
        ICCStaticObject     *StcMgr;// static object manager
        StaticArray         *StcArr;// static array manager
        ICCDynamicObject    *DynMgr;// dynamic object manager
    };
    // The "real" size of data, either one stored in Value variable,
    // or the one referenced by Ptr.
    // For stored pointers Size is always 4 both for x32 and x64 builds,
    // for the sake of cross-platform compatibility.
    int             Size;

public:
    inline bool IsValid() const
    {
        return Type != kScValUndefined;
    }
    inline bool IsNull() const
    {
        if (Type == kScValInteger)
        {
          return IValue == 0;
        }
        else if (Type == kScValFloat)
        {
          return FValue == 0.0;
        }
        else
        {
          return Value == 0 && Ptr == 0;
        }
    }
    inline ScriptValueType GetType() const
    {
        return Type;
    }

    inline intptr_t GetLong() const
    {
        return Value;
    }
    inline int32_t GetInt32() const
    {
        return IValue;
    }
    inline float GetFloat() const
    {
        return FValue;
    }
    inline bool AsBool() const
    {
        return !IsNull();
    }
    inline char *GetDataPtr() const
    {
        return Ptr;
    }
    inline char* GetDataPtrWithOffset() const
    {
        return Ptr + Value;
    }
    inline RuntimeScriptValue *GetStackEntry() const
    {
        return RValue;
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
        Value   = 0;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 0;
        return *this;
    }
    inline RuntimeScriptValue &SetLong(intptr_t val)
    {
        Type    = kScValGeneric;
        Value   = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetInt8(char val)
    {
        Type    = kScValInteger;
        Value   = 0; // zero intptr_t value for 64-bit build's safety
        IValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 1;
        return *this;
    }
    inline RuntimeScriptValue &SetInt16(int16_t val)
    {
        Type    = kScValInteger;
        Value   = 0; // zero intptr_t value for 64-bit build's safety
        IValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 2;
        return *this;
    }
    inline RuntimeScriptValue &SetInt32(int32_t val)
    {
        Type    = kScValInteger;
        Value   = 0; // zero intptr_t value for 64-bit build's safety
        IValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetFloat(float val)
    {
        Type    = kScValFloat;
        Value   = 0; // zero intptr_t value for 64-bit build's safety
        FValue  = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetAsBool(bool val)
    {
        return SetInt32(val ? 1 : 0);
    }
    inline RuntimeScriptValue &SetStackPtr(RuntimeScriptValue *stack_entry)
    {
        Type    = kScValStackPtr;
        Value   = 0;
        RValue  = stack_entry;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetDataPtr(char *data, int size)
    {
        Type    = kScValDataPtr;
        Value   = 0;
        Ptr     = data;
        MgrPtr  = NULL;
        Size    = size;
        return *this;
    }
    // A workaround for big endian builds (maybe temporary)
    inline RuntimeScriptValue &SetGlobalData(intptr_t val)
    {
        Type    = kScValGlobalData;
        Value   = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetStaticObject(void *object, ICCStaticObject *manager)
    {
        Type    = kScValStaticObject;
        Value   = 0;
        Ptr     = (char*)object;
        StcMgr  = manager;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetStaticArray(void *object, StaticArray *manager)
    {
        Type    = kScValStaticArray;
        Value   = 0;
        Ptr     = (char*)object;
        StcArr  = manager;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetDynamicObject(void *object, ICCDynamicObject *manager)
    {
        Type    = kScValDynamicObject;
        Value   = 0;
        Ptr     = (char*)object;
        DynMgr  = manager;
        Size    = 4;
        return *this;
    }
    inline RuntimeScriptValue &SetScriptData(intptr_t val)
    {
        Type    = kScValScriptData;
        Value   = val;
        Ptr     = NULL;
        MgrPtr  = NULL;
        Size    = 4;
        return *this;
    }

    inline RuntimeScriptValue operator !() const
    {
        return RuntimeScriptValue().SetAsBool(!AsBool());
    }

    inline RuntimeScriptValue &operator +=(const RuntimeScriptValue &rval)
    {
        Value += rval.Value;
        return *this;
    }
    inline RuntimeScriptValue &operator -=(const RuntimeScriptValue &rval)
    {
        Value -= rval.Value;
        return *this;
    }
    inline RuntimeScriptValue &operator *=(const RuntimeScriptValue &rval)
    {
        Value *= rval.Value;
        return *this;
    }
    inline RuntimeScriptValue &operator /=(const RuntimeScriptValue &rval)
    {
        Value /= rval.Value;
        return *this;
    }

    inline RuntimeScriptValue &operator +=(intptr_t val)
    {
        Value += val;
        return *this;
    }
    inline RuntimeScriptValue &operator -=(intptr_t val)
    {
        Value -= val;
        return *this;
    }
    inline RuntimeScriptValue &operator *=(intptr_t val)
    {
        Value *= val;
        return *this;
    }
    inline RuntimeScriptValue &operator /=(intptr_t val)
    {
        Value /= val;
        return *this;
    }

    inline bool operator ==(const RuntimeScriptValue &rval)
    {
        if (Type == kScValInteger)
        {
            return IValue == rval.IValue;
        }
        else if (Type == kScValFloat)
        {
            // As of current implementation, this branch has little chance to run,
            // because floats are usually being compared by treating them as int32.
            return FValue == rval.FValue;
        }
        else
        {
            return GetDataPtrWithOffset() == rval.GetDataPtrWithOffset();
        }
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
