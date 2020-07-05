#include <iostream>  
#include <fstream>
#include <string>

#include "script/cc_options.h"
#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

extern int cc_tokenize(
    const char * inpl,         // preprocessed text to be tokenized
    ccInternalList * targ,     // store for the tokenized text
    ccCompiledScript * scrip); // store for the strings in the text

extern ccCompiledScript *newScriptFixture();
extern char *last_seen_cc_error;

extern void WriteReducedOutput(char *fname, ccCompiledScript *scrip);

ccInternalList *newTargFixture(ccCompiledScript *scrip, char *agscode)
{
    ccInternalList *targ = new ccInternalList();
    int retval = cc_tokenize(agscode, targ, scrip);
    if (retval < 0) 
        return nullptr;
    targ->startread();
    return targ;
}

/*
TEST(Feature, P_r_o_t_o_t_y_p_e) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo(int a)      \n\
        {                   \n\
            return a*a;     \n\
        }";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);

}

*/


TEST(Feature, Recursive1) {

    char *agscode = "\
        import int Foo2 (int);    \n\
                                  \n\
        int Foo1(int a)           \n\
        {                         \n\
            if (a >= 0)           \n\
            return Foo2(a - 1);   \n\
        }                         \n\
                                  \n\
        int Foo2(int a)           \n\
        {                         \n\
            return Foo1 (a - 2);  \n\
        }                         \n\
        ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_EQ(0, compileResult);

}


TEST(Feature, DoubleBody) {

    char *agscode = "\
        import int Foo2 (int);    \n\
                                  \n\
        int Foo2(int a)           \n\
        {                         \n\
            if (a >= 0)           \n\
            return Foo2(a - 1);   \n\
        }                         \n\
                                  \n\
        int Foo2(int a)           \n\
        {                         \n\
            return Foo2 (a - '\2');  \n\
        }                         \n\
        ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_GT(0, compileResult);
}

TEST(Feature, CompatibleForward1) {

    char *agscode = "\
        import int Foo2 (int = 2, int = -7); \n\
                                  \n\
        int Foo2(int a, int b)    \n\
        {                         \n\
            return Foo2 (a - b);  \n\
        }                         \n\
        ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);

}

TEST(Feature, CompatibleForward2) {

    char *agscode = "\
        import int Foo2 (int = 9, int = 7);    \n\
        int Foo2(int z = 9, int q = 7); \n\
                                  \n\
        int Foo2(int a, int b)    \n\
        {                         \n\
            return Foo2 (a - b);  \n\
        }                         \n\
        int Foo2(int = 9, int Jux = 7); \n\
         ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);

}

TEST(Feature, IncompatibleForward1) {

    char *agscode = "\
        import int Foo2 (int, int = 7);    \n\
        import int Foo2(int z = 9, int zz); \n\
                                  \n\
        int Foo2(int a, int b)    \n\
        {                         \n\
            return Foo2 (a - b);  \n\
        }                         \n\
        ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);

}

TEST(Feature, ForForward1) {

    char *agscode = "\
        import int Inc (int);    \n\
        import int Is10 (int);   \n\
                                 \n\
        void main()              \n\
        {                        \n\
            for (int i = Inc(0); \n\
                 Is10(i);        \n\
                 i = Inc(i))     \n\
            {                    \n\
                if (i > 5)       \n\
                    continue;    \n\
            }                    \n\
        }                        \n\
                                 \n\
        int Inc(int a)           \n\
        {                        \n\
            return a + 1;        \n\
        }                        \n\
        int Is10(int a)          \n\
        {                        \n\
            return a >= 10;      \n\
        }                        \n\
        ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);
    // WriteReducedOutput("ForForward1", scrip);

    const size_t codesize = 156;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,  108,   23,    3,            2,    1,    4,    3,    // 15
       1,    2,    8,    3,            1,    1,    4,   51,    // 23
       4,    7,    3,   29,            3,    6,    3,  132,    // 31
      23,    3,    2,    1,            4,   28,   62,   51,    // 39
       4,    7,    3,   29,            3,    6,    3,    5,    // 47
      30,    4,   17,    4,            3,    3,    4,    3,    // 55
      28,   23,   51,    4,            7,    3,   29,    3,    // 63
       6,    3,  108,   23,            3,    2,    1,    4,    // 71
      51,    4,    8,    3,            6,    3,    0,   31,    // 79
     -58,   51,    4,    7,            3,   29,    3,    6,    // 87
       3,  108,   23,    3,            2,    1,    4,   51,    // 95
       4,    8,    3,   31,          -78,    2,    1,    4,    // 103
       6,    3,    0,    5,           38,  108,   51,    8,    // 111
       7,    3,   29,    3,            6,    3,    1,   30,    // 119
       4,   11,    4,    3,            3,    4,    3,    5,    // 127
       6,    3,    0,    5,           38,  132,   51,    8,    // 135
       7,    3,   29,    3,            6,    3,   10,   30,    // 143
       4,   19,    4,    3,            3,    4,    3,    5,    // 151
       6,    3,    0,    5,          -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        if (idx >= scrip->codesize) break;
        std::string prefix = "code[";
        prefix += std::to_string(idx) + "] == ";
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }
}

TEST(Feature, Struct1) {

    char *agscode = "\
        struct Struct {          \n\
           int Func(int i);      \n\
        };                       \n\
                                 \n\
        void main()              \n\
        {                        \n\
            Struct s;            \n\
            s.Func(10);          \n\
        }                        \n\
                                 \n\
        int Struct::Func(int f)  \n\
        {                        \n\
            return f;            \n\
        }                        \n\
        ";
    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);
    // WriteReducedOutput("ForForward1", scrip);
}

TEST(Feature, Struct2) {

    char *agscode = "\
        struct Struct {          \n\
           int Func(int i) {return 0; } \n\
        };                       \n\
        Struct s;                \n\
                                 \n\
        void main()              \n\
        {                        \n\
            s.Func(10);          \n\
        }                        \n\
        ";

    last_seen_cc_error = 0;
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    // No body in a struct function declaration
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);
}

TEST(Feature, Func4) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Func(int P1, int P2 = 5) \n\
        {                            \n\
            return P1 + P2;          \n\
        }                            \n\
                                     \n\
        void main()                  \n\
        {                            \n\
            int Int = Func(4);       \n\
        }                            \n\
    ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error);


    const size_t codesize = 60;
    EXPECT_EQ(codesize, scrip->codesize);

    intptr_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   11,    4,    // 15
       3,    3,    4,    3,            5,    6,    3,    0,    // 23
       5,   38,   25,    6,            3,    5,   29,    3,    // 31
       6,    3,    4,   29,            3,    6,    3,    0,    // 39
      23,    3,    2,    1,            8,    3,    1,    2,    // 47
       8,    3,    1,    1,            4,    6,    3,    0,    // 55
       2,    1,    4,    5,          -999
    };

    for (size_t idx = 0; idx < codesize; idx++)
    {
        if (idx >= scrip->codesize) break;
        std::string prefix = "code[";
        prefix += std::to_string(idx) + "] == ";
        std::string is_val = prefix + std::to_string(code[idx]);
        std::string test_val = prefix + std::to_string(scrip->code[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip->numfixups);

    intptr_t fixups[] = {
      39,  -999
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        if (idx >= scrip->numfixups) break;
        std::string prefix = "fixups[";
        prefix += std::to_string(idx) + "] == ";
        std::string   is_val = prefix + std::to_string(fixups[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixups[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    char fixuptypes[] = {
      2,  '\0'
    };

    for (size_t idx = 0; idx < numfixups; idx++)
    {
        if (idx >= scrip->numfixups) break;
        std::string prefix = "fixuptypes[";
        prefix += std::to_string(idx) + "] == ";
        std::string   is_val = prefix + std::to_string(fixuptypes[idx]);
        std::string test_val = prefix + std::to_string(scrip->fixuptypes[idx]);
        ASSERT_EQ(is_val, test_val);
    }

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };

    int idx2 = -1;
    for (size_t idx = 0; idx < scrip->numimports; idx++)
    {
        if (!strcmp(scrip->imports[idx], ""))
            continue;
        idx2++;
        ASSERT_LT(idx2, numimports);
        std::string prefix = "imports[";
        prefix += std::to_string(idx2) + "] == ";
        std::string is_val = prefix + scrip->imports[idx];
        std::string test_val = prefix + imports[idx2];
        ASSERT_EQ(is_val, test_val);
    }

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip->numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip->stringssize);

}