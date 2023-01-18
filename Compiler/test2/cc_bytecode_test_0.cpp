#include <string>

#include "gtest/gtest.h"
#include "cc_bytecode_test_lib.h"
#include "cc_parser_test_lib.h"

#include "script/cc_common.h"

#include "script2/cs_parser.h"


/* This file is for Bytecode tests ONLY.
   Testing for programs that won't compile should be done elsewhere IMHO.
   
   If a test generates wrong code, often a good way to debug it is as follows:
   - Uncomment the "WriteOutput(" line and run the test; now you have the bytes
     that the _new_ compiler emits in the file. Compare it by hand to the code
     that's in the test. See what the problem is: Has the new compiler left
     opcodes out, or has it added some in? Has it changed a single value somewhere?
   - Find the line in cs_compiledscript.cpp, void ccCompiledScript::write_code,
     where a bytecode cell is appended, and set a breakpoint.
   - Configure the breakpoint to break when the last byte comes up that is still
     correct.
   - Debug the googletest:
     Trace along from the breakpoint and find the point where the logic fails.

   But bear in mind:
   - Sometimes the compiler emits some code, then rips it out and stashes it away,
     then, later on, retrieves it and emits it again.
   - Sometimes the compiler emits some code value, then patches it afterwards.
   These are the hard cases to debug. :)
*/


// NOTE! If any "WriteOutput" lines in this file are uncommented, then the 
//  #define below _must_ be changed to a local writable temp dir. 
// (If you only want to run the tests to see if any tests fail, you do NOT 
// need that dir and you do NOT need any local files whatsoever.)
#define LOCAL_PATH "C:\\TEMP\\"


/*    PROTOTYPE

TEST_F(Bytecode0, P_r_o_t_o_t_y_p_e) {
    

    char const *inpl = "\
        int Foo(int a)      \n\
        {                   \n\
            return a*a;     \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("P_r_o_t_o_t_y_p_e", scrip);
    // run the test, comment out the previous line
    // and append its output below.
    // Then run the test in earnest after changes have been made to the code
}
*/


// The vars defined here are provided in each test that is in category "Bytecode0"
class Bytecode0 : public ::testing::Test
{
protected:
    AGS::ccCompiledScript scrip{ true };    // enable emitting line numbers

    Bytecode0()
    {
        // Initializations, will be done at the start of each test
        // Note, the parser doesn't react to SCOPT_LINENUMBERS, that's on ccCompiledScript
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        clear_error();
    }
};

TEST_F(Bytecode0, UnaryMinus1) {

    // Accept a unary minus in front of parens

    char const *inpl = "\
        void Foo()              \n\
        {                       \n\
            int bar = 5;        \n\
            int baz = -(-bar);  \n\
        }";

    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("UnaryMinus1", scrip);
    size_t const codesize = 43;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       5,   29,    3,   36,            4,   51,    4,    7,    // 15
       3,    6,    4,    0,           12,    4,    3,    3,    // 23
       4,    3,    6,    4,            0,   12,    4,    3,    // 31
       3,    4,    3,   29,            3,   36,    5,    2,    // 39
       1,    8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, UnaryMinus2) {    

    // Unary minus binds more than multiply

    char const *inpl = "\
        int main()                      \n\
        {                               \n\
            int five = 5;               \n\
            int seven = 7;              \n\
            return -five * -seven;      \n\
        }";

    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("UnaryMinus2", scrip);
    size_t const codesize = 60;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       5,   29,    3,   36,            4,    6,    3,    7,    // 15
      29,    3,   36,    5,           51,    8,    7,    3,    // 23
       6,    4,    0,   12,            4,    3,    3,    4,    // 31
       3,   29,    3,   51,            8,    7,    3,    6,    // 39
       4,    0,   12,    4,            3,    3,    4,    3,    // 47
      30,    4,    9,    4,            3,    3,    4,    3,    // 55
       2,    1,    8,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, NotNot) {

    // !!a should be interpreted as !(!a)

    char const *inpl = "\
        int main()                  \n\
        {                           \n\
            int five = 5;           \n\
            return !!(!five);       \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Notnot", scrip);

    size_t const codesize = 27;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       5,   29,    3,   36,            4,   51,    4,    7,    // 15
       3,   42,    3,   42,            3,   42,    3,    2,    // 23
       1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Float01) {   

    // Float values
    // Note that the code behind the 'return' statement is unreachable.

    char const *inpl = "\
        float Test0 = -9.9;                 \n\
        float main()                        \n\
        {                                   \n\
            float Test1 = -7.0;             \n\
            float Test2 = 7E2;              \n\
            float Test3 = -7E-2;            \n\
            float Test4 = -7.7E-0;          \n\
            float Test5 = 7.;               \n\
            float Test6 = 7.e-7;            \n\
            float Test7 = 007.e-07;         \n\
            float Test8 = .77;              \n\
            return Test1 + Test2 + Test3 +  \n\
                Test4 + Test5 + Test6 +     \n\
                Test7 + Test8;              \n\
            Test1 = 77.;                    \n\
        }                                   \n\
        ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("reach this point"));

    // WriteOutput("Float01", scrip);

    size_t const codesize = 189;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
    -1059061760,   29,    3,   36,            5,    6,    3, 1143930880,    // 15
      29,    3,   36,    6,            6,    3, -1114678231,   29,    // 23
       3,   36,    7,    6,            3, -1057593754,   29,    3,    // 31
      36,    8,    6,    3,         1088421888,   29,    3,   36,    // 39
       9,    6,    3, 893118370,           29,    3,   36,   10,    // 47
       6,    3, 893118370,   29,            3,   36,   11,    6,    // 55
       3, 1061494456,   29,    3,           36,   12,   51,   32,    // 63
       7,    3,   29,    3,           51,   32,    7,    3,    // 71
      30,    4,   57,    4,            3,    3,    4,    3,    // 79
      29,    3,   51,   28,            7,    3,   30,    4,    // 87
      57,    4,    3,    3,            4,    3,   29,    3,    // 95
      36,   13,   51,   24,            7,    3,   36,   12,    // 103
      30,    4,   57,    4,            3,    3,    4,    3,    // 111
      36,   13,   29,    3,           51,   20,    7,    3,    // 119
      30,    4,   57,    4,            3,    3,    4,    3,    // 127
      29,    3,   51,   16,            7,    3,   30,    4,    // 135
      57,    4,    3,    3,            4,    3,   29,    3,    // 143
      36,   14,   51,   12,            7,    3,   36,   13,    // 151
      30,    4,   57,    4,            3,    3,    4,    3,    // 159
      36,   14,   29,    3,           51,    8,    7,    3,    // 167
      30,    4,   57,    4,            3,    3,    4,    3,    // 175
       2,    1,   32,    5,           36,   15,    6,    3,    // 183
    1117388800,   51,   32,    8,            3,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Float02) {   

    // Positive and negative float parameter defaults

    char const *inpl = "\
        float sub (float p1 = 7.2,          \n\
                   float p2 = -2.7)         \n\
        {                                   \n\
            return -7.0 + p1 - p2;          \n\
        }                                   \n\
        float main()                        \n\
        {                                   \n\
            return sub();                   \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Float02", scrip);

    size_t const codesize = 63;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
    -1059061760,   29,    3,   51,           12,    7,    3,   30,    // 15
       4,   57,    4,    3,            3,    4,    3,   29,    // 23
       3,   51,   16,    7,            3,   30,    4,   58,    // 31
       4,    3,    3,    4,            3,    5,   36,    7,    // 39
      38,   38,   36,    8,            6,    3, -1070805811,   29,    // 47
       3,    6,    3, 1088841318,           29,    3,    6,    3,    // 55
       0,   23,    3,    2,            1,    8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      56,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Float03) { 

    char const *inpl = "\
        float a = 15.0;     \n\
        float Foo()         \n\
        {                   \n\
            float f = 3.14; \n\
            return a + f;   \n\
        }                   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Float03", scrip);

    size_t const codesize = 36;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
    1078523331,   29,    3,   36,            5,    6,    2,    0,    // 15
       7,    3,   29,    3,           51,    8,    7,    3,    // 23
      30,    4,   57,    4,            3,    3,    4,    3,    // 31
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Float04) {    

    char const *inpl = "\
        float a = 15.0;                             \n\
        float Foo()                                 \n\
        {                                           \n\
            float b = 22.2;                         \n\
            int E1 = (3.14 < 1.34) == 1;            \n\
            short E2 = 0 == (1234.5 > 5.0) && 1;    \n\
            long E3 = a <= 44.4;                    \n\
            char E4 = 55.5 >= 44.4;                 \n\
            int E5 = (((a == b) || (a != b)));      \n\
            return a - b * (a / b);                 \n\
        }";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Float04", scrip);

    size_t const codesize = 162;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
    1102158234,   29,    3,   36,            5,    6,    3,    0,    // 15
      29,    3,   36,    6,            6,    3,    0,   51,    // 23
       0,   27,    3,    1,            1,    2,   36,    7,    // 31
       6,    2,    0,    7,            3,   29,    3,    6,    // 39
       3, 1110546842,   30,    4,           62,    4,    3,    3,    // 47
       4,    3,   29,    3,           36,    8,    6,    3,    // 55
       1,   51,    0,   26,            3,    1,    1,    1,    // 63
      36,    9,    6,    2,            0,    7,    3,   29,    // 71
       3,   51,   19,    7,            3,   30,    4,   15,    // 79
       4,    3,    3,    4,            3,   70,   19,    6,    // 87
       2,    0,    7,    3,           29,    3,   51,   19,    // 95
       7,    3,   30,    4,           16,    4,    3,    3,    // 103
       4,    3,   29,    3,           36,   10,    6,    2,    // 111
       0,    7,    3,   29,            3,   51,   23,    7,    // 119
       3,   29,    3,    6,            2,    0,    7,    3,    // 127
      29,    3,   51,   31,            7,    3,   30,    4,    // 135
      56,    4,    3,    3,            4,    3,   30,    4,    // 143
      55,    4,    3,    3,            4,    3,   30,    4,    // 151
      58,    4,    3,    3,            4,    3,    2,    1,    // 159
      19,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      34,   68,   89,  112,        125,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowIfThenElse1) {

    char const *inpl = "\
    int Foo()               \n\
    {                       \n\
        readonly int vier = 4; \n\
        int a = 15 - vier * 2; \n\
        if (a < 5)          \n\
            a >>= 2;        \n\
        else                \n\
            a <<= 3;        \n\
        return a;           \n\
    }";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowIfThenElse1", scrip);

    size_t const codesize = 114;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       4,   29,    3,   36,            4,    6,    3,   15,    // 15
      29,    3,   51,    8,            7,    3,   29,    3,    // 23
       6,    3,    2,   30,            4,    9,    4,    3,    // 31
       3,    4,    3,   30,            4,   12,    4,    3,    // 39
       3,    4,    3,   29,            3,   36,    5,   51,    // 47
       4,    7,    3,   29,            3,    6,    3,    5,    // 55
      30,    4,   18,    4,            3,    3,    4,    3,    // 63
      28,   20,   36,    6,            6,    3,    2,   29,    // 71
       3,   51,    8,    7,            3,   30,    4,   44,    // 79
       3,    4,    8,    3,           31,   18,   36,    8,    // 87
       6,    3,    3,   29,            3,   51,    8,    7,    // 95
       3,   30,    4,   43,            3,    4,    8,    3,    // 103
      36,    9,   51,    4,            7,    3,    2,    1,    // 111
       8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowIfThenElse2) {

    char const *inpl = "\
    int Foo()               \n\
    {                       \n\
        readonly int deux = 2; \n\
        int a = 15 - 4 % deux; \n\
        if (a >= 5) {       \n\
            a -= 2;         \n\
        } else              \n\
            a += 3;         \n\
        return a;           \n\
    }";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowIfThenElse2", scrip);

    size_t const codesize = 114;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       2,   29,    3,   36,            4,    6,    3,   15,    // 15
      29,    3,    6,    3,            4,   29,    3,   51,    // 23
      12,    7,    3,   30,            4,   40,    4,    3,    // 31
       3,    4,    3,   30,            4,   12,    4,    3,    // 39
       3,    4,    3,   29,            3,   36,    5,   51,    // 47
       4,    7,    3,   29,            3,    6,    3,    5,    // 55
      30,    4,   19,    4,            3,    3,    4,    3,    // 63
      28,   20,   36,    6,            6,    3,    2,   29,    // 71
       3,   51,    8,    7,            3,   30,    4,   12,    // 79
       3,    4,    8,    3,           31,   18,   36,    8,    // 87
       6,    3,    3,   29,            3,   51,    8,    7,    // 95
       3,   30,    4,   11,            3,    4,    8,    3,    // 103
      36,    9,   51,    4,            7,    3,    2,    1,    // 111
       8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowWhile) {

    char const *inpl = "\
        char c = 'x';             \n\
        int Foo(int i, float f)   \n\
        {                         \n\
            int sum = 0;          \n\
            while (c >= 0)        \n\
            {                     \n\
                sum += (500 & c); \n\
                c--;              \n\
                if (c == 1)       \n\
                    continue;     \n\
            }                     \n\
            return sum;           \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowWhile", scrip);

    size_t const codesize = 114;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
       0,   29,    3,   36,            5,    6,    2,    0,    // 15
      24,    3,   29,    3,            6,    3,    0,   30,    // 23
       4,   19,    4,    3,            3,    4,    3,   28,    // 31
      71,   36,    7,    6,            3,  500,   29,    3,    // 39
       6,    2,    0,   24,            3,   30,    4,   13,    // 47
       4,    3,    3,    4,            3,   29,    3,   51,    // 55
       8,    7,    3,   30,            4,   11,    3,    4,    // 63
       8,    3,   36,    8,            6,    2,    0,   24,    // 71
       3,    2,    3,    1,           26,    3,   36,    9,    // 79
       6,    2,    0,   24,            3,   29,    3,    6,    // 87
       3,    1,   30,    4,           15,    4,    3,    3,    // 95
       4,    3,   28,    2,           31,  -91,   31,  -93,    // 103
      36,   12,   51,    4,            7,    3,    2,    1,    // 111
       4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,   42,   70,   82,        -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowWhileTrue)
{

    // Mustn't short-circuit the 'while()' body

    char const *inpl = "\
        enum bool { false = 0, true }; \n\
        int main()          \n\
        {                   \n\
            while (true)    \n\
            {               \n\
                int i = 5;  \n\
            }               \n\
        }                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowWhileTrue", scrip);

    size_t const codesize = 24;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    6,    6,    3,    // 7
       5,   29,    3,   36,            7,    2,    1,    4,    // 15
      31,  -14,   36,    8,            6,    3,    0,    5,    // 23
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowDoWhileFalse)
{

    // Don't emit back jump

    char const *inpl = "\
        enum bool { false = 0, true }; \n\
        int main()              \n\
        {                       \n\
            do                  \n\
            {                   \n\
                int i = 5;      \n\
                continue;       \n\
            } while (false);    \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowDoWhileFalse", scrip);

    size_t const codesize = 29;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    6,    6,    3,    // 7
       5,   29,    3,   36,            7,    2,    1,    4,    // 15
      31,  -14,   36,    8,            2,    1,    4,   36,    // 23
       9,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowDoNCall) {

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowDoNCall", scrip);

    size_t const codesize = 123;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
       0,   29,    3,   36,            7,    6,    3,  500,    // 15
      29,    3,    6,    2,            0,   24,    3,   30,    // 23
       4,   14,    4,    3,            3,    4,    3,   29,    // 31
       3,   51,    8,    7,            3,   30,    4,   12,    // 39
       3,    4,    8,    3,           36,    8,    6,    2,    // 47
       0,   24,    3,    2,            3,    1,   26,    3,    // 55
      36,   10,    6,    2,            0,   24,    3,   29,    // 63
       3,    6,    3,    0,           30,    4,   17,    4,    // 71
       3,    3,    4,    3,           70,  -67,   36,   11,    // 79
      51,    4,    7,    3,            2,    1,    4,    5,    // 87
      36,   15,   38,   88,           36,   16,   51,    8,    // 95
       7,    3,   29,    3,           51,   12,    7,    3,    // 103
      30,    4,   41,    4,            3,    3,    4,    3,    // 111
      29,    3,    6,    3,            0,   23,    3,    2,    // 119
       1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      20,   48,   60,  116,        -999
    };
    char fixuptypes[] = {
      1,   1,   1,   2,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowDoUnbracedIf) {    

    char const *inpl = "\
    void noloopcheck main()   \n\
    {                         \n\
        int sum = 0;          \n\
        do                    \n\
            if (sum < 100)    \n\
                sum += 10;    \n\
            else              \n\
                break;        \n\
        while (sum >= -1);    \n\
    }                         \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowDoUnbracedIf", scrip);

    size_t const codesize = 84;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           68,   36,    3,    6,    // 7
       3,    0,   29,    3,           36,    5,   51,    4,    // 15
       7,    3,   29,    3,            6,    3,  100,   30,    // 23
       4,   18,    4,    3,            3,    4,    3,   28,    // 31
      20,   36,    6,    6,            3,   10,   29,    3,    // 39
      51,    8,    7,    3,           30,    4,   11,    3,    // 47
       4,    8,    3,   31,            4,   36,    8,   31,    // 55
      21,   36,    9,   51,            4,    7,    3,   29,    // 63
       3,    6,    3,   -1,           30,    4,   19,    4,    // 71
       3,    3,    4,    3,           70,  -66,   36,   10,    // 79
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}


TEST_F(Bytecode0, FlowFor1) {  

    char const *inpl = "\
        int loop;                       \n\
        int Foo(int i, float f)         \n\
        {                               \n\
            for (loop = 0; loop < 10; loop += 3)  \n\
            {                           \n\
                int sum = loop - 4 - 7; \n\
                if (loop == 6)          \n\
                    break;              \n\
            }                           \n\
            return 0;                   \n\
        }";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor1", scrip);
    size_t const codesize = 134;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
       0,    6,    2,    0,            8,    3,   31,   19,    // 15
      36,    4,    6,    3,            3,   29,    3,    6,    // 23
       2,    0,    7,    3,           30,    4,   11,    3,    // 31
       4,    8,    3,   36,            4,    6,    2,    0,    // 39
       7,    3,   29,    3,            6,    3,   10,   30,    // 47
       4,   18,    4,    3,            3,    4,    3,   28,    // 55
      71,   36,    6,    6,            2,    0,    7,    3,    // 63
      29,    3,    6,    3,            4,   30,    4,   12,    // 71
       4,    3,    3,    4,            3,   29,    3,    6,    // 79
       3,    7,   30,    4,           12,    4,    3,    3,    // 87
       4,    3,   29,    3,           36,    7,    6,    2,    // 95
       0,    7,    3,   29,            3,    6,    3,    6,    // 103
      30,    4,   15,    4,            3,    3,    4,    3,    // 111
      28,    7,   36,    8,            2,    1,    4,   31,    // 119
       7,   36,    9,    2,            1,    4,   31, -112,    // 127
      36,   10,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   25,   39,   61,         96,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor2) {

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor2", scrip);

    size_t const codesize = 324;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,    6,            3,    0,   29,    3,    // 15
      31,   18,   36,    4,            6,    3,    1,   29,    // 23
       3,   51,   12,    7,            3,   30,    4,   11,    // 31
       3,    4,    8,    3,           36,    5,   51,    8,    // 39
       7,    3,   29,    3,           51,    8,    7,    3,    // 47
      30,    4,   11,    3,            4,    8,    3,   31,    // 55
     -39,   36,    7,   51,            8,    7,    3,   29,    // 63
       3,   51,    8,    7,            3,   30,    4,   12,    // 71
       3,    4,    8,    3,           31,  -21,   31,   18,    // 79
      36,    8,    6,    3,            3,   29,    3,   51,    // 87
      12,    7,    3,   30,            4,   11,    3,    4,    // 95
       8,    3,   36,    8,           51,    8,    7,    3,    // 103
      29,    3,    6,    3,            2,   30,    4,   18,    // 111
       4,    3,    3,    4,            3,   28,   21,   36,    // 119
       9,   51,    8,    7,            3,   29,    3,   51,    // 127
       8,    7,    3,   30,            4,    9,    3,    4,    // 135
       8,    3,   31,  -60,           36,   10,   51,    8,    // 143
       7,    3,   29,    3,            6,    3,    4,   30,    // 151
       4,   18,    4,    3,            3,    4,    3,   28,    // 159
      21,   36,   11,   51,            8,    7,    3,   29,    // 167
       3,   51,    8,    7,            3,   30,    4,   10,    // 175
       3,    4,    8,    3,           31,  -42,   36,   12,    // 183
       6,    3,    5,   51,            8,    8,    3,   31,    // 191
      18,   36,   12,    6,            3,    6,   29,    3,    // 199
      51,   12,    7,    3,           30,    4,   11,    3,    // 207
       4,    8,    3,   36,           13,   51,    8,    7,    // 215
       3,   29,    3,   51,            8,    7,    3,   30,    // 223
       4,   10,    3,    4,            8,    3,   31,  -39,    // 231
      36,   14,    6,    3,            7,   29,    3,   36,    // 239
      15,   51,    4,    7,            3,   29,    3,   51,    // 247
      12,    7,    3,   30,            4,   13,    3,    4,    // 255
       8,    3,   31,  -21,            2,    1,    4,   36,    // 263
      16,    6,    3,    8,           29,    3,   36,   16,    // 271
      51,    4,    7,    3,           29,    3,    6,    3,    // 279
       9,   30,    4,   18,            4,    3,    3,    4,    // 287
       3,   28,   21,   36,           17,   51,    4,    7,    // 295
       3,   29,    3,   51,           12,    7,    3,   30,    // 303
       4,   14,    3,    4,            8,    3,   31,  -42,    // 311
       2,    1,    4,   36,           18,    6,    3,    0,    // 319
       2,    1,    8,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor3) {

    char const *inpl = "\
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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor3", scrip);
    size_t const codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,
      36,    9,   51,    0,    // 7
      49,    1,    1,    4,           36,   11,   51,    4,    // 15
      48,    3,   29,    3,            6,    2,    0,   48,    // 23
       3,   30,    4,   15,            4,    3,    3,    4,    // 31
       3,   51,    4,   49,            2,    1,    4,    5,    // 39
      31,  -30,   36,   12,           51,    4,   49,    2,    // 47
       1,    4,   36,   13,            6,    3,   -7,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      22,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor4) {

    char const *inpl = "\
    void main()                     \n\
    {                               \n\
        for (int Loop = 0; Loop < 10; Loop++)  \n\
            if (Loop == 5)          \n\
                continue;           \n\
    }                               \n\
    ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor4", scrip);

    size_t const codesize = 78;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,   31,           11,   36,    3,   51,    // 15
       4,    7,    3,    1,            3,    1,    8,    3,    // 23
      36,    3,   51,    4,            7,    3,   29,    3,    // 31
       6,    3,   10,   30,            4,   18,    4,    3,    // 39
       3,    4,    3,   28,           25,   36,    4,   51,    // 47
       4,    7,    3,   29,            3,    6,    3,    5,    // 55
      30,    4,   15,    4,            3,    3,    4,    3,    // 63
      28,    2,   31,  -55,           31,  -57,   36,    5,    // 71
       2,    1,    4,   36,            6,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor5) {

    char const *inpl = "\
        int Start()                     \n\
        {                               \n\
            return 1;                   \n\
        }                               \n\
        int Check()                     \n\
        {                               \n\
            return 10;                  \n\
        }                               \n\
        int Cont(int x)                 \n\
        {                               \n\
            return x+1;                 \n\
        }                               \n\
                                        \n\
        void main()                     \n\
        {                               \n\
            for(int i = Start(); i < Check(); i = Cont(i))   \n\
                if (i >= 0)             \n\
                    continue;           \n\
        }                               \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor5", scrip);

    size_t const codesize = 135;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       1,    5,   36,    6,           38,   10,   36,    7,    // 15
       6,    3,   10,    5,           36,   10,   38,   20,    // 23
      36,   11,   51,    8,            7,    3,   29,    3,    // 31
       6,    3,    1,   30,            4,   11,    4,    3,    // 39
       3,    4,    3,    5,           36,   15,   38,   44,    // 47
      36,   16,    6,    3,            0,   23,    3,   29,    // 55
       3,   31,   20,   36,           16,   51,    4,    7,    // 63
       3,   29,    3,    6,            3,   20,   23,    3,    // 71
       2,    1,    4,   51,            4,    8,    3,   36,    // 79
      16,   51,    4,    7,            3,   29,    3,    6,    // 87
       3,   10,   23,    3,           30,    4,   18,    4,    // 95
       3,    3,    4,    3,           28,   25,   36,   17,    // 103
      51,    4,    7,    3,           29,    3,    6,    3,    // 111
       0,   30,    4,   19,            4,    3,    3,    4,    // 119
       3,   28,    2,   31,          -66,   31,  -68,   36,    // 127
      18,    2,    1,    4,           36,   19,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      52,   69,   89,  -999
    };
    char fixuptypes[] = {
      2,   2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor6) {

    char const *inpl = "\
        void main()                     \n\
        {                               \n\
            for(int i = Start();        \n\
                i < Check();            \n\
                i = Cont(i))            \n\
                if (i >= 0)             \n\
                    continue;           \n\
        }                               \n\
        int Start()                     \n\
        {                               \n\
            return 1;                   \n\
        }                               \n\
        int Check()                     \n\
        {                               \n\
            return 10;                  \n\
        }                               \n\
        int Cont(int x)                 \n\
        {                               \n\
            return x + 1;               \n\
        }                               \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor6", scrip);

    size_t const codesize = 135;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
      91,   23,    3,   29,            3,   31,   20,   36,    // 15
       5,   51,    4,    7,            3,   29,    3,    6,    // 23
       3,  111,   23,    3,            2,    1,    4,   51,    // 31
       4,    8,    3,   36,            4,   51,    4,    7,    // 39
       3,   29,    3,    6,            3,  101,   23,    3,    // 47
      30,    4,   18,    4,            3,    3,    4,    3,    // 55
      28,   25,   36,    6,           51,    4,    7,    3,    // 63
      29,    3,    6,    3,            0,   30,    4,   19,    // 71
       4,    3,    3,    4,            3,   28,    2,   31,    // 79
     -66,   31,  -68,   36,            7,    2,    1,    4,    // 87
      36,    8,    5,   36,           10,   38,   91,   36,    // 95
      11,    6,    3,    1,            5,   36,   14,   38,    // 103
     101,   36,   15,    6,            3,   10,    5,   36,    // 111
      18,   38,  111,   36,           19,   51,    8,    7,    // 119
       3,   29,    3,    6,            3,    1,   30,    4,    // 127
      11,    4,    3,    3,            4,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   25,   45,  -999
    };
    char fixuptypes[] = {
      2,   2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor7) {

    // Initializer and iterator of a for() need not be assignments,
    // they can be func calls.
    // Note, an error happened when 'Check()' and 'Cont()' were on
    // the same line, so leave them on the same line in this test

    char const *inpl = "\
        int i;                          \n\
        void main()                     \n\
        {                               \n\
            for (Start();               \n\
                 Check(); Cont())       \n\
                if (i >= 5)             \n\
                    i = 100 - i;        \n\
        }                               \n\
        short Start()                   \n\
        {                               \n\
            i = 1;                      \n\
            return -77;                 \n\
        }                               \n\
        int Check()                     \n\
        {                               \n\
            return i < 10;              \n\
        }                               \n\
        void Cont()                     \n\
        {                               \n\
            i++;                        \n\
        }                               \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor7", scrip);

    size_t const codesize = 145;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    3,    // 7
      81,   23,    3,   31,            7,   36,    5,    6,    // 15
       3,  126,   23,    3,           36,    5,    6,    3,    // 23
     101,   23,    3,   28,           49,   36,    6,    6,    // 31
       2,    0,    7,    3,           29,    3,    6,    3,    // 39
       5,   30,    4,   19,            4,    3,    3,    4,    // 47
       3,   28,   25,   36,            7,    6,    3,  100,    // 55
      29,    3,    6,    2,            0,    7,    3,   30,    // 63
       4,   12,    4,    3,            3,    4,    3,    6,    // 71
       2,    0,    8,    3,           31,  -65,   36,    8,    // 79
       5,   36,   10,   38,           81,   36,   11,    6,    // 87
       3,    1,    6,    2,            0,    8,    3,   36,    // 95
      12,    6,    3,  -77,            5,   36,   15,   38,    // 103
     101,   36,   16,    6,            2,    0,    7,    3,    // 111
      29,    3,    6,    3,           10,   30,    4,   18,    // 119
       4,    3,    3,    4,            3,    5,   36,   19,    // 127
      38,  126,   36,   20,            6,    2,    0,    7,    // 135
       3,    1,    3,    1,            8,    3,   36,   21,    // 143
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 9;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   17,   24,   33,         60,   73,   92,  109,    // 7
     134,  -999
    };
    char fixuptypes[] = {
      2,   2,   2,   1,      1,   1,   1,   1,    // 7
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowContinue1) {   

    // Locals only become invalid at the end of their nesting; below a "continue",
    // they remain valid, and so the offset to start of the local block
    // must not be reduced.

    char const *inpl = "\
        int main()                      \n\
        {                               \n\
            int I;                      \n\
            for(I = -1; I < 1; I++)     \n\
            {                           \n\
                int A = 7;              \n\
                int B = 77;             \n\
                if (I >= 0)             \n\
                    continue;           \n\
                int C = A;              \n\
            }                           \n\
            return I;                   \n\
        }                               \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowContinue1", scrip);

    size_t const codesize = 121;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,   36,            4,    6,    3,   -1,    // 15
      51,    4,    8,    3,           31,   11,   36,    4,    // 23
      51,    4,    7,    3,            1,    3,    1,    8,    // 31
       3,   36,    4,   51,            4,    7,    3,   29,    // 39
       3,    6,    3,    1,           30,    4,   18,    4,    // 47
       3,    3,    4,    3,           28,   57,   36,    6,    // 55
       6,    3,    7,   29,            3,   36,    7,    6,    // 63
       3,   77,   29,    3,           36,    8,   51,   12,    // 71
       7,    3,   29,    3,            6,    3,    0,   30,    // 79
       4,   19,    4,    3,            3,    4,    3,   28,    // 87
       7,   36,    9,    2,            1,    8,   31,  -74,    // 95
      36,   10,   51,    8,            7,    3,   29,    3,    // 103
      36,   11,    2,    1,           12,   31,  -89,   36,    // 111
      12,   51,    4,    7,            3,    2,    1,    4,    // 119
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowIfDoWhile) {

    char const *inpl = "\
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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowIfDoWhile", scrip);

    size_t const codesize = 190;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       5,   29,    3,    6,            3,    0,   29,    3,    // 15
       6,    3,   -2,   29,            3,   36,    4,   51,    // 23
      12,    7,    3,   29,            3,    6,    3,   10,    // 31
      30,    4,   18,    4,            3,    3,    4,    3,    // 39
      28,  102,   36,    5,            6,    3,    0,   51,    // 47
       4,    8,    3,   31,           18,   36,    5,    6,    // 55
       3,    3,   29,    3,           51,    8,    7,    3,    // 63
      30,    4,   11,    3,            4,    8,    3,   36,    // 71
       5,   51,    4,    7,            3,   29,    3,    6,    // 79
       3,   10,   30,    4,           18,    4,    3,    3,    // 87
       4,    3,   28,   50,           36,    7,   51,    4,    // 95
       7,    3,   29,    3,           51,   12,    7,    3,    // 103
      30,    4,   11,    3,            4,    8,    3,   36,    // 111
       8,   51,    4,    7,            3,   29,    3,    6,    // 119
       3,    6,   30,    4,           15,    4,    3,    3,    // 127
       4,    3,   28,    8,           51,    4,    7,    3,    // 135
       2,    1,   12,    5,           31,  -89,   31,   37,    // 143
      36,   11,    6,    3,            1,   29,    3,   51,    // 151
       8,    7,    3,   30,            4,   11,    3,    4,    // 159
       8,    3,   51,    4,            7,    3,   29,    3,    // 167
       6,    3,  100,   30,            4,   18,    4,    3,    // 175
       3,    4,    3,   70,          -37,   36,   12,    6,    // 183
       3,    0,    2,    1,           12,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch01) {    

    // The "break;" in line 6 is unreachable code

    char const *inpl = "\
    int Foo(int i, float f)         \n\
    {                               \n\
        switch (i * i)              \n\
        {                           \n\
        case 2: return 10;          \n\
                break;              \n\
        default: i *= 2; return i;  \n\
        case 3:                     \n\
        case 4: i = 0;              \n\
        case 5: i += 5 - i - 4;     \n\
                break;              \n\
        }                           \n\
        return 0;                   \n\
    }";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, AGS::FlagSet{ 0 }, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    // Line 6: Can't reach this point.
    ASSERT_LE(2u, mh.GetMessages().size());
    EXPECT_EQ(6u, mh.GetMessages().at(0).Lineno);
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("reach this"));
    // Line 10: Cases 3 & 4 fall through to case 5; "break;" might be missing
    EXPECT_EQ(6u, mh.GetMessages().at(0).Lineno);
    EXPECT_NE(std::string::npos, mh.GetMessages().at(1).Message.find("break"));

    // WriteOutput("FlowSwitch01", scrip);

    size_t const codesize = 170;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,   51,    8,    // 7
       7,    3,   29,    3,           51,   12,    7,    3,    // 15
      30,    4,    9,    4,            3,    3,    4,    3,    // 23
      36,    4,    3,    3,            4,   31,   91,   36,    // 31
       5,    6,    3,   10,            5,   36,    6,   31,    // 39
     123,   36,    7,    6,            3,    2,   29,    3,    // 47
      51,   12,    7,    3,           30,    4,    9,    3,    // 55
       4,    8,    3,   51,            8,    7,    3,    5,    // 63
      36,    9,    6,    3,            0,   51,    8,    8,    // 71
       3,   36,   10,    6,            3,    5,   29,    3,    // 79
      51,   12,    7,    3,           30,    4,   12,    4,    // 87
       3,    3,    4,    3,           29,    3,    6,    3,    // 95
       4,   30,    4,   12,            4,    3,    3,    4,    // 103
       3,   29,    3,   51,           12,    7,    3,   30,    // 111
       4,   11,    3,    4,            8,    3,   36,   11,    // 119
      31,   42,   36,    5,            6,    3,    2,   15,    // 127
       3,    4,   70, -101,           36,    8,    6,    3,    // 135
       3,   15,    3,    4,           70,  -78,   36,    9,    // 143
       6,    3,    4,   15,            3,    4,   70,  -88,    // 151
      36,   10,    6,    3,            5,   15,    3,    4,    // 159
      70,  -89,   31, -123,           36,   13,    6,    3,    // 167
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch02) {

    // Last switch clause no "break"
    char const *inpl = "\
        void main()                     \n\
        {                               \n\
            int i = 5;                  \n\
            switch(i)                   \n\
            {                           \n\
            default: break;             \n\
            case 5: i = 0;              \n\
            }                           \n\
            return;                     \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowSwitch02", scrip);

    size_t const codesize = 57;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       5,   29,    3,   36,            5,   51,    4,    7,    // 15
       3,    3,    3,    4,           31,   17,   36,    6,    // 23
      31,   25,   36,    7,            6,    3,    0,   51,    // 31
       4,    8,    3,   36,            8,   31,   12,   36,    // 39
       7,    6,    3,    5,           15,    3,    4,   70,    // 47
     -23,   31,  -29,   36,            9,    2,    1,    4,    // 55
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch03) {
    // Last case clause an empty pair of braces
    char const *inpl = "\
        int main()          \n\
        {                   \n\
            int test = 0;   \n\
            switch (test)   \n\
            {               \n\
            case 0:         \n\
                {           \n\
                }           \n\
            }               \n\
        }                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowSwitch03", scrip);

    size_t const codesize = 45;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,   36,            5,   51,    4,    7,    // 15
       3,    3,    3,    4,           31,    4,   36,    9,    // 23
      31,   10,   36,    6,            6,    3,    0,   15,    // 31
       3,    4,   70,  -14,           36,   10,    2,    1,    // 39
       4,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch04) {

    // Last case clause an empty pair of braces

    char const *inpl = "\
        int main()          \n\
        {                   \n\
            int test = 7;   \n\
            switch (test)   \n\
            {               \n\
            default:        \n\
                {           \n\
                }           \n\
            }               \n\
        }                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowSwitch04", scrip);

    size_t const codesize = 37;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       7,   29,    3,   36,            5,   51,    4,    7,    // 15
       3,    3,    3,    4,           31,    4,   36,    9,    // 23
      31,    2,   31,   -6,           36,   10,    2,    1,    // 31
       4,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch05) {

    // No 'default'/'case' clauses (zany but allowed)

    char const *inpl = "\
        int main()          \n\
        {                   \n\
            int test = 0;   \n\
            switch (test)   \n\
            {               \n\
            }               \n\
        }                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowSwitch05", scrip);

    size_t const codesize = 20;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,   36,            7,    2,    1,    4,    // 15
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch06) {

    // 'Default' and 'case 77' fall through, compiler should warn about the latter

    char const *inpl = "\
        int main()              \n\
        {                       \n\
            int test = 7;       \n\
            switch (test)       \n\
            {                   \n\
            default:            \n\
                test = 6;       \n\
                fallthrough;    \n\
            case 7:             \n\
            case 77:            \n\
                test = 5;       \n\
            case 777:           \n\
            }                   \n\
        }                       \n\
        ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0u).Message.find("execution"));
    EXPECT_EQ(11u, mh.GetMessages().at(0u).Lineno);

    // WriteOutput("FlowSwitch06", scrip);

    size_t const codesize = 85;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       7,   29,    3,   36,            5,   51,    4,    7,    // 15
       3,    3,    3,    4,           31,   22,   36,    7,    // 23
       6,    3,    6,   51,            4,    8,    3,   36,    // 31
      11,    6,    3,    5,           51,    4,    8,    3,    // 39
      36,   13,   31,   32,           36,    9,    6,    3,    // 47
       7,   15,    3,    4,           70,  -23,   36,   10,    // 55
       6,    3,   77,   15,            3,    4,   70,  -33,    // 63
      36,   12,    6,    3,          777,   15,    3,    4,    // 71
      70,  -34,   31,  -54,           36,   14,    2,    1,    // 79
       4,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowSwitch07) {

    // Expression in switch clause

    char const *inpl = "\
        readonly int two = 2;   \n\
        int main()              \n\
        {                       \n\
            switch (7)          \n\
            {                   \n\
            case two + 3:       \n\
                return 7;       \n\
            case main():        \n\
                return 11;      \n\
            case (two):         \n\
                break;          \n\
            }                   \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowSwitch07", scrip);

    size_t const codesize = 93;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    5,    6,    3,    // 7
       7,    3,    3,    4,           31,   16,   36,    7,    // 15
       6,    3,    7,    5,           36,    9,    6,    3,    // 23
      11,    5,   36,   11,           31,   57,   36,    6,    // 31
      29,    4,    6,    2,            0,    7,    3,   29,    // 39
       3,    6,    3,    3,           30,    4,   11,    4,    // 47
       3,    3,    4,    3,           30,    4,   15,    3,    // 55
       4,   70,  -45,   36,            8,   29,    4,    6,    // 63
       3,    0,   23,    3,           30,    4,   15,    3,    // 71
       4,   70,  -55,   36,           10,    6,    2,    0,    // 79
       7,    3,   15,    3,            4,   70,  -61,   36,    // 87
      13,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      36,   65,   79,  -999
    };
    char fixuptypes[] = {
      1,   2,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FreeLocalPtr) {   

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FreeLocalPtr", scrip);
    size_t const codesize = 83;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,   73,    3,    // 7
       4,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,   10,    6,    3,            0,   29,    3,   31,    // 23
      11,   36,   10,   51,            4,    7,    3,    1,    // 31
       3,    1,    8,    3,           36,   10,   51,    4,    // 39
       7,    3,   29,    3,            6,    3,   10,   30,    // 47
       4,   18,    4,    3,            3,    4,    3,   28,    // 55
      11,   36,   11,   73,            3,    4,   51,    8,    // 63
      47,    3,   31,  -43,            2,    1,    4,   36,    // 71
      12,   51,    4,   49,            2,    1,    4,    6,    // 79
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct01) {

    char const *inpl = "\
    	struct Struct                       \n\
		{                                   \n\
			float Float;                    \n\
			import int[] Func(int i);       \n\
		};                                  \n\
                                            \n\
		int[] Struct::Func(int i)           \n\
		{                                   \n\
			int Ret[];                      \n\
			this.Float = 0.0;               \n\
			Ret = new int[5];               \n\
			Ret[3] = 77;                    \n\
			return Ret;                     \n\
		}                                   \n\
                                            \n\
		void main()                         \n\
		{                                   \n\
			Struct S;                       \n\
			int I[] = S.Func(-1);           \n\
			int J = I[3];                   \n\
		}                                   \n\
    ";
 
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct01", scrip);

    size_t const codesize = 145;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,   51,    0,    // 7
      49,    1,    1,    4,           36,   10,    3,    6,    // 15
       2,   52,    6,    3,            0,    8,    3,   36,    // 23
      11,    6,    3,    5,           72,    3,    4,    0,    // 31
      51,    4,   47,    3,           36,   12,   51,    4,    // 39
      48,    2,   52,    6,            3,   77,    1,    2,    // 47
      12,    8,    3,   36,           13,   51,    4,   48,    // 55
       3,   29,    3,   51,            4,   50,    3,   51,    // 63
       8,   49,   51,    4,           48,    3,   69,   30,    // 71
       4,    2,    1,    4,            5,   36,   17,   38,    // 79
      77,   36,   18,    6,            3,    0,   29,    3,    // 87
      36,   19,   51,    4,           29,    2,    6,    3,    // 95
      -1,   29,    3,   51,            8,    7,    2,   45,    // 103
       2,    6,    3,    0,           23,    3,    2,    1,    // 111
       4,   30,    2,   51,            0,   47,    3,    1,    // 119
       1,    4,   36,   20,           51,    4,   48,    2,    // 127
      52,    1,    2,   12,            7,    3,   29,    3,    // 135
      36,   21,   51,    8,           49,    2,    1,   12,    // 143
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
     107,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct02) {   

    // test arrays; arrays in structs;
    // whether the namespace in structs is independent of the global namespace

    char const *inpl = "\
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

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct02", scrip);

    size_t const codesize = 75;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   10,   38,    0,           36,   11,    6,    3,    // 7
       5,    6,    2,   68,            8,    3,   36,   12,    // 15
       6,    3,    3,    6,            2,   80,    8,    3,    // 23
      36,   13,    6,    2,           80,    7,    3,   46,    // 31
       3,   17,   32,    3,            4,    6,    2,    0,    // 39
      11,    2,    3,    6,            3,   42,    8,    3,    // 47
      36,   14,    6,    2,           68,    7,    3,   46,    // 55
       3,   17,   32,    3,            4,    6,    2,    0,    // 63
      11,    2,    3,    6,            3,   19,    8,    3,    // 71
      36,   15,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   21,   28,   39,         52,   63,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct03) {    

    // test arrays; arrays in structs;
    // whether the namespace in structs is independent of the global namespace

    char const *inpl = "\
    struct Struct1                  \n\
    {                               \n\
        int Array[17], Ix;          \n\
    } S;                            \n\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct03", scrip);

    size_t const codesize = 75;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,    6,    3,    // 7
       5,    6,    2,   68,            8,    3,   36,   10,    // 15
       6,    3,    3,    6,            2,   80,    8,    3,    // 23
      36,   11,    6,    2,           80,    7,    3,   46,    // 31
       3,   17,   32,    3,            4,    6,    2,    0,    // 39
      11,    2,    3,    6,            3,   42,    8,    3,    // 47
      36,   12,    6,    2,           68,    7,    3,   46,    // 55
       3,   17,   32,    3,            4,    6,    2,    0,    // 63
      11,    2,    3,    6,            3,   19,    8,    3,    // 71
      36,   13,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   21,   28,   39,         52,   63,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct04) {

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct04", scrip);

    size_t const codesize = 106;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   13,   38,    0,           36,   14,   51,    0,    // 7
      63,   16,    1,    1,           16,   36,   15,   73,    // 15
       3,    4,   51,   16,           47,    3,   36,   16,    // 23
      51,   16,   48,    2,           52,    6,    3, 12345,    // 31
       8,    3,   36,   17,           51,    0,   63,   48,    // 39
       1,    1,   48,   36,           18,   73,    3,    4,    // 47
      51,   16,   47,    3,           36,   19,   51,   64,    // 55
      49,    1,    2,    4,           49,    1,    2,    4,    // 63
      49,    1,    2,    4,           49,   51,   48,    6,    // 71
       3,    3,   29,    2,           49,    1,    2,    4,    // 79
      49,    1,    2,    4,           49,    1,    2,    4,    // 87
      49,   30,    2,    1,            2,   16,    2,    3,    // 95
       1,   70,  -25,    2,            1,   64,    6,    3,    // 103
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct05) {

    // The struct 'Struct0' has zero size, its only component is a function.
    // Allow this, but warn whenever a variable of type 'Struct0' is declared.
    // Note that AGS currently doesn't offer any possibility to compare
    // non-dynamic structs as a whole.

    char const *inpl = "\
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
   
    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    EXPECT_LE(2u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0u).Message.find("zero"));
    EXPECT_NE(std::string::npos, mh.GetMessages().at(1u).Message.find("zero"));

    // WriteOutput("Struct05", scrip);

    size_t const codesize = 32;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,   10,    6,    3,    // 7
       7,   34,    3,   39,            1,    6,    3,    0,    // 15
      33,    3,   35,    1,           34,    3,   39,    1,    // 23
       6,    3,    0,   33,            3,   35,    1,    5,    // 31
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,   26,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "StructO::StInt^1",            "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct06) {

    // NOTE: S1.Array[3] is null, so S1.Array[3].Payload should dump
    // when executed in real.

    char const *inpl = "\
        managed struct Struct0;                             \n\
                                                            \n\
        struct Struct1                                      \n\
        {                                                   \n\
            Struct0 *Array[];                               \n\
        };                                                  \n\
                                                            \n\
        managed struct Struct0                              \n\
        {                                                   \n\
            int Payload;                                    \n\
        };                                                  \n\
                                                            \n\
        int main()                                          \n\
        {                                                   \n\
            Struct1 S1;                                     \n\
            S1.Array = new Struct0[5];                      \n\
            S1.Array[3].Payload = 77;                       \n\
        }                                                   \n\
    ";

    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct06", scrip);

    size_t const codesize = 54;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   14,   38,    0,           36,   15,    6,    3,    // 7
       0,   29,    3,   36,           16,    6,    3,    5,    // 15
      72,    3,    4,    1,           51,    4,   47,    3,    // 23
      36,   17,   51,    4,           48,    2,   52,    1,    // 31
       2,   12,   48,    2,           52,    6,    3,   77,    // 39
       8,    3,   36,   18,           51,    4,   49,    2,    // 47
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct07) {    

    char const *inpl = "\
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
            S1[1].CPayload[1] = S1[1].CPayload[0] - 'A';     \n\
            S1[1].CPayload[0] --;                            \n\
            return 0;                                        \n\
        }                                                    \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct07", scrip);

    size_t const codesize = 67;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   10,   38,    0,           36,   11,    6,    3,    // 7
       0,    6,    2,    8,            8,    3,   36,   12,    // 15
       6,    3,   65,    6,            2,   12,   26,    3,    // 23
      36,   13,    6,    2,           12,   24,    3,   29,    // 31
       3,    6,    3,   65,           30,    4,   12,    4,    // 39
       3,    3,    4,    3,            6,    2,   13,   26,    // 47
       3,   36,   14,    6,            2,   12,   24,    3,    // 55
       2,    3,    1,   26,            3,   36,   15,    6,    // 63
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   21,   28,   46,         53,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct08) {   

    char const *inpl = "\
        struct Struct                           \n\
        {                                       \n\
            int k;                              \n\
        };                                      \n\
                                                \n\
        struct Sub extends Struct               \n\
        {                                       \n\
            int l;                              \n\
        };                                      \n\
                                                \n\
        int Func(this Sub *, int i, int j)      \n\
        {                                       \n\
            return !i                           \n\
                   ||                           \n\
                   !(j)                         \n\
                   &&                           \n\
                   this.k                       \n\
                   ||                           \n\
                    (0                          \n\
                     !=                         \n\
                     this.l                     \n\
                    )                           \n\
                   ;                            \n\
        }                                       \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct08", scrip);

    size_t const codesize = 71;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   12,   38,    0,           36,   13,   51,    8,    // 7
       7,    3,   42,    3,           36,   14,   70,   20,    // 15
      36,   15,   51,   12,            7,    3,   42,    3,    // 23
      36,   16,   28,    8,           36,   17,    3,    6,    // 31
       2,   52,    7,    3,           36,   18,   70,   28,    // 39
      36,   19,    6,    3,            0,   29,    3,   36,    // 47
      21,    3,    6,    2,           52,    1,    2,    4,    // 55
       7,    3,   36,   20,           30,    4,   16,    4,    // 63
       3,    3,    4,    3,           36,   23,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct09) {

    // Should be able to find SetCharacter as a component of
    // VehicleBase as an extension of Vehicle Cars[5];
    // should generate call of VehicleBase::SetCharacter()

    char const *inpl = "\
        enum CharacterDirection                                     \n\
        {                                                           \n\
            eDirectionUp = 3                                        \n\
        };                                                          \n\
                                                                    \n\
        builtin managed struct Character                            \n\
        {                                                           \n\
            readonly import attribute int ID;                       \n\
        };                                                          \n\
        import Character character[7];                              \n\
        import Character cAICar1;                                   \n\
                                                                    \n\
        struct VehicleBase                                          \n\
        {                                                           \n\
            import void SetCharacter(Character *c,                  \n\
                                int carSprite,                      \n\
                                CharacterDirection carSpriteDir,    \n\
                                int view = 0,                       \n\
                                int loop = 0,                       \n\
                                int frame = 0);                     \n\
        };                                                          \n\
                                                                    \n\
        struct Vehicle extends VehicleBase                          \n\
        {                                                           \n\
            float bodyMass;                                         \n\
        };                                                          \n\
        import Vehicle Cars[6];                                     \n\
                                                                    \n\
        int main()                                                  \n\
        {                                                           \n\
            int drivers[] = new int[6];                             \n\
            int i = 5;                                              \n\
            Cars[i].SetCharacter(                                   \n\
                character[cAICar1.ID + i],                          \n\
                7 + drivers[i],                                     \n\
                eDirectionUp,                                       \n\
                3 + i, 0, 0);                                       \n\
        }                                                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct09", scrip);

    size_t const codesize = 205;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   30,   38,    0,           36,   31,    6,    3,    // 7
       6,   72,    3,    4,            0,   51,    0,   47,    // 15
       3,    1,    1,    4,           36,   32,    6,    3,    // 23
       5,   29,    3,   36,           33,   51,    4,    7,    // 31
       3,   46,    3,    6,           32,    3,    4,    6,    // 39
       2,    4,   11,    2,            3,   29,    2,   36,    // 47
      37,    6,    3,    0,           34,    3,    6,    3,    // 55
       0,   34,    3,    6,            3,    3,   29,    3,    // 63
      51,   12,    7,    3,           30,    4,   11,    4,    // 71
       3,    3,    4,    3,           34,    3,   36,   36,    // 79
       6,    3,    3,   34,            3,   36,   35,    6,    // 87
       3,    7,   29,    3,           51,   16,   48,    2,    // 95
      52,   29,    2,   51,           16,    7,    3,   30,    // 103
       2,   32,    3,    4,           71,    3,   11,    2,    // 111
       3,    7,    3,   30,            4,   11,    4,    3,    // 119
       3,    4,    3,   34,            3,   36,   34,    6,    // 127
       2,    2,   29,    6,           45,    2,   39,    0,    // 135
       6,    3,    0,   33,            3,   30,    6,   29,    // 143
       3,   51,   12,    7,            3,   30,    4,   11,    // 151
       4,    3,    3,    4,            3,   46,    3,    7,    // 159
      32,    3,    0,    6,            2,    1,   11,    2,    // 167
       3,    3,    2,    3,           34,    3,   36,   37,    // 175
      51,    4,    7,    2,           45,    2,   39,    6,    // 183
       6,    3,    3,   33,            3,   35,    6,   30,    // 191
       2,   36,   38,   51,            8,   49,    2,    1,    // 199
       8,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      41,  129,  138,  165,        186,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 5;
    std::string imports[] = {
    "Character::get_ID^0",        "character",   "cAICar1",     "VehicleBase::SetCharacter^6",               // 3
    "Cars",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct10) {

    // When accessing a component of an import variable,
    // the import variable must be read first so that the fixup can be
    // applied. Only then may the offset be added to it.

    char const *inpl = "\
        import struct Struct                                 \n\
        {                                                    \n\
            int fluff;                                       \n\
            int k;                                           \n\
        } ss;                                                \n\
                                                             \n\
        int main()                                           \n\
        {                                                    \n\
            return ss.k;                                     \n\
        }                                                    \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct10", scrip);

    size_t const codesize = 15;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,    6,    2,    // 7
       0,    1,    2,    4,            7,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "ss",           "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct11) {

    // Structs may contain variables that are structs themselves.
    // Since 'Inner1' is managed, 'In1' will convert into an 'Inner1 *'.

    char const *inpl = "\
        managed struct Inner1                               \n\
        {                                                   \n\
            short Fluff;                                    \n\
            int Payload;                                    \n\
        };                                                  \n\
        struct Inner2                                       \n\
        {                                                   \n\
            short Fluff;                                    \n\
            int Payload;                                    \n\
        };                                                  \n\
        import int Foo;                                     \n\
        import struct Struct                                \n\
        {                                                   \n\
            Inner1 In1;                                     \n\
            Inner2 In2;                                     \n\
        } SS;                                               \n\
                                                            \n\
        int main()                                          \n\
        {                                                   \n\
            SS.In1 = new Inner1;                            \n\
            SS.In1.Payload = 77;                            \n\
            SS.In2.Payload = 777;                           \n\
            return SS.In1.Payload + SS.In2.Payload;         \n\
        }                                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct11", scrip);

    size_t const codesize = 75;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   19,   38,    0,           36,   20,   73,    3,    // 7
       8,    6,    2,    1,           47,    3,   36,   21,    // 15
       6,    2,    1,   48,            2,   52,    6,    3,    // 23
      77,    1,    2,    2,            8,    3,   36,   22,    // 31
       6,    3,  777,    6,            2,    1,    1,    2,    // 39
       6,    8,    3,   36,           23,    6,    2,    1,    // 47
      48,    2,   52,    1,            2,    2,    7,    3,    // 55
      29,    3,    6,    2,            1,    1,    2,    6,    // 63
       7,    3,   30,    4,           11,    4,    3,    3,    // 71
       4,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   18,   37,   47,         60,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "SS",           "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct12) {

    // Can have managed components in non-managed struct.
    // 'SS.IntArray[i]' requires run time calculations.
    // 'SS.IntArray[3]' can largely be evaluated at compile time.

    char const *inpl = "\
        struct NonManaged               \n\
        {                               \n\
            long Dummy;                 \n\
            int  IntArray[];            \n\
        } SS;                           \n\
                                        \n\
        int main()                      \n\
        {                               \n\
           int i;                       \n\
           SS.IntArray = new int[10];   \n\
           SS                           \n\
             .                          \n\
              IntArray                  \n\
                      [                 \n\
                       3                \n\
                        ] = 7;          \n\
           SS                           \n\
             .                          \n\
              IntArray                  \n\
                      [                 \n\
                       i                \n\
                        ]               \n\
                         = 7;           \n\
            i =                         \n\
                SS                      \n\
                  .                     \n\
                   IntArray             \n\
                           [            \n\
                            i           \n\
                             ]          \n\
                              ;         \n\
            return                      \n\
                SS                      \n\
                  .                     \n\
                   IntArray             \n\
                           [            \n\
                            3           \n\
                             ]          \n\
                              ;         \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct12", scrip);

    size_t const codesize = 133;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,    6,    3,    // 7
       0,   29,    3,   36,           10,    6,    3,   10,    // 15
      72,    3,    4,    0,            6,    2,    4,   47,    // 23
       3,   36,   14,    6,            2,    4,   48,    2,    // 31
      52,   36,   16,    6,            3,    7,    1,    2,    // 39
      12,    8,    3,   36,           20,    6,    2,    4,    // 47
      48,    2,   52,   36,           21,   29,    2,   51,    // 55
       8,    7,    3,   30,            2,   32,    3,    4,    // 63
      71,    3,   11,    2,            3,   36,   22,    6,    // 71
       3,    7,    8,    3,           36,   28,    6,    2,    // 79
       4,   48,    2,   52,           36,   29,   29,    2,    // 87
      51,    8,    7,    3,           30,    2,   32,    3,    // 95
       4,   71,    3,   11,            2,    3,   36,   30,    // 103
       7,    3,   36,   24,           51,    4,    8,    3,    // 111
      36,   36,    6,    2,            4,   48,    2,   52,    // 119
       1,    2,   12,   36,           38,    7,    3,   36,    // 127
      39,    2,    1,    4,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      22,   29,   47,   80,        116,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct13) {

    char const *inpl = "\
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

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct13", scrip);

    size_t const codesize = 43;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,   73,    3,    // 7
      40,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,    9,   51,    4,           48,    2,   52,    6,    // 23
       3,    1,    1,    2,           16,    8,    3,   36,    // 31
      10,   51,    4,   49,            2,    1,    4,    6,    // 39
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct14) {

    // Static arrays can be multidimensional

    char const *inpl = "\
    managed struct Struct                \n\
    {                                    \n\
        int Int1[5, 4];                  \n\
        int Int2[2][3];                  \n\
    };                                   \n\
                                         \n\
    int main()                           \n\
    {                                    \n\
        Struct S = new Struct;           \n\
        S.Int1[4, 2] = 1;                \n\
        S.Int2[1][2] = S.Int1[4, 2];     \n\
    }                                    \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct14", scrip);

    size_t const codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,   73,    3,    // 7
     104,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,   10,   51,    4,           48,    2,   52,    6,    // 23
       3,    1,    1,    2,           72,    8,    3,   36,    // 31
      11,   51,    4,   48,            2,   52,    1,    2,    // 39
      72,    7,    3,   51,            4,   48,    2,   52,    // 47
       1,    2,  100,    8,            3,   36,   12,   51,    // 55
       4,   49,    2,    1,            4,    6,    3,    0,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func01) {

    char const *inpl = "\
        int Foo()       \n\
        {               \n\
            return 15;  \n\
        }               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func01", scrip);

    size_t const codesize = 10;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
      15,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func02) {

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func02", scrip);

    size_t const codesize = 61;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   13,   38,    0,           36,   14,   51,    0,    // 7
      49,    1,    1,    4,           36,   15,   51,    0,    // 15
      49,    1,    1,    4,           36,   16,   51,    4,    // 23
      48,    3,   34,    3,           51,    8,   48,    3,    // 31
      34,    3,   39,    2,            6,    3,    0,   33,    // 39
       3,   35,    2,   29,            3,   36,   17,   51,    // 47
       4,    7,    3,   51,           12,   49,   51,    8,    // 55
      49,    2,    1,   12,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      38,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func03) {   

    char const *inpl = "\
    managed struct Struct1          \n\
    {                               \n\
        float Payload1;             \n\
    };                              \n\
    managed struct Struct2          \n\
    {                               \n\
        char Payload2;              \n\
    };                              \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct1 *SS1;               \n\
        Struct2 *SS2;               \n\
        int Ret = Func(SS1, SS2);   \n\
        return Ret;                 \n\
    }                               \n\
                                    \n\
    import int Func(Struct1 *S1, Struct2 *S2);  \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func03", scrip);

    size_t const codesize = 61;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   11,   38,    0,           36,   12,   51,    0,    // 7
      49,    1,    1,    4,           36,   13,   51,    0,    // 15
      49,    1,    1,    4,           36,   14,   51,    4,    // 23
      48,    3,   34,    3,           51,    8,   48,    3,    // 31
      34,    3,   39,    2,            6,    3,    0,   33,    // 39
       3,   35,    2,   29,            3,   36,   15,   51,    // 47
       4,    7,    3,   51,           12,   49,   51,    8,    // 55
      49,    2,    1,   12,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      38,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func04) {    

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func04", scrip);

    size_t const codesize = 88;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   13,   38,    0,           51,    8,    7,    3,    // 7
      50,    3,   51,   12,            7,    3,   50,    3,    // 15
      36,   14,    6,    3,            0,   51,    8,   49,    // 23
      51,   12,   49,    5,           36,   18,   38,   28,    // 31
      36,   19,   51,    0,           49,    1,    1,    4,    // 39
      36,   20,   51,    0,           49,    1,    1,    4,    // 47
      36,   21,   51,    4,           48,    3,   29,    3,    // 55
      51,   12,   48,    3,           29,    3,    6,    3,    // 63
       0,   23,    3,    2,            1,    8,   29,    3,    // 71
      36,   22,   51,    4,            7,    3,   51,   12,    // 79
      49,   51,    8,   49,            2,    1,   12,    5,    // 87
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      64,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func05) {
    
    char const *inpl = "\
    managed struct Struct1          \n\
    {                               \n\
        float Payload1;             \n\
    };                              \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        Struct1 *SS1 = Func(5);     \n\
        return -1;                  \n\
    }                               \n\
                                    \n\
    Struct1 *Func(int Int)          \n\
    {                               \n\
        return new Struct1;         \n\
    }                               \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func05", scrip);

    size_t const codesize = 48;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    3,    // 7
       5,   29,    3,    6,            3,   38,   23,    3,    // 15
       2,    1,    4,   51,            0,   47,    3,    1,    // 23
       1,    4,   36,    9,            6,    3,   -1,   51,    // 31
       4,   49,    2,    1,            4,    5,   36,   13,    // 39
      38,   38,   36,   14,           73,    3,    4,    5,    // 47
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      13,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func06) {
   
    char const *inpl = "\
        import int Func(int, int = 5); \n\
                                     \n\
        int Func(int P1, int P2)     \n\
        {                            \n\
            return P1 + P2;          \n\
        }                            \n\
                                     \n\
        void main()                  \n\
        {                            \n\
            int Int = Func(4);       \n\
        }                            \n\
    ";

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func06", scrip);

    size_t const codesize = 57;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,   51,    8,    // 7
       7,    3,   29,    3,           51,   16,    7,    3,    // 15
      30,    4,   11,    4,            3,    3,    4,    3,    // 23
       5,   36,    9,   38,           25,   36,   10,    6,    // 31
       3,    5,   29,    3,            6,    3,    4,   29,    // 39
       3,    6,    3,    0,           23,    3,    2,    1,    // 47
       8,   29,    3,   36,           11,    2,    1,    4,    // 55
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      43,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func07) {  

    char const *inpl = "\
        import int Func(int, int = 5); \n\
                                     \n\
        void main()                  \n\
        {                            \n\
            int Int1 = Func(4);      \n\
            int Int2 = Func(4, 1);   \n\
        }                            \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func07", scrip);

    size_t const codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,    6,    3,    // 7
       5,   34,    3,    6,            3,    4,   34,    3,    // 15
      39,    2,    6,    3,            0,   33,    3,   35,    // 23
       2,   29,    3,   36,            6,    6,    3,    1,    // 31
      34,    3,    6,    3,            4,   34,    3,   39,    // 39
       2,    6,    3,    0,           33,    3,   35,    2,    // 47
      29,    3,   36,    7,            2,    1,    8,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      20,   43,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func08) {   

    char const *inpl = "\
        void main()                  \n\
        {                            \n\
            int Int1 = Func(4);      \n\
            int Int2 = Func(4, 1);   \n\
        }                            \n\
                                     \n\
        import int Func(int, int = 5); \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func08", scrip);

    size_t const codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       5,   34,    3,    6,            3,    4,   34,    3,    // 15
      39,    2,    6,    3,            0,   33,    3,   35,    // 23
       2,   29,    3,   36,            4,    6,    3,    1,    // 31
      34,    3,    6,    3,            4,   34,    3,   39,    // 39
       2,    6,    3,    0,           33,    3,   35,    2,    // 47
      29,    3,   36,    5,            2,    1,    8,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      20,   43,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func09) {    

    char const *inpl = "\
        import int Func(int f, int = 5); \n\
        import int Func(int, int = 5); \n\
                                     \n\
        void main()                  \n\
        {                            \n\
            int Int1 = Func(4);      \n\
            int Int2 = Func(4, 1);   \n\
        }                            \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func09", scrip);

    size_t const codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    5,   38,    0,           36,    6,    6,    3,    // 7
       5,   34,    3,    6,            3,    4,   34,    3,    // 15
      39,    2,    6,    3,            0,   33,    3,   35,    // 23
       2,   29,    3,   36,            7,    6,    3,    1,    // 31
      34,    3,    6,    3,            4,   34,    3,   39,    // 39
       2,    6,    3,    0,           33,    3,   35,    2,    // 47
      29,    3,   36,    8,            2,    1,    8,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      20,   43,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func10) {
    
    char const *inpl = "\
        import int Func(int, int = 5); \n\
                                     \n\
        int Func(int P1, int P2)     \n\
        {                            \n\
            return P1 + P2;          \n\
        }                            \n\
                                     \n\
        void main()                  \n\
        {                            \n\
            int Int = Func(4,-99);   \n\
        }                            \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func10", scrip);

    size_t const codesize = 57;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,   51,    8,    // 7
       7,    3,   29,    3,           51,   16,    7,    3,    // 15
      30,    4,   11,    4,            3,    3,    4,    3,    // 23
       5,   36,    9,   38,           25,   36,   10,    6,    // 31
       3,  -99,   29,    3,            6,    3,    4,   29,    // 39
       3,    6,    3,    0,           23,    3,    2,    1,    // 47
       8,   29,    3,   36,           11,    2,    1,    4,    // 55
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      43,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func11) {   

    char const *inpl = "\
    struct Struct                   \n\
    {                               \n\
        float Float;                \n\
        int Func();                 \n\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func11", scrip);

    size_t const codesize = 57;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,    6,    3,    // 7
       5,    5,   36,   13,           38,   10,   36,   14,    // 15
       6,    3,    0,   29,            3,   36,   15,   51,    // 23
       4,   45,    2,    6,            3,    0,   23,    3,    // 31
      29,    3,    6,    3,            3,   30,    4,   40,    // 39
       4,    3,    3,    4,            3,   29,    3,   36,    // 47
      16,   51,    4,    7,            3,    2,    1,    8,    // 55
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      29,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func12) {

    // Function with float default, or default "0", for float parameter
    // The latter will make the compiler slightly unhappy

    char const *inpl = "\
    float Func1(float F = 7.2)          \n\
    {                                   \n\
        return F;                       \n\
    }                                   \n\
                                        \n\
    float Func2(float F = 0)            \n\
    {                                   \n\
        return F;                       \n\
    }                                   \n\
                                        \n\
    float Call()                        \n\
    {                                   \n\
        return Func1() + Func2();       \n\
    }                                   \n\
    ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0u).Message.find("'0'"));

    // WriteOutput("Func12", scrip);

    size_t const codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,   51,    8,    // 7
       7,    3,    5,   36,            7,   38,   11,   36,    // 15
       8,   51,    8,    7,            3,    5,   36,   12,    // 23
      38,   22,   36,   13,            6,    3, 1088841318,   29,    // 31
       3,    6,    3,    0,           23,    3,    2,    1,    // 39
       4,   29,    3,    6,            3,    0,   29,    3,    // 47
       6,    3,   11,   23,            3,    2,    1,    4,    // 55
      30,    4,   57,    4,            3,    3,    4,    3,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      35,   50,  -999
    };
    char fixuptypes[] = {
      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func13) {

    // Function with default null or 0 for managed parameter
    // The latter will make the compiler slightly unhappy

    char const *inpl = "\
    managed struct S                    \n\
    {                                   \n\
        float f;                        \n\
    };                                  \n\
                                        \n\
    S *Func1(S s = null)                \n\
    {                                   \n\
        return s;                       \n\
    }                                   \n\
                                        \n\
    S *Func2(S s = 0)                   \n\
    {                                   \n\
        return s;                       \n\
    }                                   \n\
                                        \n\
    int Call()                          \n\
    {                                   \n\
        return Func1() == Func2();      \n\
    }                                   \n\
    ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0u).Message.find("'0'"));

    // WriteOutput("Func13", scrip);

    size_t const codesize = 109;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           51,    8,    7,    3,    // 7
      50,    3,   36,    8,           51,    8,   48,    3,    // 15
      29,    3,   51,    4,           50,    3,   51,   12,    // 23
      49,   51,    4,   48,            3,   69,   30,    4,    // 31
       5,   36,   12,   38,           33,   51,    8,    7,    // 39
       3,   50,    3,   36,           13,   51,    8,   48,    // 47
       3,   29,    3,   51,            4,   50,    3,   51,    // 55
      12,   49,   51,    4,           48,    3,   69,   30,    // 63
       4,    5,   36,   17,           38,   66,   36,   18,    // 71
       6,    3,    0,   29,            3,    6,    3,    0,    // 79
      23,    3,    2,    1,            4,   29,    3,    6,    // 87
       3,    0,   29,    3,            6,    3,   33,   23,    // 95
       3,    2,    1,    4,           30,    4,   15,    4,    // 103
       3,    3,    4,    3,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      79,   94,  -999
    };
    char fixuptypes[] = {
      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func14) {

    // Strange misalignment due to bad function protocol

    char const *inpl = "\
        struct Struct               \n\
        {                           \n\
            int A[];                \n\
            int B[];                \n\
                                    \n\
            import void Test(int Arg);  \n\
        };                          \n\
                                    \n\
        void Struct::Test(int Arg)  \n\
        {                           \n\
            this.A = new int[1];    \n\
            this.B = new int[1];    \n\
            this.B[0] = 123;        \n\
            Display(this.A[0], this.B[0]); \n\
        }                           \n\
                                    \n\
        void Display(int X, int Y)  \n\
        {                           \n\
        }                           \n\
                                    \n\
        int main()                  \n\
        {                           \n\
            Struct TS;              \n\
            TS.Test(7);             \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func14", scrip);

    size_t const codesize = 159;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   10,   38,    0,           36,   11,    6,    3,    // 7
       1,   72,    3,    4,            0,    3,    6,    2,    // 15
      52,   47,    3,   36,           12,    6,    3,    1,    // 23
      72,    3,    4,    0,            3,    6,    2,   52,    // 31
       1,    2,    4,   47,            3,   36,   13,    3,    // 39
       6,    2,   52,    1,            2,    4,   48,    2,    // 47
      52,    6,    3,  123,            8,    3,   36,   14,    // 55
      29,    6,    3,    6,            2,   52,    1,    2,    // 63
       4,   48,    2,   52,            7,    3,   29,    3,    // 71
       3,    6,    2,   52,           48,    2,   52,    7,    // 79
       3,   29,    3,    6,            3,   96,   23,    3,    // 87
       2,    1,    8,   30,            6,   36,   15,    5,    // 95
      36,   18,   38,   96,           36,   19,    5,   36,    // 103
      22,   38,  103,   36,           23,   51,    0,   63,    // 111
       8,    1,    1,    8,           36,   24,   51,    8,    // 119
      29,    2,    6,    3,            7,   29,    3,   51,    // 127
       8,    7,    2,   45,            2,    6,    3,    0,    // 135
      23,    3,    2,    1,            4,   30,    2,   36,    // 143
      25,   51,    8,   49,            1,    2,    4,   49,    // 151
       2,    1,    8,    6,            3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      85,  135,  -999
    };
    char fixuptypes[] = {
      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func15) {

    // Simple void function

    char const *inpl = "\
        void Foo()          \n\
        {                   \n\
            return;         \n\
        }";

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func15", scrip);

    size_t const codesize = 7;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func16) {

    char const *inpl = "\
        int a = 15;     \n\
        int Foo1( )     \n\
        {               \n\
            return a;   \n\
        }               \n\
                        \n\
        int Foo2(int a) \n\
        {               \n\
            return a;   \n\
        }               \n\
                        \n\
        int Foo3()      \n\
        {               \n\
            int a = 15; \n\
            return a;   \n\
        }               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func16", scrip);

    size_t const codesize = 44;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    2,    // 7
       0,    7,    3,    5,           36,    8,   38,   12,    // 15
      36,    9,   51,    8,            7,    3,    5,   36,    // 23
      13,   38,   23,   36,           14,    6,    3,   15,    // 31
      29,    3,   36,   15,           51,    4,    7,    3,    // 39
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func17) {

    // NON-managed dynpointers must be read/rewritten at function start, too.

    char const *inpl = "\
        int Random(int X)                       \n\
        {                                       \n\
            Shuffle(new int[15], 10);           \n\
        }                                       \n\
                                                \n\
        void Shuffle(int Ints[], int Length)    \n\
        {                                       \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func17", scrip);

    size_t const codesize = 50;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
      10,   29,    3,    6,            3,   15,   72,    3,    // 15
       4,    0,   29,    3,            6,    3,   34,   23,    // 23
       3,    2,    1,    8,           36,    4,    6,    3,    // 31
       0,    5,   36,    7,           38,   34,   51,    8,    // 39
       7,    3,   50,    3,           36,    8,   51,    8,    // 47
      49,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      22,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func18) {

    // Multiple forward calls should _all_ resolve to the correct function address

    char const *inpl = "\
        int main()                      \n\
        {                               \n\
            MKB(0, 7);                  \n\
            MKB(1, 9);                  \n\
            MKB(2, 7);                  \n\
            MKB(3, 11);                 \n\
            MKB(4, 9);                  \n\
            MKB(5, 8);                  \n\
        }                               \n\
                                        \n\
        int MKB(int cp, int lastbook)   \n\
        {                               \n\
            return cp + lastbook;       \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func18", scrip);

    size_t const codesize = 155;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       7,   29,    3,    6,            3,    0,   29,    3,    // 15
       6,    3,  130,   23,            3,    2,    1,    8,    // 23
      36,    4,    6,    3,            9,   29,    3,    6,    // 31
       3,    1,   29,    3,            6,    3,  130,   23,    // 39
       3,    2,    1,    8,           36,    5,    6,    3,    // 47
       7,   29,    3,    6,            3,    2,   29,    3,    // 55
       6,    3,  130,   23,            3,    2,    1,    8,    // 63
      36,    6,    6,    3,           11,   29,    3,    6,    // 71
       3,    3,   29,    3,            6,    3,  130,   23,    // 79
       3,    2,    1,    8,           36,    7,    6,    3,    // 87
       9,   29,    3,    6,            3,    4,   29,    3,    // 95
       6,    3,  130,   23,            3,    2,    1,    8,    // 103
      36,    8,    6,    3,            8,   29,    3,    6,    // 111
       3,    5,   29,    3,            6,    3,  130,   23,    // 119
       3,    2,    1,    8,           36,    9,    6,    3,    // 127
       0,    5,   36,   12,           38,  130,   36,   13,    // 135
      51,    8,    7,    3,           29,    3,   51,   16,    // 143
       7,    3,   30,    4,           11,    4,    3,    3,    // 151
       4,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,   38,   58,   78,         98,  118,  -999
    };
    char fixuptypes[] = {
      2,   2,   2,   2,      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Export) {
    
    char const *inpl = "\
    struct Struct                   \n\
    {                               \n\
        float Float;                \n\
        int Int;                    \n\
    };                              \n\
    Struct StructyStructy;          \n\
    export StructyStructy;          \n\
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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Export", scrip);

    size_t const codesize = 40;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   14,   38,    0,           36,   15,   51,    0,    // 7
      63,    8,    1,    1,            8,   36,   16,    6,    // 15
       3,    3,   51,    4,            8,    3,   36,   17,    // 23
       6,    3, 1056964608,   51,            8,    8,    3,   36,    // 31
      18,    6,    3,   -2,            2,    1,    8,    5,    // 39
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 4;
    EXPECT_EQ(numexports, scrip.numexports);

    std::string exports[] = {
    "StructyStructy", "Floaty",   "Inty",     "main$0",   // 3
     "[[SENTINEL]]"
    };
    int32_t export_addr[] = {
    0x2000000, 0x200000c,    0x2000008, 0x1000000, // 3
     0
    };
    CompareExports(&scrip, numexports, exports, export_addr);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, ArrayOfPointers1) {
   
    char const *inpl = "\
    managed struct Struct                \n\
    {                                    \n\
        float Float;                     \n\
        protected int Int;               \n\
    };                                   \n\
    Struct *arr[50];                     \n\
                                         \n\
    int main()                           \n\
    {                                    \n\
        for (int i = 0; i < 9; ++i)      \n\
            arr[i] = new Struct;         \n\
                                         \n\
        int test = (arr[10] == null);    \n\
    }                                    \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("ArrayOfPointers1", scrip);
    size_t const codesize = 108;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    9,   38,    0,           36,   10,    6,    3,    // 7
       0,   29,    3,   31,           11,   36,   10,   51,    // 15
       4,    7,    3,    1,            3,    1,    8,    3,    // 23
      36,   10,   51,    4,            7,    3,   29,    3,    // 31
       6,    3,    9,   30,            4,   18,    4,    3,    // 39
       3,    4,    3,   28,           29,   36,   11,   73,    // 47
       3,    8,   29,    3,           51,    8,    7,    3,    // 55
      46,    3,   50,   32,            3,    4,    6,    2,    // 63
       0,   11,    2,    3,           30,    3,   47,    3,    // 71
      31,  -61,    2,    1,            4,   36,   13,    6,    // 79
       2,   40,   48,    3,           29,    3,    6,    3,    // 87
       0,   30,    4,   15,            4,    3,    3,    4,    // 95
       3,   29,    3,   36,           14,    2,    1,    4,    // 103
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      64,   81,  -999
    };
    char fixuptypes[] = {
      1,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, ArrayOfPointers2) {    

    char const *inpl = "\
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("ArrayOfPointers2", scrip);
    size_t const codesize = 131;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,   51,    0,    // 7
      63,  200,    1,    1,          200,   36,   10,    6,    // 15
       3,    0,   29,    3,           31,   11,   36,   10,    // 23
      51,    4,    7,    3,            1,    3,    1,    8,    // 31
       3,   36,   10,   51,            4,    7,    3,   29,    // 39
       3,    6,    3,   20,           30,    4,   18,    4,    // 47
       3,    3,    4,    3,           28,   28,   36,   11,    // 55
      73,    3,    8,   29,            3,   51,    8,    7,    // 63
       3,   46,    3,   50,           32,    3,    4,   51,    // 71
     208,   11,    2,    3,           30,    3,   47,    3,    // 79
      31,  -60,   36,   12,            2,    1,    4,   36,    // 87
      13,   51,  180,   48,            2,   52,    6,    3,    // 95
    1074580685,    8,    3,   36,           14,    6,    3,    0,    // 103
      51,  184,   47,    3,           36,   15,   51,  200,    // 111
       6,    3,   50,   49,            1,    2,    4,    2,    // 119
       3,    1,   70,   -9,            2,    1,  200,    6,    // 127
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Writeprotected) {
    
    // Directly taken from the doc on writeprotected, simplified.

    char const *inpl = "\
        struct Weapon {                         \n\
            short Beauty;                       \n\
            writeprotected int Damage;          \n\
            import int SetDamage(int damage);   \n\
        } wp;                                   \n\
                                                \n\
        int  Weapon::SetDamage(int damage)      \n\
        {                                       \n\
            this.Damage = damage;               \n\
            return 0;                           \n\
        }                                       \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            return wp.Damage;                   \n\
        }                                       \n\
        ";
 
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Writeprotected", scrip);

    size_t const codesize = 37;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,   51,    8,    // 7
       7,    3,    3,    6,            2,   52,    1,    2,    // 15
       2,    8,    3,   36,           10,    6,    3,    0,    // 23
       5,   36,   14,   38,           25,   36,   15,    6,    // 31
       2,    2,    7,    3,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      33,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Protected1) {

    // Directly taken from the doc on protected, simplified.

    char const *inpl = "\
        struct Weapon {                        \n\
            protected int Damage;              \n\
            import int SetDamage(int damage);  \n\
        };                                     \n\
                                               \n\
        Weapon wp;                             \n\
                                               \n\
        int  Weapon::SetDamage(int damage)     \n\
        {                                      \n\
            this.Damage = damage;              \n\
            return 0;                          \n\
        }                                      \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Protected1", scrip);

    size_t const codesize = 22;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    9,   38,    0,           36,   10,   51,    8,    // 7
       7,    3,    3,    6,            2,   52,    8,    3,    // 15
      36,   11,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Protected2) {

    // In a struct func, a variable that can't be found otherwise
    // should be taken to be out of the current struct.

    char const *inpl = "\
        struct Weapon {                        \n\
            protected int Damage;              \n\
            import int SetDamage(int damage);  \n\
        };                                     \n\
                                               \n\
        Weapon wp;                             \n\
                                               \n\
        int Weapon::SetDamage(int damage)      \n\
        {                                      \n\
            Damage = damage;                   \n\
            return 0;                          \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Protected2", scrip);

    size_t const codesize = 22;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    9,   38,    0,           36,   10,   51,    8,    // 7
       7,    3,    3,    6,            2,   52,    8,    3,    // 15
      36,   11,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Static1) {

    char const *inpl = "\
        struct Weapon {                             \n\
            import static int CalcDamage(           \n\
            int Lifepoints, int Hitpoints = 5);     \n\
        };                                          \n\
                                                    \n\
        static int Weapon::CalcDamage(int Lifepoints, int Hitpoints)  \n\
        {                                           \n\
            return Lifepoints - Hitpoints;          \n\
        }                                           \n\
                                                    \n\
        int main()                                  \n\
        {                                           \n\
            int hp = Weapon.CalcDamage(9) + Weapon.CalcDamage(9, 40);  \n\
            return hp + Weapon.CalcDamage(100);     \n\
        }                                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Static1", scrip);

    size_t const codesize = 117;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,   51,    8,    // 7
       7,    3,   29,    3,           51,   16,    7,    3,    // 15
      30,    4,   12,    4,            3,    3,    4,    3,    // 23
       5,   36,   12,   38,           25,   36,   13,    6,    // 31
       3,    5,   29,    3,            6,    3,    9,   29,    // 39
       3,    6,    3,    0,           23,    3,    2,    1,    // 47
       8,   29,    3,    6,            3,   40,   29,    3,    // 55
       6,    3,    9,   29,            3,    6,    3,    0,    // 63
      23,    3,    2,    1,            8,   30,    4,   11,    // 71
       4,    3,    3,    4,            3,   29,    3,   36,    // 79
      14,   51,    4,    7,            3,   29,    3,    6,    // 87
       3,    5,   29,    3,            6,    3,  100,   29,    // 95
       3,    6,    3,    0,           23,    3,    2,    1,    // 103
       8,   30,    4,   11,            4,    3,    3,    4,    // 111
       3,    2,    1,    4,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      43,   63,   99,  -999
    };
    char fixuptypes[] = {
      2,   2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Static2) {

    char const *inpl = "\
        struct Weapon {                        \n\
        };                                     \n\
                                               \n\
        int CalcDamage(static Weapon, int Lifepoints, int Hitpoints)  \n\
        {                                      \n\
            return Lifepoints - Hitpoints;     \n\
        }                                      \n\
                                               \n\
        int main()                             \n\
        {                                      \n\
            return Weapon.CalcDamage(9, 40);   \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Static2", scrip);

    size_t const codesize = 50;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    5,   38,    0,           36,    6,   51,    8,    // 7
       7,    3,   29,    3,           51,   16,    7,    3,    // 15
      30,    4,   12,    4,            3,    3,    4,    3,    // 23
       5,   36,   10,   38,           25,   36,   11,    6,    // 31
       3,   40,   29,    3,            6,    3,    9,   29,    // 39
       3,    6,    3,    0,           23,    3,    2,    1,    // 47
       8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      43,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Import) {    

    char const *inpl = "\
        import int Weapon;                      \n\
                                                \n\
        int Func(int damage)                    \n\
        {                                       \n\
            int Int = 0;                        \n\
            Weapon = 77;                        \n\
            if (Weapon < 0)                     \n\
                Weapon =                        \n\
                    damage -                    \n\
                    (Int - Weapon) / Int;       \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Import", scrip);

    size_t const codesize = 112;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,    6,    3,    // 7
       0,   29,    3,   36,            6,    6,    3,   77,    // 15
       6,    2,    0,    8,            3,   36,    7,    6,    // 23
       2,    0,    7,    3,           29,    3,    6,    3,    // 31
       0,   30,    4,   18,            4,    3,    3,    4,    // 39
       3,   28,   60,   36,            9,   51,   12,    7,    // 47
       3,   29,    3,   36,           10,   51,    8,    7,    // 55
       3,   29,    3,    6,            2,    0,    7,    3,    // 63
      30,    4,   12,    4,            3,    3,    4,    3,    // 71
      29,    3,   51,   12,            7,    3,   30,    4,    // 79
      10,    4,    3,    3,            4,    3,   36,    9,    // 87
      30,    4,   12,    4,            3,    3,    4,    3,    // 95
      36,    8,    6,    2,            0,    8,    3,   36,    // 103
      11,    2,    1,    4,            6,    3,    0,    5,    // 111
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,   25,   61,  100,        -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Weapon",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);}
