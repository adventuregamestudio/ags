
#include "ac/statobj/agsstaticobject.h"

AGSStaticObject GlobalStaticManager;

uint8_t AGSStaticObject::GetPropertyUInt8(const char *address, intptr_t offset)
{
    return *(uint8_t*)(address + offset);
}

int16_t AGSStaticObject::GetPropertyInt16(const char *address, intptr_t offset)
{
    return *(uint16_t*)(address + offset);
}

int32_t AGSStaticObject::GetPropertyInt32(const char *address, intptr_t offset)
{
    return *(uint32_t*)(address + offset);
}

void AGSStaticObject::SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value)
{
    *(uint8_t*)(address + offset) = value;
}

void AGSStaticObject::SetPropertyInt16(const char *address, intptr_t offset, int16_t value)
{
    *(int16_t*)(address + offset) = value;
}

void AGSStaticObject::SetPropertyInt32(const char *address, intptr_t offset, int32_t value)
{
    *(int32_t*)(address + offset) = value;
}
