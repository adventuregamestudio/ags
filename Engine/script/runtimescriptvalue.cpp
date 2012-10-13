
#include "script/cc_error.h"
#include "script/runtimescriptvalue.h"

//
// NOTE to future optimizers: I am using 'this' ptr here to better
// distinguish Runtime Values.
//

uint8_t RuntimeScriptValue::ReadByte()
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            return *(uint8_t*)(RValue->GetDataPtrWithOffset() + this->Value);
        }
        else
        {
            return RValue->GetInt(); // get RValue as int
        }
    }
    return *((uint8_t*)this->GetDataPtrWithOffset());
}

int16_t RuntimeScriptValue::ReadInt16()
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            return *(int16_t*)(RValue->GetDataPtrWithOffset() + this->Value);
        }
        else
        {
            return RValue->GetInt(); // get RValue as int
        }
    }
#if defined(AGS_BIG_ENDIAN)
    else if (this->Type == kScValGlobalData)
    {
        int16_t temp = *((int16_t*)this->GetDataPtrWithOffset());
        AGS::Common::BitByteOperations::SwapBytesInt16(temp);
        return temp;
    }
#endif // AGS_BIG_ENDIAN
    return *((int16_t*)this->GetDataPtrWithOffset());
}

int32_t RuntimeScriptValue::ReadInt32()
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            return *(int32_t*)(RValue->GetDataPtrWithOffset() + this->Value);
        }
        else
        {
            return RValue->GetInt(); // get RValue as int
        }
    }
#if defined(AGS_BIG_ENDIAN)
    else if (this->Type == kScValGlobalData)
    {
        int32_t temp = *((int32_t*)this->GetDataPtrWithOffset());
        AGS::Common::BitByteOperations::SwapBytesInt32(temp);
        return temp;
    }
#endif // AGS_BIG_ENDIAN
    return *((int32_t*)this->GetDataPtrWithOffset());
}

RuntimeScriptValue RuntimeScriptValue::ReadValue()
{
    RuntimeScriptValue rval;
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            rval.SetInt32(*(int32_t*)(RValue->GetDataPtrWithOffset() + this->Value));
        }
        else
        {
            rval = *RValue;
        }
    }
#if defined(AGS_BIG_ENDIAN)
    else if (this->Type == kScValGlobalData)
    {
        int32_t temp = *((int32_t*)this->GetDataPtrWithOffset());
        AGS::Common::BitByteOperations::SwapBytesInt32(temp);
        rval.SetLong(temp);
    }
#endif // AGS_BIG_ENDIAN
    else
    {
        // 64 bit: Memory reads are still 32 bit
        rval.SetLong(*(int32_t*)GetDataPtrWithOffset());
    }
    return rval;
}

bool RuntimeScriptValue::WriteByte(uint8_t val)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            *(uint8_t*)(RValue->GetDataPtrWithOffset() + this->Value) = val;
        }
        else
        {
            RValue->SetInt8(val); // set RValue as int
        }
    }
    else
    {
        *((uint8_t*)GetDataPtrWithOffset()) = val;
    }
    return true;
}

bool RuntimeScriptValue::WriteInt16(int16_t val)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            *(int16_t*)(RValue->GetDataPtrWithOffset() + this->Value) = val;
        }
        else
        {
            RValue->SetInt16(val); // set RValue as int
        }
    }
#if defined(AGS_BIG_ENDIAN)
    else if (this->Type == kScValGlobalData)
    {
        AGS::Common::BitByteOperations::SwapBytesInt16(val);
        *((int16_t*)GetDataPtrWithOffset()) = val;
    }
#endif // AGS_BIG_ENDIAN
    else
    {
        *((int16_t*)GetDataPtrWithOffset()) = val;
    }
    return true;
}

bool RuntimeScriptValue::WriteInt32(int32_t val)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            *(int32_t*)(RValue->GetDataPtrWithOffset() + this->Value) = val;
        }
        else
        {
            RValue->SetInt32(val); // set RValue as int
        }
    }
#if defined(AGS_BIG_ENDIAN)
    else if (this->Type == kScValGlobalData)
    {
        AGS::Common::BitByteOperations::SwapBytesInt32(val);
        *((int32_t*)GetDataPtrWithOffset()) = val;
    }
#endif // AGS_BIG_ENDIAN
    else
    {
        *((int32_t*)GetDataPtrWithOffset()) = val;
    }
    return true;
}

bool RuntimeScriptValue::WriteValue(const RuntimeScriptValue &rval)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValDataPtr)
        {
            *(int32_t*)(RValue->GetDataPtrWithOffset() + this->Value) = rval.GetInt();
        }
        else
        {
            *RValue = rval;
        }
    }
#if defined(AGS_BIG_ENDIAN)
    else if (this->Type == kScValGlobalData)
    {
        int32_t temp = rval.GetLong();
        AGS::Common::BitByteOperations::SwapBytesInt32(temp);
        *((int32_t*)GetDataPtrWithOffset()) = temp;
    }
#endif // AGS_BIG_ENDIAN
    else
    {
        // 64 bit: Memory writes are still 32 bit
        *((int32_t*)GetDataPtrWithOffset()) = rval.GetLong();
    }
    return true;
}
