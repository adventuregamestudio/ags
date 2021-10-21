#include <string>

#include "gtest/gtest.h"
#include "cc_bytecode_test_lib.h"
#include "cc_parser_test_lib.h"

#include "script/cc_options.h"

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
    

    char *inpl = "\
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
    AGS::ccCompiledScript scrip{ false };

    Bytecode0()
    {
        // Initializations, will be done at the start of each test
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        ccSetOption(SCOPT_LINENUMBERS, false);
        clear_error();
    }
};

TEST_F(Bytecode0, UnaryMinus1) {

    // Accept a unary minus in front of parens

    char *inpl = "\
        void Foo()              \n\
        {                       \n\
            int bar = 5;        \n\
            int baz = -(-bar);  \n\
        }";

    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("UnaryMinus1", scrip);
    const size_t codesize = 35;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,   51,    // 7
       4,    7,    3,    6,            4,    0,   12,    4,    // 15
       3,    3,    4,    3,            6,    4,    0,   12,    // 23
       4,    3,    3,    4,            3,   29,    3,    2,    // 31
       1,    8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, UnaryMinus2) {    

    // Unary minus binds more than multiply

    char *inpl = "\
        int main()                      \n\
        {                               \n\
            int five = 5;               \n\
            int seven = 7;              \n\
            return -five * -seven;      \n\
        }";

    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("UnaryMinus2", scrip);
    size_t const codesize = 52;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,    6,    // 7
       3,    7,   29,    3,           51,    8,    7,    3,    // 15
       6,    4,    0,   12,            4,    3,    3,    4,    // 23
       3,   29,    3,   51,            8,    7,    3,    6,    // 31
       4,    0,   12,    4,            3,    3,    4,    3,    // 39
      30,    4,    9,    4,            3,    3,    4,    3,    // 47
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
    char *inpl = "\
        int main()                  \n\
        {                           \n\
            int five = 5;           \n\
            return !!(!five);       \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Notnot", scrip);
    size_t const codesize = 21;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,   51,    // 7
       4,    7,    3,   42,            3,   42,    3,   42,    // 15
       3,    2,    1,    4,            5,  -999
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

    char inpl[] = "\
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
            Test1 = 77f;                    \n\
        }                                   \n\
        ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("reach this point"));

    // WriteOutput("Float01", scrip);
    size_t const codesize = 155;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         -1059061760,   29,    3,    6,    // 7
       3, 1143930880,   29,    3,            6,    3, -1114678231,   29,    // 15
       3,    6,    3, -1057593754,           29,    3,    6,    3,    // 23
    1088421888,   29,    3,    6,            3, 893118370,   29,    3,    // 31
       6,    3, 893118370,   29,            3,    6,    3, 1061494456,    // 39
      29,    3,   51,   32,            7,    3,   29,    3,    // 47
      51,   32,    7,    3,           30,    4,   57,    4,    // 55
       3,    3,    4,    3,           29,    3,   51,   28,    // 63
       7,    3,   30,    4,           57,    4,    3,    3,    // 71
       4,    3,   29,    3,           51,   24,    7,    3,    // 79
      30,    4,   57,    4,            3,    3,    4,    3,    // 87
      29,    3,   51,   20,            7,    3,   30,    4,    // 95
      57,    4,    3,    3,            4,    3,   29,    3,    // 103
      51,   16,    7,    3,           30,    4,   57,    4,    // 111
       3,    3,    4,    3,           29,    3,   51,   12,    // 119
       7,    3,   30,    4,           57,    4,    3,    3,    // 127
       4,    3,   29,    3,           51,    8,    7,    3,    // 135
      30,    4,   57,    4,            3,    3,    4,    3,    // 143
       2,    1,   32,    5,            6,    3, 1117388800,   51,    // 151
      32,    8,    3,  -999
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

    char inpl[] = "\
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
    size_t const codesize = 55;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         -1059061760,   29,    3,   51,    // 7
      12,    7,    3,   30,            4,   57,    4,    3,    // 15
       3,    4,    3,   29,            3,   51,   16,    7,    // 23
       3,   30,    4,   58,            4,    3,    3,    4,    // 31
       3,    5,   38,   34,            6,    3, -1070805811,   29,    // 39
       3,    6,    3, 1088841318,           29,    3,    6,    3,    // 47
       0,   23,    3,    2,            1,    8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      48,  -999
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

    char *inpl = "\
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
    size_t const codesize = 30;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         1078523331,   29,    3,    6,    // 7
       2,    0,    7,    3,           29,    3,   51,    8,    // 15
       7,    3,   30,    4,           57,    4,    3,    3,    // 23
       4,    3,    2,    1,            4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,  -999
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

    char *inpl = "\
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
    size_t const codesize = 156;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         1102158234,   29,    3,    6,    // 7
       3,    0,   29,    3,            6,    3,    0,   51,    // 15
       0,   27,    3,    1,            1,    2,    6,    2,    // 23
       0,    7,    3,   29,            3,    6,    3, 1110546842,    // 31
      30,    4,   62,    4,            3,    3,    4,    3,    // 39
      29,    3,    6,    3,            1,   51,    0,   26,    // 47
       3,    1,    1,    1,            6,    2,    0,    7,    // 55
       3,   29,    3,   51,           19,    7,    3,   30,    // 63
       4,   15,    4,    3,            3,    4,    3,   70,    // 71
      29,   29,    3,    6,            2,    0,    7,    3,    // 79
      29,    3,   51,   23,            7,    3,   30,    4,    // 87
      16,    4,    3,    3,            4,    3,   30,    4,    // 95
      22,    4,    3,    3,            4,    3,   29,    3,    // 103
       6,    2,    0,    7,            3,   29,    3,   51,    // 111
      23,    7,    3,   29,            3,    6,    2,    0,    // 119
       7,    3,   29,    3,           51,   31,    7,    3,    // 127
      30,    4,   56,    4,            3,    3,    4,    3,    // 135
      30,    4,   55,    4,            3,    3,    4,    3,    // 143
      30,    4,   58,    4,            3,    3,    4,    3,    // 151
       2,    1,   19,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      24,   54,   77,  106,        119,  -999
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

    char *inpl = "\
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
    size_t const codesize = 100;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            4,   29,    3,    6,    // 7
       3,   15,   29,    3,           51,    8,    7,    3,    // 15
      29,    3,    6,    3,            2,   30,    4,    9,    // 23
       4,    3,    3,    4,            3,   30,    4,   12,    // 31
       4,    3,    3,    4,            3,   29,    3,   51,    // 39
       4,    7,    3,   29,            3,    6,    3,    5,    // 47
      30,    4,   18,    4,            3,    3,    4,    3,    // 55
      28,   18,    6,    3,            2,   29,    3,   51,    // 63
       8,    7,    3,   30,            4,   44,    3,    4,    // 71
       8,    3,   31,   16,            6,    3,    3,   29,    // 79
       3,   51,    8,    7,            3,   30,    4,   43,    // 87
       3,    4,    8,    3,           51,    4,    7,    3,    // 95
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

TEST_F(Bytecode0, FlowIfThenElse2) {

    char *inpl = "\
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
    size_t const codesize = 100;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            2,   29,    3,    6,    // 7
       3,   15,   29,    3,            6,    3,    4,   29,    // 15
       3,   51,   12,    7,            3,   30,    4,   40,    // 23
       4,    3,    3,    4,            3,   30,    4,   12,    // 31
       4,    3,    3,    4,            3,   29,    3,   51,    // 39
       4,    7,    3,   29,            3,    6,    3,    5,    // 47
      30,    4,   19,    4,            3,    3,    4,    3,    // 55
      28,   18,    6,    3,            2,   29,    3,   51,    // 63
       8,    7,    3,   30,            4,   12,    3,    4,    // 71
       8,    3,   31,   16,            6,    3,    3,   29,    // 79
       3,   51,    8,    7,            3,   30,    4,   11,    // 87
       3,    4,    8,    3,           51,    4,    7,    3,    // 95
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

TEST_F(Bytecode0, While) {

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

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("While", scrip);
    size_t const codesize = 100;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       2,    0,   24,    3,           29,    3,    6,    3,    // 15
       0,   30,    4,   19,            4,    3,    3,    4,    // 23
       3,   28,   65,    6,            3,  500,   29,    3,    // 31
       6,    2,    0,   24,            3,   30,    4,   13,    // 39
       4,    3,    3,    4,            3,   29,    3,   51,    // 47
       8,    7,    3,   30,            4,   11,    3,    4,    // 55
       8,    3,    6,    2,            0,   24,    3,    2,    // 63
       3,    1,   26,    3,            6,    2,    0,   24,    // 71
       3,   29,    3,    6,            3,    1,   30,    4,    // 79
      15,    4,    3,    3,            4,    3,   28,    2,    // 87
      31,  -83,   31,  -85,           51,    4,    7,    3,    // 95
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,   34,   60,   70,        -999
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

TEST_F(Bytecode0, FlowDoNCall) {

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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowDoNCall", scrip);
    size_t const codesize = 107;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,  500,   29,    3,            6,    2,    0,   24,    // 15
       3,   30,    4,   14,            4,    3,    3,    4,    // 23
       3,   29,    3,   51,            8,    7,    3,   30,    // 31
       4,   12,    3,    4,            8,    3,    6,    2,    // 39
       0,   24,    3,    2,            3,    1,   26,    3,    // 47
       6,    2,    0,   24,            3,   29,    3,    6,    // 55
       3,    0,   30,    4,           17,    4,    3,    3,    // 63
       4,    3,   70,  -61,           51,    4,    7,    3,    // 71
       2,    1,    4,    5,           38,   76,   51,    8,    // 79
       7,    3,   29,    3,           51,   12,    7,    3,    // 87
      30,    4,   41,    4,            3,    3,    4,    3,    // 95
      29,    3,    6,    3,            0,   23,    3,    2,    // 103
       1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      14,   40,   50,  100,        -999
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

    char *inpl = "\
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
    const size_t codesize = 70;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   68,    6,            3,    0,   29,    3,    // 7
      51,    4,    7,    3,           29,    3,    6,    3,    // 15
     100,   30,    4,   18,            4,    3,    3,    4,    // 23
       3,   28,   18,    6,            3,   10,   29,    3,    // 31
      51,    8,    7,    3,           30,    4,   11,    3,    // 39
       4,    8,    3,   31,            2,   31,   19,   51,    // 47
       4,    7,    3,   29,            3,    6,    3,   -1,    // 55
      30,    4,   19,    4,            3,    3,    4,    3,    // 63
      70,  -58,    2,    1,            4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}


TEST_F(Bytecode0, FlowFor1) {  

    char *inpl = "\
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
    size_t const codesize = 114;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,    6,    2,    0,    // 7
       8,    3,    6,    2,            0,    7,    3,   29,    // 15
       3,    6,    3,   10,           30,    4,   18,    4,    // 23
       3,    3,    4,    3,           28,   80,    6,    2,    // 31
       0,    7,    3,   29,            3,    6,    3,    4,    // 39
      30,    4,   12,    4,            3,    3,    4,    3,    // 47
      29,    3,    6,    3,            7,   30,    4,   12,    // 55
       4,    3,    3,    4,            3,   29,    3,    6,    // 63
       2,    0,    7,    3,           29,    3,    6,    3,    // 71
       6,   30,    4,   15,            4,    3,    3,    4,    // 79
       3,   28,    5,    2,            1,    4,   31,   22,    // 87
       2,    1,    4,    6,            3,    3,   29,    3,    // 95
       6,    2,    0,    7,            3,   30,    4,   11,    // 103
       3,    4,    8,    3,           31, -100,    6,    3,    // 111
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   12,   32,   65,         98,  -999
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor2", scrip);
    size_t const codesize = 300;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,    0,   29,    3,            6,    3,    1,   28,    // 15
      35,   51,    8,    7,            3,   29,    3,   51,    // 23
       8,    7,    3,   30,            4,   11,    3,    4,    // 31
       8,    3,    6,    3,            1,   29,    3,   51,    // 39
      12,    7,    3,   30,            4,   11,    3,    4,    // 47
       8,    3,   31,  -40,            6,    3,    1,   28,    // 55
      19,   51,    8,    7,            3,   29,    3,   51,    // 63
       8,    7,    3,   30,            4,   12,    3,    4,    // 71
       8,    3,   31,  -24,           51,    8,    7,    3,    // 79
      29,    3,    6,    3,            2,   30,    4,   18,    // 87
       4,    3,    3,    4,            3,   28,   35,   51,    // 95
       8,    7,    3,   29,            3,   51,    8,    7,    // 103
       3,   30,    4,    9,            3,    4,    8,    3,    // 111
       6,    3,    3,   29,            3,   51,   12,    7,    // 119
       3,   30,    4,   11,            3,    4,    8,    3,    // 127
      31,  -54,   51,    8,            7,    3,   29,    3,    // 135
       6,    3,    4,   30,            4,   18,    4,    3,    // 143
       3,    4,    3,   28,           19,   51,    8,    7,    // 151
       3,   29,    3,   51,            8,    7,    3,   30,    // 159
       4,   10,    3,    4,            8,    3,   31,  -38,    // 167
       6,    3,    5,   51,            8,    8,    3,    6,    // 175
       3,    1,   28,   35,           51,    8,    7,    3,    // 183
      29,    3,   51,    8,            7,    3,   30,    4,    // 191
      10,    3,    4,    8,            3,    6,    3,    6,    // 199
      29,    3,   51,   12,            7,    3,   30,    4,    // 207
      11,    3,    4,    8,            3,   31,  -40,    6,    // 215
       3,    7,   29,    3,            6,    3,    1,   28,    // 223
      19,   51,    4,    7,            3,   29,    3,   51,    // 231
      12,    7,    3,   30,            4,   13,    3,    4,    // 239
       8,    3,   31,  -24,            2,    1,    4,    6,    // 247
       3,    8,   29,    3,           51,    4,    7,    3,    // 255
      29,    3,    6,    3,            9,   30,    4,   18,    // 263
       4,    3,    3,    4,            3,   28,   19,   51,    // 271
       4,    7,    3,   29,            3,   51,   12,    7,    // 279
       3,   30,    4,   14,            3,    4,    8,    3,    // 287
      31,  -38,    2,    1,            4,    6,    3,    0,    // 295
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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowFor3", scrip);
    size_t const codesize = 51;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
       6,    3,    1,   28,           28,   51,    4,   48,    // 15
       3,   29,    3,    6,            2,    0,   48,    3,    // 23
      30,    4,   15,    4,            3,    3,    4,    3,    // 31
      51,    4,   49,    2,            1,    4,    5,   31,    // 39
     -33,   51,    4,   49,            2,    1,    4,    6,    // 47
       3,   -7,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      21,  -999
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

    char *inpl = "\
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
    const size_t codesize = 71;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,   51,    // 7
       4,    7,    3,   29,            3,    6,    3,   10,    // 15
      30,    4,   18,    4,            3,    3,    4,    3,    // 23
      28,   41,   51,    4,            7,    3,   29,    3,    // 31
       6,    3,    5,   30,            4,   15,    4,    3,    // 39
       3,    4,    3,   28,           11,   51,    4,    7,    // 47
       3,    1,    3,    1,            8,    3,   31,  -49,    // 55
      51,    4,    7,    3,            1,    3,    1,    8,    // 63
       3,   31,  -60,    2,            1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, FlowFor5) {

    char *inpl = "\
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
    size_t const codesize = 125;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            1,    5,   38,    6,    // 7
       6,    3,   10,    5,           38,   12,   51,    8,    // 15
       7,    3,   29,    3,            6,    3,    1,   30,    // 23
       4,   11,    4,    3,            3,    4,    3,    5,    // 31
      38,   32,    6,    3,            0,   23,    3,   29,    // 39
       3,   51,    4,    7,            3,   29,    3,    6,    // 47
       3,    6,   23,    3,           30,    4,   18,    4,    // 55
       3,    3,    4,    3,           28,   59,   51,    4,    // 63
       7,    3,   29,    3,            6,    3,    0,   30,    // 71
       4,   19,    4,    3,            3,    4,    3,   28,    // 79
      20,   51,    4,    7,            3,   29,    3,    6,    // 87
       3,   12,   23,    3,            2,    1,    4,   51,    // 95
       4,    8,    3,   31,          -60,   51,    4,    7,    // 103
       3,   29,    3,    6,            3,   12,   23,    3,    // 111
       2,    1,    4,   51,            4,    8,    3,   31,    // 119
     -80,    2,    1,    4,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      36,   49,   89,  109,        -999
    };
    char fixuptypes[] = {
      2,   2,   2,   2,     '\0'
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

    char *inpl = "\
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
    size_t const codesize = 125;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           93,   23,    3,   29,    // 7
       3,   51,    4,    7,            3,   29,    3,    6,    // 15
       3,   99,   23,    3,           30,    4,   18,    4,    // 23
       3,    3,    4,    3,           28,   59,   51,    4,    // 31
       7,    3,   29,    3,            6,    3,    0,   30,    // 39
       4,   19,    4,    3,            3,    4,    3,   28,    // 47
      20,   51,    4,    7,            3,   29,    3,    6,    // 55
       3,  105,   23,    3,            2,    1,    4,   51,    // 63
       4,    8,    3,   31,          -60,   51,    4,    7,    // 71
       3,   29,    3,    6,            3,  105,   23,    3,    // 79
       2,    1,    4,   51,            4,    8,    3,   31,    // 87
     -80,    2,    1,    4,            5,   38,   93,    6,    // 95
       3,    1,    5,   38,           99,    6,    3,   10,    // 103
       5,   38,  105,   51,            8,    7,    3,   29,    // 111
       3,    6,    3,    1,           30,    4,   11,    4,    // 119
       3,    3,    4,    3,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   17,   57,   77,        -999
    };
    char fixuptypes[] = {
      2,   2,   2,   2,     '\0'
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

    char *inpl = "\
        int i;                          \n\
        void main()                     \n\
        {                               \n\
            for(Start(); Check(); Cont())   \n\
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
    size_t const codesize = 113;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           65,   23,    3,    6,    // 7
       3,   79,   23,    3,           28,   50,    6,    2,    // 15
       0,    7,    3,   29,            3,    6,    3,    5,    // 23
      30,    4,   19,    4,            3,    3,    4,    3,    // 31
      28,   23,    6,    3,          100,   29,    3,    6,    // 39
       2,    0,    7,    3,           30,    4,   12,    4,    // 47
       3,    3,    4,    3,            6,    2,    0,    8,    // 55
       3,    6,    3,  100,           23,    3,   31,  -57,    // 63
       5,   38,   65,    6,            3,    1,    6,    2,    // 71
       0,    8,    3,    6,            3,  -77,    5,   38,    // 79
      79,    6,    2,    0,            7,    3,   29,    3,    // 87
       6,    3,   10,   30,            4,   18,    4,    3,    // 95
       3,    4,    3,    5,           38,  100,    6,    2,    // 103
       0,    7,    3,    1,            3,    1,    8,    3,    // 111
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 9;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,    9,   16,   41,         54,   59,   72,   83,    // 7
     104,  -999
    };
    char fixuptypes[] = {
      2,   2,   1,   1,      1,   2,   1,   1,    // 7
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

    char *inpl = "\
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
    size_t const codesize = 104;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,   -1,   51,    4,            8,    3,   51,    4,    // 15
       7,    3,   29,    3,            6,    3,    1,   30,    // 23
       4,   18,    4,    3,            3,    4,    3,   28,    // 31
      63,    6,    3,    7,           29,    3,    6,    3,    // 39
      77,   29,    3,   51,           12,    7,    3,   29,    // 47
       3,    6,    3,    0,           30,    4,   19,    4,    // 55
       3,    3,    4,    3,           28,   14,    2,    1,    // 63
       8,   51,    4,    7,            3,    1,    3,    1,    // 71
       8,    3,   31,  -62,           51,    8,    7,    3,    // 79
      29,    3,    2,    1,           12,   51,    4,    7,    // 87
       3,    1,    3,    1,            8,    3,   31,  -82,    // 95
      51,    4,    7,    3,            2,    1,    4,    5,    // 103
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

TEST_F(Bytecode0, FlowIfDoWhile) {

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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FlowIfDoWhile", scrip);
    size_t const codesize = 168;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,    6,    // 7
       3,    0,   29,    3,            6,    3,   -2,   29,    // 15
       3,   51,   12,    7,            3,   29,    3,    6,    // 23
       3,   10,   30,    4,           18,    4,    3,    3,    // 31
       4,    3,   28,   90,            6,    3,    0,   51,    // 39
       4,    8,    3,   51,            4,    7,    3,   29,    // 47
       3,    6,    3,   10,           30,    4,   18,    4,    // 55
       3,    3,    4,    3,           28,   62,   51,    4,    // 63
       7,    3,   29,    3,           51,   12,    7,    3,    // 71
      30,    4,   11,    3,            4,    8,    3,   51,    // 79
       4,    7,    3,   29,            3,    6,    3,    6,    // 87
      30,    4,   15,    4,            3,    3,    4,    3,    // 95
      28,    8,   51,    4,            7,    3,    2,    1,    // 103
      12,    5,    6,    3,            3,   29,    3,   51,    // 111
       8,    7,    3,   30,            4,   11,    3,    4,    // 119
       8,    3,   31,  -81,           31,   35,    6,    3,    // 127
       1,   29,    3,   51,            8,    7,    3,   30,    // 135
       4,   11,    3,    4,            8,    3,   51,    4,    // 143
       7,    3,   29,    3,            6,    3,  100,   30,    // 151
       4,   18,    4,    3,            3,    4,    3,   70,    // 159
     -35,    6,    3,    0,            2,    1,   12,    5,    // 167
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

TEST_F(Bytecode0, FlowSwitch01) {    

    // The "break;" in line 6 is unreachable code

    char *inpl = "\
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
    FlagSet options = SCOPT_LINENUMBERS;
    int compileResult = cc_compile(inpl, options, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    // Line 6: Can't reach this point.
    EXPECT_EQ(6u, mh.GetMessages().at(0).Lineno);
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("reach this"));
    // Line 10: Cases 3 & 4 fall through to case 5; "break;" might be missing
    EXPECT_EQ(6u, mh.GetMessages().at(0).Lineno);
    EXPECT_NE(std::string::npos, mh.GetMessages().at(1).Message.find("break"));

    // WriteOutput("FlowSwitch01", scrip);
    size_t const codesize = 158;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   12,    7,    3,           30,    4,    9,    4,    // 15
       3,    3,    4,    3,            3,    3,    4,   31,    // 23
      79,    6,    3,   10,            5,   31,  123,    6,    // 31
       3,    2,   29,    3,           51,   12,    7,    3,    // 39
      30,    4,    9,    3,            4,    8,    3,   51,    // 47
       8,    7,    3,    5,            6,    3,    0,   51,    // 55
       8,    8,    3,    6,            3,    5,   29,    3,    // 63
      51,   12,    7,    3,           30,    4,   12,    4,    // 71
       3,    3,    4,    3,           29,    3,    6,    3,    // 79
       4,   30,    4,   12,            4,    3,    3,    4,    // 87
       3,   29,    3,   51,           12,    7,    3,   30,    // 95
       4,   11,    3,    4,            8,    3,   31,   50,    // 103
      29,    4,    6,    3,            2,   30,    4,   15,    // 111
       3,    4,   70,  -91,           29,    4,    6,    3,    // 119
       3,   30,    4,   15,            3,    4,   70,  -76,    // 127
      29,    4,    6,    3,            4,   30,    4,   15,    // 135
       3,    4,   70,  -88,           29,    4,    6,    3,    // 143
       5,   30,    4,   15,            3,    4,   70,  -93,    // 151
      31, -123,    6,    3,            0,    5,  -999
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
    char *inpl = "\
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

    WriteOutput("FlowSwitch02", scrip);
    size_t const codesize = 45;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,   51,    // 7
       4,    7,    3,    3,            3,    4,   31,   11,    // 15
      31,   23,    6,    3,            0,   51,    4,    8,    // 23
       3,   31,   14,   29,            4,    6,    3,    5,    // 31
      30,    4,   15,    3,            4,   70,  -21,   31,    // 39
     -25,    2,    1,    4,            5,  -999
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
    char *inpl = "\
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

    size_t const codesize = 37;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,   51,    // 7
       4,    7,    3,    3,            3,    4,   31,    2,    // 15
      31,   12,   29,    4,            6,    3,    0,   30,    // 23
       4,   15,    3,    4,           70,  -14,    2,    1,    // 31
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

    char *inpl = "\
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
    size_t const codesize = 27;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            7,   29,    3,   51,    // 7
       4,    7,    3,    3,            3,    4,   31,    2,    // 15
      31,    2,   31,   -4,            2,    1,    4,    6,    // 23
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

TEST_F(Bytecode0, FlowSwitch05) {

    // No default/case clauses (zany but allowed)

    char *inpl = "\
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
    size_t const codesize = 14;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    2,    // 7
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

TEST_F(Bytecode0, FlowSwitch06) {

    // 'Default' and 'case 77' fall through, compiler should warn about the latter

    char *inpl = "\
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
    size_t const codesize = 77;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            7,   29,    3,   51,    // 7
       4,    7,    3,    3,            3,    4,   31,   16,    // 15
       6,    3,    6,   51,            4,    8,    3,    6,    // 23
       3,    5,   51,    4,            8,    3,   31,   38,    // 31
      29,    4,    6,    3,            7,   30,    4,   15,    // 39
       3,    4,   70,  -21,           29,    4,    6,    3,    // 47
      77,   30,    4,   15,            3,    4,   70,  -33,    // 55
      29,    4,    6,    3,          777,   30,    4,   15,    // 63
       3,    4,   70,  -38,           31,  -54,    2,    1,    // 71
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



TEST_F(Bytecode0, FreeLocalPtr) {   

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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("FreeLocalPtr", scrip);
    const size_t codesize = 67;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            4,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3,    0,   29,    // 15
       3,   51,    4,    7,            3,   29,    3,    6,    // 23
       3,   10,   30,    4,           18,    4,    3,    3,    // 31
       4,    3,   28,   18,           73,    3,    4,   51,    // 39
       8,   47,    3,   51,            4,    7,    3,    1,    // 47
       3,    1,    8,    3,           31,  -37,    2,    1,    // 55
       4,   51,    4,   49,            2,    1,    4,    6,    // 63
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct01) {

    char *inpl = "\
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
    size_t const codesize = 127;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
       6,    3,    0,    3,            6,    2,   52,    8,    // 15
       3,    6,    3,    5,           72,    3,    4,    0,    // 23
      51,    4,   47,    3,            6,    3,   77,   29,    // 31
       3,   51,    8,   48,            2,   52,    1,    2,    // 39
      12,   30,    3,    8,            3,   51,    4,   48,    // 47
       3,   29,    3,   51,            4,   50,    3,   51,    // 55
       8,   49,   51,    4,           48,    3,   69,   30,    // 63
       4,    2,    1,    4,            5,   38,   69,    6,    // 71
       3,    0,   29,    3,           51,    4,   29,    2,    // 79
       6,    3,   -1,   29,            3,   51,    8,    7,    // 87
       2,   45,    2,    6,            3,    0,   23,    3,    // 95
       2,    1,    4,   30,            2,   51,    0,   47,    // 103
       3,    1,    1,    4,           51,    4,   48,    2,    // 111
      52,    1,    2,   12,            7,    3,   29,    3,    // 119
      51,    8,   49,    2,            1,   12,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      93,  -999
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

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct02", scrip);
    size_t const codesize = 83;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,    6,    2,   68,    // 7
       8,    3,    6,    3,            3,   29,    3,    6,    // 15
       2,   80,   30,    3,            8,    3,    6,    3,    // 23
      42,   29,    3,    6,            2,    0,   29,    2,    // 31
       6,    2,   80,    7,            3,   30,    2,   46,    // 39
       3,   17,   32,    3,            4,   11,    2,    3,    // 47
      30,    3,    8,    3,            6,    3,   19,   29,    // 55
       3,    6,    2,    0,           29,    2,    6,    2,    // 63
      68,    7,    3,   30,            2,   46,    3,   17,    // 71
      32,    3,    4,   11,            2,    3,   30,    3,    // 79
       8,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   17,   29,   34,         59,   64,  -999
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

    char *inpl = "\
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
    size_t const codesize = 83;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,    6,    2,   68,    // 7
       8,    3,    6,    3,            3,   29,    3,    6,    // 15
       2,   80,   30,    3,            8,    3,    6,    3,    // 23
      42,   29,    3,    6,            2,    0,   29,    2,    // 31
       6,    2,   80,    7,            3,   30,    2,   46,    // 39
       3,   17,   32,    3,            4,   11,    2,    3,    // 47
      30,    3,    8,    3,            6,    3,   19,   29,    // 55
       3,    6,    2,    0,           29,    2,    6,    2,    // 63
      68,    7,    3,   30,            2,   46,    3,   17,    // 71
      32,    3,    4,   11,            2,    3,   30,    3,    // 79
       8,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   17,   29,   34,         59,   64,  -999
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct04", scrip);
    size_t const codesize = 100;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,   16,    1,    1,    // 7
      16,   73,    3,    4,           51,   16,   47,    3,    // 15
       6,    3, 12345,   51,           16,   48,    2,   52,    // 23
       8,    3,   51,    0,           63,   48,    1,    1,    // 31
      48,   73,    3,    4,           29,    3,   51,   20,    // 39
      30,    3,   47,    3,           51,   64,   49,    1,    // 47
       2,    4,   49,    1,            2,    4,   49,    1,    // 55
       2,    4,   49,   51,           48,    6,    3,    3,    // 63
      29,    2,   29,    3,           49,    1,    2,    4,    // 71
      49,    1,    2,    4,           49,    1,    2,    4,    // 79
      49,   30,    3,   30,            2,    1,    2,   16,    // 87
       2,    3,    1,   70,          -29,    2,    1,   64,    // 95
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

TEST_F(Bytecode0, Struct05) {   

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
   
    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    EXPECT_LE(2u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0u).Message.find("zero"));
    EXPECT_NE(std::string::npos, mh.GetMessages().at(1u).Message.find("zero"));

    // WriteOutput("Struct05", scrip);
    size_t const codesize = 28;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            7,   34,    3,   39,    // 7
       1,    6,    3,    0,           33,    3,   35,    1,    // 15
      34,    3,   39,    1,            6,    3,    0,   33,    // 23
       3,   35,    1,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   22,  -999
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

    char *inpl = "\
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
    size_t const codesize = 48;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,    5,   72,    3,            4,    1,   51,    4,    // 15
      47,    3,    6,    3,           77,   29,    3,   51,    // 23
       8,   48,    2,   52,            1,    2,   12,   48,    // 31
       2,   52,   30,    3,            8,    3,   51,    4,    // 39
      49,    2,    1,    4,            6,    3,    0,    5,    // 47
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

TEST_F(Bytecode0, Struct07) {    

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
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       2,    8,   30,    3,            8,    3,    6,    3,    // 15
      65,   29,    3,    6,            2,   12,   30,    3,    // 23
      26,    3,    6,    2,           12,   24,    3,   29,    // 31
       3,    6,    3,   65,           30,    4,   12,    4,    // 39
       3,    3,    4,    3,           29,    3,    6,    2,    // 47
      13,   30,    3,   26,            3,    6,    2,   12,    // 55
      24,    3,    2,    3,            1,   26,    3,    6,    // 63
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,   21,   28,   48,         55,  -999
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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct08", scrip);
    size_t const codesize = 79;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   42,    3,    // 7
      70,   34,   29,    3,           51,   16,    7,    3,    // 15
      42,    3,   28,   16,           29,    3,    3,    6,    // 23
       2,   52,    7,    3,           30,    4,   21,    4,    // 31
       3,    3,    4,    3,           30,    4,   22,    4,    // 39
       3,    3,    4,    3,           70,   32,   29,    3,    // 47
       6,    3,    0,   29,            3,    3,    6,    2,    // 55
      52,    1,    2,    4,            7,    3,   30,    4,    // 63
      16,    4,    3,    3,            4,    3,   30,    4,    // 71
      22,    4,    3,    3,            4,    3,    5,  -999
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

    char *inpl = "\
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
    const size_t codesize = 193;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            6,   72,    3,    4,    // 7
       0,   51,    0,   47,            3,    1,    1,    4,    // 15
       6,    3,    5,   29,            3,    6,    2,    4,    // 23
      29,    2,   51,    8,            7,    3,   30,    2,    // 31
      46,    3,    6,   32,            3,    4,   11,    2,    // 39
       3,   29,    2,    6,            3,    0,   34,    3,    // 47
       6,    3,    0,   34,            3,    6,    3,    3,    // 55
      29,    3,   51,   12,            7,    3,   30,    4,    // 63
      11,    4,    3,    3,            4,    3,   34,    3,    // 71
       6,    3,    3,   34,            3,    6,    3,    7,    // 79
      29,    3,   51,   16,           48,    2,   52,   29,    // 87
       2,   51,   16,    7,            3,   30,    2,   32,    // 95
       3,    4,   71,    3,           11,    2,    3,    7,    // 103
       3,   30,    4,   11,            4,    3,    3,    4,    // 111
       3,   34,    3,    6,            2,    1,   29,    2,    // 119
       6,    2,    2,   29,            6,   45,    2,   39,    // 127
       0,    6,    3,    0,           33,    3,   30,    6,    // 135
      29,    3,   51,   16,            7,    3,   30,    4,    // 143
      11,    4,    3,    3,            4,    3,   30,    2,    // 151
      46,    3,    7,   32,            3,    0,   11,    2,    // 159
       3,    3,    2,    3,           34,    3,   51,    4,    // 167
       7,    2,   45,    2,           39,    6,    6,    3,    // 175
       3,   33,    3,   35,            6,   30,    2,   51,    // 183
       8,   49,    2,    1,            8,    6,    3,    0,    // 191
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      23,  117,  122,  131,        176,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 5;
    std::string imports[] = {
    "Character::get_ID^0",        "character",   "cAICar1",     "VehicleBase::SetCharacter^6",               // 3
    "Cars",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Struct10) {

    // When accessing a component of an import variable,
    // the import variable must be read first so that the fixup can be
    // applied. Only then may the offset be added to it.

    char *inpl = "\
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
    size_t const codesize = 11;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    1,    2,    4,    // 7
       7,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,  -999
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
    // Since Inner1 is managed, In1 will convert into an Inner1 *.

    char *inpl = "\
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
    size_t const codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            8,    6,    2,    1,    // 7
      47,    3,    6,    3,           77,    6,    2,    1,    // 15
      48,    2,   52,    1,            2,    2,    8,    3,    // 23
       6,    3,  777,    6,            2,    1,    1,    2,    // 31
       6,    8,    3,    6,            2,    1,   48,    2,    // 39
      52,    1,    2,    2,            7,    3,   29,    3,    // 47
       6,    2,    1,    1,            2,    6,    7,    3,    // 55
      30,    4,   11,    4,            3,    3,    4,    3,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   15,   29,   37,         50,  -999
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

    char *inpl = "\
        struct NonManaged           \n\
        {                           \n\
            long Dummy;             \n\
            int  IntArray[];        \n\
        } SS;                       \n\
                                    \n\
        int main()                  \n\
        {                           \n\
            SS.IntArray = new int[10];  \n\
            SS.IntArray[3] = 7;     \n\
            return SS.IntArray[3];  \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct12", scrip);
    size_t const codesize = 44;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           10,   72,    3,    4,    // 7
       0,    6,    2,    4,           47,    3,    6,    3,    // 15
       7,   29,    3,    6,            2,    4,   48,    2,    // 23
      52,    1,    2,   12,           30,    3,    8,    3,    // 31
       6,    2,    4,   48,            2,   52,    1,    2,    // 39
      12,    7,    3,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   21,   34,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,  '\0'
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

TEST_F(Bytecode0, StructArray01) {

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
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StructArray01", scrip);
    const size_t codesize = 39;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,           40,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3,    1,   29,    // 15
       3,   51,    8,   48,            2,   52,    1,    2,    // 23
      16,   30,    3,    8,            3,   51,    4,   49,    // 31
       2,    1,    4,    6,            3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, StructArray02) {

    // Static arrays can be multidimensional

    char *inpl = "\
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

    // WriteOutput("StructArray02", scrip);
    const size_t codesize = 63;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,          104,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3,    1,   29,    // 15
       3,   51,    8,   48,            2,   52,    1,    2,    // 23
      72,   30,    3,    8,            3,   51,    4,   48,    // 31
       2,   52,    1,    2,           72,    7,    3,   29,    // 39
       3,   51,    8,   48,            2,   52,    1,    2,    // 47
     100,   30,    3,    8,            3,   51,    4,   49,    // 55
       2,    1,    4,    6,            3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func01) {

    char *inpl = "\
        int Foo()      \n\
    {                  \n\
        return 15;     \n\
    }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func01", scrip);
    size_t const codesize = 6;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,    5,  -999
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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func02", scrip);
    size_t const codesize = 51;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
      51,    0,   49,    1,            1,    4,   51,    4,    // 15
      48,    3,   34,    3,           51,    8,   48,    3,    // 23
      34,    3,   39,    2,            6,    3,    0,   33,    // 31
       3,   35,    2,   29,            3,   51,    4,    7,    // 39
       3,   51,   12,   49,           51,    8,   49,    2,    // 47
       1,   12,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      30,  -999
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
    size_t const codesize = 51;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
      51,    0,   49,    1,            1,    4,   51,    4,    // 15
      48,    3,   34,    3,           51,    8,   48,    3,    // 23
      34,    3,   39,    2,            6,    3,    0,   33,    // 31
       3,   35,    2,   29,            3,   51,    4,    7,    // 39
       3,   51,   12,   49,           51,    8,   49,    2,    // 47
       1,   12,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      30,  -999
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
    size_t const codesize = 74;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   50,    3,    // 7
      51,   12,    7,    3,           50,    3,    6,    3,    // 15
       0,   51,    8,   49,           51,   12,   49,    5,    // 23
      38,   24,   51,    0,           49,    1,    1,    4,    // 31
      51,    0,   49,    1,            1,    4,   51,    4,    // 39
      48,    3,   29,    3,           51,   12,   48,    3,    // 47
      29,    3,    6,    3,            0,   23,    3,    2,    // 55
       1,    8,   29,    3,           51,    4,    7,    3,    // 63
      51,   12,   49,   51,            8,   49,    2,    1,    // 71
      12,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      52,  -999
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
    
    char *inpl = "\
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
    size_t const codesize = 38;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,    6,    // 7
       3,   32,   23,    3,            2,    1,    4,   51,    // 15
       0,   47,    3,    1,            1,    4,    6,    3,    // 23
      -1,   51,    4,   49,            2,    1,    4,    5,    // 31
      38,   32,   73,    3,            4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,  -999
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
   
    char *inpl = "\
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
    size_t const codesize = 47;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   11,    4,    // 15
       3,    3,    4,    3,            5,   38,   21,    6,    // 23
       3,    5,   29,    3,            6,    3,    4,   29,    // 31
       3,    6,    3,    0,           23,    3,    2,    1,    // 39
       8,   29,    3,    2,            1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      35,  -999
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

    char *inpl = "\
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
    const size_t codesize = 48;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   34,    3,    6,    // 7
       3,    4,   34,    3,           39,    2,    6,    3,    // 15
       0,   33,    3,   35,            2,   29,    3,    6,    // 23
       3,    1,   34,    3,            6,    3,    4,   34,    // 31
       3,   39,    2,    6,            3,    0,   33,    3,    // 39
      35,    2,   29,    3,            2,    1,    8,    5,    // 47
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      16,   37,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func08) {   

    char *inpl = "\
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
    const size_t codesize = 48;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   34,    3,    6,    // 7
       3,    4,   34,    3,           39,    2,    6,    3,    // 15
       0,   33,    3,   35,            2,   29,    3,    6,    // 23
       3,    1,   34,    3,            6,    3,    4,   34,    // 31
       3,   39,    2,    6,            3,    0,   33,    3,    // 39
      35,    2,   29,    3,            2,    1,    8,    5,    // 47
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      16,   37,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "Func",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func09) {    

    char *inpl = "\
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
    size_t const codesize = 48;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   34,    3,    6,    // 7
       3,    4,   34,    3,           39,    2,    6,    3,    // 15
       0,   33,    3,   35,            2,   29,    3,    6,    // 23
       3,    1,   34,    3,            6,    3,    4,   34,    // 31
       3,   39,    2,    6,            3,    0,   33,    3,    // 39
      35,    2,   29,    3,            2,    1,    8,    5,    // 47
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      16,   37,  -999
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
    
    char *inpl = "\
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
    size_t const codesize = 47;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   11,    4,    // 15
       3,    3,    4,    3,            5,   38,   21,    6,    // 23
       3,  -99,   29,    3,            6,    3,    4,   29,    // 31
       3,    6,    3,    0,           23,    3,    2,    1,    // 39
       8,   29,    3,    2,            1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      35,  -999
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

    char *inpl = "\
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
    size_t const codesize = 45;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,    5,   38,    6,    // 7
       6,    3,    0,   29,            3,   51,    4,   45,    // 15
       2,    6,    3,    0,           23,    3,   29,    3,    // 23
       6,    3,    3,   30,            4,   40,    4,    3,    // 31
       3,    4,    3,   29,            3,   51,    4,    7,    // 39
       3,    2,    1,    8,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      19,  -999
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

    char *inpl = "\
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
    size_t const codesize = 53;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,    5,   38,    // 7
       7,   51,    8,    7,            3,    5,   38,   14,    // 15
       6,    3, 1088841318,   29,            3,    6,    3,    0,    // 23
      23,    3,    2,    1,            4,   29,    3,    6,    // 31
       3,    0,   29,    3,            6,    3,    7,   23,    // 39
       3,    2,    1,    4,           30,    4,   57,    4,    // 47
       3,    3,    4,    3,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      23,   38,  -999
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

    char *inpl = "\
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
    int Call()                           \n\
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
    size_t const codesize = 97;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   50,    3,    // 7
      51,    8,   48,    3,           29,    3,   51,    4,    // 15
      50,    3,   51,   12,           49,   51,    4,   48,    // 23
       3,   69,   30,    4,            5,   38,   29,   51,    // 31
       8,    7,    3,   50,            3,   51,    8,   48,    // 39
       3,   29,    3,   51,            4,   50,    3,   51,    // 47
      12,   49,   51,    4,           48,    3,   69,   30,    // 55
       4,    5,   38,   58,            6,    3,    0,   29,    // 63
       3,    6,    3,    0,           23,    3,    2,    1,    // 71
       4,   29,    3,    6,            3,    0,   29,    3,    // 79
       6,    3,   29,   23,            3,    2,    1,    4,    // 87
      30,    4,   15,    4,            3,    3,    4,    3,    // 95
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      67,   82,  -999
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

    char *inpl = "\
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
    const size_t codesize = 139;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            1,   72,    3,    4,    // 7
       0,    3,    6,    2,           52,   47,    3,    6,    // 15
       3,    1,   72,    3,            4,    0,    3,    6,    // 23
       2,   52,    1,    2,            4,   47,    3,    6,    // 31
       3,  123,   29,    3,            3,    6,    2,   52,    // 39
       1,    2,    4,   48,            2,   52,   30,    3,    // 47
       8,    3,   29,    6,            3,    6,    2,   52,    // 55
       1,    2,    4,   48,            2,   52,    7,    3,    // 63
      29,    3,    3,    6,            2,   52,   48,    2,    // 71
      52,    7,    3,   29,            3,    6,    3,   88,    // 79
      23,    3,    2,    1,            8,   30,    6,    5,    // 87
      38,   88,    5,   38,           91,   51,    0,   63,    // 95
       8,    1,    1,    8,           51,    8,   29,    2,    // 103
       6,    3,    7,   29,            3,   51,    8,    7,    // 111
       2,   45,    2,    6,            3,    0,   23,    3,    // 119
       2,    1,    4,   30,            2,   51,    8,   49,    // 127
       1,    2,    4,   49,            2,    1,    8,    6,    // 135
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      79,  117,  -999
    };
    char fixuptypes[] = {
      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Func15) {

    char *inpl = "\
        int a = 15;    \n\
        int Foo( )     \n\
        {              \n\
            return a;  \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func15", scrip);
    size_t const codesize = 8;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    7,    3,    5,    // 7
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,  -999
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

TEST_F(Bytecode0, Func16) {

    char *inpl = "\
        int Foo(int a) \n\
        {              \n\
            return a;  \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func16", scrip);
    size_t const codesize = 7;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,    5,  -999
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

TEST_F(Bytecode0, Func17) {

    char *inpl = "\
        int Foo()       \n\
        {               \n\
            int a = 15; \n\
            return a;   \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func17", scrip);
    size_t const codesize = 15;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,   29,    3,   51,    // 7
       4,    7,    3,    2,            1,    4,    5,  -999
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

TEST_F(Bytecode0, Func18) {

    // NON-managed dynpointers must be read/rewritten at function start, too.

    char inpl[] = "\
        int Random(int X)                       \n\
        {                                       \n\
            Shuffle(new int[15], 10);           \n\
        }                                       \n\
        void Shuffle(int Ints[], int Length)    \n\
        {                                       \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func18", scrip);
    const size_t codesize = 40;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           10,   29,    3,    6,    // 7
       3,   15,   72,    3,            4,    0,   29,    3,    // 15
       6,    3,   28,   23,            3,    2,    1,    8,    // 23
       6,    3,    0,    5,           38,   28,   51,    8,    // 31
       7,    3,   50,    3,           51,    8,   49,    5,    // 39
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Function19) {

    // Simple void function
    char *inpl = "\
        void Foo()          \n\
        {                   \n\
            return;         \n\
        }";

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Function19", scrip);
    size_t const codesize = 3;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    5,  -999
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

TEST_F(Bytecode0, Func20) {

    // Multiple forward calls should _all_ resolve to the correct function address

    char inpl[] = "\
        int main()                      \n\
        {                               \n\
            MKB(0, 7);                  \n\
            MKB(1, 9);                  \n\
            MKB(2, 7);                  \n\
            MKB(3, 11);                 \n\
            MKB(4, 9);                  \n\
            MKB(5, 8);                  \n\
        }                               \n\
        int MKB(int cp, int lastbook)   \n\
        {                               \n\
            return cp + lastbook;       \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func20", scrip);
    size_t const codesize = 135;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            7,   29,    3,    6,    // 7
       3,    0,   29,    3,            6,    3,  114,   23,    // 15
       3,    2,    1,    8,            6,    3,    9,   29,    // 23
       3,    6,    3,    1,           29,    3,    6,    3,    // 31
     114,   23,    3,    2,            1,    8,    6,    3,    // 39
       7,   29,    3,    6,            3,    2,   29,    3,    // 47
       6,    3,  114,   23,            3,    2,    1,    8,    // 55
       6,    3,   11,   29,            3,    6,    3,    3,    // 63
      29,    3,    6,    3,          114,   23,    3,    2,    // 71
       1,    8,    6,    3,            9,   29,    3,    6,    // 79
       3,    4,   29,    3,            6,    3,  114,   23,    // 87
       3,    2,    1,    8,            6,    3,    8,   29,    // 95
       3,    6,    3,    5,           29,    3,    6,    3,    // 103
     114,   23,    3,    2,            1,    8,    6,    3,    // 111
       0,    5,   38,  114,           51,    8,    7,    3,    // 119
      29,    3,   51,   16,            7,    3,   30,    4,    // 127
      11,    4,    3,    3,            4,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      14,   32,   50,   68,         86,  104,  -999
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
    
    char *inpl = "\
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
    size_t const codesize = 30;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    8,    1,    1,    // 7
       8,    6,    3,    3,           51,    4,    8,    3,    // 15
       6,    3, 1056964608,   51,            8,    8,    3,    6,    // 23
       3,   -2,    2,    1,            8,    5,  -999
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
        for (int i = 0; i < 9; i++)      \n\
            arr[i] = new Struct;         \n\
                                         \n\
        int test = (arr[10] == null);    \n\
    }                                    \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("ArrayOfPointers1", scrip);
    const size_t codesize = 96;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,   51,    // 7
       4,    7,    3,   29,            3,    6,    3,    9,    // 15
      30,    4,   18,    4,            3,    3,    4,    3,    // 23
      28,   40,   73,    3,            8,   29,    3,    6,    // 31
       2,    0,   29,    2,           51,   12,    7,    3,    // 39
      30,    2,   46,    3,           50,   32,    3,    4,    // 47
      11,    2,    3,   30,            3,   47,    3,   51,    // 55
       4,    7,    3,    1,            3,    1,    8,    3,    // 63
      31,  -59,    2,    1,            4,    6,    2,   40,    // 71
      48,    3,   29,    3,            6,    3,    0,   30,    // 79
       4,   15,    4,    3,            3,    4,    3,   29,    // 87
       3,    2,    1,    4,            6,    3,    0,    5,    // 95
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      33,   71,  -999
    };
    char fixuptypes[] = {
      1,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, ArrayOfPointers2) {    

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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("ArrayOfPointers2", scrip);
    size_t const codesize = 121;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,  200,    1,    1,    // 7
     200,    6,    3,    0,           29,    3,   51,    4,    // 15
       7,    3,   29,    3,            6,    3,   20,   30,    // 23
       4,   18,    4,    3,            3,    4,    3,   28,    // 31
      39,   73,    3,    8,           29,    3,   51,  208,    // 39
      29,    2,   51,   12,            7,    3,   30,    2,    // 47
      46,    3,   50,   32,            3,    4,   11,    2,    // 55
       3,   30,    3,   47,            3,   51,    4,    7,    // 63
       3,    1,    3,    1,            8,    3,   31,  -58,    // 71
       2,    1,    4,    6,            3, 1074580685,   29,    3,    // 79
      51,  184,   48,    2,           52,   30,    3,    8,    // 87
       3,    6,    3,    0,           29,    3,   51,  188,    // 95
      30,    3,   47,    3,           51,  200,    6,    3,    // 103
      50,   49,    1,    2,            4,    2,    3,    1,    // 111
      70,   -9,    2,    1,          200,    6,    3,    0,    // 119
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

TEST_F(Bytecode0, Writeprotected) {
    
    // Directly taken from the doc on writeprotected, simplified.

    char *inpl = "\
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
    size_t const codesize = 27;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,    3,    6,    // 7
       2,   52,    1,    2,            2,    8,    3,    6,    // 15
       3,    0,    5,   38,           19,    6,    2,    2,    // 23
       7,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      23,  -999
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

    char *inpl = "\
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
    size_t const codesize = 16;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,    3,    6,    // 7
       2,   52,    8,    3,            6,    3,    0,    5,    // 15
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

TEST_F(Bytecode0, Static1) {   

    char *inpl = "\
        struct Weapon {                         \n\
            import static int CalcDamage(       \n\
            int Lifepoints, int Hitpoints = 5); \n\
        };                                      \n\
                                                \n\
        static int Weapon::CalcDamage(int Lifepoints, int Hitpoints)  \n\
        {                                       \n\
            return Lifepoints - Hitpoints;      \n\
        }                                       \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            int hp = Weapon.CalcDamage(9) + Weapon.CalcDamage(9, 40);  \n\
            return hp + Weapon.CalcDamage(100);     \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Static1", scrip);
    size_t const codesize = 107;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   12,    4,    // 15
       3,    3,    4,    3,            5,   38,   21,    6,    // 23
       3,    5,   29,    3,            6,    3,    9,   29,    // 31
       3,    6,    3,    0,           23,    3,    2,    1,    // 39
       8,   29,    3,    6,            3,   40,   29,    3,    // 47
       6,    3,    9,   29,            3,    6,    3,    0,    // 55
      23,    3,    2,    1,            8,   30,    4,   11,    // 63
       4,    3,    3,    4,            3,   29,    3,   51,    // 71
       4,    7,    3,   29,            3,    6,    3,    5,    // 79
      29,    3,    6,    3,          100,   29,    3,    6,    // 87
       3,    0,   23,    3,            2,    1,    8,   30,    // 95
       4,   11,    4,    3,            3,    4,    3,    2,    // 103
       1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      35,   55,   89,  -999
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

    char *inpl = "\
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
    size_t const codesize = 42;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   12,    4,    // 15
       3,    3,    4,    3,            5,   38,   21,    6,    // 23
       3,   40,   29,    3,            6,    3,    9,   29,    // 31
       3,    6,    3,    0,           23,    3,    2,    1,    // 39
       8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      35,  -999
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

TEST_F(Bytecode0, Protected2) {

    // In a struct func, a variable that can't be found otherwise
    // should be taken to be out of the current struct.
    // (Note that this will currently compile to slightly more
    // inefficient code than "this.Damage = damage")

    char *inpl = "\
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
    size_t const codesize = 20;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
       3,    6,    2,   52,           30,    3,    8,    3,    // 15
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

TEST_F(Bytecode0, Import) {    

    char *inpl = "\
        import int Weapon;                     \n\
                                               \n\
        int Func(int damage)                   \n\
        {                                      \n\
            int Int = 0;                       \n\
            Weapon = 77;                       \n\
            if (Weapon < 0)                    \n\
                Weapon = damage - (Int - Weapon) / Int; \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Import", scrip);
    const size_t codesize = 94;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,   77,    6,    2,            0,    8,    3,    6,    // 15
       2,    0,    7,    3,           29,    3,    6,    3,    // 23
       0,   30,    4,   18,            4,    3,    3,    4,    // 31
       3,   28,   52,   51,           12,    7,    3,   29,    // 39
       3,   51,    8,    7,            3,   29,    3,    6,    // 47
       2,    0,    7,    3,           30,    4,   12,    4,    // 55
       3,    3,    4,    3,           29,    3,   51,   12,    // 63
       7,    3,   30,    4,           10,    4,    3,    3,    // 71
       4,    3,   30,    4,           12,    4,    3,    3,    // 79
       4,    3,    6,    2,            0,    8,    3,    2,    // 87
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      12,   17,   49,   84,        -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "Weapon",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}
