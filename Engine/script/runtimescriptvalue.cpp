//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "script/runtimescriptvalue.h"
#include <string.h> // for memcpy()
#include "ac/dynobj/cc_scriptobject.h"
#include "util/memory.h"

using namespace AGS::Common;

//
// NOTE to future optimizers: I am using 'this' ptr here to better
// distinguish Runtime Values.
//

uint8_t RuntimeScriptValue::ReadByte() const
{
    switch (this->Type)
    {
    case kScValStackPtr:
    case kScValGlobalVar:
        if (RValue->Type == kScValData)
        {
            return *(uint8_t*)(GetRValuePtrWithOffset());
        }
        else
        {
            return static_cast<uint8_t>(RValue->IValue);
        }
    case kScValStaticArray:
    case kScValScriptObject:
        return this->ObjMgr->ReadInt8(this->Ptr, this->IValue);
    default:
        return *((uint8_t*)this->GetPtrWithOffset());
    }
}

int16_t RuntimeScriptValue::ReadInt16() const
{
    switch (this->Type)
    {
    case kScValStackPtr:
        if (RValue->Type == kScValData)
        {
            return *(int16_t*)(GetRValuePtrWithOffset());
        }
        else
        {
            return static_cast<int16_t>(RValue->IValue);
        }
    case kScValGlobalVar:
        if (RValue->Type == kScValData)
        {
            return Memory::ReadInt16LE(GetRValuePtrWithOffset());
        }
        else
        {
            return static_cast<int16_t>(RValue->IValue);
        }
    case kScValStaticArray:
    case kScValScriptObject:
        return this->ObjMgr->ReadInt16(this->Ptr, this->IValue);
    default:
        return *((int16_t*)this->GetPtrWithOffset());
    }
}

int32_t RuntimeScriptValue::ReadInt32() const
{
    switch (this->Type)
    {
    case kScValStackPtr:
        if (RValue->Type == kScValData)
        {
            return *(int32_t*)(GetRValuePtrWithOffset());
        }
        else
        {
            return static_cast<int32_t>(RValue->IValue);
        }
    case kScValGlobalVar:
        if (RValue->Type == kScValData)
        {
            return Memory::ReadInt32LE(GetRValuePtrWithOffset());
        }
        else
        {
            return static_cast<uint32_t>(RValue->IValue);
        }
    case kScValStaticArray:
    case kScValScriptObject:
        return this->ObjMgr->ReadInt32(this->Ptr, this->IValue);
    default:
        return *((int32_t*)this->GetPtrWithOffset());
    }
}

void RuntimeScriptValue::WriteByte(uint8_t val)
{
    switch (this->Type)
    {
    case kScValStackPtr:
    case kScValGlobalVar:
        if (RValue->Type == kScValData)
        {
            *(uint8_t*)(GetRValuePtrWithOffset()) = val;
        }
        else
        {
            RValue->SetUInt8(val); // set RValue as int
        }
        break;
    case kScValStaticArray:
    case kScValScriptObject:
        this->ObjMgr->WriteInt8(this->Ptr, this->IValue, val);
        break;
    default:
        *((uint8_t*)this->GetPtrWithOffset()) = val;
        break;
    }
}

void RuntimeScriptValue::WriteInt16(int16_t val)
{
    switch (this->Type)
    {
    case kScValStackPtr:
        if (RValue->Type == kScValData)
        {
            *(int16_t*)(GetRValuePtrWithOffset()) = val;
        }
        else
        {
            RValue->SetInt16(val); // set RValue as int
        }
        break;
    case kScValGlobalVar:
        if (RValue->Type == kScValData)
        {
            Memory::WriteInt16LE(GetRValuePtrWithOffset(), val);
        }
        else
        {
            RValue->SetInt16(val); // set RValue as int
        }
        break;
    case kScValStaticArray:
    case kScValScriptObject:
        this->ObjMgr->WriteInt16(this->Ptr, this->IValue, val);
        break;
    default:
        *((int16_t*)this->GetPtrWithOffset()) = val;
        break;
    }
}

void RuntimeScriptValue::WriteInt32(int32_t val)
{
    switch (this->Type)
    {
    case kScValStackPtr:
        if (RValue->Type == kScValData)
        {
            *(int32_t*)(GetRValuePtrWithOffset()) = val;
        }
        else
        {
            RValue->SetInt32(val); // set RValue as int
        }
        break;
    case kScValGlobalVar:
        if (RValue->Type == kScValData)
        {
            Memory::WriteInt32LE(GetRValuePtrWithOffset(), val);
        }
        else
        {
            RValue->SetInt32(val); // set RValue as int
        }
        break;
    case kScValStaticArray:
    case kScValScriptObject:
        this->ObjMgr->WriteInt32(this->Ptr, this->IValue, val);
        break;
    default:
        *((int32_t*)this->GetPtrWithOffset()) = val;
        break;
    }
}

RuntimeScriptValue &RuntimeScriptValue::DirectPtr()
{
    if (Type == kScValGlobalVar || Type == kScValStackPtr)
    {
        int ival = IValue;
        *this = *RValue;
        IValue += ival;
    }

    if (Ptr)
    {
        if (Type == kScValScriptObject)
            Ptr = ObjMgr->GetFieldPtr(Ptr, IValue);
        else
            Ptr = PtrU8 + IValue;
        IValue = 0;
    }
    return *this;
}

RuntimeScriptValue &RuntimeScriptValue::DirectPtrObj()
{
    if (Type == kScValGlobalVar || Type == kScValStackPtr)
        *this = *RValue;
    return *this;
}

void *RuntimeScriptValue::GetDirectPtr() const
{
    const RuntimeScriptValue *temp_val = this;
    int ival = temp_val->IValue;
    if (temp_val->Type == kScValGlobalVar || temp_val->Type == kScValStackPtr)
    {
        temp_val  = temp_val->RValue;
        ival     += temp_val->IValue;
    }
    if (temp_val->Type == kScValScriptObject)
        return temp_val->ObjMgr->GetFieldPtr(temp_val->Ptr, ival);
    else
        return temp_val->PtrU8 + ival;
}
