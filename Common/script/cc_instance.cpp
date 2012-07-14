
#include <stdio.h>
#include "cc_instance.h"

ccInstance *current_instance;

ccInstance *ccGetCurrentInstance()
{
    return current_instance;
}
