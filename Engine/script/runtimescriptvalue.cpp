
#include "script/runtimescriptvalue.h"

uint8_t RuntimeScriptValue::ReadByte()
{
    return *((uint8_t*)GetLong());
}

uint16_t RuntimeScriptValue::ReadInt16()
{
    return *((uint16_t*)GetLong());
}

uint32_t RuntimeScriptValue::ReadInt32()
{
    return *((uint32_t*)GetLong());
}

RuntimeScriptValue RuntimeScriptValue::ReadValue()
{
    RuntimeScriptValue rval;
    rval.SetLong(*(uint32_t*)GetLong());
    return rval;
}

bool RuntimeScriptValue::WriteByte(uint8_t val)
{
    *((uint8_t*)GetLong()) = val;
    return true;
}

bool RuntimeScriptValue::WriteInt16(uint16_t val)
{
    *((uint16_t*)GetLong()) = val;
    return true;
}

bool RuntimeScriptValue::WriteInt32(uint32_t val)
{
    *((uint32_t*)GetLong()) = val;
    return true;
}

bool RuntimeScriptValue::WriteValue(const RuntimeScriptValue &rval)
{
    *((uint32_t*)GetLong()) = rval.GetLong();
    return true;
}
