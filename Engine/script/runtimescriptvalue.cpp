
#include "script/cc_error.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_dynamicobject.h"
#include "ac/statobj/staticobject.h"

//
// NOTE to future optimizers: I am using 'this' ptr here to better
// distinguish Runtime Values.
//

// TODO: test again if stack entry really can hold an offset itself

// TODO: use endian-agnostic method to access global vars

uint8_t RuntimeScriptValue::ReadByte()
{
    if (this->Type == kScValStackPtr || this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            return *(uint8_t*)(RValue->GetPtrWithOffset() + this->IValue);
        }
        else
        {
            return RValue->GetInt32(); // get RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        return this->GetStaticManager()->ReadInt8(this->Ptr, this->IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        return this->GetDynamicManager()->ReadInt8(this->Ptr, this->IValue);
    }
    return *((uint8_t*)this->GetPtrWithOffset());
}

int16_t RuntimeScriptValue::ReadInt16()
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            return *(int16_t*)(RValue->GetPtrWithOffset() + this->IValue);
        }
        else
        {
            return RValue->GetInt32(); // get RValue as int
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            int16_t temp = *(int16_t*)(RValue->GetPtrWithOffset() + this->IValue);
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt16(temp);
#endif
            return temp;
        }
        else
        {
            return RValue->GetInt32(); // get RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        return this->GetStaticManager()->ReadInt16(this->Ptr, this->IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        return this->GetDynamicManager()->ReadInt16(this->Ptr, this->IValue);
    }
    return *((int16_t*)this->GetPtrWithOffset());
}

int32_t RuntimeScriptValue::ReadInt32()
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            return *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue);
        }
        else
        {
            return RValue->GetInt32(); // get RValue as int
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            int32_t temp = *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue);
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            return temp;
        }
        else
        {
            return RValue->GetInt32(); // get RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        return this->GetStaticManager()->ReadInt32(this->Ptr, this->IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        return this->GetDynamicManager()->ReadInt32(this->Ptr, this->IValue);
    }
    return *((int32_t*)this->GetPtrWithOffset());
}

// FIXME: find out all certain cases when we are reading a pointer and store it
// as 32-bit value here. There should be a solution to distinct these cases and
// store value differently, otherwise it won't work for 64-bit build.
RuntimeScriptValue RuntimeScriptValue::ReadValue()
{
    RuntimeScriptValue rval;
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            rval.SetInt32(*(int32_t*)(RValue->GetPtrWithOffset() + this->IValue));
        }
        else
        {
            rval = *RValue;
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            int32_t temp = *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue);
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            rval.SetInt32(temp);
        }
        else
        {
            rval = *RValue;
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        rval.SetInt32(this->GetStaticManager()->ReadInt32(this->Ptr, this->IValue));
    }
    else if (this->Type == kScValDynamicObject)
    {
        rval.SetInt32(this->GetDynamicManager()->ReadInt32(this->Ptr, this->IValue));
    }
    else
    {
        // 64 bit: Memory reads are still 32 bit
        rval.SetInt32(*(int32_t*)this->GetPtrWithOffset());
    }
    return rval;
}

bool RuntimeScriptValue::WriteByte(uint8_t val)
{
    if (this->Type == kScValStackPtr || this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            *(uint8_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
        }
        else
        {
            RValue->SetInt8(val); // set RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        this->GetStaticManager()->WriteInt8(this->Ptr, this->IValue, val);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->GetDynamicManager()->WriteInt8(this->Ptr, this->IValue, val);
    }
    else
    {
        *((uint8_t*)this->GetPtrWithOffset()) = val;
    }
    return true;
}

bool RuntimeScriptValue::WriteInt16(int16_t val)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            *(int16_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
        }
        else
        {
            RValue->SetInt16(val); // set RValue as int
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt16(val);
#endif
            *(int16_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
        }
        else
        {
            RValue->SetInt16(val); // set RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        this->GetStaticManager()->WriteInt16(this->Ptr, this->IValue, val);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->GetDynamicManager()->WriteInt16(this->Ptr, this->IValue, val);
    }
    else
    {
        *((int16_t*)this->GetPtrWithOffset()) = val;
    }
    return true;
}

bool RuntimeScriptValue::WriteInt32(int32_t val)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
        }
        else
        {
            RValue->SetInt32(val); // set RValue as int
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(val);
#endif
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
        }
        else
        {
            RValue->SetInt32(val); // set RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        this->GetStaticManager()->WriteInt32(this->Ptr, this->IValue, val);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->GetDynamicManager()->WriteInt32(this->Ptr, this->IValue, val);
    }
    else
    {
        *((int32_t*)this->GetPtrWithOffset()) = val;
    }
    return true;
}

// FIXME: make a type check here to find out if we are actually writing a
// pointer, and not int32 (which could be common too).
// If that is a pointer we are writing here, it won't work for 64-bit build
// as it is now. A solution should be implemented, depending on each distinct
// case.
bool RuntimeScriptValue::WriteValue(const RuntimeScriptValue &rval)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = (int32_t)rval.GetPtrWithOffset();
        }
        else
        {
            *RValue = rval;
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            int32_t val = (int32_t)rval.GetPtrWithOffset();
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(val);
#endif
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
        }
        else
        {
            *RValue = rval;
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        this->GetStaticManager()->WriteInt32(this->Ptr, this->IValue, (int32_t)rval.GetPtrWithOffset());
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->GetDynamicManager()->WriteInt32(this->Ptr, this->IValue, (int32_t)rval.GetPtrWithOffset());
    }
    else
    {
        // 64 bit: Memory writes are still 32 bit
        *((int32_t*)this->GetPtrWithOffset()) = (int32_t)rval.GetPtrWithOffset();
    }
    return true;
}
