
#include <string.h>
#include "ac/statobj/agsstaticobject.h"

AGSStaticObject GlobalStaticManager;

void AGSStaticObject::Read(const char *address, int offset, void *dest, int size)
{
    memcpy(dest, address + offset, size);
}

uint8_t AGSStaticObject::ReadInt8(const char *address, long offset)
{
    return *(uint8_t*)(address + offset);
}

int16_t AGSStaticObject::ReadInt16(const char *address, long offset)
{
    return *(int16_t*)(address + offset);
}

int32_t AGSStaticObject::ReadInt32(const char *address, long offset)
{
    return *(int32_t*)(address + offset);
}

float AGSStaticObject::ReadFloat(const char *address, long offset)
{
    return *(float*)(address + offset);
}

void AGSStaticObject::Write(const char *address, int offset, void *src, int size)
{
    memcpy((void*)(address + offset), src, size);
}

void AGSStaticObject::WriteInt8(const char *address, long offset, uint8_t val)
{
    *(uint8_t*)(address + offset) = val;
}

void AGSStaticObject::WriteInt16(const char *address, long offset, int16_t val)
{
    *(int16_t*)(address + offset) = val;
}

void AGSStaticObject::WriteInt32(const char *address, long offset, int32_t val)
{
    *(int32_t*)(address + offset) = val;
}

void AGSStaticObject::WriteFloat(const char *address, long offset, float val)
{
    *(float*)(address + offset) = val;
}
