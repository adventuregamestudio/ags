#include <iostream>  
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"


// Note: This file contains googletests that check that the
// pre-rewrite parser generates exactly the same bytes as the
// rewritten parser. 
// This file is for testing correct AGS code, i.e., AGS code that is
// parsed into bytecode. (Incorrect AGS code that results in an error
// message instead of bytecode should be checked elsewhere.)
//
// The tests are created as follows:
// 1. Go to the branch of the pre-rewrite parser.
// 2. Copy a prototype (search for "PROTOTYPE") to the end of this file.
// 3. Edit the name of the googletest and the filename in the prototype.
// 4. Change the AGS code in the prototype, 
//      i.e., create what is to be checked.
// 5. Uncomment the "writeoutput" line.
// 6. Run the test.
//    This will write a file to disk.
// 7. Open the disk file, copy all its lines to the clipboard
//    and paste them directly above the "}" at the end of the googletest.
// 8. Comment the "writeoutput" line.
// 9. Copy this whole googletest file into the clipboard.
// 10. Go to the branch of the rewritten parser.
// 11. Open this file and replace its complete content with the clipboard
// 12. Run the tests. They will fail if the rewritten parser generates
//       any bytes that are different from the pre-rewrite bytecode.


// If tests fail, a good way of debugging is uncommenting 
// the "writeoutput" line - this will write the bytecode that the 
// rewritten parser generates to file. Compare that by hand to the bytecode
// in the test and find out whether bytes are left out, changed, 
// or added. Make the debugger break in cc_compiledscript.cpp function
// ccCompiledScript::write_code() at the point where the last correct 
// byte is generated. Then trace along and see where the logic fails.
// Note that sometimes the parser generates some bytes, then stashes
// them away, then generates other bytes in their place.


extern ccCompiledScript *newScriptFixture();
extern const char *last_seen_cc_error();
extern void clear_error();

void writeoutput(char *fname, ccCompiledScript *scrip)
{
    std::string path = "C:\\TEMP\\";
    std::ofstream of;
    of.open(path.append(fname).append(".txt"));

    // export the code
    of << "const size_t codesize = " << scrip->codesize << ";" << std::endl;
    of << "EXPECT_EQ(codesize, scrip->codesize);" << std::endl << std::endl;

    if (scrip->codesize > 0)
    {
        of << "intptr_t code[] = {" << std::endl;
        for (size_t idx = 0; idx < scrip->codesize; idx++)
        {
            of.width(4);
            of << scrip->code[idx] << ", ";
            if (idx % 8 == 3) of << "        ";
            if (idx % 8 == 7) of << "   // " << idx << std::endl;
        }
        of << " -999 " << std::endl << "};" << std::endl << std::endl;

        of << "for (size_t idx = 0; idx < codesize; idx++)" << std::endl;
        of << "{" << std::endl;
        of << "     std::string prefix = \"code[\";" << std::endl;
        of << "     prefix += (std::to_string(idx)) + std::string(\"] == \");" << std::endl;
        of << "     std::string is_val = prefix + std::to_string(code[idx]);" << std::endl;
        of << "     std::string test_val = prefix + std::to_string(scrip->code[idx]);" << std::endl;
        of << "     ASSERT_EQ(is_val, test_val);" << std::endl;
        of << "}" << std::endl;
    }
    // export the fixups
    of << "const size_t numfixups = " << scrip->numfixups << ";" << std::endl;
    of << "EXPECT_EQ(numfixups, scrip->numfixups);" << std::endl << std::endl;

    if (scrip->numfixups > 0)
    {
        of << "intptr_t fixups[] = {" << std::endl;
        for (size_t idx = 0; idx < scrip->numfixups; idx++)
        {
            of.width(4);
            of << scrip->fixups[idx] << ", ";
            if (idx % 8 == 3) of << "      ";
            if (idx % 8 == 7) of << std::endl;
        }
        of << " -999 " << std::endl << "};" << std::endl << std::endl;

        of << "for (size_t idx = 0; idx < numfixups; idx++)" << std::endl;
        of << "{" << std::endl;
        of << "     std::string prefix = \"fixups[\";" << std::endl;
        of << "     prefix += (std::to_string(idx)) + std::string(\"] == \");" << std::endl;
        of << "     std::string   is_val = prefix + std::to_string(fixups[idx]);" << std::endl;
        of << "     std::string test_val = prefix + std::to_string(scrip->fixups[idx]);" << std::endl;
        of << "     ASSERT_EQ(is_val, test_val);" << std::endl;
        of << "}" << std::endl << std::endl;

        of << "char fixuptypes[] = {" << std::endl;
        for (size_t idx = 0; idx < scrip->numfixups; idx++)
        {
            of.width(3);
            of << static_cast<int>(scrip->fixuptypes[idx]) << ", ";
            if (idx % 8 == 3) of << "   ";
            if (idx % 8 == 7) of << std::endl;
        }
        of << " '\\0' " << std::endl << "};" << std::endl << std::endl;

        of << "for (size_t idx = 0; idx < numfixups; idx++)" << std::endl;
        of << "{" << std::endl;
        of << "     std::string prefix = \"fixuptypes[\";" << std::endl;
        of << "     prefix += (std::to_string(idx)) + std::string(\"] == \");" << std::endl;
        of << "     std::string   is_val = prefix + std::to_string(fixuptypes[idx]);" << std::endl;
        of << "     std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);" << std::endl;
        of << "     ASSERT_EQ(is_val, test_val);" << std::endl;
        of << "}" << std::endl;
    }
    of.close();
}

/*    PROTOTYPE
TEST(Compatibility, p_r_o_t_o_t_y_p_e) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo(int a)      \n\
        {                   \n\
            return a*a;     \n\
        }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("P_R_O_T_O_T_Y_P_E", scrip);
    // run the test, comment out the previous line
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

}
*/

TEST(Compatibility, SimpleVoidFunction) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        void Foo()          \n\
        {                   \n\
            return;         \n\
        }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("SimpleVoidFunction", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 10;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,    5,    6,    3,
       0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

}

TEST(Compatibility, SimpleIntFunction) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo()      \n\
    {                  \n\
        return 15;     \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("SimpleIntFunction", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 10;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,           15,    5,    6,    3,
       0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

}

TEST(Compatibility, IntFunctionLocalV) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo()      \n\
    {                  \n\
        int a = 15;    \n\
        return a;     \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    //writeoutput("IntFunctionLocalV", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 28;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,           15,    3,    1,    2,
       8,    3,    1,    1,            4,   51,    4,    7,
       3,    2,    1,    4,            5,    6,    3,    0,
       2,    1,    4,    5,          -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

}

TEST(Compatibility, IntFunctionParam) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo(int a) \n\
    {                  \n\
        return a;      \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("IntFunctionParam", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 11;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   51,    8,            7,    3,    5,    6,
       3,    0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);
}

TEST(Compatibility, IntFunctionGlobalV) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int a = 15;    \n\
        int Foo( )     \n\
    {                  \n\
        return a;      \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("IntFunctionGlobalV", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 12;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    2,            0,    7,    3,    5,
       6,    3,    0,    5,          -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    ASSERT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
       4,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

}


TEST(Compatibility, FloatExpr1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        float a = 15.0;     \n\
        float Foo()     \n\
    {                   \n\
        float f = 3.14; \n\
        return a + f;   \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("FloatExpr1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 43;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,         1078523331,    3,    1,    2,
       8,    3,    1,    1,            4,    6,    2,    0,
       7,    3,   29,    3,           51,    8,    7,    3,
      30,    4,   57,    4,            3,    3,    4,    3,
       2,    1,    4,    5,            6,    3,    0,    2,
       1,    4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    ASSERT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      15,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

}


TEST(Compatibility, FloatExpr2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        float a = 15.0;           \n\
        float Foo()               \n\
    {                             \n\
        float b = 22.2;           \n\
        int E1 = (3.14 < 1.34) == 1;         \n\
        short E2 = 0 == (1234.5 > 5.0) && 1; \n\
        long E3 = a <= 44.4;      \n\
        char E4 = 55.5 >= 44.4;   \n\
        int E5 = (((a == b) || (a != b))); \n\
        return a - b * (a / b);   \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("FloatExpr2", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 269;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,         1102158234,    3,    1,    2,    // 7
       8,    3,    1,    1,            4,    6,    3, 1078523331,    // 15
      29,    3,    6,    3,         1068205343,   30,    4,   60,    // 23
       4,    3,    3,    4,            3,   29,    3,    6,    // 31
       3,    1,   30,    4,           15,    4,    3,    3,    // 39
       4,    3,    3,    1,            2,    8,    3,    1,    // 47
       1,    4,    6,    3,            0,   29,    3,    6,    // 55
       3, 1150963712,   29,    3,            6,    3, 1084227584,   30,    // 63
       4,   59,    4,    3,            3,    4,    3,   30,    // 71
       4,   15,    4,    3,            3,    4,    3,   28,    // 79
      13,   29,    3,    6,            3,    1,   30,    4,    // 87
      21,    4,    3,    3,            4,    3,    3,    1,    // 95
       2,   27,    3,    1,            1,    2,    6,    2,    // 103
       0,    7,    3,   29,            3,    6,    3, 1110546842,    // 111
      30,    4,   62,    4,            3,    3,    4,    3,    // 119
       3,    1,    2,    8,            3,    1,    1,    4,    // 127
       6,    3, 1113456640,   29,            3,    6,    3, 1110546842,    // 135
      30,    4,   61,    4,            3,    3,    4,    3,    // 143
       3,    1,    2,   26,            3,    1,    1,    1,    // 151
       6,    2,    0,    7,            3,   29,    3,   51,    // 159
      19,    7,    3,   30,            4,   15,    4,    3,    // 167
       3,    4,    3,   70,           29,   29,    3,    6,    // 175
       2,    0,    7,    3,           29,    3,   51,   23,    // 183
       7,    3,   30,    4,           16,    4,    3,    3,    // 191
       4,    3,   30,    4,           22,    4,    3,    3,    // 199
       4,    3,    3,    1,            2,    8,    3,    1,    // 207
       1,    4,    6,    2,            0,    7,    3,   29,    // 215
       3,   51,   23,    7,            3,   29,    3,    6,    // 223
       2,    0,    7,    3,           29,    3,   51,   31,    // 231
       7,    3,   30,    4,           56,    4,    3,    3,    // 239
       4,    3,   30,    4,           55,    4,    3,    3,    // 247
       4,    3,   30,    4,           58,    4,    3,    3,    // 255
       4,    3,    2,    1,           19,    5,    6,    3,    // 263
       0,    2,    1,   19,            5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
     104,  154,  177,  212,        225,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

}

TEST(Compatibility, IfThenElse1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Foo()               \n\
    {                       \n\
        int a = 15 - 4 * 2; \n\
        if (a < 5)          \n\
            a >>= 2;        \n\
        else                \n\
            a <<= 3;        \n\
        return a;           \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("IfThenElse1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 111;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,           15,   29,    3,    6,
       3,    4,   29,    3,            6,    3,    2,   30,
       4,    9,    4,    3,            3,    4,    3,   30,
       4,   12,    4,    3,            3,    4,    3,    3,
       1,    2,    8,    3,            1,    1,    4,   51,
       4,    7,    3,   29,            3,    6,    3,    5,
      30,    4,   18,    4,            3,    3,    4,    3,
      28,   20,    6,    3,            2,   29,    3,   51,
       8,    7,    3,   30,            4,   44,    3,    4,
      51,    4,    8,    3,           31,   18,    6,    3,
       3,   29,    3,   51,            8,    7,    3,   30,
       4,   43,    3,    4,           51,    4,    8,    3,
      51,    4,    7,    3,            2,    1,    4,    5,
       6,    3,    0,    2,            1,    4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

}


TEST(Compatibility, IfThenElse2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Foo()               \n\
    {                       \n\
        int a = 15 - 4 % 2; \n\
        if (a >= 5) {       \n\
            a -= 2;         \n\
        } else {            \n\
            a += 3;         \n\
        }                   \n\
        return a;           \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("IfThenElse2", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 111;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,           15,   29,    3,    6,
       3,    4,   29,    3,            6,    3,    2,   30,
       4,   40,    4,    3,            3,    4,    3,   30,
       4,   12,    4,    3,            3,    4,    3,    3,
       1,    2,    8,    3,            1,    1,    4,   51,
       4,    7,    3,   29,            3,    6,    3,    5,
      30,    4,   19,    4,            3,    3,    4,    3,
      28,   20,    6,    3,            2,   29,    3,   51,
       8,    7,    3,   30,            4,   12,    3,    4,
      51,    4,    8,    3,           31,   18,    6,    3,
       3,   29,    3,   51,            8,    7,    3,   30,
       4,   11,    3,    4,           51,    4,    8,    3,
      51,    4,    7,    3,            2,    1,    4,    5,
       6,    3,    0,    2,            1,    4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
     -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
     '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}

TEST(Compatibility, While) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    char c = 'x';             \n\
    int Foo(int i, float f)   \n\
    {                         \n\
        int sum = 0;          \n\
        while (c >= 0)        \n\
        {                     \n\
            sum += (500 & c); \n\
            c--;              \n\
            if (c == 1) continue; \n\
        }                     \n\
        return sum;           \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("While", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 118;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,    3,    1,    2,
       8,    3,    1,    1,            4,    6,    2,    0,
      24,    3,   29,    3,            6,    3,    0,   30,
       4,   19,    4,    3,            3,    4,    3,   28,
      70,    6,    3,  500,           29,    3,    6,    2,
       0,   24,    3,   30,            4,   13,    4,    3,
       3,    4,    3,   29,            3,   51,    8,    7,
       3,   30,    4,   11,            3,    4,   51,    4,
       8,    3,    6,    2,            0,   24,    3,    2,
       3,    1,   26,    3,            6,    2,    0,   24,
       3,   29,    3,    6,            3,    1,   30,    4,
      15,    4,    3,    3,            4,    3,   28,    5,
       6,    3,    0,   31,          -88,   31,  -90,   51,
       4,    7,    3,    2,            1,    4,    5,    6,
       3,    0,    2,    1,            4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 4;
    ASSERT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      15,   40,   68,   78,        -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   1,   1,     '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

}


TEST(Compatibility, DoNCall) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    char c = 'x';             \n\
    int Foo(int i)            \n\
    {                         \n\
        int sum = 0;          \n\
        do                    \n\
        {                     \n\
            sum -= (500 | c); \n\
            c--;              \n\
        }                     \n\
        while (c > 0);        \n\
        return sum;           \n\
    }                         \n\
                              \n\
    int Bar(int x)            \n\
    {                         \n\
        return Foo(x^x);      \n\
    }                         \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("DoNCall", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 130;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,    3,    1,    2,
       8,    3,    1,    1,            4,   31,    2,   31,
      63,    6,    3,  500,           29,    3,    6,    2,
       0,   24,    3,   30,            4,   14,    4,    3,
       3,    4,    3,   29,            3,   51,    8,    7,
       3,   30,    4,   12,            3,    4,   51,    4,
       8,    3,    6,    2,            0,   24,    3,    2,
       3,    1,   26,    3,            6,    2,    0,   24,
       3,   29,    3,    6,            3,    0,   30,    4,
      17,    4,    3,    3,            4,    3,   70,  -63,
      51,    4,    7,    3,            2,    1,    4,    5,
       6,    3,    0,    2,            1,    4,    5,   38,
      95,   51,    8,    7,            3,   29,    3,   51,
      12,    7,    3,   30,            4,   41,    4,    3,
       3,    4,    3,   29,            3,    6,    3,    0,
      23,    3,    2,    1,            4,    5,    6,    3,
       0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 4;
    ASSERT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      24,   52,   62,  119,        -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   1,   2,     '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

}

TEST(Compatibility, For1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int loop;                       \n\
    int Foo(int i, float f)         \n\
    {                               \n\
        for (loop = 0; loop < 10; loop += 3)  \n\
        {                           \n\
            int sum = loop - 4 - 7; \n\
            if (loop == 6) break;   \n\
        }                           \n\
        return 0;                   \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("For1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 130;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,    6,    2,    0,
       8,    3,    6,    2,            0,    7,    3,   29,
       3,    6,    3,   10,           30,    4,   18,    4,
       3,    3,    4,    3,           28,   92,    6,    2,
       0,    7,    3,   29,            3,    6,    3,    4,
      30,    4,   12,    4,            3,    3,    4,    3,
      29,    3,    6,    3,            7,   30,    4,   12,
       4,    3,    3,    4,            3,    3,    1,    2,
       8,    3,    1,    1,            4,    6,    2,    0,
       7,    3,   29,    3,            6,    3,    6,   30,
       4,   15,    4,    3,            3,    4,    3,   28,
       8,    2,    1,    4,            6,    3,    0,   31,
     -69,    2,    1,    4,            6,    3,    3,   29,
       3,    6,    2,    0,            7,    3,   30,    4,
      11,    3,    4,    6,            2,    0,    8,    3,
      31, -112,    6,    3,            0,    5,    6,    3,
       0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 6;
    ASSERT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
       7,   12,   32,   71,        107,  117,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, For2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Foo(int i, float f)         \n\
    {                               \n\
        int lp, sum;                \n\
        for (; ; lp += 1)           \n\
            sum += lp;              \n\
        for ( ;; )                  \n\
            sum -= lp;              \n\
        for (; lp < 2; lp += 3)     \n\
            sum *= lp;              \n\
        for (; lp < 4; )            \n\
            sum /= lp;              \n\
        for (lp = 5; ; lp += 6)     \n\
            sum /= lp;              \n\
        for (int loop = 7; ; )      \n\
            sum &= loop;            \n\
        for (int loop = 8; loop < 9; )  \n\
            sum |= loop;            \n\
        return 0;                   \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("For2", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 345;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,    4,    1,    // 7
       1,    4,    3,    1,            2,   63,    4,    1,    // 15
       1,    4,    6,    3,            1,   28,   39,   51,    // 23
       8,    7,    3,   29,            3,   51,    8,    7,    // 31
       3,   30,    4,   11,            3,    4,   51,    4,    // 39
       8,    3,    6,    3,            1,   29,    3,   51,    // 47
      12,    7,    3,   30,            4,   11,    3,    4,    // 55
      51,    8,    8,    3,           31,  -44,    6,    3,    // 63
       1,   28,   21,   51,            8,    7,    3,   29,    // 71
       3,   51,    8,    7,            3,   30,    4,   12,    // 79
       3,    4,   51,    4,            8,    3,   31,  -26,    // 87
      51,    8,    7,    3,           29,    3,    6,    3,    // 95
       2,   30,    4,   18,            4,    3,    3,    4,    // 103
       3,   28,   39,   51,            8,    7,    3,   29,    // 111
       3,   51,    8,    7,            3,   30,    4,    9,    // 119
       3,    4,   51,    4,            8,    3,    6,    3,    // 127
       3,   29,    3,   51,           12,    7,    3,   30,    // 135
       4,   11,    3,    4,           51,    8,    8,    3,    // 143
      31,  -58,   51,    8,            7,    3,   29,    3,    // 151
       6,    3,    4,   30,            4,   18,    4,    3,    // 159
       3,    4,    3,   28,           21,   51,    8,    7,    // 167
       3,   29,    3,   51,            8,    7,    3,   30,    // 175
       4,   10,    3,    4,           51,    4,    8,    3,    // 183
      31,  -40,    6,    3,            5,   51,    8,    8,    // 191
       3,    6,    3,    1,           28,   39,   51,    8,    // 199
       7,    3,   29,    3,           51,    8,    7,    3,    // 207
      30,    4,   10,    3,            4,   51,    4,    8,    // 215
       3,    6,    3,    6,           29,    3,   51,   12,    // 223
       7,    3,   30,    4,           11,    3,    4,   51,    // 231
       8,    8,    3,   31,          -44,    6,    3,    7,    // 239
       3,    1,    2,    8,            3,    1,    1,    4,    // 247
       6,    3,    1,   28,           21,   51,    4,    7,    // 255
       3,   29,    3,   51,           12,    7,    3,   30,    // 263
       4,   13,    3,    4,           51,    8,    8,    3,    // 271
      31,  -26,    2,    1,            4,    6,    3,    8,    // 279
       3,    1,    2,    8,            3,    1,    1,    4,    // 287
      51,    4,    7,    3,           29,    3,    6,    3,    // 295
       9,   30,    4,   18,            4,    3,    3,    4,    // 303
       3,   28,   21,   51,            4,    7,    3,   29,    // 311
       3,   51,   12,    7,            3,   30,    4,   14,    // 319
       3,    4,   51,    8,            8,    3,   31,  -40,    // 327
       2,    1,    4,    6,            3,    0,    2,    1,    // 335
       8,    5,    6,    3,            0,    2,    1,    8,    // 343
       5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);


}


TEST(Compatibility, For3) {
    ccCompiledScript *scrip = newScriptFixture();



    char *inpl = "\
    managed struct Struct           \n\
    {                               \n\
        float Payload[1];           \n\
    };                              \n\
    Struct *S;                      \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        for (Struct *loop; ;)       \n\
        {                           \n\
            return ((loop == S));   \n\
        }                           \n\
        return -7;                  \n\
    }                               \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("For3", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 65;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,    4,    1,    // 7
       1,    4,    6,    3,            1,   28,   36,   51,    // 15
       4,   29,    2,   30,            2,   48,    3,   29,    // 23
       3,    6,    2,    0,           29,    2,   30,    2,    // 31
      48,    3,   30,    4,           15,    4,    3,    3,    // 39
       4,    3,   51,    4,           69,    2,    1,    4,    // 47
       5,   31,  -41,   51,            4,   49,    2,    1,    // 55
       4,    6,    3,   -7,            5,    6,    3,    0,    // 63
       5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      27,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, IfDoWhile) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Foo(int i, float f)                      \n\
    {                                            \n\
        int five = 5, sum, loop = -2;            \n\
        if (five < 10)                           \n\
            for (loop = 0; loop < 10; loop += 3) \n\
            {                                    \n\
                sum += loop;                     \n\
                if (loop == 6) return loop;      \n\
            }                                    \n\
        else                                     \n\
            do { loop += 1; } while (loop < 100);   \n\
        return 0;                                \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("IfDoWhile", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 200;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            5,    3,    1,    2,
       8,    3,    1,    1,            4,    3,    1,    2,
      63,    4,    1,    1,            4,    6,    3,   -2,
       3,    1,    2,    8,            3,    1,    1,    4,
      51,   12,    7,    3,           29,    3,    6,    3,
      10,   30,    4,   18,            4,    3,    3,    4,
       3,   28,   94,    6,            3,    0,   51,    4,
       8,    3,   51,    4,            7,    3,   29,    3,
       6,    3,   10,   30,            4,   18,    4,    3,
       3,    4,    3,   28,           66,   51,    4,    7,
       3,   29,    3,   51,           12,    7,    3,   30,
       4,   11,    3,    4,           51,    8,    8,    3,
      51,    4,    7,    3,           29,    3,    6,    3,
       6,   30,    4,   15,            4,    3,    3,    4,
       3,   28,    8,   51,            4,    7,    3,    2,
       1,   12,    5,    6,            3,    3,   29,    3,
      51,    8,    7,    3,           30,    4,   11,    3,
       4,   51,    4,    8,            3,   31,  -85,   31,
      41,   31,    2,   31,           37,    6,    3,    1,
      29,    3,   51,    8,            7,    3,   30,    4,
      11,    3,    4,   51,            4,    8,    3,   51,
       4,    7,    3,   29,            3,    6,    3,  100,
      30,    4,   18,    4,            3,    3,    4,    3,
      70,  -37,    6,    3,            0,    2,    1,   12,
       5,    6,    3,    0,            2,    1,   12,    5,
     -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

}

TEST(Compatibility, Switch) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Foo(int i, float f)         \n\
    {                               \n\
        switch (i * i)              \n\
        {                           \n\
        case 2: return 10; break;   \n\
        default: i *= 2; return i;  \n\
        case 3:                     \n\
        case 4: i = 0;              \n\
        case 5: i += 5 - i - 4;  break; \n\
        }                           \n\
        return 0;                   \n\
    }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Switch", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 174;
    ASSERT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,
      51,   12,    7,    3,           30,    4,    9,    4,
       3,    3,    4,    3,            3,    3,    4,   31,
      91,   31,  139,    6,            3,   10,    5,    6,
       3,    0,   31,  -11,            6,    3,    2,   29,
       3,   51,   12,    7,            3,   30,    4,    9,
       3,    4,   51,    8,            8,    3,   51,    8,
       7,    3,    5,    6,            3,    0,   51,    8,
       8,    3,    6,    3,            5,   29,    3,   51,
      12,    7,    3,   30,            4,   12,    4,    3,
       3,    4,    3,   29,            3,    6,    3,    4,
      30,    4,   12,    4,            3,    3,    4,    3,
      29,    3,   51,   12,            7,    3,   30,    4,
      11,    3,    4,   51,            8,    8,    3,    6,
       3,    0,   31,   50,           29,    4,    6,    3,
       2,   30,    4,   16,            3,    4,   28, -101,
      29,    4,    6,    3,            3,   30,    4,   16,
       3,    4,   28,  -81,           29,    4,    6,    3,
       4,   30,    4,   16,            3,    4,   28,  -93,
      29,    4,    6,    3,            5,   30,    4,   16,
       3,    4,   28,  -98,           31, -130,    6,    3,
       0,    5,    6,    3,            0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    ASSERT_EQ(numfixups, scrip->numfixups);

}

TEST(Compatibility, FreeLocalPtr) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct S                  \n\
    {                                 \n\
        int i;                        \n\
    };                                \n\
                                      \n\
    int foo()                         \n\
    {                                 \n\
        S *sptr = new S;              \n\
                                      \n\
        for (int i = 0; i < 10; i++)  \n\
            sptr = new S;             \n\
    }                                 \n\
    ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("FreeLocalPtr", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 78;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   73,    3,            4,    3,    1,    2,    // 7
      50,    3,    1,    1,            4,    6,    3,    0,    // 15
       3,    1,    2,    8,            3,    1,    1,    4,    // 23
      51,    4,    7,    3,           29,    3,    6,    3,    // 31
      10,   30,    4,   18,            4,    3,    3,    4,    // 39
       3,   28,   22,   73,            3,    4,   51,    8,    // 47
      29,    2,   30,    2,           47,    3,   51,    4,    // 55
       7,    3,    1,    3,            1,    8,    3,   31,    // 63
     -41,    2,    1,    4,            6,    3,    0,   51,    // 71
       4,   69,    2,    1,            4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);

}

TEST(Compatibility, Strings1) {
#include "script/cc_options.h"
    ccSetOption(SCOPT_OLDSTRINGS, true);
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        string GLOBAL; \
\
        string MyFunction(int a)\
        {\
            string x;\
            return GLOBAL;\
        }\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Strings1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 32;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            5,    1,    1,  200,    // 7
       3,    1,    2,    8,            5,    1,    1,    4,    // 15
       6,    2,  200,    7,            3,    2,    1,  204,    // 23
       5,    6,    3,    0,            2,    1,  204,    5,    // 31
     -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
     200,   18,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      5,   1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}

TEST(Compatibility, Struct1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Struct               \n\
    {                               \n\
        float Float;                \n\
        import int[] Func(int i);   \n\
    };                              \n\
                                    \n\
    int Ret[];                      \n\
                                    \n\
    int[] Struct::Func(int i)       \n\
    {                               \n\
        this.Float = 0.0;           \n\
        Ret = new int[5];           \n\
        return Ret;                 \n\
    }                               \n\
                                    \n\
    void main()                     \n\
    {                               \n\
        Struct S;                   \n\
        S.Func(-1);                 \n\
    }                               \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Struct1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 90;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,    4,    4,    0,    // 7
       1,    1,    4,    6,            3,    0,   29,    6,    // 15
      30,    2,   52,    8,            3,    6,    3,    5,    // 23
      72,    3,    4,    0,            6,    2,    0,   47,    // 31
       3,    6,    2,    0,           48,    3,    2,    1,    // 39
       4,    5,    6,    3,            0,    2,    1,    4,    // 47
       5,   38,   49,    3,            1,    2,   63,    4,    // 55
       1,    1,    4,   29,            6,    6,    3,   -1,    // 63
      29,    3,   51,   12,            3,    2,    3,   45,    // 71
       3,    6,    3,    0,           23,    3,    2,    1,    // 79
       4,   30,    6,    6,            3,    0,    2,    1,    // 87
       4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      30,   35,   75,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   2,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}

TEST(Compatibility, Struct2) {
    ccCompiledScript *scrip = newScriptFixture();

    // test arrays; arrays in structs;
    // whether the namespace in structs is independent of the global namespace

    char *inpl = "\
    struct Struct1                  \n\
    {                               \n\
        int Array[17], Ix;          \n\
    };                              \n\
                                    \n\
    Struct1 S;                      \n\
    int Array[5];                   \n\
                                    \n\
    void main()                     \n\
    {                               \n\
        S.Ix = 5;                   \n\
        Array[2] = 3;               \n\
        S.Array[Array[2]] = 42;     \n\
        S.Array[S.Ix] = 19;         \n\
        return;                     \n\
    }";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Struct2", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 118;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            5,    6,    2,   68,    // 7
       8,    3,    6,    3,            3,   29,    3,    6,    // 15
       3,    2,   46,    3,            5,   32,    3,    4,    // 23
       3,    3,    5,   30,            3,    6,    2,   72,    // 31
      11,    2,    5,    8,            3,    6,    3,   42,    // 39
      29,    3,    6,    3,            2,   46,    3,    5,    // 47
      32,    3,    4,    3,            3,    5,    6,    2,    // 55
      72,   11,    2,    5,            7,    3,   46,    3,    // 63
      17,   32,    3,    4,            3,    3,    5,   30,    // 71
       3,    6,    2,    0,           11,    2,    5,    8,    // 79
       3,    6,    3,   19,           29,    3,    6,    2,    // 87
      68,    7,    3,   46,            3,   17,   32,    3,    // 95
       4,    3,    3,    5,           30,    3,    6,    2,    // 103
       0,   11,    2,    5,            8,    3,    6,    3,    // 111
       0,    5,    6,    3,            0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 6;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
       7,   31,   56,   75,         88,  104,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}

TEST(Compatibility, Struct3) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct StructI                               \n\
        {                                                    \n\
            int k;                                           \n\
        };                                                   \n\
                                                             \n\
        struct StructO                                       \n\
        {                                                    \n\
            StructI *SI;                                     \n\
            StructI *SJ[3];                                  \n\
        };                                                   \n\
                                                             \n\
        int main()                                           \n\
        {                                                    \n\
            StructO SO;                                      \n\
            SO.SI = new StructI;                             \n\
            SO.SI.k = 12345;                                 \n\
            StructO SOA[3];                                  \n\
            SOA[2].SI = new StructI;                         \n\
        }                                                    \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Struct3", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 147;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,   16,    1,    // 7
       1,   16,   73,    3,            4,   51,   16,   29,    // 15
       2,   30,    2,   47,            3,    6,    3, 12345,    // 23
      51,   16,   29,    2,           30,    2,   29,    3,    // 31
      48,    3,   30,    4,           29,    3,    3,    4,    // 39
       3,   30,    2,   52,            8,    3,    3,    1,    // 47
       2,   63,   48,    1,            1,   48,   73,    3,    // 55
       4,   29,    3,    6,            3,    2,   46,    3,    // 63
       3,   32,    3,   16,            3,    3,    5,   30,    // 71
       3,   51,   48,   29,            2,   30,    2,   11,    // 79
       2,    5,   47,    3,            6,    3,    0,   51,    // 87
      64,   49,   51,   60,           49,    1,    2,    4,    // 95
      49,    1,    2,    4,           49,   51,   48,   49,    // 103
      51,   32,   49,   51,           16,   49,   51,   44,    // 111
      49,    1,    2,    4,           49,    1,    2,    4,    // 119
      49,   51,   28,   49,            1,    2,    4,   49,    // 127
       1,    2,    4,   49,           51,   12,   49,    1,    // 135
       2,    4,   49,    1,            2,    4,   49,    2,    // 143
       1,   64,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}


TEST(Compatibility, Struct4) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct StructO                                       \n\
        {                                                    \n\
            static import int StInt(int i);                  \n\
        };                                                   \n\
        StructO        S1;                                   \n\
                                                             \n\
        int main()                                           \n\
        {                                                    \n\
             StructO        S2;                              \n\
             return S1.StInt(S2.StInt(7));                   \n\
        }                                                    \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Struct4", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 37;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,    0,    6,    // 7
       3,    7,   34,    3,           39,    1,    6,    3,    // 15
       0,   33,    3,   35,            1,   34,    3,   39,    // 23
       1,    6,    3,    0,           33,    3,   35,    1,    // 31
       5,    6,    3,    0,            5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      16,   27,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      4,   4,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, Struct5) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct Struct0;                              \n\
                                                             \n\
        struct Struct1                                       \n\
        {                                                    \n\
            Struct0 *Array[];                                \n\
        };                                                   \n\
                                                             \n\
        managed struct Struct0                               \n\
        {                                                    \n\
            int Payload;                                     \n\
        };                                                   \n\
                                                             \n\
        int main()                                           \n\
        {                                                    \n\
             Struct1 S;                                      \n\
                                                             \n\
             S.Array = new Struct0[5];                       \n\
             S.Array[3].Payload ++;                          \n\
        }                                                    \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Struct5", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 72;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,    4,    1,    // 7
       1,    4,    6,    3,            5,   72,    3,    4,    // 15
       1,   51,    4,   29,            2,   30,    2,   47,    // 23
       3,    6,    3,    3,            3,    3,    7,   51,    // 31
       4,   32,    7,    4,           48,    2,   52,   71,    // 39
       7,   11,    2,    7,           29,    2,   30,    2,    // 47
      48,    3,   29,    3,           30,    2,   52,    7,    // 55
       3,    1,    3,    1,            8,    3,    6,    3,    // 63
       0,   51,    4,   49,            2,    1,    4,    5,    // 71
     -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}


TEST(Compatibility, Struct6) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Struct1                                       \n\
        {                                                    \n\
            int IPayload;                                    \n\
            char CPayload[3];                                \n\
        };                                                   \n\
                                                             \n\
        Struct1 S1[3];                                       \n\
                                                             \n\
        int main()                                           \n\
        {                                                    \n\
            S1[1].IPayload = 0;                              \n\
            S1[1].CPayload[0] = 'A';                         \n\
            S1[1].CPayload[1] = S1[1].CPayload[0] - 'A';     \n\                        \n\
            S1[1].CPayload[0] --;                            \n\
            return 0;                                        \n\
        }                                                    \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Struct6", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 218;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,    1,   46,    3,            3,   32,    3,    8,    // 15
       3,    3,    5,   30,            3,    6,    2,    0,    // 23
      11,    2,    5,    8,            3,    6,    3,   65,    // 31
      29,    3,    6,    3,            1,   46,    3,    3,    // 39
      32,    3,    8,    3,            3,    5,   30,    3,    // 47
      29,    3,   29,    5,            6,    3,    0,   46,    // 55
       3,    3,   32,    3,            1,   30,    5,   11,    // 63
       5,    3,   30,    3,            6,    2,    4,   11,    // 71
       2,    5,   26,    3,            6,    3,    1,   46,    // 79
       3,    3,   32,    3,            8,    3,    3,    5,    // 87
      29,    5,    6,    3,            0,   46,    3,    3,    // 95
      32,    3,    1,   30,            5,   11,    5,    3,    // 103
       6,    2,    4,   11,            2,    5,   24,    3,    // 111
      29,    3,    6,    3,           65,   30,    4,   12,    // 119
       4,    3,    3,    4,            3,   29,    3,    6,    // 127
       3,    1,   46,    3,            3,   32,    3,    8,    // 135
       3,    3,    5,   30,            3,   29,    3,   29,    // 143
       5,    6,    3,    1,           46,    3,    3,   32,    // 151
       3,    1,   30,    5,           11,    5,    3,   30,    // 159
       3,    6,    2,    4,           11,    2,    5,   26,    // 167
       3,    6,    3,    1,           46,    3,    3,   32,    // 175
       3,    8,    3,    3,            5,   29,    5,    6,    // 183
       3,    0,   46,    3,            3,   32,    3,    1,    // 191
      30,    5,   11,    5,            3,    6,    2,    4,    // 199
      11,    2,    5,   24,            3,    2,    3,    1,    // 207
      26,    3,    6,    3,            0,    5,    6,    3,    // 215
       0,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      23,   70,  106,  163,        199,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, StructExtender) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Struct                                        \n\
        {                                                    \n\
            int k;                                           \n\
        };                                                   \n\
                                                             \n\
        struct Sub extends Struct                            \n\
        {                                                    \n\
            int l;                                           \n\
        };                                                   \n\
                                                             \n\
        int Func(this Sub *, int i, int j)                   \n\
        {                                                    \n\
            return !i || !(j) && this.k || (0 != this.l);    \n\
        }                                                    \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("StructExtender", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 100;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,    4,    4,    0,    // 7
       1,    1,    4,   51,           12,    7,    3,   42,    // 15
       3,   70,   35,   29,            3,   51,   20,    7,    // 23
       3,   42,    3,   28,           17,   29,    3,   29,    // 31
       6,   30,    2,   52,            7,    3,   30,    4,    // 39
      21,    4,    3,    3,            4,    3,   30,    4,    // 47
      22,    4,    3,    3,            4,    3,   70,   33,    // 55
      29,    3,    6,    3,            0,   29,    3,   29,    // 63
       6,   30,    2,   52,            1,    2,    4,    7,    // 71
       3,   30,    4,   16,            4,    3,    3,    4,    // 79
       3,   30,    4,   22,            4,    3,    3,    4,    // 87
       3,    2,    1,    4,            5,    6,    3,    0,    // 95
       2,    1,    4,    5,          -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}


TEST(Compatibility, Func1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct Struct1          \n\
    {                               \n\
        float Payload1;             \n\
    };                              \n\
    managed struct Struct2          \n\
    {                               \n\
        char Payload2;              \n\
    };                              \n\
                                    \n\
    import int Func(Struct1 *S1, Struct2 *S2);  \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct1 *SS1;               \n\
        Struct2 *SS2;               \n\
        int Ret = Func(SS1, SS2);   \n\
        return Ret;                 \n\
    }                               \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Func1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 82;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,    4,    1,    // 7
       1,    4,    3,    1,            2,   63,    4,    1,    // 15
       1,    4,   51,    4,           29,    2,   30,    2,    // 23
      48,    3,   34,    3,           51,    8,   29,    2,    // 31
      30,    2,   48,    3,           34,    3,   39,    2,    // 39
       6,    3,    0,   33,            3,   35,    2,    3,    // 47
       1,    2,    8,    3,            1,    1,    4,   51,    // 55
       4,    7,    3,   51,           12,   69,   51,    8,    // 63
      69,    2,    1,   12,            5,    6,    3,    0,    // 71
      51,   12,   69,   51,            8,   69,    2,    1,    // 79
      12,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      42,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      4,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, Func2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct Struct1          \n\
    {                               \n\
        float Payload1;             \n\
    };                              \n\
    managed struct Struct2          \n\
    {                               \n\
        char Payload2;              \n\
    };                              \n\
                                    \n\
    import  int Func(Struct1 *S1, Struct2 *S2);  \n\
                                    \n\
    int Func(Struct1 *S1, Struct2 *S2)  \n\
    {                               \n\
        return 0;                   \n\
    }                               \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct1 *SS1;               \n\
        Struct2 *SS2;               \n\
        int Ret = Func(SS1, SS2);   \n\
        return Ret;                 \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Func2", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 115;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   51,    8,            7,    3,   50,    3,    // 7
      51,   12,    7,    3,           50,    3,    6,    3,    // 15
       0,   51,    8,   69,           51,   12,   69,    5,    // 23
       6,    3,    0,   51,            8,   69,   51,   12,    // 31
      69,    5,   38,   34,            3,    1,    2,   63,    // 39
       4,    1,    1,    4,            3,    1,    2,   63,    // 47
       4,    1,    1,    4,           51,    4,   29,    2,    // 55
      30,    2,   48,    3,           29,    3,   51,   12,    // 63
      29,    2,   30,    2,           48,    3,   29,    3,    // 71
       6,    3,    0,   23,            3,    2,    1,    8,    // 79
       3,    1,    2,    8,            3,    1,    1,    4,    // 87
      51,    4,    7,    3,           51,   12,   69,   51,    // 95
       8,   69,    2,    1,           12,    5,    6,    3,    // 103
       0,   51,   12,   69,           51,    8,   69,    2,    // 111
       1,   12,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      74,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      2,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, Func3) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct Struct1          \n\
    {                               \n\
        float Payload1;             \n\
    };                              \n\
                                    \n\
    Struct1 *Func(int Int)          \n\
    {                               \n\
        return new Struct1;         \n\
    }                               \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct1 *SS1 = Func(5);     \n\
        return -1;                  \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Func3", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 53;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   73,    3,            4,    5,    6,    3,    // 7
       0,    5,   38,   10,            6,    3,    5,   29,    // 15
       3,    6,    3,    0,           23,    3,    2,    1,    // 23
       4,    3,    1,    2,           50,    3,    1,    1,    // 31
       4,    6,    3,   -1,           51,    4,   69,    2,    // 39
       1,    4,    5,    6,            3,    0,   51,    4,    // 47
      69,    2,    1,    4,            5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      19,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      2,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, FuncCall) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Struct               \n\
    {                               \n\
        float Float;                \n\
        import int Func();          \n\
    };                              \n\
                                    \n\
    int Struct::Func()              \n\
    {                               \n\
        return 5;                   \n\
    }                               \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct s;                   \n\
        int Int = s.Func() % 3;     \n\
        return Int;                 \n\
    }                               \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("FuncCall", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 72;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            5,    5,    6,    3,    // 7
       0,    5,   38,   10,            3,    1,    2,   63,    // 15
       4,    1,    1,    4,           29,    6,   51,    8,    // 23
       3,    2,    3,   45,            3,    6,    3,    0,    // 31
      23,    3,   30,    6,           29,    3,    6,    3,    // 39
       3,   30,    4,   40,            4,    3,    3,    4,    // 47
       3,    3,    1,    2,            8,    3,    1,    1,    // 55
       4,   51,    4,    7,            3,    2,    1,    8,    // 63
       5,    6,    3,    0,            2,    1,    8,    5,    // 71
     -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      31,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      2,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

}


TEST(Compatibility, Export) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    struct Struct                   \n\
    {                               \n\
        float Float;                \n\
        int Int;                    \n\
    };                              \n\
    Struct Structy;                 \n\
    export Structy;                 \n\
                                    \n\
    int Inty;                       \n\
    float Floaty;                   \n\
    export Floaty, Inty;            \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct s;                   \n\
        s.Int = 3;                  \n\
        s.Float = 1.1 / 2.2;        \n\
        return -2;                  \n\
    }                               \n\
    export main;                    \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("Export", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 51;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,    8,    1,    // 7
       1,    8,    6,    3,            3,   51,    4,    8,    // 15
       3,    6,    3, 1066192077,           29,    3,    6,    3,    // 23
    1074580685,   30,    4,   56,            4,    3,    3,    4,    // 31
       3,   51,    8,    8,            3,    6,    3,   -2,    // 39
       2,    1,    8,    5,            6,    3,    0,    2,    // 47
       1,    8,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}


TEST(Compatibility, ArrayOfPointers1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct Struct                \n\
    {                                    \n\
        float Float;                     \n\
        protected int Int;               \n\
    };                                   \n\
    Struct *arr[50];                     \n\
                                         \n\
    int main()                           \n\
    {                                    \n\
        for(int i = 0; i < 9; i++)       \n\
            arr[i] = new Struct;         \n\
                                         \n\
        int test = (arr[10] == null);    \n\
                                         \n\
    }                                    \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("ArrayOfPointers1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 130;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,    3,    1,    2,    // 7
       8,    3,    1,    1,            4,   51,    4,    7,    // 15
       3,   29,    3,    6,            3,    9,   30,    4,    // 23
      18,    4,    3,    3,            4,    3,   28,   43,    // 31
      73,    3,    8,   29,            3,   51,    8,    7,    // 39
       3,   46,    3,   50,            3,    3,    7,   30,    // 47
       3,    6,    2,    0,           32,    7,    4,   11,    // 55
       2,    7,   29,    2,           30,    2,   47,    3,    // 63
      51,    4,    7,    3,            1,    3,    1,    8,    // 71
       3,   31,  -62,    2,            1,    4,    6,    3,    // 79
      10,   46,    3,   50,            3,    3,    7,    6,    // 87
       2,    0,   32,    7,            4,   11,    2,    7,    // 95
      29,    2,   30,    2,           48,    3,   29,    3,    // 103
       6,    3,    0,   30,            4,   15,    4,    3,    // 111
       3,    4,    3,    3,            1,    2,    8,    3,    // 119
       1,    1,    4,    6,            3,    0,    2,    1,    // 127
       4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      51,   89,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixups[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      1,   1,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        std::string prefix = "fixuptypes[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}


TEST(Compatibility, ArrayOfPointers2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct Struct                \n\
    {                                    \n\
        float Float;                     \n\
        protected int Int;               \n\
    };                                   \n\
                                         \n\
    int main()                           \n\
    {                                    \n\
        Struct *arr2[50];                \n\
        for (int i = 0; i < 20; i++) {   \n\
                arr2[i] = new Struct;    \n\
        }                                \n\
        arr2[5].Float = 2.2 - 0.0 * 3.3; \n\
        arr2[4] = null;                  \n\
    }                                    \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("ArrayOfPointers2", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 391;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,   63,  200,    1,    // 7
       1,  200,    6,    3,            0,    3,    1,    2,    // 15
       8,    3,    1,    1,            4,   51,    4,    7,    // 23
       3,   29,    3,    6,            3,   20,   30,    4,    // 31
      18,    4,    3,    3,            4,    3,   28,   42,    // 39
      73,    3,    8,   29,            3,   51,    8,    7,    // 47
       3,   46,    3,   50,            3,    3,    7,   30,    // 55
       3,   51,  204,   32,            7,    4,   11,    2,    // 63
       7,   29,    2,   30,            2,   47,    3,   51,    // 71
       4,    7,    3,    1,            3,    1,    8,    3,    // 79
      31,  -61,    2,    1,            4,    6,    3, 1074580685,    // 87
      29,    3,    6,    3,            0,   29,    3,    6,    // 95
       3, 1079194419,   30,    4,           55,    4,    3,    3,    // 103
       4,    3,   30,    4,           58,    4,    3,    3,    // 111
       4,    3,   29,    3,            6,    3,    5,   46,    // 119
       3,   50,    3,    3,            7,   30,    3,   51,    // 127
     200,   32,    7,    4,           11,    2,    7,   29,    // 135
       2,   30,    2,   29,            3,   48,    3,   30,    // 143
       4,   29,    3,    3,            4,    3,   30,    2,    // 151
      52,    8,    3,    6,            3,    0,   29,    3,    // 159
       6,    3,    4,   46,            3,   50,    3,    3,    // 167
       7,   30,    3,   51,          200,   32,    7,    4,    // 175
      11,    2,    7,   29,            2,   30,    2,   47,    // 183
       3,    6,    3,    0,           51,  200,   69,    1,    // 191
       2,    4,   69,    1,            2,    4,   69,    1,    // 199
       2,    4,   69,    1,            2,    4,   69,    1,    // 207
       2,    4,   69,    1,            2,    4,   69,    1,    // 215
       2,    4,   69,    1,            2,    4,   69,    1,    // 223
       2,    4,   69,    1,            2,    4,   69,    1,    // 231
       2,    4,   69,    1,            2,    4,   69,    1,    // 239
       2,    4,   69,    1,            2,    4,   69,    1,    // 247
       2,    4,   69,    1,            2,    4,   69,    1,    // 255
       2,    4,   69,    1,            2,    4,   69,    1,    // 263
       2,    4,   69,    1,            2,    4,   69,    1,    // 271
       2,    4,   69,    1,            2,    4,   69,    1,    // 279
       2,    4,   69,    1,            2,    4,   69,    1,    // 287
       2,    4,   69,    1,            2,    4,   69,    1,    // 295
       2,    4,   69,    1,            2,    4,   69,    1,    // 303
       2,    4,   69,    1,            2,    4,   69,    1,    // 311
       2,    4,   69,    1,            2,    4,   69,    1,    // 319
       2,    4,   69,    1,            2,    4,   69,    1,    // 327
       2,    4,   69,    1,            2,    4,   69,    1,    // 335
       2,    4,   69,    1,            2,    4,   69,    1,    // 343
       2,    4,   69,    1,            2,    4,   69,    1,    // 351
       2,    4,   69,    1,            2,    4,   69,    1,    // 359
       2,    4,   69,    1,            2,    4,   69,    1,    // 367
       2,    4,   69,    1,            2,    4,   69,    1,    // 375
       2,    4,   69,    1,            2,    4,   69,    1,    // 383
       2,    4,   69,    2,            1,  200,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}


TEST(Compatibility, ArrayInStruct) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    managed struct Struct                \n\
    {                                    \n\
        int Int[10];                     \n\
    };                                   \n\
                                         \n\
    int main()                           \n\
    {                                    \n\
        Struct *S = new Struct;          \n\
        S.Int[4] =  1;                   \n\
    }                                    \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("ArrayInStruct", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 67;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   73,    3,           40,    3,    1,    2,    // 7
      50,    3,    1,    1,            4,    6,    3,    1,    // 15
      51,    4,   29,    2,           30,    2,   29,    3,    // 23
      48,    3,   30,    4,           29,    3,    3,    4,    // 31
       3,   29,    3,    6,            3,    4,   46,    3,    // 39
      10,   32,    3,    4,            3,    3,    5,   30,    // 47
       3,   30,    2,   52,           11,    2,    5,    8,    // 55
       3,    6,    3,    0,           51,    4,   69,    2,    // 63
       1,    4,    5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}


TEST(Compatibility, FuncVarargs1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Func(int I, ...)                 \n\
    {                                    \n\
        return I + I / I;                \n\
    }                                    \n\
                                         \n\
    int main()                           \n\
    {                                    \n\
        return 0;                        \n\
    }                                    \n\
    ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // writeoutput("FuncVarargs1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code

    const size_t codesize = 49;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   12,    7,    3,           29,    3,   51,   16,    // 15
       7,    3,   30,    4,           10,    4,    3,    3,    // 23
       4,    3,   30,    4,           11,    4,    3,    3,    // 31
       4,    3,    5,    6,            3,    0,    5,   38,    // 39
      39,    6,    3,    0,            5,    6,    3,    0,    // 47
       5,  -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        std::string prefix = "code[";
        prefix += (std::to_string(idx)) + std::string("] == ");
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip->numfixups);
}
