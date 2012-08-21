
#ifdef _DEBUG

#include "test/test_string.h"
#include "test/test_file.h"

void Test_DoAllTests()
{
    Test_String();
    Test_File();
}

#endif // _DEBUG
