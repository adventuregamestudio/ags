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
    ::ccCompiledScript scrip;

    Bytecode0()
    {
        // Initializations, will be done at the start of each test
        scrip.init();
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        ccSetOption(SCOPT_LINENUMBERS, false);
        clear_error();
    }
};


TEST_F(Bytecode0, SimpleVoidFunction) {

    char *inpl = "\
        void Foo()          \n\
        {                   \n\
            return;         \n\
        }";

    
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("SimpleVoidFunction", scrip);
    const size_t codesize = 5;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   31,    0,            5,  -999
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
    const size_t codesize = 60;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,    6,    // 7
       3,    7,   29,    3,           51,    8,    7,    3,    // 15
       6,    4,    0,   12,            4,    3,    3,    4,    // 23
       3,   29,    3,   51,            8,    7,    3,    6,    // 31
       4,    0,   12,    4,            3,    3,    4,    3,    // 39
      30,    4,    9,    4,            3,    3,    4,    3,    // 47
       2,    1,    8,   31,            6,    2,    1,    8,    // 55
       6,    3,    0,    5,          -999
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
    const size_t codesize = 29;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,   51,    // 7
       4,    7,    3,   42,            3,   42,    3,   42,    // 15
       3,    2,    1,    4,           31,    6,    2,    1,    // 23
       4,    6,    3,    0,            5,  -999
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

TEST_F(Bytecode0, SimpleIntFunction) { 

    char *inpl = "\
        int Foo()      \n\
    {                  \n\
        return 15;     \n\
    }";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("SimpleIntFunction", scrip);
    const size_t codesize = 11;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,   31,    3,    6,    // 7
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

TEST_F(Bytecode0, IntFunctionLocalV) {

    char *inpl = "\
        int Foo()       \n\
        {               \n\
            int a = 15; \n\
            return a;   \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("IntFunctionLocalV", scrip);
    const size_t codesize = 23;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,   29,    3,   51,    // 7
       4,    7,    3,    2,            1,    4,   31,    6,    // 15
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

TEST_F(Bytecode0, IntFunctionParam) {

    char *inpl = "\
        int Foo(int a) \n\
    {                  \n\
        return a;      \n\
    }";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("IntFunctionParam", scrip);
    const size_t codesize = 12;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   31,    3,    // 7
       6,    3,    0,    5,          -999
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

TEST_F(Bytecode0, IntFunctionGlobalV) {    

    char *inpl = "\
        int a = 15;    \n\
        int Foo( )     \n\
    {                  \n\
        return a;      \n\
    }";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("IntFunctionGlobalV", scrip);
    const size_t codesize = 13;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    7,    3,   31,    // 7
       3,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
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

TEST_F(Bytecode0, Float1) {   

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
        }                                   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Float1", scrip);
    const size_t codesize = 156;
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
       2,    1,   32,   31,            6,    2,    1,   32,    // 151
       6,    3,    0,    5,          -999
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

TEST_F(Bytecode0, Float2) {   

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

    // WriteOutput("Float2", scrip);
    const size_t codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         -1059061760,   29,    3,   51,    // 7
      12,    7,    3,   30,            4,   57,    4,    3,    // 15
       3,    4,    3,   29,            3,   51,   16,    7,    // 23
       3,   30,    4,   58,            4,    3,    3,    4,    // 31
       3,   31,    3,    6,            3,    0,    5,   38,    // 39
      39,    6,    3, -1070805811,           29,    3,    6,    3,    // 47
    1088841318,   29,    3,    6,            3,    0,   23,    3,    // 55
       2,    1,    8,   31,            3,    6,    3,    0,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      53,  -999
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

TEST_F(Bytecode0, FloatExpr1) { 

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

    // WriteOutput("FloatExpr1", scrip);
    const size_t codesize = 38;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         1078523331,   29,    3,    6,    // 7
       2,    0,    7,    3,           29,    3,   51,    8,    // 15
       7,    3,   30,    4,           57,    4,    3,    3,    // 23
       4,    3,    2,    1,            4,   31,    6,    2,    // 31
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
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

TEST_F(Bytecode0, FloatExpr2) {    

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

    // WriteOutput("FloatExpr2", scrip);
    const size_t codesize = 244;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         1102158234,   29,    3,    6,    // 7
       3, 1078523331,   29,    3,            6,    3, 1068205343,   30,    // 15
       4,   60,    4,    3,            3,    4,    3,   29,    // 23
       3,    6,    3,    1,           30,    4,   15,    4,    // 31
       3,    3,    4,    3,           29,    3,    6,    3,    // 39
       0,   29,    3,    6,            3, 1150963712,   29,    3,    // 47
       6,    3, 1084227584,   30,            4,   59,    4,    3,    // 55
       3,    4,    3,   30,            4,   15,    4,    3,    // 63
       3,    4,    3,   28,           13,   29,    3,    6,    // 71
       3,    1,   30,    4,           21,    4,    3,    3,    // 79
       4,    3,   51,    0,           27,    3,    1,    1,    // 87
       2,    6,    2,    0,            7,    3,   29,    3,    // 95
       6,    3, 1110546842,   30,            4,   62,    4,    3,    // 103
       3,    4,    3,   29,            3,    6,    3, 1113456640,    // 111
      29,    3,    6,    3,         1110546842,   30,    4,   61,    // 119
       4,    3,    3,    4,            3,   51,    0,   26,    // 127
       3,    1,    1,    1,            6,    2,    0,    7,    // 135
       3,   29,    3,   51,           19,    7,    3,   30,    // 143
       4,   15,    4,    3,            3,    4,    3,   70,    // 151
      29,   29,    3,    6,            2,    0,    7,    3,    // 159
      29,    3,   51,   23,            7,    3,   30,    4,    // 167
      16,    4,    3,    3,            4,    3,   30,    4,    // 175
      22,    4,    3,    3,            4,    3,   29,    3,    // 183
       6,    2,    0,    7,            3,   29,    3,   51,    // 191
      23,    7,    3,   29,            3,    6,    2,    0,    // 199
       7,    3,   29,    3,           51,   31,    7,    3,    // 207
      30,    4,   56,    4,            3,    3,    4,    3,    // 215
      30,    4,   55,    4,            3,    3,    4,    3,    // 223
      30,    4,   58,    4,            3,    3,    4,    3,    // 231
       2,    1,   19,   31,            6,    2,    1,   19,    // 239
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      91,  134,  157,  186,        199,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
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

TEST_F(Bytecode0, IfThenElse1) {  

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
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("IfThenElse1", scrip);
    const size_t codesize = 102;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,   29,    3,    6,    // 7
       3,    4,   29,    3,            6,    3,    2,   30,    // 15
       4,    9,    4,    3,            3,    4,    3,   30,    // 23
       4,   12,    4,    3,            3,    4,    3,   29,    // 31
       3,   51,    4,    7,            3,   29,    3,    6,    // 39
       3,    5,   30,    4,           18,    4,    3,    3,    // 47
       4,    3,   28,   18,            6,    3,    2,   29,    // 55
       3,   51,    8,    7,            3,   30,    4,   44,    // 63
       3,    4,    8,    3,           31,   16,    6,    3,    // 71
       3,   29,    3,   51,            8,    7,    3,   30,    // 79
       4,   43,    3,    4,            8,    3,   51,    4,    // 87
       7,    3,    2,    1,            4,   31,    6,    2,    // 95
       1,    4,    6,    3,            0,    5,  -999
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

TEST_F(Bytecode0, IfThenElse2) {    

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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("IfThenElse2", scrip);
    const size_t codesize = 102;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,   29,    3,    6,    // 7
       3,    4,   29,    3,            6,    3,    2,   30,    // 15
       4,   40,    4,    3,            3,    4,    3,   30,    // 23
       4,   12,    4,    3,            3,    4,    3,   29,    // 31
       3,   51,    4,    7,            3,   29,    3,    6,    // 39
       3,    5,   30,    4,           19,    4,    3,    3,    // 47
       4,    3,   28,   18,            6,    3,    2,   29,    // 55
       3,   51,    8,    7,            3,   30,    4,   12,    // 63
       3,    4,    8,    3,           31,   16,    6,    3,    // 71
       3,   29,    3,   51,            8,    7,    3,   30,    // 79
       4,   11,    3,    4,            8,    3,   51,    4,    // 87
       7,    3,    2,    1,            4,   31,    6,    2,    // 95
       1,    4,    6,    3,            0,    5,  -999
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
    const size_t codesize = 108;
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
       2,    1,    4,   31,            6,    2,    1,    4,    // 103
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,   34,   60,   70,        -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,     '\0'
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

TEST_F(Bytecode0, DoNCall) {

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

    // WriteOutput("DoNCall", scrip);
    const size_t codesize = 120;
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
       2,    1,    4,   31,            6,    2,    1,    4,    // 79
       6,    3,    0,    5,           38,   84,   51,    8,    // 87
       7,    3,   29,    3,           51,   12,    7,    3,    // 95
      30,    4,   41,    4,            3,    3,    4,    3,    // 103
      29,    3,    6,    3,            0,   23,    3,    2,    // 111
       1,    4,   31,    3,            6,    3,    0,    5,    // 119
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      14,   40,   50,  108,        -999
    };
    char fixuptypes[] = {
      1,   1,   1,   2,     '\0'
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

TEST_F(Bytecode0, DoUnbracedIf) {    

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

    // WriteOutput("DoUnbracedIf", scrip);
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


TEST_F(Bytecode0, For1) {  

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

    // WriteOutput("For1", scrip);
    const size_t codesize = 119;
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
       0,   31,    3,    6,            3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   12,   32,   65,         98,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
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

TEST_F(Bytecode0, For2) {    

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

    // WriteOutput("For2", scrip);
    const size_t codesize = 312;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    4,    1,    1,    // 7
       4,   51,    0,   63,            4,    1,    1,    4,    // 15
       6,    3,    1,   28,           35,   51,    8,    7,    // 23
       3,   29,    3,   51,            8,    7,    3,   30,    // 31
       4,   11,    3,    4,            8,    3,    6,    3,    // 39
       1,   29,    3,   51,           12,    7,    3,   30,    // 47
       4,   11,    3,    4,            8,    3,   31,  -40,    // 55
       6,    3,    1,   28,           19,   51,    8,    7,    // 63
       3,   29,    3,   51,            8,    7,    3,   30,    // 71
       4,   12,    3,    4,            8,    3,   31,  -24,    // 79
      51,    8,    7,    3,           29,    3,    6,    3,    // 87
       2,   30,    4,   18,            4,    3,    3,    4,    // 95
       3,   28,   35,   51,            8,    7,    3,   29,    // 103
       3,   51,    8,    7,            3,   30,    4,    9,    // 111
       3,    4,    8,    3,            6,    3,    3,   29,    // 119
       3,   51,   12,    7,            3,   30,    4,   11,    // 127
       3,    4,    8,    3,           31,  -54,   51,    8,    // 135
       7,    3,   29,    3,            6,    3,    4,   30,    // 143
       4,   18,    4,    3,            3,    4,    3,   28,    // 151
      19,   51,    8,    7,            3,   29,    3,   51,    // 159
       8,    7,    3,   30,            4,   10,    3,    4,    // 167
       8,    3,   31,  -38,            6,    3,    5,   51,    // 175
       8,    8,    3,    6,            3,    1,   28,   35,    // 183
      51,    8,    7,    3,           29,    3,   51,    8,    // 191
       7,    3,   30,    4,           10,    3,    4,    8,    // 199
       3,    6,    3,    6,           29,    3,   51,   12,    // 207
       7,    3,   30,    4,           11,    3,    4,    8,    // 215
       3,   31,  -40,    6,            3,    7,   29,    3,    // 223
       6,    3,    1,   28,           19,   51,    4,    7,    // 231
       3,   29,    3,   51,           12,    7,    3,   30,    // 239
       4,   13,    3,    4,            8,    3,   31,  -24,    // 247
       2,    1,    4,    6,            3,    8,   29,    3,    // 255
      51,    4,    7,    3,           29,    3,    6,    3,    // 263
       9,   30,    4,   18,            4,    3,    3,    4,    // 271
       3,   28,   19,   51,            4,    7,    3,   29,    // 279
       3,   51,   12,    7,            3,   30,    4,   14,    // 287
       3,    4,    8,    3,           31,  -38,    2,    1,    // 295
       4,    6,    3,    0,            2,    1,    8,   31,    // 303
       6,    2,    1,    8,            6,    3,    0,    5,    // 311
     -999
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

TEST_F(Bytecode0, For3) {

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

    // WriteOutput("For3", scrip);
    const size_t codesize = 57;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
       6,    3,    1,   28,           29,   51,    4,   48,    // 15
       3,   29,    3,    6,            2,    0,   48,    3,    // 23
      30,    4,   15,    4,            3,    3,    4,    3,    // 31
      51,    4,   49,    2,            1,    4,   31,   16,    // 39
      31,  -34,   51,    4,           49,    2,    1,    4,    // 47
       6,    3,   -7,   31,            3,    6,    3,    0,    // 55
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      21,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
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

TEST_F(Bytecode0, For4) {    

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

    // WriteOutput("For4", scrip);
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

TEST_F(Bytecode0, For5) {    

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

    // WriteOutput("For5", scrip);
    const size_t codesize = 140;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            1,   31,    3,    6,    // 7
       3,    0,    5,   38,           11,    6,    3,   10,    // 15
      31,    3,    6,    3,            0,    5,   38,   22,    // 23
      51,    8,    7,    3,           29,    3,    6,    3,    // 31
       1,   30,    4,   11,            4,    3,    3,    4,    // 39
       3,   31,    3,    6,            3,    0,    5,   38,    // 47
      47,    6,    3,    0,           23,    3,   29,    3,    // 55
      51,    4,    7,    3,           29,    3,    6,    3,    // 63
      11,   23,    3,   30,            4,   18,    4,    3,    // 71
       3,    4,    3,   28,           59,   51,    4,    7,    // 79
       3,   29,    3,    6,            3,    0,   30,    4,    // 87
      19,    4,    3,    3,            4,    3,   28,   20,    // 95
      51,    4,    7,    3,           29,    3,    6,    3,    // 103
      22,   23,    3,    2,            1,    4,   51,    4,    // 111
       8,    3,   31,  -60,           51,    4,    7,    3,    // 119
      29,    3,    6,    3,           22,   23,    3,    2,    // 127
       1,    4,   51,    4,            8,    3,   31,  -80,    // 135
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      51,   64,  104,  124,        -999
    };
    char fixuptypes[] = {
      2,   2,   2,   2,     '\0'
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

TEST_F(Bytecode0, For6) {  

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

    // WriteOutput("For6", scrip);
    const size_t codesize = 140;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           93,   23,    3,   29,    // 7
       3,   51,    4,    7,            3,   29,    3,    6,    // 15
       3,  104,   23,    3,           30,    4,   18,    4,    // 23
       3,    3,    4,    3,           28,   59,   51,    4,    // 31
       7,    3,   29,    3,            6,    3,    0,   30,    // 39
       4,   19,    4,    3,            3,    4,    3,   28,    // 47
      20,   51,    4,    7,            3,   29,    3,    6,    // 55
       3,  115,   23,    3,            2,    1,    4,   51,    // 63
       4,    8,    3,   31,          -60,   51,    4,    7,    // 71
       3,   29,    3,    6,            3,  115,   23,    3,    // 79
       2,    1,    4,   51,            4,    8,    3,   31,    // 87
     -80,    2,    1,    4,            5,   38,   93,    6,    // 95
       3,    1,   31,    3,            6,    3,    0,    5,    // 103
      38,  104,    6,    3,           10,   31,    3,    6,    // 111
       3,    0,    5,   38,          115,   51,    8,    7,    // 119
       3,   29,    3,    6,            3,    1,   30,    4,    // 127
      11,    4,    3,    3,            4,    3,   31,    3,    // 135
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   17,   57,   77,        -999
    };
    char fixuptypes[] = {
      2,   2,   2,   2,     '\0'
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

TEST_F(Bytecode0, For7) {

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

    // WriteOutput("For7", scrip);
    const size_t codesize = 123;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           65,   23,    3,    6,    // 7
       3,   84,   23,    3,           28,   50,    6,    2,    // 15
       0,    7,    3,   29,            3,    6,    3,    5,    // 23
      30,    4,   19,    4,            3,    3,    4,    3,    // 31
      28,   23,    6,    3,          100,   29,    3,    6,    // 39
       2,    0,    7,    3,           30,    4,   12,    4,    // 47
       3,    3,    4,    3,            6,    2,    0,    8,    // 55
       3,    6,    3,  110,           23,    3,   31,  -57,    // 63
       5,   38,   65,    6,            3,    1,    6,    2,    // 71
       0,    8,    3,    6,            3,  -77,   31,    3,    // 79
       6,    3,    0,    5,           38,   84,    6,    2,    // 87
       0,    7,    3,   29,            3,    6,    3,   10,    // 95
      30,    4,   18,    4,            3,    3,    4,    3,    // 103
      31,    3,    6,    3,            0,    5,   38,  110,    // 111
       6,    2,    0,    7,            3,    1,    3,    1,    // 119
       8,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 9;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,    9,   16,   41,         54,   59,   72,   88,    // 7
     114,  -999
    };
    char fixuptypes[] = {
      2,   2,   1,   1,      1,   2,   1,   1,    // 7
      1,  '\0'
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

TEST_F(Bytecode0, Continue1) {   

    // Locals only become invalid at the end of their nesting; below a "continue", they
    // remain valid so the offset to start of the local block
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

    // WriteOutput("Continue1", scrip);
    const size_t codesize = 114;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    4,    1,    1,    // 7
       4,    6,    3,   -1,           51,    4,    8,    3,    // 15
      51,    4,    7,    3,           29,    3,    6,    3,    // 23
       1,   30,    4,   18,            4,    3,    3,    4,    // 31
       3,   28,   63,    6,            3,    7,   29,    3,    // 39
       6,    3,   77,   29,            3,   51,   12,    7,    // 47
       3,   29,    3,    6,            3,    0,   30,    4,    // 55
      19,    4,    3,    3,            4,    3,   28,   14,    // 63
       2,    1,    8,   51,            4,    7,    3,    1,    // 71
       3,    1,    8,    3,           31,  -62,   51,    8,    // 79
       7,    3,   29,    3,            2,    1,   12,   51,    // 87
       4,    7,    3,    1,            3,    1,    8,    3,    // 95
      31,  -82,   51,    4,            7,    3,    2,    1,    // 103
       4,   31,    6,    2,            1,    4,    6,    3,    // 111
       0,    5,  -999
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

TEST_F(Bytecode0, IfDoWhile) {   

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

    // WriteOutput("IfDoWhile", scrip);
    const size_t codesize = 179;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,   51,    // 7
       0,   63,    4,    1,            1,    4,    6,    3,    // 15
      -2,   29,    3,   51,           12,    7,    3,   29,    // 23
       3,    6,    3,   10,           30,    4,   18,    4,    // 31
       3,    3,    4,    3,           28,   91,    6,    3,    // 39
       0,   51,    4,    8,            3,   51,    4,    7,    // 47
       3,   29,    3,    6,            3,   10,   30,    4,    // 55
      18,    4,    3,    3,            4,    3,   28,   63,    // 63
      51,    4,    7,    3,           29,    3,   51,   12,    // 71
       7,    3,   30,    4,           11,    3,    4,    8,    // 79
       3,   51,    4,    7,            3,   29,    3,    6,    // 87
       3,    6,   30,    4,           15,    4,    3,    3,    // 95
       4,    3,   28,    9,           51,    4,    7,    3,    // 103
       2,    1,   12,   31,           69,    6,    3,    3,    // 111
      29,    3,   51,    8,            7,    3,   30,    4,    // 119
      11,    3,    4,    8,            3,   31,  -82,   31,    // 127
      35,    6,    3,    1,           29,    3,   51,    8,    // 135
       7,    3,   30,    4,           11,    3,    4,    8,    // 143
       3,   51,    4,    7,            3,   29,    3,    6,    // 151
       3,  100,   30,    4,           18,    4,    3,    3,    // 159
       4,    3,   70,  -35,            6,    3,    0,    2,    // 167
       1,   12,   31,    6,            2,    1,   12,    6,    // 175
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

TEST_F(Bytecode0, Switch01) {    

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

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Switch01", scrip);
    const size_t codesize = 165;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   12,    7,    3,           30,    4,    9,    4,    // 15
       3,    3,    4,    3,            3,    3,    4,   31,    // 23
      81,    6,    3,   10,           31,  134,   31,  124,    // 31
       6,    3,    2,   29,            3,   51,   12,    7,    // 39
       3,   30,    4,    9,            3,    4,    8,    3,    // 47
      51,    8,    7,    3,           31,  110,    6,    3,    // 55
       0,   51,    8,    8,            3,    6,    3,    5,    // 63
      29,    3,   51,   12,            7,    3,   30,    4,    // 71
      12,    4,    3,    3,            4,    3,   29,    3,    // 79
       6,    3,    4,   30,            4,   12,    4,    3,    // 87
       3,    4,    3,   29,            3,   51,   12,    7,    // 95
       3,   30,    4,   11,            3,    4,    8,    3,    // 103
      31,   50,   29,    4,            6,    3,    2,   30,    // 111
       4,   15,    3,    4,           70,  -93,   29,    4,    // 119
       6,    3,    3,   30,            4,   15,    3,    4,    // 127
      70,  -76,   29,    4,            6,    3,    4,   30,    // 135
       4,   15,    3,    4,           70,  -88,   29,    4,    // 143
       6,    3,    5,   30,            4,   15,    3,    4,    // 151
      70,  -93,   31, -124,            6,    3,    0,   31,    // 159
       3,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };

    int idx2 = -1;
    for (size_t idx = 0; static_cast<int>(idx) < scrip.numimports; idx++)
    {
        if (!strcmp(scrip.imports[idx], ""))
            continue;
        idx2++;
        ASSERT_LT(idx2, numimports);
        std::string prefix = "imports[";
        prefix += std::to_string(idx2) + "] == ";
        std::string is_val = prefix + scrip.imports[idx];
        std::string test_val = prefix + imports[idx2];
        ASSERT_EQ(is_val, test_val);
    }

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
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

TEST_F(Bytecode0, StringOldstyle01) {
    ccSetOption(SCOPT_OLDSTRINGS, true);    

    char *inpl = "\
        int Sentinel1;              \n\
        string GLOBAL;              \n\
        int Sentinel2;              \n\
                                    \n\
        string MyFunction(int a)    \n\
        {                           \n\
            string x;               \n\
            char   Sentinel3;       \n\
            return GLOBAL;          \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle01", scrip);
    const size_t codesize = 34;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
          38,    0,   51,    0,           63,  200,    1,    1,    // 7
         200,   51,    0,   63,            1,    1,    1,    1,    // 15
           6,    2,    4,    3,            2,    3,    2,    1,    // 23
         201,   31,    6,    2,            1,  201,    6,    3,    // 31
           0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
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

TEST_F(Bytecode0, StringOldstyle02) {

    char *inpl = "\
        int sub(const string s) \n\
        {                       \n\
            return;             \n\
        }                       \n\
                                \n\
        int main()              \n\
        {                       \n\
            sub(\"Foo\");       \n\
        }                       \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle02", scrip);
    const size_t codesize = 30;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   31,    3,    6,    // 7
       3,    0,    5,   38,           11,    6,    3,    0,    // 15
      29,    3,    6,    3,            0,   23,    3,    2,    // 23
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,   20,  -999
    };
    char fixuptypes[] = {
      3,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 4;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'F',  'o',  'o',    0,          '\0'
    };

    for (size_t idx = 0; idx < stringssize; idx++)
    {
        if (static_cast<int>(idx) >= scrip.stringssize) break;
        std::string prefix = "strings[";
        prefix += std::to_string(idx) + "] == ";
        std::string is_val = prefix + std::to_string(strings[idx]);
        std::string test_val = prefix + std::to_string(scrip.strings[idx]);
        ASSERT_EQ(is_val, test_val);
    }
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
    const size_t codesize = 140;
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
       4,    2,    1,    4,           31,    9,   51,    4,    // 71
      49,    2,    1,    4,            6,    3,    0,    5,    // 79
      38,   80,   51,    0,           63,    4,    1,    1,    // 87
       4,   51,    4,   29,            2,    6,    3,   -1,    // 95
      29,    3,   51,    8,            7,    2,   45,    2,    // 103
       6,    3,    0,   23,            3,    2,    1,    4,    // 111
      30,    2,   51,    0,           47,    3,    1,    1,    // 119
       4,   51,    4,   48,            2,   52,    1,    2,    // 127
      12,    7,    3,   29,            3,   51,    8,   49,    // 135
       2,    1,   12,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
     106,  -999
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
    const size_t codesize = 85;
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
       8,    3,   31,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   17,   29,   34,         59,   64,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,  '\0'
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
    const size_t codesize = 85;
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
       8,    3,   31,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   17,   29,   34,         59,   64,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,  '\0'
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
    const size_t codesize = 104;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,   16,    1,    1,    // 7
      16,   73,    3,    4,           51,   16,   47,    3,    // 15
       6,    3, 12345,   29,            3,   51,   20,   48,    // 23
       2,   52,   30,    3,            8,    3,   51,    0,    // 31
      63,   48,    1,    1,           48,   73,    3,    4,    // 39
      29,    3,   51,   20,           30,    3,   47,    3,    // 47
      51,   64,   49,    1,            2,    4,   49,    1,    // 55
       2,    4,   49,    1,            2,    4,   49,   51,    // 63
      48,    6,    3,    3,           29,    2,   29,    3,    // 71
      49,    1,    2,    4,           49,    1,    2,    4,    // 79
      49,    1,    2,    4,           49,   30,    3,   30,    // 87
       2,    1,    2,   16,            2,    3,    1,   70,    // 95
     -29,    2,    1,   64,            6,    3,    0,    5,    // 103
     -999
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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct05", scrip);
    const size_t codesize = 40;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    0,    1,    1,    // 7
       0,    6,    3,    7,           34,    3,   39,    1,    // 15
       6,    3,    0,   33,            3,   35,    1,   34,    // 23
       3,   39,    1,    6,            3,    0,   33,    3,    // 31
      35,    1,   31,    3,            6,    3,    0,    5,    // 39
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,   29,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "StructO::StInt^1",            "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
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
    const size_t codesize = 50;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    4,    1,    1,    // 7
       4,    6,    3,    5,           72,    3,    4,    1,    // 15
      51,    4,   47,    3,            6,    3,   77,   29,    // 23
       3,   51,    8,   48,            2,   52,    1,    2,    // 31
      12,   48,    2,   52,           30,    3,    8,    3,    // 39
      51,    4,   49,    2,            1,    4,    6,    3,    // 47
       0,    5,  -999
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
    const size_t codesize = 72;
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
       3,    0,   31,    3,            6,    3,    0,    5,    // 71
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,   21,   28,   48,         55,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,  '\0'
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
    const size_t codesize = 84;
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
      22,    4,    3,    3,            4,    3,   31,    3,    // 79
       6,    3,    0,    5,          -999
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

    // WriteOutput("Func01", scrip);
    const size_t codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
      51,    0,   49,    1,            1,    4,   51,    4,    // 15
      48,    3,   34,    3,           51,    8,   48,    3,    // 23
      34,    3,   39,    2,            6,    3,    0,   33,    // 31
       3,   35,    2,   29,            3,   51,    4,    7,    // 39
       3,   51,   12,   49,           51,    8,   49,    2,    // 47
       1,   12,   31,   12,           51,   12,   49,   51,    // 55
       8,   49,    2,    1,           12,    6,    3,    0,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      30,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
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

    // WriteOutput("Func02", scrip);
    const size_t codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
      51,    0,   49,    1,            1,    4,   51,    4,    // 15
      48,    3,   34,    3,           51,    8,   48,    3,    // 23
      34,    3,   39,    2,            6,    3,    0,   33,    // 31
       3,   35,    2,   29,            3,   51,    4,    7,    // 39
       3,   51,   12,   49,           51,    8,   49,    2,    // 47
       1,   12,   31,   12,           51,   12,   49,   51,    // 55
       8,   49,    2,    1,           12,    6,    3,    0,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      30,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
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

    // WriteOutput("Func03", scrip);
    const size_t codesize = 99;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   50,    3,    // 7
      51,   12,    7,    3,           50,    3,    6,    3,    // 15
       0,   51,    8,   49,           51,   12,   49,   31,    // 23
       9,   51,    8,   49,           51,   12,   49,    6,    // 31
       3,    0,    5,   38,           35,   51,    0,   49,    // 39
       1,    1,    4,   51,            0,   49,    1,    1,    // 47
       4,   51,    4,   48,            3,   29,    3,   51,    // 55
      12,   48,    3,   29,            3,    6,    3,    0,    // 63
      23,    3,    2,    1,            8,   29,    3,   51,    // 71
       4,    7,    3,   51,           12,   49,   51,    8,    // 79
      49,    2,    1,   12,           31,   12,   51,   12,    // 87
      49,   51,    8,   49,            2,    1,   12,    6,    // 95
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      63,  -999
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

TEST_F(Bytecode0, Func04) {
    
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

    // WriteOutput("Func04", scrip);
    const size_t codesize = 54;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,    6,    // 7
       3,   43,   23,    3,            2,    1,    4,   51,    // 15
       0,   47,    3,    1,            1,    4,    6,    3,    // 23
      -1,   51,    4,   49,            2,    1,    4,   31,    // 31
       9,   51,    4,   49,            2,    1,    4,    6,    // 39
       3,    0,    5,   38,           43,   73,    3,    4,    // 47
      31,    3,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,  -999
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

TEST_F(Bytecode0, Func05) {
   
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

    // WriteOutput("Func05", scrip);
    const size_t codesize = 52;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   11,    4,    // 15
       3,    3,    4,    3,           31,    3,    6,    3,    // 23
       0,    5,   38,   26,            6,    3,    5,   29,    // 31
       3,    6,    3,    4,           29,    3,    6,    3,    // 39
       0,   23,    3,    2,            1,    8,   29,    3,    // 47
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      40,  -999
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

TEST_F(Bytecode0, Func06) {  

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

    // WriteOutput("Func06", scrip);
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

TEST_F(Bytecode0, Func07) {   

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

    // WriteOutput("Func09", scrip);
    const size_t codesize = 52;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   11,    4,    // 15
       3,    3,    4,    3,           31,    3,    6,    3,    // 23
       0,    5,   38,   26,            6,    3,  -99,   29,    // 31
       3,    6,    3,    4,           29,    3,    6,    3,    // 39
       0,   23,    3,    2,            1,    8,   29,    3,    // 47
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      40,  -999
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

TEST_F(Bytecode0, Func10) {   

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

    // WriteOutput("Func10", scrip);
    const size_t codesize = 60;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   31,    3,    6,    // 7
       3,    0,    5,   38,           11,   51,    0,   63,    // 15
       4,    1,    1,    4,           51,    4,   45,    2,    // 23
       6,    3,    0,   23,            3,   29,    3,    6,    // 31
       3,    3,   30,    4,           40,    4,    3,    3,    // 39
       4,    3,   29,    3,           51,    4,    7,    3,    // 47
       2,    1,    8,   31,            6,    2,    1,    8,    // 55
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      26,  -999
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
    size_t const codesize = 51;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    8,    1,    1,    // 7
       8,    6,    3,    3,           51,    4,    8,    3,    // 15
       6,    3, 1066192077,   29,            3,    6,    3, 1074580685,    // 23
      30,    4,   56,    4,            3,    3,    4,    3,    // 31
      51,    8,    8,    3,            6,    3,   -2,    2,    // 39
       1,    8,   31,    6,            2,    1,    8,    6,    // 47
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
    const size_t codesize = 147;
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
       6,    3,    0,   29,            3,    6,    3, 1079194419,    // 87
      30,    4,   55,    4,            3,    3,    4,    3,    // 95
      30,    4,   58,    4,            3,    3,    4,    3,    // 103
      29,    3,   51,  184,           48,    2,   52,   30,    // 111
       3,    8,    3,    6,            3,    0,   29,    3,    // 119
      51,  188,   30,    3,           47,    3,   51,  200,    // 127
       6,    3,   50,   49,            1,    2,    4,    2,    // 135
       3,    1,   70,   -9,            2,    1,  200,    6,    // 143
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

TEST_F(Bytecode0, ArrayInStruct1) {

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

    // WriteOutput("ArrayInStruct1", scrip);
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

TEST_F(Bytecode0, ArrayInStruct2) {
    
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

    // WriteOutput("ArrayInStruct2", scrip);
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

TEST_F(Bytecode0, Func11) { 

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
  
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func11", scrip);
    const size_t codesize = 51;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   12,    7,    3,           29,    3,   51,   16,    // 15
       7,    3,   30,    4,           10,    4,    3,    3,    // 23
       4,    3,   30,    4,           11,    4,    3,    3,    // 31
       4,    3,   31,    3,            6,    3,    0,    5,    // 39
      38,   40,    6,    3,            0,   31,    3,    6,    // 47
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

TEST_F(Bytecode0, Func12) {

    // Function with float default, or default "0", for float parameter

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

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func12", scrip);
    const size_t codesize = 68;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   31,    3,    // 7
       6,    3,    0,    5,           38,   12,   51,    8,    // 15
       7,    3,   31,    3,            6,    3,    0,    5,    // 23
      38,   24,    6,    3,         1088841318,   29,    3,    6,    // 31
       3,    0,   23,    3,            2,    1,    4,   29,    // 39
       3,    6,    3,    0,           29,    3,    6,    3,    // 47
      12,   23,    3,    2,            1,    4,   30,    4,    // 55
      57,    4,    3,    3,            4,    3,   31,    3,    // 63
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      33,   48,  -999
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

TEST_F(Bytecode0, Func13) {

    // Function with default null or 0 for managed parameter

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
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Func13", scrip);
    const size_t codesize = 118;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   50,    3,    // 7
      51,    8,   48,    3,           29,    3,   51,    4,    // 15
      50,    3,   51,   12,           49,   51,    4,   48,    // 23
       3,   69,   30,    4,           31,    6,   51,    8,    // 31
      49,    6,    3,    0,            5,   38,   37,   51,    // 39
       8,    7,    3,   50,            3,   51,    8,   48,    // 47
       3,   29,    3,   51,            4,   50,    3,   51,    // 55
      12,   49,   51,    4,           48,    3,   69,   30,    // 63
       4,   31,    6,   51,            8,   49,    6,    3,    // 71
       0,    5,   38,   74,            6,    3,    0,   29,    // 79
       3,    6,    3,    0,           23,    3,    2,    1,    // 87
       4,   29,    3,    6,            3,    0,   29,    3,    // 95
       6,    3,   37,   23,            3,    2,    1,    4,    // 103
      30,    4,   15,    4,            3,    3,    4,    3,    // 111
      31,    3,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      83,   98,  -999
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

TEST_F(Bytecode0, FuncStart1) {

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

    // WriteOutput("FuncStart1", scrip);
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
    const size_t codesize = 37;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,    3,    6,    // 7
       2,   52,    1,    2,            2,    8,    3,    6,    // 15
       3,    0,   31,    3,            6,    3,    0,    5,    // 23
      38,   24,    6,    2,            2,    7,    3,   31,    // 31
       3,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      28,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
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
    const size_t codesize = 21;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,    3,    6,    // 7
       2,   52,    8,    3,            6,    3,    0,   31,    // 15
       3,    6,    3,    0,            5,  -999
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

TEST_F(Bytecode0, Static1) {   

    char *inpl = "\
        struct Weapon {                         \n\
            import static int CalcDamage(       \n\
            int Lifepoints, int Hitpoints = 5);   \n\
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
    const size_t codesize = 120;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   12,    4,    // 15
       3,    3,    4,    3,           31,    3,    6,    3,    // 23
       0,    5,   38,   26,            6,    3,    5,   29,    // 31
       3,    6,    3,    9,           29,    3,    6,    3,    // 39
       0,   23,    3,    2,            1,    8,   29,    3,    // 47
       6,    3,   40,   29,            3,    6,    3,    9,    // 55
      29,    3,    6,    3,            0,   23,    3,    2,    // 63
       1,    8,   30,    4,           11,    4,    3,    3,    // 71
       4,    3,   29,    3,           51,    4,    7,    3,    // 79
      29,    3,    6,    3,            5,   29,    3,    6,    // 87
       3,  100,   29,    3,            6,    3,    0,   23,    // 95
       3,    2,    1,    8,           30,    4,   11,    4,    // 103
       3,    3,    4,    3,            2,    1,    4,   31,    // 111
       6,    2,    1,    4,            6,    3,    0,    5,    // 119
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      40,   60,   94,  -999
    };
    char fixuptypes[] = {
      2,   2,   2,  '\0'
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

    const size_t codesize = 52;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
      51,   16,    7,    3,           30,    4,   12,    4,    // 15
       3,    3,    4,    3,           31,    3,    6,    3,    // 23
       0,    5,   38,   26,            6,    3,   40,   29,    // 31
       3,    6,    3,    9,           29,    3,    6,    3,    // 39
       0,   23,    3,    2,            1,    8,   31,    3,    // 47
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      40,  -999
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
        int  Weapon::SetDamage(int damage)     \n\
        {                                      \n\
            Damage = damage;                   \n\
            return 0;                          \n\
        }                                      \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Protected2", scrip);
    const size_t codesize = 25;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
       3,    6,    2,   52,           30,    3,    8,    3,    // 15
       6,    3,    0,   31,            3,    6,    3,    0,    // 23
       5,  -999
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

TEST_F(Bytecode0, Switch02) {
    
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

    // WriteOutput("Switch02", scrip);
    const size_t codesize = 50;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   29,    3,   51,    // 7
       4,    7,    3,    3,            3,    4,   31,   11,    // 15
      31,   23,    6,    3,            0,   51,    4,    8,    // 23
       3,   31,   14,   29,            4,    6,    3,    5,    // 31
      30,    4,   15,    3,            4,   70,  -21,   31,    // 39
     -25,    2,    1,    4,           31,    3,    2,    1,    // 47
       4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };

    int idx2 = -1;
    for (size_t idx = 0; static_cast<int>(idx) < scrip.numimports; idx++)
    {
        if (!strcmp(scrip.imports[idx], ""))
            continue;
        idx2++;
        ASSERT_LT(idx2, numimports);
        std::string prefix = "imports[";
        prefix += std::to_string(idx2) + "] == ";
        std::string is_val = prefix + scrip.imports[idx];
        std::string test_val = prefix + imports[idx2];
        ASSERT_EQ(is_val, test_val);
    }

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Attributes01) {    

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            readonly import attribute bool Flipped;     \n\
            import attribute int Graphic;               \n\
            readonly import attribute float AsFloat;    \n\
        };                                              \n\
                                                        \n\
        int main()                                      \n\
        {                                               \n\
            ViewFrame *VF;                              \n\
            if (VF.Flipped)                             \n\
            {                                           \n\
                VF.Graphic = 17;                        \n\
                float f = VF.AsFloat + VF.AsFloat;      \n\
                return VF.Graphic;                      \n\
            }                                           \n\
            return VF.Flipped;                          \n\
        }                                               \n\
        ";

    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes01", scrip);
    const size_t codesize = 166;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
      51,    4,   48,    2,           52,   29,    6,   45,    // 15
       2,   39,    0,    6,            3,    0,   33,    3,    // 23
      30,    6,   28,  102,            6,    3,   17,   51,    // 31
       4,   48,    2,   52,           29,    6,   34,    3,    // 39
      45,    2,   39,    1,            6,    3,    2,   33,    // 47
       3,   35,    1,   30,            6,   51,    4,   48,    // 55
       2,   52,   29,    6,           45,    2,   39,    0,    // 63
       6,    3,    3,   33,            3,   30,    6,   29,    // 71
       3,   51,    8,   48,            2,   52,   29,    6,    // 79
      45,    2,   39,    0,            6,    3,    3,   33,    // 87
       3,   30,    6,   30,            4,   57,    4,    3,    // 95
       3,    4,    3,   29,            3,   51,    8,   48,    // 103
       2,   52,   29,    6,           45,    2,   39,    0,    // 111
       6,    3,    1,   33,            3,   30,    6,   51,    // 119
       8,   49,    2,    1,            8,   31,   38,    2,    // 127
       1,    4,   51,    4,           48,    2,   52,   29,    // 135
       6,   45,    2,   39,            0,    6,    3,    0,    // 143
      33,    3,   30,    6,           51,    4,   49,    2,    // 151
       1,    4,   31,    9,           51,    4,   49,    2,    // 159
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      21,   46,   66,   86,        114,  143,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 4;
    std::string imports[] = {
    "ViewFrame::get_Flipped^0",   "ViewFrame::get_Graphic^0",   "ViewFrame::set_Graphic^1",   // 2
    "ViewFrame::get_AsFloat^0",    "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, Attributes02) {

    // The getter and setter functions are defined locally, so
    // they ought to be exported instead of imported.
    // Assigning to the attribute should generate the same call
    // as calling the setter; reading the same as calling the getter.
    // Armor:: functions should be allowed to access _Damage.

    char *inpl = "\
        managed struct Armor {                          \n\
            attribute int Damage;                       \n\
            writeprotected short _Aura;                 \n\
            protected int _Damage;                      \n\
        };                                              \n\
                                                        \n\
        int main()                                      \n\
        {                                               \n\
            Armor *armor = new Armor;                   \n\
            armor.Damage = 17;                          \n\
            return (armor.Damage > 10);                 \n\
        }                                               \n\
                                                        \n\
        void Armor::set_Damage(int damage)              \n\
        {                                               \n\
            if (damage >= 0)                            \n\
                _Damage = damage;                       \n\
        }                                               \n\
                                                        \n\
        int Armor::get_Damage()                         \n\
        {                                               \n\
            return _Damage;                             \n\
        }                                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes02", scrip);
    const size_t codesize = 139;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            8,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3,   17,   51,    // 15
       4,   48,    2,   52,           29,    6,   29,    3,    // 23
      45,    2,    6,    3,           83,   23,    3,    2,    // 31
       1,    4,   30,    6,           51,    4,   48,    2,    // 39
      52,   29,    6,   45,            2,    6,    3,  122,    // 47
      23,    3,   30,    6,           29,    3,    6,    3,    // 55
      10,   30,    4,   17,            4,    3,    3,    4,    // 63
       3,   51,    4,   49,            2,    1,    4,   31,    // 71
       9,   51,    4,   49,            2,    1,    4,    6,    // 79
       3,    0,    5,   38,           83,   51,    8,    7,    // 87
       3,   29,    3,    6,            3,    0,   30,    4,    // 95
      19,    4,    3,    3,            4,    3,   28,   17,    // 103
      51,    8,    7,    3,           29,    3,    3,    6,    // 111
       2,   52,    1,    2,            2,   30,    3,    8,    // 119
       3,    5,   38,  122,            3,    6,    2,   52,    // 127
       1,    2,    2,    7,            3,   31,    3,    6,    // 135
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      28,   47,  -999
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

TEST_F(Bytecode0, Attributes03) {

    // The getters and setters are NOT defined locally, so
    // import decls should be generated for them.
    // The getters and setters should be called as import funcs.

    char *inpl = "\
        managed struct Armor {                          \n\
            attribute int Damage;                       \n\
            writeprotected short _aura;                 \n\
            protected int _damage;                      \n\
        };                                              \n\
                                                        \n\
        int main()                                      \n\
        {                                               \n\
            Armor *armor = new Armor;                   \n\
            armor.Damage = 17;                          \n\
            return (armor.Damage > 10);                 \n\
        }";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes03", scrip);
    const size_t codesize = 86;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            8,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3,   17,   51,    // 15
       4,   48,    2,   52,           29,    6,   34,    3,    // 23
      45,    2,   39,    1,            6,    3,    1,   33,    // 31
       3,   35,    1,   30,            6,   51,    4,   48,    // 39
       2,   52,   29,    6,           45,    2,   39,    0,    // 47
       6,    3,    0,   33,            3,   30,    6,   29,    // 55
       3,    6,    3,   10,           30,    4,   17,    4,    // 63
       3,    3,    4,    3,           51,    4,   49,    2,    // 71
       1,    4,   31,    9,           51,    4,   49,    2,    // 79
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      30,   50,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 2;
    std::string imports[] = {
    "Armor::get_Damage^0",        "Armor::set_Damage^1",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode0, StringStandard01) {    

    char inpl[] = "\
        int main()                         \n\
        {                                  \n\
            String s = \"Hello, world!\";  \n\
            if (s != \"Bye\")              \n\
                return 1;                  \n\
            return 0;                      \n\
        }                                  \n\
        ";
    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    
    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringStandard01", scrip);
    size_t const codesize = 65;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   64,    3,   51,    // 7
       0,   47,    3,    1,            1,    4,   51,    4,    // 15
      48,    3,   29,    3,            6,    3,   14,   30,    // 23
       4,   66,    4,    3,            3,    4,    3,   28,    // 31
      11,    6,    3,    1,           51,    4,   49,    2,    // 39
       1,    4,   31,   20,            6,    3,    0,   51,    // 47
       4,   49,    2,    1,            4,   31,    9,   51,    // 55
       4,   49,    2,    1,            4,    6,    3,    0,    // 63
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   22,  -999
    };

    char fixuptypes[] = {
      3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 18;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'e',  'l',  'l',          'o',  ',',  ' ',  'w',     // 7
    'o',  'r',  'l',  'd',          '!',    0,  'B',  'y',     // 15
    'e',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}
