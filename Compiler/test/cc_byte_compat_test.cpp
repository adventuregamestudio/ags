#include <iostream>  
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"


extern ccCompiledScript *newScriptFixture();
extern int ccErrorLine;
extern char * last_seen_cc_error;

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
        for (size_t idx = 0; idx < static_cast<size_t>(scrip->codesize); idx++)
        {
            of.width(4);
            of << scrip->code[idx] << ", ";
            if (idx % 8 == 3) of << "        ";
            if (idx % 8 == 7) of << "// " << idx << std::endl;
        }
        of << " -999 " << std::endl << "};" << std::endl << std::endl;

        of << "for (size_t idx = 0; idx < scrip->codesize; idx++)" << std::endl;
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
        for (size_t idx = 0; idx < static_cast<size_t>(scrip->numfixups); idx++)
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
        for (size_t idx = 0; idx < static_cast<size_t>(scrip->numfixups); idx++)
        {
            of.width(3);
            of << static_cast<int>(scrip->fixuptypes[idx]) << ", ";
            if (idx % 8 == 3) of << "   ";
            if (idx % 8 == 7) of << std::endl;
        }
        of << " '\\0' " << std::endl << "};" << std::endl << std::endl;

        of << "for (size_t idx = 0; idx < scrip->numfixups; idx++)" << std::endl;
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
// 1. Run those tests in a snapshot that does not have the changes made.
// 2. Append the generated lines, as explained below
// 3. Comment out the "writeoutput" line.
// 4. Export this file to a snapshot that does have the changes.
// 5. Run the test.
TEST(Compatibility, SimpleFunction) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo(int a)      \n\
        {                   \n\
            return a*a;     \n\
        }";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);

    // writeoutput("SimpleFunction", scrip);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    // writeoutput("IntFunctionLocalV", scrip);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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


TEST(Compatibility, Expression1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        float a = 15.0;     \n\
        float Foo()     \n\
    {                   \n\
        float f = 3.14; \n\
        return a + f;   \n\
    }";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    // writeoutput("Expression1", scrip);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

TEST(Compatibility, For) {
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    // writeoutput("For", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 130;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,    6,    2,    0, // 7
       8,    3,    6,    2,            0,    7,    3,   29, // 15
       3,    6,    3,   10,           30,    4,   18,    4, // 23
       3,    3,    4,    3,           28,   92,    6,    2, // 31
       0,    7,    3,   29,            3,    6,    3,    4, // 39
      30,    4,   12,    4,            3,    3,    4,    3, // 47
      29,    3,    6,    3,            7,   30,    4,   12, // 55
       4,    3,    3,    4,            3,    3,    1,    2, // 63
       8,    3,    1,    1,            4,    6,    2,    0, // 71
       7,    3,   29,    3,            6,    3,    6,   30, // 79
       4,   15,    4,    3,            3,    4,    3,   28, // 87
       8,    2,    1,    4,            6,    3,    0,   31, // 95
     -69,    2,    1,    4,            6,    3,    3,   29, // 103
       3,    6,    2,    0,            7,    3,   30,    4, // 111
      11,    3,    4,    6,            2,    0,    8,    3, // 119
      31, -112,    6,    3,            0,    5,    6,    3, // 127
       0,    5,  -999
    };

    for (size_t idx = 0; idx < static_cast<size_t>(scrip->codesize); idx++)
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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
    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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
        string GLOBAL;          \
                                \
        string MyFunction(int a)\
        {                       \
            string x;           \
            return GLOBAL;      \
        }                       \
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

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
    int[] Struct::Func(int i)       \n\
    {                               \n\
        this.Float = 0.0;           \n\
        Ret = new int[5];           \n\
        return Ret;                 \n\
    }";


    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);

    // writeoutput("Struct1", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 49;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    3,    1,            2,    4,    4,    0,    // 7
       1,    1,    4,    6,            3,    0,   29,    6,    // 15
      30,    2,   52,    8,            3,    6,    3,    5,    // 23
      72,    3,    4,    0,            6,    2,    0,   47,    // 31
       3,    6,    2,    0,           48,    3,    2,    1,    // 39
       4,    5,    6,    3,            0,    2,    1,    4,    // 47
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
    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      30,   35,  -999
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0)? "Ok" : last_seen_cc_error);

    writeoutput("StructExtender", scrip);
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
