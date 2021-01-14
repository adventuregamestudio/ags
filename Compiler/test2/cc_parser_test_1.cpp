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

    char *inpl = "\
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

    char *inpl = "\
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

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = 15 ! 2;                   \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("inary op"));
}

TEST_F(Compile1, UnaryDivideBy) { 

    // '/' can't be unary

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = (/ 2);                    \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("unary op"));
}

TEST_F(Compile1, UnaryPlus) {

    // '/' can't be unary

    char *inpl = "\
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

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = 4 / 2.0;                  \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST_F(Compile1, FloatInt2) {

    // Can't negate float

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = !2.0;                     \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("after '!'"));
}

TEST_F(Compile1, StringInt1) {    

    // Can't mix string and int

    char *inpl = "\
        int main()                              \n\
        {                                       \n\
            int Var = (\"Holzschuh\" == 2);     \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST_F(Compile1, ExpressionVoid) {  

    // Can't mix void

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var = 1 + Func() * 3;           \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST_F(Compile1, ExpressionLoneUnary1) { 

    // Unary -, nothing following

    char *inpl = "\
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

    char *inpl = "\
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

    char *inpl = "\
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
    EXPECT_NE(std::string::npos, msg.find("right hand side"));
}

TEST_F(Compile1, StaticArrayIndex1) {

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
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("oo high"));
}

TEST_F(Compile1, StaticArrayIndex2) {

    // Constant array index, is out ouf bounds

    char *inpl = "\
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

    char *inpl = "\
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
    EXPECT_NE(std::string::npos, msg.find("ype mismatch"));
}

TEST_F(Compile1, ExpressionArray2) { 

    // Can't mix void

    char *inpl = "\
        import void Func();                     \n\
        int main()                              \n\
        {                                       \n\
            int Var[8];                         \n\
            Var;                                \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("array as a whole"));
}

TEST_F(Compile1, FuncTypeClash1) {

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
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'('"));
}

TEST_F(Compile1, FloatOutOfBounds) {

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
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("ut of bounds"));
}

TEST_F(Compile1, DoWhileSemicolon) { 

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
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("';'"));
}

TEST_F(Compile1, ExtenderExtender1) {    

    // No extending a struct with a compound function

    char *inpl = "\
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
    EXPECT_NE(std::string::npos, msg.find("'static'"));
}

TEST_F(Compile1, ExtenderExtender2) {    

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

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("'::'"));
}

TEST_F(Compile1, NonManagedStructParameter) {

    // Can't pass a non-managed struct as a function parameter

    char *inpl = "\
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

    char *inpl = "\
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

    char *inpl = "\
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

    // Can't give a parameter a default here, not a default there

    char *inpl = "\
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

    // Can't give a parameter a default here, not a default there

    char *inpl = "\
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

TEST_F(Compile1, FuncParamDefaults3) {   

    // Can't give a parameter differing defaults

    char *inpl = "\
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

    char *inpl = "\
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

    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, FuncVariadicCollision) {    

    // Variadic / non-variadic

    char *inpl = "\
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

TEST_F(Compile1, FuncReturnTypes) {   

    // Return types

    char *inpl = "\
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

    char *inpl = "\
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

    // Should work -- Compiler will imply the '*'

    char *inpl = "\
        managed struct Struct {  };             \n\
        Struct Func()                           \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, FuncReturnStruct3) {

    // Should work

    char *inpl = "\
        managed struct Struct {  };             \n\
        Struct[] Func()                         \n\
        {                                       \n\
        }                                       \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, FuncDouble) {

    // No two equally-named functions with body

    char *inpl = "\
        void Func()                             \n\
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

    char *inpl = "\
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

    // Can't give a parameter differing defaults

    char *inpl = "\
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

TEST_F(Compile1, TypeEqComponent) {

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
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, ExtenderFuncClash) {

    // Don't remember the struct of extender functions past their definition (and body, if applicable)

    char *inpl = "\
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

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("orget a"));
}

TEST_F(Compile1, NewBuiltin1) {    

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
   
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("built-in"));
}

TEST_F(Compile1, NewArrayBuiltin1) { 

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
 
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, MissingFunc) {   

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
    
    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_STREQ("HauntedHouse", ccCurScriptName);
}

TEST_F(Compile1, FixupMismatch) {  

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

    char *inpl = "\
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

    char *inpl = "\
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

    char *inpl = "\
\"__NEWSCRIPTSTART_FOO\"     \n\
\"__NEWSCRIPTSTART_BAR\"      \n\
        ";

    int compile_result = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compile_result >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile1, AutoptrDisplay) {

    // Autopointered types should not be shown with trailing ' ' in messages
    char *inpl = "\
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

TEST_F(Compile1, ImportAutoptr1) {

    // Import decls of funcs with autopointered returns must be processed correctly.

    char *inpl = "\
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

    char *inpl = "\
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
