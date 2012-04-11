// Functions missing in the OSX Standard C library.

#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>

// Only a dummy. It is used in a function of alfont, but never called in AGS.
size_t malloc_usable_size(void* allocation)
{
  return 0;
}
