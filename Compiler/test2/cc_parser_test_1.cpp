#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "script/cc_options.h"
#include "script/cc_error.h"

#include "script2/cc_symboltable.h"
#include "script2/cc_internallist.h"
#include "script2/cs_parser.h"

#include "cc_parser_test_lib.h"

TEST(Compile1, Sections)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Errors in globalscript must be reported for globalscript
    // "__NEWSCRIPTSTART..." begins a line #0,
    // so the error must be reported on line 3.

    char *inpl = "\
        \"__NEWSCRIPTSTART_globalscript.ash\"   \n\
        int main()                              \n\
        {                                       \n\
            return 2 < 1 ? ;                    \n\
                    break;                      \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    EXPECT_EQ(3, currentline);
    ASSERT_STREQ("globalscript.ash", ccCurScriptName);
}

TEST(Compile1, Autoptr)
{
    ccCompiledScript *scrip = newScriptFixture();

    // String is autoptr so should not print as "String *"

    char *inpl = "\
        managed autoptr builtin struct String   \n\
        {};                                     \n\
        int main()                              \n\
        {                                       \n\
            String var = 15;                    \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    ASSERT_EQ(std::string::npos, msg.find("String *"));
}

TEST(Compile1, BinaryNot)
{
    ccCompiledScript *scrip = newScriptFixture();

    // '!' can't be binary

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = 15 ! 2;                   \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("inary op"));
}

TEST(Compile1, UnaryDivideBy)
{
    ccCompiledScript *scrip = newScriptFixture();

    // '/' can't be unary

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = (/ 2);                    \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("unary op"));
}

TEST(Compile1, FloatInt1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't mix float and int

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = 4 / 2.0;                  \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST(Compile1, FloatInt2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't negate float

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = !2.0;                     \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("after '!'"));
}

TEST(Compile1, StringInt1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't mix string and int

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = (\"Holzschuh\" == 2);     \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST(Compile1, ExpressionVoid)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't mix void

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var = 1 + Func() * 3;           \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST(Compile1, ExpressionLoneUnary1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Unary -, nothing following

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var;                            \n\
            Var = -;                            \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("xpected a term"));
}

TEST(Compile1, ExpressionLoneUnary2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Unary ~, nothing following

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var;                            \n\
            Var = ~;                            \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("xpected a term"));
}

TEST(Compile1, ExpressionBinaryWithoutRHS)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Binary %, nothing following

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var;                            \n\
            Var = 5 %;                          \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("right hand side"));
}

TEST(Compile1, StaticArrayIndex1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Constant array index, is out ouf bounds

    char *inpl = "\
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
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of bounds"));
}

TEST(Compile1, StaticArrayIndex2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Constant array index, is out ouf bounds

    char *inpl = "\
        enum E                          \n\
        {                               \n\
            MinusFive = -5,             \n\
        };                              \n\
        int main()                      \n\
        {                               \n\
            int Var[5];                 \n\
            Var[MinusFive];            \n\
        }                               \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of bounds"));
}

TEST(Compile1, ExpressionArray1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't mix void

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var[8];                         \n\
            Var++;                              \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST(Compile1, ExpressionArray2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't mix void

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var[8];                         \n\
            Var;                                \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("array as a whole"));
}

TEST(Compile1, FuncTypeClash1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't use func except in a func call

    char *inpl = "\
        int Func()                              \n\
        {                                       \n\
        }                                       \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            int Var = Func;                     \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'('"));
}

TEST(Compile1, FloatOutOfRange)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Too small

    char *inpl = "\
        int Func()                              \n\
        {                                       \n\
        }                                       \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            int Var = -77E7777;                 \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of range"));
}

TEST(Compile1, DoWhileSemicolon)
{
    ccCompiledScript *scrip = newScriptFixture();

    // ';' missing

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int I = 1;                          \n\
            do {                                \n\
                I *= 2;                         \n\
            } while (I < 10)                    \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("';'"));
}

TEST(Compile1, ExtenderExtender1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // No extending a struct with a compound function

    char *inpl = "\
        struct Struct1                      \n\
        {                                   \n\
        };                                  \n\
        struct Struct2                      \n\
        {                                   \n\
        };                                  \n\
        void Struct1::Func(static Struct2)  \n\
        {                                   \n\
        }                                   \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("static"));
}

TEST(Compile1, ExtenderExtender2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // No extending a struct with a compound function

    char *inpl = "\
        struct Struct1                      \n\
        {                                   \n\
        };                                  \n\
        struct Struct2                      \n\
        {                                   \n\
            void Struct1::Func(short i);    \n\
        };                                  \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'::'"));
}

TEST(Compile1, NonManagedStructParameter)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't pass a non-managed struct as a function parameter

    char *inpl = "\
        struct Struct                           \n\
        {                                       \n\
        };                                      \n\
        int Func(Struct S)                      \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("non-managed"));
}

TEST(Compile1, StrangeParameterName)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't use keyword as parameter name

    char *inpl = "\
        void Func(int while)                    \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("while"));
}

TEST(Compile1, DoubleParameterName)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't use keyword as parameter name

    char *inpl = "\
        void Func(int PI, float PI)             \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("PI"));
}

TEST(Compile1, FuncParamDefaults1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't give a parameter a default here, not a default there

    char *inpl = "\
        void Func(int i = 5, float j = 6.0);    \n\
        void Func(int i = 5, float j)           \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#2"));
}

TEST(Compile1, FuncParamDefaults2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't give a parameter a default here, not a default there

    char *inpl = "\
        void Func(int i, float j);          \n\
        void Func(int i, float j = 6.0)     \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#2"));
}

TEST(Compile1, FuncParamDefaults3)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't give a parameter differing defaults

    char *inpl = "\
        void Func(float J = -6.0);              \n\
        void Func(float J = 6.0)                \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("#1"));
}

TEST(Compile1, FuncParamNumber1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Differing number of parameters

    char *inpl = "\
        void Func(int, float);                  \n\
        void Func(int I, float J, short K)      \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("parameters"));
}

TEST(Compile1, FuncParamNumber2)
{
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Test                                         \n\
        {                                                   \n\
            import void Func(int a, int b, int c, int d);   \n\
        };                                                  \n\
                                                            \n\
        void Test::Func(int a)                              \n\
        {                                                   \n\
        }                                                   \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, FuncVarargsCollision)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Varargs / non-Varargs

    char *inpl = "\
        void Func(int, float, short, ...);      \n\
        void Func(int I, float J, short K)      \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("additional"));
}

TEST(Compile1, FuncReturnTypes)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Return types

    char *inpl = "\
        int Func(int, float, short);            \n\
        short Func(int I, float J, short K)     \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("eturn"));
}

TEST(Compile1, FuncReturnStruct1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Return vartype must be managed when it is a struct

    char *inpl = "\
        struct Struct {  };                     \n\
        Struct Func()                           \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("managed"));
}

TEST(Compile1, FuncReturnStruct2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Should work -- Compiler will imply the '*'

    char *inpl = "\
        managed struct Struct {  };             \n\
        Struct Func()                           \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, FuncReturnStruct3)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Should work

    char *inpl = "\
        managed struct Struct {  };             \n\
        Struct[] Func()                         \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, FuncDouble)
{
    ccCompiledScript *scrip = newScriptFixture();

    // No two equally-named functions with body

    char *inpl = "\
        void Func()                             \n\
        {                                       \n\
        }                                       \n\
        void Func()                             \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("with body"));
}

TEST(Compile1, FuncProtected)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Protected functions must be part of a struct

    char *inpl = "\
        protected void Func(int I = 6)          \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("protected"));
}

TEST(Compile1, FuncNameClash1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can't give a parameter differing defaults

    char *inpl = "\
        int Func;                               \n\
        void Func(int I = 6)                    \n\
        {                                       \n\
        }                                       \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("declared as function"));
}

TEST(Compile1, TypeEqComponent)
{
    ccCompiledScript *scrip = newScriptFixture();

    // A struct component may have the same name as a type.

    char *inpl = "\
        builtin managed struct Room             \n\
        {                                       \n\
        };                                      \n\
                                                \n\
        builtin managed struct Character        \n\
        {                                       \n\
            readonly import attribute int Room; \n\
        };                                      \n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, ExtenderFuncClash)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Don't remember the struct of extender functions past their definition (and body, if applicable)

    char *inpl = "\
        builtin struct Maths 							\n\
        { 												\n\
        }; 												\n\
        import int Abs(static Maths, int value);		\n\
        import int Max(static Maths, int a, int b);		\n\
        ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, MissingSemicolonAfterStruct1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Missing ";" after struct declaration; isn't a var decl either

    char *inpl = "\
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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("orget a"));
}

TEST(Compile1, AttributeGet1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Accept a readonly attribute and a non-readonly getter
    
    char *inpl = "\
        enum bool { false = 0, true = 1 };      \n\
        struct CameraEx                         \n\
        {                                       \n\
            import static readonly attribute bool StaticTarget;  \n\
        };                                      \n\
                                                \n\
        bool get_StaticTarget(static CameraEx)  \n\
        {                                       \n\
            return 0;                           \n\
        }                                       \n\
        ";
        
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, AttributeGet2)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Do not accept a static attribute and a non-static getter

    char *inpl = "\
        enum bool { false = 0, true = 1 };      \n\
        struct CameraEx                         \n\
        {                                       \n\
            import static readonly attribute bool StaticTarget;  \n\
        };                                      \n\
                                                \n\
        bool get_StaticTarget(this CameraEx *)  \n\
        {                                       \n\
            return 0;                           \n\
        }                                       \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("static"));
}

TEST(Compile1, NewBuiltin1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Cannot do "new X;" when X is a builtin type

    char *inpl = "\
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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("built-in"));
}

TEST(Compile1, NewArrayBuiltin1)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Can do "new X[77];" when X is a builtin type because this will only
    // allocate a dynarray of pointers, not of X chunks

    char *inpl = "\
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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST(Compile1, MissingFunc)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Must either import or define a function with body if you want to call it.
    // Also, check that the section is set correctly.

    char *inpl = "\
\"__NEWSCRIPTSTART_HauntedHouse\"                       \n\
        int main()                                      \n\
        {                                               \n\
            return GhostFunc();                         \n\
        }                                               \n\
                                                        \n\
        int GhostFunc(float f = 0.0);                   \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_STREQ("HauntedHouse", ccCurScriptName);
}

TEST(Compile1, FixupMismatch)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Code cells that have an "import" fixup must point to the corresponding imports.
    // (This used to fail in combination with linenumbers turned on)

    char *inpl = "\
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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // Sanity check for import fixups: The corresponding Bytecode must
    // point into the imports[] array, and the corresponding slot must
    // contain a non-empty string.
    for (size_t fixup_idx = 0; fixup_idx < scrip->numfixups; fixup_idx++)
    {
        if (FIXUP_IMPORT != scrip->fixuptypes[fixup_idx])
            continue;
        int const code_idx = scrip->fixups[fixup_idx];
        EXPECT_TRUE(code_idx < scrip->codesize);

        int const cv = scrip->code[code_idx];
        EXPECT_TRUE(cv >= 0);
        EXPECT_TRUE(cv < scrip->numimports);
        EXPECT_FALSE('\0' == scrip->imports[cv][0]);
    }
}
