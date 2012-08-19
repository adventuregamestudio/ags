
#ifdef _DEBUG

#include <string.h>
#include "util/string.h"
#include "test/test_internal.h"

using AGS::Common::CString;

void Test_String()
{
    //-----------------------------------------------------
    // Operations
    CString s1;
    CString s2 = "waddya want?";
    CString s3 = "nothing at all";
    s2.Append(s3);
    s1 = s3;

    int s1_cmp_s2 = s1.Compare(s2);
    int s1_cmp_s3 = s1.Compare(s3);
    int s2_cmp_s3 = s2.Compare(s3);

    CString s4 = s1;
    s4.SetAt(8, 'b');

    CString s_left = s4.Left(4);
    CString s_mid1 = s4.Mid(4);
    CString s_mid2 = s4.Mid(4, 4);
    CString s_right = s4.Right(4);

    CString fmts;
    fmts.Format("Make %d me %.2f some %03d format, %s",
        1, 2.5, 22, "bro");
    CString made_string = CString::MakeString("This is a %s test number %d", "string", 5);

    CString direct_write = "initial value";
    char * buff = direct_write.GetBuffer(100);
    char keep_buff[100];
    strcpy(keep_buff, buff);
    strcpy(buff, "new value new value new value");
    direct_write.ReleaseBuffer();

    CString filled1('a',5);
    CString filled2;
    filled2.FillString('b',6);

    //-----------------------------------------------------
    // Assertions
    assert(s1.GetLength() == 14);
    assert(s2.GetLength() == 26);
    assert(s3.GetLength() == 14);

    assert(strcmp(s1.GetCStr(), "nothing at all") == 0);
    assert(strcmp(s2.GetCStr(), "waddya want?nothing at all") == 0);
    assert(strcmp(s3.GetCStr(), "nothing at all") == 0);

    assert(s1_cmp_s2 < 0);
    assert(s1_cmp_s3 == 0);
    assert(s2_cmp_s3 > 0);

    assert(s4[8] == 'b');
    assert(strcmp(s4.GetCStr(), "nothing bt all") == 0);

    assert(strcmp(s_left.GetCStr(), "noth") == 0);
    assert(strcmp(s_mid1.GetCStr(), "ing bt all") == 0);
    assert(strcmp(s_mid2.GetCStr(), "ing ") == 0);
    assert(strcmp(s_right.GetCStr(), " all") == 0);

    assert(strcmp(fmts.GetCStr(), "Make 1 me 2.50 some 022 format, bro") == 0);
    assert(strcmp(made_string.GetCStr(), "This is a string test number 5") == 0);

    assert(strcmp(keep_buff, "initial value") == 0);
    assert(strcmp(direct_write.GetCStr(), "new value new value new value") == 0);

    assert(strcmp(filled1.GetCStr(), "aaaaa") == 0);
    assert(strcmp(filled2.GetCStr(), "bbbbbb") == 0);
}

#endif // _DEBUG
