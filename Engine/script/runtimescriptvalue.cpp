
#include "script/cc_error.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_dynamicobject.h"
#include "ac/statobj/staticobject.h"
#include "util/bbop.h"

#include <string.h> // for memcpy()

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
            return RValue->IValue; // get RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        return this->StcMgr->ReadInt8(this->Ptr, this->IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        return this->DynMgr->ReadInt8(this->Ptr, this->IValue);
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
            return RValue->IValue; // get RValue as int
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
            return RValue->IValue; // get RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        return this->StcMgr->ReadInt16(this->Ptr, this->IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        return this->DynMgr->ReadInt16(this->Ptr, this->IValue);
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
            return RValue->IValue; // get RValue as int
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            int32_t temp;

#if defined(AGS_STRICT_ALIGNMENT)
            char *source = RValue->GetPtrWithOffset() + this->IValue;
            memcpy(&temp, source, sizeof(int32_t));
#else
            temp = *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue);
#endif

#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            return temp;
        }
        else
        {
            return RValue->IValue; // get RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        return this->StcMgr->ReadInt32(this->Ptr, this->IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        return this->DynMgr->ReadInt32(this->Ptr, this->IValue);
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
            int32_t temp;

#if defined(AGS_STRICT_ALIGNMENT)
            char *source = RValue->GetPtrWithOffset() + this->IValue;
            memcpy(&temp, source, sizeof(int32_t));
#else
            temp = *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue);
#endif

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
        rval.SetInt32(this->StcMgr->ReadInt32(this->Ptr, this->IValue));
    }
    else if (this->Type == kScValDynamicObject)
    {
        rval.SetInt32(this->DynMgr->ReadInt32(this->Ptr, this->IValue));
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
        this->StcMgr->WriteInt8(this->Ptr, this->IValue, val);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->DynMgr->WriteInt8(this->Ptr, this->IValue, val);
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
        this->StcMgr->WriteInt16(this->Ptr, this->IValue, val);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->DynMgr->WriteInt16(this->Ptr, this->IValue, val);
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

#if defined(AGS_STRICT_ALIGNMENT)
            char *destination = RValue->GetPtrWithOffset() + this->IValue;
            memcpy(destination, &val, sizeof(int32_t));
#else
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
#endif
        }
        else
        {
            RValue->SetInt32(val); // set RValue as int
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        this->StcMgr->WriteInt32(this->Ptr, this->IValue, val);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->DynMgr->WriteInt32(this->Ptr, this->IValue, val);
    }
    else
    {
        *((int32_t*)this->GetPtrWithOffset()) = val;
    }
    return true;
}

// Notice, that there are only two valid cases when a pointer may be written:
// when the destination is a stack entry or global variable of free type
// (not kScValData type).
// In any other case, only the numeric value (integer/float) will be written.
bool RuntimeScriptValue::WriteValue(const RuntimeScriptValue &rval)
{
    if (this->Type == kScValStackPtr)
    {
        if (RValue->Type == kScValData)
        {
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = rval.IValue;
        }
        else
        {
            // NOTE: we cannot just WriteValue here because when an integer
            // is pushed to the stack, script assumes that it is always 4
            // bytes and uses that size when calculating offsets to local
            // variables;
            // Therefore if pushed value is of integer type, we should rather
            // act as WriteInt32 (for int8, int16 and int32).
            if (rval.Type == kScValInteger)
            {
                RValue->SetInt32(rval.IValue);
            }
            else
            {
                *RValue = rval;
            }
        }
    }
    else if (this->Type == kScValGlobalVar)
    {
        if (RValue->Type == kScValData)
        {
            int32_t val = rval.IValue;
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(val);
#endif

#if defined(AGS_STRICT_ALIGNMENT)
            char *destination = RValue->GetPtrWithOffset() + this->IValue;
            memcpy(destination, &val, sizeof(int32_t));
#else
            *(int32_t*)(RValue->GetPtrWithOffset() + this->IValue) = val;
#endif
        }
        else
        {
            *RValue = rval;
        }
    }
    else if (this->Type == kScValStaticObject || this->Type == kScValStaticArray)
    {
        this->StcMgr->WriteInt32(this->Ptr, this->IValue, rval.IValue);
    }
    else if (this->Type == kScValDynamicObject)
    {
        this->DynMgr->WriteInt32(this->Ptr, this->IValue, rval.IValue);
    }
    else
    {
        *((int32_t*)this->GetPtrWithOffset()) = rval.IValue;
    }
    return true;
}

RuntimeScriptValue &RuntimeScriptValue::DirectPtr()
{
    while (Type == kScValGlobalVar || Type == kScValStackPtr)
    {
        int ival = IValue;
        *this = *RValue;
        IValue += ival;
    }

    if (Ptr)
    {
        Ptr += IValue;
        IValue = 0;
    }
    return *this;
}
