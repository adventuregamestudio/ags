#include <iostream>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"


extern ccCompiledScript *newScriptFixture();
int last_seen_cc_error;

void writeoutput(char *fname, ccCompiledScript *scrip)
{
    std::string path = "C:\\TEMP\\";
    std::ofstream of;
    of.open(path.append(fname).append(".txt"));

    // export the code
    of << "const size_t codesize = " << scrip->codesize << ";" << std::endl;
    of << "ASSERT_EQ(codesize, " << scrip->codesize << ");" << std::endl << std::endl;

    of << "intptr_t code[] = {" << std::endl;
    for (size_t idx = 0; idx < scrip->codesize; idx++)
    {
        of.width(4);
        of << scrip->code[idx] << ", ";
        if (idx % 8 == 3) of << "        ";
        if (idx % 8 == 7) of << std::endl;
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
    
    // export the fixups
    of << "const size_t numfixups = " << scrip->numfixups << ";" << std::endl;
    of << "ASSERT_EQ(numfixups, " << scrip->numfixups << ");" << std::endl << std::endl;

    of << "intptr_t fixups[] = {" << std::endl ;
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
    ASSERT_EQ(codesize, 10);

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
    ASSERT_EQ(numfixups, 0);

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
    ASSERT_EQ(codesize, 10);

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
    ASSERT_EQ(numfixups, 0);

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

    writeoutput("IntFunctionLocalV", scrip);
    // run the test, comment out the previous line 
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
    const size_t codesize = 28;
    ASSERT_EQ(codesize, 28);

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
    ASSERT_EQ(numfixups, 0);

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
    ASSERT_EQ(codesize, 11);

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
    ASSERT_EQ(numfixups, 0);

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
    ASSERT_EQ(codesize, 12);

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
    ASSERT_EQ(numfixups, 1);

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
    ASSERT_EQ(codesize, 43);

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
    ASSERT_EQ(numfixups, 1);

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