#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "script/cc_common.h"

#include "script2/cc_symboltable.h"
#include "script2/cc_internallist.h"
#include "script2/cs_parser.h"

#include "cc_parser_test_lib.h"


// The vars defined here are provided in each test that is in category "Compile1"
class Compile1 : public ::testing::Test
{
protected:
    AGS::ccCompiledScript scrip = AGS::ccCompiledScript(); // Note: calls Init();

    Compile1()
    {
        // Initializations, will be done at the start of each test
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        ccSetOption(SCOPT_LINENUMBERS, true);
        clear_error();
    }
};


TEST_F(Compile1, Sections) {

    // Errors in globalscript must be reported for globalscript
    // "__NEWSCRIPTSTART..." begins a line #0,
    // so the error must be reported on line 3.

    char const *inpl = "\
        \"__NEWSCRIPTSTART_globalscript.ash\"   \n\
        int main()                              \n\
        {                                       \n\
            return 2 < 1 ? ;                    \n\
                    break;                      \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_EQ(3, currentline);
    ASSERT_STREQ("globalscript.ash", ccCurScriptName);
}

TEST_F(Compile1, Autoptr) {

    // String is autoptr so should not print as "String *"

    char const *inpl = "\
        managed autoptr builtin struct String   \n\
        {};                                     \n\
        int main()                              \n\
        {                                       \n\
            String var = 15;                    \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    ASSERT_EQ(std::string::npos, msg.find("String *"));
}

TEST_F(Compile1, BinaryNot)
{    

    // '!' can't be binary

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = 15 ! 2;                   \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("inary or postfix op"));
}

TEST_F(Compile1, UnaryDivideBy) { 

    // '/' can't be unary

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = (/ 2);                    \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("refix operator"));
}

TEST_F(Compile1, UnaryPlus) {

    // '/' can't be unary

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return +42;                         \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, FloatInt1) {  

    // Can't mix float and int

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = 4 / 2.0;                  \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'/'"));
}

TEST_F(Compile1, FloatInt2) {

    // Can't negate float

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = !2.0;                     \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("convert"));
}

TEST_F(Compile1, StringInt1) {    

    // Can't mix string and int

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = (\"Holzschuh\" == 2);     \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("compare"));
}

TEST_F(Compile1, ExpressionVoid) {  

    // Can't mix void

    char const *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var = 1 + Func() * 3;           \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("convert"));
}

TEST_F(Compile1, ExpressionLoneUnary1) { 

    // Unary -, nothing following

    char const *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var;                            \n\
            Var = -;                            \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("xpected a term"));
}

TEST_F(Compile1, ExpressionLoneUnary2) {

    // Unary ~, nothing following

    char const *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var;                            \n\
            Var = ~;                            \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("xpected a term"));
}

TEST_F(Compile1, ExpressionBinaryWithoutRHS) {

    // Binary %, nothing following

    char const *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var;                            \n\
            Var = 5 %;                          \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ollowing"));
}

TEST_F(Compile1, LocalTypes1)
{
    char const *inpl = "\
        void Test1()            \n\
        {                       \n\
            struct MyStruct     \n\
            {                   \n\
                int a;          \n\
            };                  \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("struct type"));
}

TEST_F(Compile1, LocalTypes2)
{
    char const *inpl = "\
        void Test1()            \n\
        {                       \n\
            enum Foo            \n\
            {                   \n\
                a,              \n\
            };                  \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("enum type"));
}

TEST_F(Compile1, StaticArrayIndex1) {

    // Constant array index, is out ouf bounds

    char const *inpl = "\
        enum E                          \n\
        {                               \n\
            MinusFive = -5,             \n\
        };                              \n\
        int main()                      \n\
        {                               \n\
            int Var[5];                 \n\
            Var[-MinusFive];            \n\
        }                               \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("oo high"));
}

TEST_F(Compile1, StaticArrayIndex2) {

    // Constant array index, is out ouf bounds

    char const *inpl = "\
        enum E                          \n\
        {                               \n\
            MinusFive = -5,             \n\
        };                              \n\
        int main()                      \n\
        {                               \n\
            int Var[5];                 \n\
            Var[MinusFive];             \n\
        }                               \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("oo low"));
}

TEST_F(Compile1, ExpressionArray1) {    

    // Can't mix void

    char const *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var[8];                         \n\
            Var++;                              \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("convert"));
}

TEST_F(Compile1, FuncTypeClash1) {

    // Can't use func here except in a func call

    char const *inpl = "\
        int Func()                              \n\
        {                                       \n\
        }                                       \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            int Var = Func;                     \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'('"));
}

TEST_F(Compile1, FloatOutOfBounds) {

    // Too small

    char const *inpl = "\
        int Func()                              \n\
        {                                       \n\
        }                                       \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            int Var = -77E7777;                 \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of bounds"));
}

TEST_F(Compile1, DoWhileSemicolon) { 

    // ';' missing

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int I = 1;                          \n\
            do {                                \n\
                I *= 2;                         \n\
            } while (I < 10)                    \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("';'"));
}

TEST_F(Compile1, ExtenderExtender1) {    

    // No extending a struct with a compound function

    char const *inpl = "\
        struct Struct1                      \n\
        {                                   \n\
            void Func();                    \n\
        };                                  \n\
        struct Struct2                      \n\
        {                                   \n\
        };                                  \n\
        void Struct1::Func(static Struct2)  \n\
        {                                   \n\
        }                                   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'::'"));
}

TEST_F(Compile1, ExtenderExtender2) {    

    // No extending a struct with a compound function

    char const *inpl = "\
        struct Struct1                      \n\
        {                                   \n\
        };                                  \n\
        struct Struct2                      \n\
        {                                   \n\
            void Struct1::Func(short i);    \n\
        };                                  \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'::'"));
}

TEST_F(Compile1, NonManagedStructParameter) {

    // Can't pass a non-managed struct as a function parameter

    char const *inpl = "\
        struct Struct                           \n\
        {                                       \n\
        };                                      \n\
        int Func(Struct S)                      \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("non-managed"));
}

TEST_F(Compile1, StrangeParameterName) {   

    // Can't use keyword as parameter name

    char const *inpl = "\
        void Func(int while)                    \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("while"));
}

TEST_F(Compile1, DoubleParameterName) { 

    // Can't use keyword as parameter name

    char const *inpl = "\
        void Func(int PI, float PI)             \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("PI"));
}

TEST_F(Compile1, FuncParamDefaults1) {

    // Either give no defaults or give them all

    char const *inpl = "\
        void Func(int i = 5, float j = 6.0);    \n\
        void Func(int i = 5, float j)           \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#2"));
}

TEST_F(Compile1, FuncParamDefaults2) {

    // All parameters that follow a default parameter must have a default

    char const *inpl = "\
        import void Func(int i = 5, float j);   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#2"));
}

TEST_F(Compile1, FuncParamDefaults3) {

    // Can't give a parameter a default here, not a default there

    char const *inpl = "\
        void Func(int i, float j);          \n\
        void Func(int i, float j = 6.0)     \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#2"));
}

TEST_F(Compile1, FuncParamDefaults4) {   

    // Can't give a parameter differing defaults

    char const *inpl = "\
        void Func(float J = -6.0);              \n\
        void Func(float J = 6.0)                \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#1"));
}

TEST_F(Compile1, FuncParamNumber1) {  

    // Differing number of parameters

    char const *inpl = "\
        void Func(int, float);                  \n\
        void Func(int I, float J, short K)      \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("parameters"));
}

TEST_F(Compile1, FuncParamNumber2) {  

    // Instantiation has number of parameters than is different from declaration

    char const *inpl = "\
        struct Test                                         \n\
        {                                                   \n\
            import void Func(int a, int b, int c, int d);   \n\
        };                                                  \n\
                                                            \n\
        void Test::Func(int a)                              \n\
        {                                                   \n\
        }                                                   \n\
        ";

    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, FuncVariadicCollision) {    

    // Variadic / non-variadic

    char const *inpl = "\
        void Func(int, float, short, ...);      \n\
        void Func(int I, float J, short K)      \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("additional"));
}

TEST_F(Compile1, FuncReturnVartypes) {   

    // Return vartypes

    char const *inpl = "\
        int Func(int, float, short);            \n\
        short Func(int I, float J, short K)     \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("eturn"));
}

TEST_F(Compile1, FuncReturnStruct1) {  

    // Return vartype must be managed when it is a struct

    char const *inpl = "\
        struct Struct {  };                     \n\
        Struct Func()                           \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("managed"));
}

TEST_F(Compile1, FuncReturnStruct2) { 

    // Compiler will imply the '*'
    // but should be slightly unhappy about the missing return statement

    char const *inpl = "\
        managed struct Struct {  }; \n\
        Struct Func()               \n\
        {                           \n\
        }                           \n\
        ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("return"));
}

TEST_F(Compile1, FuncReturnStruct3) {

    // Compiler should be slightly unhappy about the missing return statement

    char const *inpl = "\
        managed struct Struct {  };             \n\
        Struct[] Func()                         \n\
        {                                       \n\
        }                                       \n\
        ";
    
    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("return"));
}

TEST_F(Compile1, FuncReturn1) {

    // Should detect that the 'I' define can't be reached
    // Should not warn about a missing return at end of function body.

    char const *inpl = "\
        import int Random(int); \n\
        float Func()            \n\
        {                       \n\
            if (Random(2))      \n\
                return 1.0;     \n\
            else                \n\
                return 2.0;     \n\
            int I = 0;          \n\
        }                       \n\
        ";
    
    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("Code execution"));
}

TEST_F(Compile1, FuncReturn2) {

    // Should detect that the 'I' assignment can't be reached
    // Should warn about a missing return at end of function body.

    char const *inpl = "\
        import int Random(int); \n\
        float Func()            \n\
        {                       \n\
            int I = 9;          \n\
            if (Random(2))      \n\
            {                   \n\
                return 1.0;     \n\
                I = -9'999;     \n\
            }                   \n\
            else                \n\
                I = 0;          \n\
        }                       \n\
        ";
    
    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("Code execution"));
}

TEST_F(Compile1, FuncDouble) {

    // No two equally-named functions with body

    char const *inpl = "\
        int Func()                              \n\
        {                                       \n\
        }                                       \n\
        void Func()                             \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("with body"));
}

TEST_F(Compile1, FuncProtected) {

    // Protected functions must be part of a struct

    char const *inpl = "\
        protected void Func(int I = 6)          \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("protected"));
}

TEST_F(Compile1, FuncNameClash1) {

    // Function name mustn't equal a variable name.

    char const *inpl = "\
        int Func;                               \n\
        void Func(int I = 6)                    \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("declared as a function"));
}

TEST_F(Compile1, FuncDeclWrong1) {

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
    int Func(Struct2 *S1, Struct1 *S2)          \n\
    {                               \n\
        return 0;                   \n\
    }                               \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        return 0;                   \n\
    }                               \n\
   ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("parameter"));
}

TEST_F(Compile1, FuncDeclWrong2) {


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
        return 0;                   \n\
    }                               \n\
                                    \n\
    int Func(Struct2 *S1, Struct1 *S2)  \n\
    {                               \n\
        return 0;                   \n\
    }                               \n\
                                    \n\
    int Func(Struct1 *S1, Struct2 *S2);  \n\
   ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("parameter"));
}

TEST_F(Compile1, FuncDeclReturnVartype) {

    // Should compile.

    char const *inpl = "\
    managed struct DynamicSprite                                    \n\
    {                                                               \n\
    };                                                              \n\
                                                                    \n\
    struct RotatedView                                              \n\
    {                                                               \n\
        import static DynamicSprite *[]                             \n\
            CreateLoop(int view, int loop, int base_loop = 7);      \n\
    };                                                              \n\
                                                                    \n\
    static DynamicSprite *[]                                        \n\
        RotatedView::CreateLoop(int view, int loop, int base_loop)  \n\
    {                                                               \n\
        return null;                                                \n\
    }                                                               \n\
   ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile1, FuncHeader1) {

    // Can't have a specific array size in func parameters

    char const *inpl = "\
        void main(int a[15])                   \n\
        {                                      \n\
             return;                           \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("'15'"));
}

TEST_F(Compile1, FuncHeader2) {

    // Default for float parameter, an int value. Should fail

    char const *inpl = "\
        void Foo(float Param = 7);              \n\
        {                                      \n\
             return;                           \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("loat expression"));
}

TEST_F(Compile1, FuncHeader3) {

    // Integer default for managed parameter. Should fail
    char const *inpl = "\
        managed struct Payload                  \n\
        {                                       \n\
            float foo;                          \n\
        };                                      \n\
                                                \n\
        void Foo(Payload Param = 7);            \n\
        {                                       \n\
             return;                            \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("rameter default"));
}

TEST_F(Compile1, FuncExtenderHeaderFault1a) {

    char const *inpl = "\
        struct Weapon {                        \n\
            int Damage;                        \n\
        };                                     \n\
                                               \n\
        void Foo(this Holzschuh)               \n\
        {                                      \n\
            return;                            \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("Holzschuh"));
}

TEST_F(Compile1, FuncExtenderHeaderFault1b) {

    // A comma or paren should follow 'Weapon'

    char const *inpl = "\
        struct Weapon {                        \n\
            int Damage;                        \n\
        };                                     \n\
                                               \n\
        void Foo(static Weapon Of Mass Destruction) \n\
        {                                      \n\
            return;                            \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("'Of'"));
}

TEST_F(Compile1, FuncExtenderHeaderFault1c) {

    char const *inpl = "\
        struct Weapon {                        \n\
            int Damage;                        \n\
        };                                     \n\
                                               \n\
        void Foo(static Weapon *)              \n\
        {                                      \n\
            return;                            \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("tatic extender function"));
}

TEST_F(Compile1, FuncExtenderHeaderFault2) {

    char const *inpl = "\
        struct Weapon {                        \n\
            int Damage;                        \n\
        };                                     \n\
                                               \n\
        void Foo(this int)                     \n\
        {                                      \n\
            return;                            \n\
        }                                      \n\
        ";


    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("struct"));
}

TEST_F(Compile1, FuncDoubleExtender) {

    // Must not define a function with body twice.

    char const *inpl = "\
        struct Weapon {                         \n\
            int Damage;                         \n\
        };                                      \n\
                                                \n\
        int Foo(this Weapon *)                  \n\
        {                                       \n\
            return 1;                           \n\
        }                                       \n\
                                                \n\
        int Weapon::Foo(void)                   \n\
        {                                       \n\
            return 2;                           \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("defined"));
}

TEST_F(Compile1, FuncDoubleNonExtender) {

    char const *inpl = "\
        int Foo(int Bar)                       \n\
        {                                      \n\
            return 1;                          \n\
        }                                      \n\
        int Foo(int Bat)                       \n\
        {                                      \n\
            return 2;                          \n\
        }                                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("defined"));
}

TEST_F(Compile1, FuncUndeclaredStruct1) {

    // Should fail, Struct doesn't have Func

    char const *inpl = "\
        managed struct Struct                       \n\
        {                                           \n\
            int Component;                          \n\
        };                                          \n\
                                                    \n\
        void Struct::Func(int Param)                \n\
        {                                           \n\
        }                                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string message = last_seen_cc_error();
    EXPECT_NE(std::string::npos, message.find("Func"));
}

TEST_F(Compile1, FuncUndeclaredStruct2) {

    // Should succeed, Struct has Func

    char const *inpl = "\
        void Struct::Func(int Param)                \n\
        {                                           \n\
        }                                           \n\
                                                    \n\
        managed struct Struct                       \n\
        {                                           \n\
            void Func(int);                         \n\
        };                                          \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile1, FuncCall_MultidimensionalArray) {

    // Function parameter should be recognized as 1 parameter
    // even though it is an element of a 3-dimensional array.

    char const *inpl = "\
        int arr[10, 10, 10];        \n\
        import void Test(int x);    \n\
                                    \n\
        int game_start()            \n\
        {                           \n\
            Test(arr[1, 2, 3]);     \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile1, TypeEqComponent) {

    // A struct component may have the same name as a type.

    char const *inpl = "\
        builtin managed struct Room             \n\
        {                                       \n\
        };                                      \n\
                                                \n\
        builtin managed struct Character        \n\
        {                                       \n\
            readonly import attribute int Room; \n\
        };                                      \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, ExtenderFuncClash) {

    // Don't remember the struct of extender functions past their definition (and body, if applicable)

    char const *inpl = "\
        builtin struct Maths 							\n\
        { 												\n\
        }; 												\n\
        import int Abs(static Maths, int value);		\n\
        import int Max(static Maths, int a, int b);		\n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, MissingSemicolonAfterStruct1) {

    // Missing ";" after struct declaration; isn't a var decl either

    char const *inpl = "\
        enum bool { false = 0, true = 1 };      \n\
        struct CameraEx                         \n\
        {                                       \n\
            import static readonly attribute bool StaticTarget;  \n\
        }                                       \n\
                                                \n\
        bool get_StaticTarget(static CameraEx)  \n\
        {                                       \n\
            return 0;                           \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("orget a"));
}

TEST_F(Compile1, NewBuiltin1) {    

    // Cannot do "new X;" when X is a builtin type

    char const *inpl = "\
        builtin managed struct DynamicSprite            \n\
        {                                               \n\
        };                                              \n\
                                                        \n\
        struct SpriteFont                               \n\
        {                                               \n\
            DynamicSprite *Glyph;                       \n\
            import void    CreateFromSprite();          \n\
        };                                              \n\
                                                        \n\
        void SpriteFont::CreateFromSprite()             \n\
        {                                               \n\
            this.Glyph = new DynamicSprite;             \n\
        }                                               \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("built-in"));
}

TEST_F(Compile1, NewArrayBuiltin1) { 

    // Can do "new X[77];" when X is a builtin type because this will only
    // allocate a dynarray of pointers, not of X chunks

    char const *inpl = "\
        builtin managed struct DynamicSprite            \n\
        {                                               \n\
        };                                              \n\
                                                        \n\
        struct SpriteFont                               \n\
        {                                               \n\
            DynamicSprite *Glyphs[];                    \n\
            import void    CreateFromSprite();          \n\
        };                                              \n\
                                                        \n\
        void SpriteFont::CreateFromSprite()             \n\
        {                                               \n\
            this.Glyphs = new DynamicSprite[77];        \n\
        }                                               \n\
        ";
 
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, MissingFunc) {   

    // Must either import or define a function with body if you want to call it.
    // Also, check that the section is set correctly.

    char const *inpl = "\
\"__NEWSCRIPTSTART_HauntedHouse\"                       \n\
        int main()                                      \n\
        {                                               \n\
            return GhostFunc();                         \n\
        }                                               \n\
                                                        \n\
        int GhostFunc(float f = 0.0);                   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_STREQ("HauntedHouse", ccCurScriptName);
}

TEST_F(Compile1, FixupMismatch) {  

    // Code cells that have an "import" fixup must point to the corresponding imports.
    // (This used to fail in combination with linenumbers turned on)

    char const *inpl = "\
        builtin managed struct InventoryItem    \n\
        {                                       \n\
            readonly int reserved[2];           \n\
        };                                      \n\
        import InventoryItem i400Mirror;        \n\
        import InventoryItem i400Key;           \n\
        import InventoryItem inventory[3];      \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            switch (inventory[1])               \n\
            {                                   \n\
            case i400Mirror:                    \n\
                break;                          \n\
            case i400Key:                       \n\
                break;                          \n\
            }                                   \n\
        }                                       \n\
        ";

    ccSetOption(SCOPT_LINENUMBERS, true);

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // Sanity check for import fixups: The corresponding Bytecode must
    // point into the imports[] array, and the corresponding slot must
    // contain a non-empty string.
    for (size_t fixup_idx = 0; fixup_idx < static_cast<size_t>(scrip.numfixups); fixup_idx++)
    {
        if (FIXUP_IMPORT != scrip.fixuptypes[fixup_idx])
            continue;
        int const code_idx = scrip.fixups[fixup_idx];
        EXPECT_TRUE(code_idx < scrip.codesize);

        int const cv = scrip.code[code_idx];
        EXPECT_TRUE(cv >= 0);
        EXPECT_TRUE(cv < scrip.numimports);
        EXPECT_FALSE('\0' == scrip.imports[cv][0]);
    }
}

TEST_F(Compile1, ComponentOfNonStruct1) {

    // If a '.' follows something other than a struct then complain about that fact.
    // Do not complain about expecting and not finding a component.

    char const *inpl = "\
        struct MyStruct             \n\
        {                           \n\
            int i;                  \n\
        };                          \n\
                                    \n\
        void TestArray()            \n\
        {                           \n\
            MyStruct arr[100];      \n\
            arr.i = 0;              \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'.'"));
}

TEST_F(Compile1, ComponentOfNonStruct2) {

    // If a '.' follows something other than a struct then complain about that fact.
    // Do not complain about expecting and not finding a component.

    char const *inpl = "\
        void Test()     \n\
        {               \n\
            int i;      \n\
            i.j = 0;    \n\
        }               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'.'"));
}

TEST_F(Compile1, EmptySection) {

    // An empty last section should not result in an endless loop.

    char const *inpl = "\
\"__NEWSCRIPTSTART_FOO\"     \n\
\"__NEWSCRIPTSTART_BAR\"      \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, AutoptrDisplay) {

    // Autopointered types should not be shown with trailing ' ' in messages
    char const *inpl = "\
        internalstring autoptr builtin      \n\
            managed struct String           \n\
        {                                   \n\
        };                                  \n\
        void main() {                       \n\
            String s = 17;                  \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    ASSERT_EQ(std::string::npos, msg.find("'String '"));
}

TEST_F(Compile1, ReadonlyObjectWritableAttribute)
{
    // player is readonly, but player.InventoryQuantity[...] can be written to.

    char const *inpl = "\
        builtin managed struct Character                \n\
        {                                               \n\
            import attribute int InventoryQuantity[];   \n\
        };                                              \n\
                                                        \n\
        import readonly Character *player;              \n\
                                                        \n\
        void main()                                     \n\
        {                                               \n\
            player.InventoryQuantity[15] = 0;           \n\
        }                                               \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, ImportAutoptr1) {

    // Import decls of funcs with autopointered returns must be processed correctly.

    char const *inpl = "\
        internalstring autoptr builtin      \n\
            managed struct String           \n\
        {                                   \n\
        };                                  \n\
                                            \n\
        import String foo(int);             \n\
                                            \n\
        String foo(int bar)                 \n\
        {                                   \n\
            return null;                    \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, ImportAutoptr2) {

    // Import decls of autopointered variables must be processed correctly.

    char const *inpl = "\
        internalstring autoptr builtin      \n\
            managed struct String           \n\
        {                                   \n\
        };                                  \n\
                                            \n\
        import String foo;                  \n\
        String foo;                         \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, DynptrDynarrayMismatch1)
{
    // It is an error to assign a Dynpointer to a Dynarray variable

    char const *inpl = "\
        managed struct Strct                \n\
        {                                   \n\
            int Payload;                    \n\
        };                                  \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            Strct *o[] = new Strct;         \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("assign"));
}

TEST_F(Compile1, DynptrDynarrayMismatch1a)
{
    // It is an error to assign a Dynpointer to a Dynarray variable

    char const *inpl = "\
        managed struct Strct                \n\
        {                                   \n\
            int Payload;                    \n\
        };                                  \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            Strct *o[];                     \n\
            o= new Strct;                   \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("assign"));
}

TEST_F(Compile1, DynptrDynarrayMismatch2)
{
    // It is an error to assign a Dynarray to a Dynpointer variable

    char const *inpl = "\
        builtin managed struct Object       \n\
        {                                   \n\
            int Payload;                    \n\
        };                                  \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            Object *o = new Object[10];     \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("assign"));
}

TEST_F(Compile1, ZeroMemoryAllocation1)
{

    // If a struct type doesn't contain any variables then there are zero bytes 
    // to allocate. However, it _is_ legal to allocate a dynarray for the
    // struct. (Its elements could be initialized via other means than new.)

    char const *inpl = "\
        managed struct Strct                \n\
        {                                   \n\
        };                                  \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            Strct *o[] = new Strct[10];     \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, ZeroMemoryAllocation2)
{
    // If a struct type doesn't contain any variables then there are zero
    // bytes to allocate. The Engine really doesn't like allocating 0 bytes

    char const *inpl = "\
        managed struct Strct                \n\
        {                                   \n\
        };                                  \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            Strct *o = new Strct;           \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'Strct'"));
}

TEST_F(Compile1,ForwardStructManaged)
{
    // Forward-declared structs must be 'managed', so the
    // actual declaration must have the 'managed' keyword

    char const *inpl = "\
        managed struct Object;              \n\
        struct Object                       \n\
        {                                   \n\
            import attribute int Graphic;   \n\
        } obj;                              \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'managed'"));
}

TEST_F(Compile1, ForwardStructBuiltin)
{
    // Either the forward decl and the actual decl must both be 'builtin'
    // or both be non-'builtin'.

    char const *inpl1 = "\
        managed struct Object;              \n\
        managed builtin struct Object       \n\
        {                                   \n\
        };                                  \n\
        ";

    int compile_result1 = cc_compile(inpl1, scrip);
    std::string msg1 = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result1 >= 0) ? "Ok" : msg1.c_str());
    EXPECT_NE(std::string::npos, msg1.find("'builtin'"));

    char const *inpl2 = "\
        builtin managed struct Object;      \n\
        managed struct Object               \n\
        {                                   \n\
        };                                  \n\
        ";

    int compile_result2 = cc_compile(inpl2, scrip);
    std::string msg2 = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result2 >= 0) ? "Ok" : msg2.c_str());
    EXPECT_NE(std::string::npos, msg2.find("'builtin'"));
}

TEST_F(Compile1, ForwardStructAutoptr)
{
    // Either the forward decl and the actual decl must both be 'autoptr'
    // or both be non-'autoptr'.

    char const *inpl1 = "\
        managed struct Object;              \n\
        managed builtin struct Object       \n\
        {                                   \n\
        };                                  \n\
        ";

    int compile_result1 = cc_compile(inpl1, scrip);
    std::string msg1 = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result1 >= 0) ? "Ok" : msg1.c_str());
    EXPECT_NE(std::string::npos, msg1.find("'builtin'"));

    char const *inpl2 = "\
        builtin managed struct Object;      \n\
        managed struct Object               \n\
        {                                   \n\
        };                                  \n\
        ";

    int compile_result2 = cc_compile(inpl2, scrip);
    std::string msg2 = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result2 >= 0) ? "Ok" : msg2.c_str());
    EXPECT_NE(std::string::npos, msg2.find("'builtin'"));
}

TEST_F(Compile1, FuncThenAssign)
{
    // A function symbol in front of an assignment
    // The compiler should complain about a missing '('  

    char const *inpl2 = "\
        import int GetTextHeight                    \n\
            (const string text, int, int width);    \n\
                                                    \n\
        builtin managed struct Character            \n\
        {                                           \n\
            readonly import attribute int Baseline; \n\
        };                                          \n\
                                                    \n\
        import readonly Character *player;          \n\
                                                    \n\
        int game_start()                            \n\
        {                                           \n\
            GetTextHeight                           \n\
            player.Baseline = 1;                    \n\
        }                                           \n\
        ";

    int compile_result2 = cc_compile(inpl2, scrip);
    std::string msg2 = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result2 >= 0) ? "Ok" : msg2.c_str());
    EXPECT_NE(std::string::npos, msg2.find("'player'"));
}

TEST_F(Compile1, BuiltinForbidden)
{
    // Function names must not start with '__Builtin_'.

    char const *inpl = "\
        void __Builtin_TestFunc()   \n\
        {                           \n\
            return;                 \n\
        }                           \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("__Builtin_"));
}

TEST_F(Compile1, ReadonlyParameters1) {

    // Parameters may be declared "readonly" so that they cannot be
    // assigned to within the function.

    char const *inpl = "\
        int foo(readonly int bar)           \n\
        {                                   \n\
            bar++;                          \n\
            return bar;                     \n\
        }                                   \n\
                                            \n\
        int main ()                         \n\
        {                                   \n\
            return foo(5);                  \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_EQ(std::string::npos, msg.find("parameter list"));
    EXPECT_NE(std::string::npos, msg.find("readonly"));
}

TEST_F(Compile1, ReadonlyParameters2) {

    // "readonly" parameters can be assigned to other variables,
    // but they may not be modified themselves.
    // Contrast this to "const" parameters, they
    // may only assigned to variables that are "const" and 
    // may not be returned unless the return vartype is "const".
    // "Readonly" does NOT imply "const".
    // All the assignments in the function should be allowed.

    char const *inpl = "\
        int ReadonlyTest2(readonly int ReadOnly)    \n\
        {                                   \n\
            readonly int A = ReadOnly;      \n\
            int B;                          \n\
            B = ReadOnly;                   \n\
            int C = ReadOnly;               \n\
            return ReadOnly;                \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, BinaryCompileTimeEval1) {

    // Checks binary compile time evaluations for integers.

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (4 + 3) / 0;                 \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'7 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073741824 + 1073741824);   \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("Overflow"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073741824 + 1073741823);   \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, CTEvalIntPlus) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (4 + 3) / 0;                 \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'7 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073741824 + 1073741824);   \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok2", (compileResult >= 0) ? "Ok2" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("Overflow"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073741824 + -1073741823); \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STREQ("Ok3", (compileResult >= 0) ? "Ok3" : msg.c_str());
}

TEST_F(Compile1, CTEvalIntMinus) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (83 - 95) / 0;               \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'-12 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073741824 - 1073741824);  \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok2", (compileResult >= 0) ? "Ok2" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("Overflow"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073741824 - 1073741823);  \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STREQ("Ok3", (compileResult >= 0) ? "Ok3" : msg.c_str());
}

TEST_F(Compile1, CTEvalIntMultiply) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (33 * -39) / 0;              \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'-1287 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (46341 * 46341);             \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok2", (compileResult >= 0) ? "Ok2" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("Overflow"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (46341 * 46340);             \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STREQ("Ok3", (compileResult >= 0) ? "Ok3" : msg.c_str());
}

TEST_F(Compile1, CTEvalIntDivide) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (52 / 8) / 0;                \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'6 /"));
}

TEST_F(Compile1, CTEvalIntModulo) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (95 % 17) / 0;               \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'10 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (46341 % -0);                \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok2", (compileResult >= 0) ? "Ok2" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("Modulo zero"));
}

TEST_F(Compile1, CTEvalIntShiftLeft) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (60 << 3) / 0;               \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'480 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return 536870912 << 2;              \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok2", (compileResult >= 0) ? "Ok2" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("Overflow"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073 << 4) / 0; \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok3", (compileResult >= 0) ? "Ok3" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'-17168 /"));
    
    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073 << 0) / 0; \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok4", (compileResult >= 0) ? "Ok4" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'1073 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return 1073 << -5;                  \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok5", (compileResult >= 0) ? "Ok5" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("egative shift"));
}

TEST_F(Compile1, CTEvalIntShiftRight) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (60 >> 3) / 0;               \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'7 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-10730 >> 4) / 0; \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok3", (compileResult >= 0) ? "Ok3" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'-671 /"));
    
    inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073 >> 0) / 0; \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok4", (compileResult >= 0) ? "Ok4" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'1073 /"));

    inpl = "\
        int main()                              \n\
        {                                       \n\
            return 1073 >> -5;                  \n\
        }                                       \n\
        ";

    compileResult = cc_compile(inpl, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok5", (compileResult >= 0) ? "Ok5" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("egative shift"));
}

TEST_F(Compile1, CTEvalIntComparisons) {

    // Will fail as soon as any one of those comparisons go awry

    char const *inpl = "\
        int main()                          \n\
        {                                   \n\
            return                          \n\
                (          (7 == 77)        \n\
                + 2 *      (7 >= 77)        \n\
                + 4 *      (7 >  77)        \n\
                + 8 *      (7 <= 77)        \n\
                + 16 *     (7 <  77)        \n\
                + 32 *     (7 != 77)        \n\
                + 64 *     (77 == 7)        \n\
                + 128 *    (77 >= 7)        \n\
                + 256 *    (77 >  7)        \n\
                + 512 *    (77 <= 7)        \n\
                + 1024 *   (77 <  7)        \n\
                + 2048 *   (77 != 7)        \n\
                + 4096 *   (77 == 77)       \n\
                + 81928 *  (77 >= 77)       \n\
                + 16384 *  (77 >  77)       \n\
                + 32768 *  (77 <= 77)       \n\
                + 65536 *  (77 <  77)       \n\
                + 131072 * (77 != 77)) / 0; \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'121280 /"));
}

TEST_F(Compile1, CTEvalBitOps) {

    char const *inpl = "\
        int main()                        \n\
        {                                 \n\
            return                        \n\
                (          (0 & 0)        \n\
                + 2 *      (0 | 0)        \n\
                + 4 *      (0 ^ 0)        \n\
                + 8 *      (0 & 3)        \n\
                + 16 *     (0 | 3)        \n\
                + 32 *     (0 ^ 3)        \n\
                + 64 *     (3 & 0)        \n\
                + 128 *    (3 | 0)        \n\
                + 256 *    (3 ^ 0)        \n\
                + 512 *    (3 & 3)        \n\
                + 1024 *   (3 | 3)        \n\
                + 2048 *   (3 ^ 3)) / 0;  \n\
        }                                 \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'5904 /"));
}

TEST_F(Compile1, CTEvalBitNeg) {

    char const *inpl = "\
        int main()                        \n\
        {                                 \n\
            return (~660753869) / 0;      \n\
        }                                 \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'-660753870 /"));
}

TEST_F(Compile1, CTEvalLogicalOps) {

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (  100000000 *   (!!7) +     \n\
                      10000000 *    (! 7) +     \n\
                      1000000 *     (! 0) +     \n\
                      100000 *      (5 || 7) +  \n\
                      10000 *       (7 || 0) +  \n\
                      1000 *        (0 || 7) +  \n\
                      100 *         (5 && 7) +  \n\
                      10 *          (7 && 0) +  \n\
                                    (0 && 7) ) / 0;   \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok1", (compileResult >= 0) ? "Ok1" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'101577700 /"));
}

TEST_F(Compile1, EnumConstantExpressions)
{
    // Enum values to be evaluated at compile time

    char const *inpl = "\
        enum Bytes              \n\
        {                       \n\
            zero = 1 << 0,      \n\
            one = 1 << 1,       \n\
            two = 1 << 2,       \n\
        };                      \n\
                                \n\
        int main() {            \n\
            int i = two / 0;    \n\
        }                       \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'4 /"));
}

TEST_F(Compile1, IncrementReadonly)
{
    // No incrementing readonly vars

    char const *inpl = "\
        readonly int I;         \n\
                                \n\
        int main() {            \n\
            return ++I;         \n\
        }                       \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("eadonly"));
}

TEST_F(Compile1, SpuriousExpression)
{
    // Warn that '77' doesn't have any effect

    char const *inpl = "\
        int main() {            \n\
            77;                 \n\
        }                       \n\
        ";

    MessageHandler mh;
    int compile_result = cc_compile(inpl, 0, scrip, mh);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    ASSERT_EQ(1, mh.GetMessages().size());
}

TEST_F(Compile1, CompileTimeConstant1)
{
    char const *inpl = "\
        const int CI = 4711;                    \n\
        const float Euler = 2.718281828459045;  \n\
        const float AroundOne = Euler / Euler;  \n\
        float Array[CI];                        \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, CompileTimeConstant2)
{
    char const *inpl = "\
        int main() {                            \n\
            while (1)                           \n\
            {                                   \n\
                const int CI2 = 4712;           \n\
            }                                   \n\
            float CI2;                          \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, CompileTimeConstant3)
{
    char const *inpl = "\
        struct Str                          \n\
        {                                   \n\
            int stuff;                      \n\
            const int foo = 17;             \n\
            static const int foo_squared =  \n\
                Str.foo * Str.foo;          \n\
        } s;                                \n\
                                            \n\
        int main() {                        \n\
            return s.foo;                   \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, CompileTimeConstant4)
{
    char const *inpl = "\
        import const int C = 42; \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("import"));

    char const *inpl2 = "\
        readonly const int C = 42; \n\
        ";

    compile_result = cc_compile(inpl2, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("readonly"));
}

TEST_F(Compile1, CompileTimeConstant5)
{
    // Cannot define a compile-time constant of type 'short'

    char const *inpl = "\
        const short S = 42; \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'short'"));

    // Cannot define a compile-time constant array
    char const *inpl2 = "\
        const int C[]; \n\
        ";

    compile_result = cc_compile(inpl2, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("array"));

    // Misplaced '[]'
    char const *inpl3 = "\
        const int[] C; \n\
        ";

    compile_result = cc_compile(inpl3, scrip);
    msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'['"));
}

TEST_F(Compile1, CompileTimeConstant6)
{
    char const *inpl = "\
            const float pi = 3.14;  \n\
        int main() {                \n\
            float pi = 3.141;       \n\
        }                           \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("in use"));
}

TEST_F(Compile1, StaticThisExtender)
{
    // A 'static' extender function cannot have a 'this'.
    // This declaration should be written
    //     "import int foo (static Struct);"

    char const *inpl = "\
        managed struct Struct                   \n\
        {                                       \n\
            int Payload;                        \n\
        };                                      \n\
        import static int Foo(this Struct *);   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'static'"));
}

TEST_F(Compile1, ReachabilityAndSwitch1)
{
    // Mustn't complain about unreachable code at the end of a switch case

    char const *inpl = "\
        int main()          \n\
        {                   \n\
            int i;          \n\
            switch (i)      \n\
            {               \n\
            case 7:         \n\
                return -5;  \n\
            case 77:        \n\
                return -55; \n\
            default:        \n\
                return 555; \n\
            }               \n\
        }                   \n\
        ";

    AGS::MessageHandler mh;
    int const compile_result = cc_compile(inpl, 0, scrip, mh);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    ASSERT_EQ(0, mh.GetMessages().size());
}

TEST_F(Compile1, IfClauseFloat)
{
    // Should complain that the if clause isn't vartype 'int'

    char const *inpl = "\
        managed struct Strct                \n\
        {                                   \n\
        };                                  \n\
                                            \n\
        float foo()                         \n\
        {                                   \n\
            if (foo())                      \n\
                return 1.0;                 \n\
            return 0.1;                     \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'float'"));
}

TEST_F(Compile1, SideEffectExpression1)
{
    // If a bracketed subexpression has a side effect then
    // the expression has a side effect.
    // Compiler shouldn't warn about an expression without side effects

    char const *inpl = "\
        builtin managed struct Character {              \n\
            int Payload;                                \n\
        };                                              \n\
        import readonly Character character[];          \n\
                                                        \n\
        int game_start()                                \n\
        {                                               \n\
            int i = 0;                                  \n\
            character[i++].Payload;                     \n\
        }                                               \n\
        ";

    AGS::MessageHandler mh;
    int const compile_result = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_EQ(0u, mh.GetMessages().size());
}

TEST_F(Compile1, SideEffectExpression2)
{
    // A function symbol isn't suitable for an expression
    // that should have side effects.
    // Compiler should complain about 'SaveTheWorld;'

    char const *inpl = "\
        import int SaveTheWorld();      \n\
                                        \n\
        int game_start()                \n\
        {                               \n\
            SaveTheWorld;               \n\
        }                               \n\
        ";

    AGS::MessageHandler mh;
    int const compile_result = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : mh.GetError().Message.c_str());
    EXPECT_NE(std::string::npos, mh.GetError().Message.find("'('"));
}

TEST_F(Compile1, SideEffectExpression3)
{
    // A function symbol isn't suitable for an expression
    // that should have side effects.
    // Compiler should complain about 'Initialize;' in the 'for' loop

    char const *inpl = "\
        import int Initialize();        \n\
                                        \n\
        int game_start()                \n\
        {                               \n\
            for (Initialize; true; )    \n\
            { }                         \n\
        }                               \n\
        ";

    AGS::MessageHandler mh;
    int const compile_result = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : mh.GetError().Message.c_str());
    EXPECT_NE(std::string::npos, mh.GetError().Message.find("'('"));
}

TEST_F(Compile1, SideEffectExpression4)
{
    // A function symbol isn't suitable for an expression
    // that should have side effects.
    // Compiler should complain about 'Increment' in the 'for' loop

    char const *inpl = "\
        import int Increment();         \n\
                                        \n\
        int game_start()                \n\
        {                               \n\
            for (; ; Increment)         \n\
            { }                         \n\
        }                               \n\
        ";

    AGS::MessageHandler mh;
    int const compile_result = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : mh.GetError().Message.c_str());
}

TEST_F(Compile1, DisallowStaticVariables)
{
    // AGS does not have static variables.

    char const *inpl = "\
        struct Struct       \n\
        {                   \n\
            static int Var; \n\
        };                  \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("tatic "));

}

TEST_F(Compile1, LongMin01) {

    // LONG_MAX + 1 is too large (when there isn't a '-' in front)

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int i = 2147483648;                 \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of bounds"));
}

TEST_F(Compile1, LongMin02) {

    // LONG_MAX + 1 is too large (when there isn't a UNARY '-' in front)

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            int i = (5 - 2147483648);           \n\
        }                                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of bounds"));
}

TEST_F(Compile1, LongMin03) {

    // Can subtract LONG_MIN from LONG_MIN (result is 0)

    char const *inpl = "\
        int main()                                  \n\
        {                                           \n\
            int i = (- 2147483648 - -2147483648);   \n\
        }                                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, AssignmentInParameterList1)
{
    // An expression cannot contain an assignment symbol '='.
    // Each parameter must comprise an expression, no trailing symbols

    char const *inpl = "\
        int test(int x)     \n\
        {                   \n\
            int i = 0;      \n\
            test(i = 99);   \n\
        }                   \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'='"));
}

TEST_F(Compile1, CallUndefinedFunc) {

    // Function is called, but not defined with body or external
    // This should be flagged naming the function

    char const *inpl = "\
        struct Test             \n\
        {                       \n\
            static int F();     \n\
        };                      \n\
                                \n\
        int game_start()        \n\
        {                       \n\
            Test.F();           \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_NE(std::string::npos, msg.find("'Test::F'"));
    EXPECT_NE(std::string::npos, msg.find("never defined"));
}

TEST_F(Compile1, CrementAsBinary1) {

    // Increment operator can't be used as a binary operator.

    char const *inpl = "\
        int game_start() {      \n\
            int a = 10;         \n\
            int b = 5;          \n\
            int c = a ++ b;     \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_NE(std::string::npos, msg.find("'++'"));
    EXPECT_NE(std::string::npos, msg.find("binary"));
}

TEST_F(Compile1, ReportMissingFunction) {

    // Function is called, but not defined with body or external
    // This should be flagged naming the function

    char const *inpl = "\
        void TpNZaFLjz3ajd();   \n\
                                \n\
        int game_start()        \n\
        {                       \n\
            TpNZaFLjz3ajd();    \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_NE(std::string::npos, msg.find("pNZaFLjz3ajd"));
}

TEST_F(Compile1, ParensAfterNew) {

    // Function is called, but not defined with body or external
    // This should be flagged naming the function

    char const *inpl = "\
        managed struct Struct       \n\
        {                           \n\
            int Payload;            \n\
        };                          \n\
                                    \n\
        int game_start()            \n\
        {                           \n\
            Struct *s = new Struct();   \n\
        }                           \n\
        ";


    AGS::MessageHandler mh;
    int compileResult = cc_compile(inpl, 0u, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_EQ(1u, mh.GetMessages().size());
}
