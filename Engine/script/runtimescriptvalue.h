//=============================================================================
//
// Runtime script value struct
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H
#define __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H

#include "core/types.h"
#include "debug/assert.h"

struct ICCDynamicObject;

enum ScriptValueType
{
    kScValUndefined,    // to detect errors
    kScValGeneric,      // as long
    kScValInteger,      // as strictly 32-bit integer (for integer math)
    kScValFloat,        // as float (for floating point math)
    kScValStackPtr,     // as a pointer to stack
    kScValDataPtr,      // as a pointer to randomly sized data (usually array)
    kScValGlobalData,   // a workaround for big endian builds (maybe temporary);
                        // works exactly as kScValGeneric for the rest
    kScValDynamicObject,// as a pointer to managed script object
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
        ICCDynamicObject    *DynMgr;// dynamic object manager;
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
    inline ScriptValueType GetType() const
    {
        return Type;
    }

    inline intptr_t GetLong() const
    {
        return Value;
    }
    inline int32_t GetInt() const
    {
        return IValue;
    }
    inline float GetFloat() const
    {
        return FValue;
    }
    inline bool AsBool() const
    {
        return Value != 0 || Ptr != 0;
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
    inline RuntimeScriptValue &SetDynamicObject(void *object, ICCDynamicObject *manager)
    {
        Type    = kScValDynamicObject;
        Value   = 0;
        Ptr     = (char*)object;
        DynMgr  = manager;
        Size    = 4;
        return *this;
    }

    inline RuntimeScriptValue operator !() const
    {
        return RuntimeScriptValue().SetLong(AsBool() ? 0 : 1);
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
    inline bool operator ==(intptr_t val)
    {
        return (intptr_t)(Ptr + Value) == val;
    }
    inline bool operator !=(intptr_t val)
    {
        return (intptr_t)(Ptr + Value) != val;
    }

    inline bool operator ==(const RuntimeScriptValue &rval)
    {
        return (Type == rval.Type && Ptr == rval.Ptr && Value == rval.Value) ||
            // FIXME later: temporary workaround for dynamic object addresses
            GetDataPtrWithOffset() == rval.GetDataPtrWithOffset();
    }
    inline bool operator !=(const RuntimeScriptValue &rval)
    {
        return !(*this == rval);
    }
    inline bool operator >(const RuntimeScriptValue &rval) const
    {
        return Value > rval.Value;
    }
    inline bool operator <(const RuntimeScriptValue &rval) const
    {
        return Value < rval.Value;
    }
    inline bool operator >=(const RuntimeScriptValue &rval) const
    {
        return Value >= rval.Value;
    }
    inline bool operator <=(const RuntimeScriptValue &rval) const
    {
        return Value <= rval.Value;
    }

    inline bool operator >(long val) const
    {
        return Value > val;
    }
    inline bool operator <(long val) const
    {
        return Value < val;
    }
    inline bool operator >=(long val) const
    {
        return Value >= val;
    }
    inline bool operator <=(long val) const
    {
        return Value <= val;
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
