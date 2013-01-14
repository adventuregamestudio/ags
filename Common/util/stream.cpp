
#include "util/stream.h"

namespace AGS
{
namespace Common
{

size_t Stream::ReadArrayOfIntPtr32(intptr_t *buffer, size_t count)
{
    // Read 32-bit values to array; this will always be safe,
    // because array is either 32-bit or 64-bit; in the last
    // case only first half of array will be used.
    count = ReadArrayOfInt32((int32_t*)buffer, count);

    if (count == 0)
    {
        return 0;
    }

#if defined (AGS_64BIT) || defined (TEST_64BIT)
    {
        // If we need 64-bit array, then copy 32-bit values to their
        // correct 64-bit slots, starting from the last element and
        // moving towards array head.
        int32_t *buffer32 = (int32_t*)buffer;
        buffer   += count - 1;
        buffer32 += count - 1;
        for (int i = count - 1; i >= 0; --i, --buffer, --buffer32)
        {
            *buffer = *buffer32 & 0xFFFFFFFF;
        }
    }
#endif // AGS_64BIT
    return count;
}

size_t Stream::WriteArrayOfIntPtr32(const intptr_t *buffer, size_t count)
{
    if (!CanWrite())
    {
        return 0;
    }

    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int32_t val = (int32_t)*buffer;
        if (WriteInt32(val) < sizeof(int32_t))
        {
            break;
        }
    }
    return elem;
}

} // namespace Common
} // namespace AGS
