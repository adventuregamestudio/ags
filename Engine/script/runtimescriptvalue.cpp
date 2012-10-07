
#include "script/runtimescriptvalue.h"

bool RuntimeScriptValue::ReadByteFromAddr(const RuntimeScriptValue &src)
{
    this->Value = *(uint8_t*)src.Value;
    return true;
}

bool RuntimeScriptValue::ReadInt16FromAddr(const RuntimeScriptValue &src)
{
    this->Value = *(int16_t*)src.Value;
    return true;
}

bool RuntimeScriptValue::ReadFromAddr(const RuntimeScriptValue &src)
{
    this->Value = *(int32_t*)src.Value;
    return true;
}

bool RuntimeScriptValue::WriteByteToAddr(RuntimeScriptValue &dest) const
{
    *(uint8_t*)dest.Value = (uint8_t)this->Value;
    return true;
}

bool RuntimeScriptValue::WriteInt16ToAddr(RuntimeScriptValue &dest) const
{
    *(int16_t*)dest.Value = (int16_t)this->Value;
    return true;
}

bool RuntimeScriptValue::WriteToAddr(RuntimeScriptValue &dest) const
{
    *(int32_t*)dest.Value = (int32_t)this->Value;
    return true;
}
