//=============================================================================
//
// Runtime script value struct
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H
#define __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H

#include "core/types.h"

enum ScriptValueType
{
    kScValUndefined,    // to detect errors
    kScValGeneric,      // as long
    kScValInteger,      // as strictly 32-bit integer (for integer math)
    kScValFloat,        // as float (for floating point math)
};

struct RuntimeScriptValue
{
public:
    RuntimeScriptValue()
    {
        Type        = kScValUndefined;
        Value		= 0;
    }

private:
    ScriptValueType Type;
    union
    {
        intptr_t    Value;	// generic Value
        int32_t     IValue; // access Value as int32 type
        float	    FValue;	// access Value as float type
    };

public:
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
        return Value != 0;
    }

    inline RuntimeScriptValue &SetLong(intptr_t val)
    {
        Type = kScValGeneric;
        Value = val;
        return *this;
    }
    inline RuntimeScriptValue &SetInt(int32_t val)
    {
        Type = kScValInteger;
        IValue = val;
        return *this;
    }
    inline RuntimeScriptValue &SetFloat(float val)
    {
        Type = kScValFloat;
        FValue = val;
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
        return Value == val;
    }
    inline bool operator !=(intptr_t val)
    {
        return Value != val;
    }

    inline bool operator ==(const RuntimeScriptValue &rval)
    {
        return Value == rval.Value;
    }
    inline bool operator !=(const RuntimeScriptValue &rval)
    {
        return Value != rval.Value;
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
    uint16_t    ReadInt16();
    uint32_t    ReadInt32();
    RuntimeScriptValue ReadValue();
    bool        WriteByte(uint8_t val);
    bool        WriteInt16(uint16_t val);
    bool        WriteInt32(uint32_t val);
    bool        WriteValue(const RuntimeScriptValue &rval);
};

#endif // __AGS_EE_SCRIPT__RUNTIMESCRIPTVALUE_H
