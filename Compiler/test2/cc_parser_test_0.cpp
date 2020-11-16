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

int cc_compile(std::string const &inpl, AGS::ccCompiledScript &scrip)
{
    AGS::MessageHandler mh;
    AGS::FlagSet const options =
        (0 != ccGetOption(SCOPT_EXPORTALL)) * SCOPT_EXPORTALL |
        (0 != ccGetOption(SCOPT_SHOWWARNINGS)) * SCOPT_SHOWWARNINGS |
        (0 != ccGetOption(SCOPT_LINENUMBERS)) * SCOPT_LINENUMBERS |
        (0 != ccGetOption(SCOPT_AUTOIMPORT)) * SCOPT_AUTOIMPORT |
        (0 != ccGetOption(SCOPT_DEBUGRUN)) * SCOPT_DEBUGRUN |
        (0 != ccGetOption(SCOPT_NOIMPORTOVERRIDE)) * SCOPT_NOIMPORTOVERRIDE |
        (0 != ccGetOption(SCOPT_OLDSTRINGS)) * SCOPT_OLDSTRINGS |
        false;

    int const error_code = cc_compile(inpl, options, scrip, mh);
    if (error_code >= 0)
    {
        // Here if there weren't any errors.
        ccError = 0;
        ccErrorLine = 0;
        return error_code;
    }

    // Here if there was an error. Scaffolding around cc_error()
    AGS::MessageHandler::Entry const &err = mh.GetError();
    static char buffer[256];
    ccCurScriptName = buffer;
    strncpy_s(
        buffer,
        err.Section.c_str(),
        sizeof(buffer) / sizeof(char) - 1);
    currentline = err.Lineno;
    cc_error(err.Message.c_str());
    return error_code;
}

// The vars defined here are provided in each test that is in category "Compile0"
class Compile0 : public ::testing::Test
{
protected:
    AGS::ccCompiledScript scrip = AGS::ccCompiledScript(); // Note: calls Init();

    Compile0()
    {
        // Initializations, will be done at the start of each test
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        ccSetOption(SCOPT_LINENUMBERS, true);
        clear_error();
    }
};


TEST_F(Compile0, UnknownVartypeAfterReadonly) {   

    // Must have a known vartype in struct

    char *inpl = "\
        struct MyStruct     \n\
        {                   \n\
          readonly int2 a;  \n\
          readonly int2 b;  \n\
        };                  \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("int2"));
}

TEST_F(Compile0, DynamicArrayReturnValueErrorText) {

    // Can't convert DynamicSprite[] to int[]

    char *inpl = "\
        managed struct DynamicSprite { };   \n\
                                            \n\
        int[] Func()                        \n\
        {                                   \n\
          DynamicSprite *r[] = new DynamicSprite[10]; \n\
          return r;                         \n\
        }                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_STREQ("Type mismatch: cannot convert 'DynamicSprite *[]' to 'int[]'", last_seen_cc_error());
}

TEST_F(Compile0, StructMemberQualifierOrder) {    

    // The order of qualifiers shouldn't matter.
    // Note, "_tryimport" isn't legal for struct components.
    // Can only use one of "protected", "writeprotected" and "readonly".

    char *inpl = "                                                          \n\
        struct BothOrders {                                                 \n\
            protected import static attribute int something;                \n\
            attribute static import readonly int another;                   \n\
            readonly import attribute int MyAttrib;                         \n\
            import readonly attribute int YourAttrib;                       \n\
        };\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ParsingIntSuccess) {  

    char *inpl = "\
        import  int  importedfunc(int data1 = 1, int data2=2, int data3=3); \n\
        int testfunc(int x ) { int y = 42; } \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ParsingIntLimits) {    

    // Note, 2147483648 will result in an overflow found by the scanner

    char *inpl = "\
        import int int_limits(int param_min = -2147483647, int param_max = 2147483647); \n\
        int int_limits(int param_min, int param_max)    \n\
        {                                               \n\
            int var_min = - 2147483647;                 \n\
            int var_max = 2147483647;                   \n\
        }\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ParsingIntDefaultOverflowPositive) {    

    char *inpl = "\
        import int importedfunc(int data1 = 9999999999999999999999, int data2=2, int data3=3);    \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("9999999999999999999999"));
}

TEST_F(Compile0, ParsingIntDefaultOverflowNegative) {

    char *inpl = "\
        import  int  importedfunc(int data1 = -9999999999999999999999, int data2=2, int data3=3);   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("9999999999999999999999"));
}

TEST_F(Compile0, ParsingIntOverflow) {
    
    char *inpl = "\
        int testfunc(int x ) { int y = 4200000000000000000000; }    \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("4200000000000000000000"));
}

TEST_F(Compile0, ParsingNegIntOverflow) {
    
    char *inpl = "\
        int testfunc(int x ) { int y = -4200000000000000000000; }   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("4200000000000000000000"));
}

TEST_F(Compile0, EnumNegative) {
    
    std::vector<AGS::Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    AGS::SymbolTable sym;
    AGS::MessageHandler mh;
    AGS::FlagSet const options = ~SCOPT_NOIMPORTOVERRIDE | SCOPT_LINENUMBERS;

    char *inpl = "\
        enum TestMyEnums {      \n\
            cat,                \n\
            dog,                \n\
            fish,               \n\
            money=100,          \n\
            death,              \n\
            taxes,              \n\
            popularity=-3,      \n\
            x,                  \n\
            y,                  \n\
            z,                  \n\
            intmin=-2147483647, \n\
            intmax=2147483647   \n\
        };\
        ";

    // Call cc_scan() and cc_parse() by hand so that we can see the symbol table
    ASSERT_LE(0, cc_scan(inpl, targ, scrip, sym, mh));
    int compileResult = cc_parse(targ, options, scrip, sym, mh);
    ASSERT_EQ(0, compileResult);

    // C enums start with 0, but AGS enums with 1
    EXPECT_EQ(sym.Find("1"), sym.entries.at(sym.Find("cat")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("2"), sym.entries.at(sym.Find("dog")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("3"), sym.entries.at(sym.Find("fish")).ConstantD->ValueSym);

    EXPECT_EQ(sym.Find("100"), sym.entries.at(sym.Find("money")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("101"), sym.entries.at(sym.Find("death")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("102"), sym.entries.at(sym.Find("taxes")).ConstantD->ValueSym);

    EXPECT_EQ(sym.Find("-3"), sym.entries.at(sym.Find("popularity")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("-2"), sym.entries.at(sym.Find("x")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("-1"), sym.entries.at(sym.Find("y")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("0"), sym.entries.at(sym.Find("z")).ConstantD->ValueSym);

    // Note: -2147483648 makes the scanner (!) find an int overflow.
    EXPECT_EQ(sym.Find("-2147483647"), sym.entries.at(sym.Find("intmin")).ConstantD->ValueSym);
    EXPECT_EQ(sym.Find("2147483647"), sym.entries.at(sym.Find("intmax")).ConstantD->ValueSym);
}

TEST_F(Compile0, DefaultParametersLargeInts) {
    
    std::vector<AGS::Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    AGS::SymbolTable sym;
    AGS::MessageHandler mh;
    AGS::FlagSet const options = ~SCOPT_NOIMPORTOVERRIDE | SCOPT_LINENUMBERS;

    char *inpl = "\
        import int importedfunc(    \n\
            int data1 = 0,          \n\
            int data2 = 1,          \n\
            int data3 = 2,          \n\
            int data4 = -32000,     \n\
            int  = 32001,           \n\
            int data6 = 2147483647, \n\
            int data7 = -2147483647 , \n\
            int data8 = -1,         \n\
            int data9 = -2          \n\
            );                      \n\
        ";

    
    ASSERT_LE(0, cc_scan(inpl, targ, scrip, sym, mh));
    int compileResult = cc_parse(targ, options, scrip, sym, mh);
    ASSERT_EQ(0, compileResult);

    AGS::Symbol const funcidx = sym.Find("importedfunc");

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[1].Vartype);
    EXPECT_EQ(sym.Find("0"), sym.entries.at(funcidx).FunctionD->Parameters[1].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[2].Vartype);
    EXPECT_EQ(sym.Find("1"), sym.entries.at(funcidx).FunctionD->Parameters[2].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[3].Vartype);
    EXPECT_EQ(sym.Find("2"), sym.entries.at(funcidx).FunctionD->Parameters[3].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[4].Vartype);
    EXPECT_EQ(sym.Find("-32000"), sym.entries.at(funcidx).FunctionD->Parameters[4].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[5].Vartype);
    EXPECT_EQ(sym.Find("32001"), sym.entries.at(funcidx).FunctionD->Parameters[5].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[6].Vartype);
    EXPECT_EQ(sym.Find("2147483647"), sym.entries.at(funcidx).FunctionD->Parameters[6].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[7].Vartype);
    EXPECT_EQ(sym.Find("-2147483647"), sym.entries.at(funcidx).FunctionD->Parameters[7].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[8].Vartype);
    EXPECT_EQ(sym.Find("-1"), sym.entries.at(funcidx).FunctionD->Parameters[8].Default);

    EXPECT_EQ(AGS::kKW_Int, sym.entries.at(funcidx).FunctionD->Parameters[9].Vartype);
    EXPECT_EQ(sym.Find("-2"), sym.entries.at(funcidx).FunctionD->Parameters[9].Default);
}

TEST_F(Compile0, ImportFunctionReturningDynamicArray) {
    
    std::vector<AGS::Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    AGS::SymbolTable sym;
    AGS::MessageHandler mh;
    AGS::FlagSet const options = ~SCOPT_NOIMPORTOVERRIDE | SCOPT_LINENUMBERS;

    char *inpl = "\
        struct A                            \n\
        {                                   \n\
            import static int[] MyFunc();   \n\
        };                                  \n\
        ";

    ASSERT_LE(0, cc_scan(inpl, targ, scrip, sym, mh));
    int compileResult = cc_parse(targ, options, scrip, sym, mh);
    ASSERT_EQ(0, compileResult);

    int funcidx;
    funcidx = sym.Find("A::MyFunc");

    ASSERT_TRUE(funcidx != -1);

    EXPECT_TRUE(sym.IsDynarrayVartype(sym.entries.at(funcidx).FunctionD->Parameters[0].Vartype));
}

TEST_F(Compile0, DoubleNegatedConstant) {
    
    // Can't handle compile time calculations yet

    char *inpl = "\
        import int MyFunction(  \n\
            int data0 = - -69   \n\
            );                  \n\
        ";
        
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string msg = last_seen_cc_error();
    EXPECT_NE(std::string::npos, msg.find("parameter default"));
}

TEST_F(Compile0, SubtractionWithoutSpaces) {

    char *inpl = "\
        int MyFunction()        \n\
        {                       \n\
            int data0 = 2-4;    \n\
        }                       \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, NegationLHSOfExpression) {   

    char *inpl = "\
        enum MyEnum         \n\
        {                   \n\
            cat             \n\
        };                  \n\
                            \n\
        int MyFunctionA()   \n\
        {                   \n\
            return 0;       \n\
        }                   \n\
                            \n\
        int MyFunctionB()   \n\
        {                   \n\
            int data0 = - 4 * 4;                \n\
            int data1 = - MyFunctionA() * 4;    \n\
            int data2 = -cat * 4;               \n\
                            \n\
            return 0;       \n\
        }                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, NegationRHSOfExpression) {

    char *inpl = "\
        enum MyEnum\
        {\
            cat\
        };\
        \
        int MyFunctionA()\
        {\
            return 0;\
        }\
        \
        int MyFunctionB()\
        {\
            int data0 = 3 - - 4 * 4;\
            int data1 = 3 - - MyFunctionA() * 4;\
            int data2 = 3 - -cat * 4;\
            \
            return 0;\
        }\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, FuncDeclWrong1) {    

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

TEST_F(Compile0, FuncDeclWrong2) {
    

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

TEST_F(Compile0, FuncDeclReturnVartype) {
    
    // Should compile.

    char *inpl = "\
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
    }                                                               \n\
   ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Writeprotected) {
    
    // Directly taken from the doc on writeprotected, simplified.
    // Should fail, no modifying of writeprotected components from the outside.

    char *inpl = "\
        struct Weapon {                        \n\
            writeprotected int Damage;         \n\
        };                                     \n\
                                               \n\
        Weapon wp;                             \n\
                                               \n\
        void main()                            \n\
        {                                      \n\
            wp.Damage = 7;                     \n\
            return false;                      \n\
        }                                      \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("Damage"));
}

TEST_F(Compile0, Protected1) {   

    // Directly taken from the doc on protected, simplified.
    // Should fail, no modifying of protected components from the outside.

    char *inpl = "\
        struct Weapon {                        \n\
            protected int Damage;              \n\
        };                                     \n\
                                               \n\
        Weapon wp;                             \n\
                                               \n\
        void main()                            \n\
        {                                      \n\
            wp.Damage = 7;                     \n\
            return;                            \n\
        }                                      \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("Damage"));
}

TEST_F(Compile0, Protected2) {
    
    // Directly taken from the doc on protected, simplified.
    // Should fail, no modifying of protected components from the outside.

    char *inpl = "\
        struct Weapon {                        \n\
            protected int Damage;              \n\
        };                                     \n\
                                               \n\
        Weapon wp;                             \n\
                                               \n\
        int main()                             \n\
        {                                      \n\
            return wp.Damage;                  \n\
        }                                      \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("Damage"));
}

TEST_F(Compile0, Protected3) {

    // Should succeed; protected is allowed for struct component functions.

    char *inpl = "\
        managed struct VectorF                      \n\
        {                                           \n\
            float x, y;                             \n\
        };                                          \n\
                                                    \n\
        managed struct VehicleBase                  \n\
        {                                           \n\
            protected void ResetBase(               \n\
                VectorF *, VectorF *);              \n\
        };                                          \n\
                                                    \n\
        protected void VehicleBase::ResetBase(      \n\
            VectorF *pos, VectorF *dir)             \n\
        {                                           \n\
        }                                           \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Protected4) {
    
    // Should succeed

    char *inpl = "\
        managed struct VectorF                      \n\
        {                                           \n\
            float x, y;                             \n\
        };                                          \n\
                                                    \n\
        managed struct VehicleBase                  \n\
        {                                           \n\
            protected void ResetBase(               \n\
                VectorF *, VectorF *);              \n\
        };                                          \n\
                                                    \n\
        protected void VehicleBase::ResetBase(      \n\
            VectorF *pos, VectorF *dir)             \n\
        {                                           \n\
        }                                           \n\
        ";
 
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Protected5) {    

    // Should succeed; protected is allowed for extender functions.

    char *inpl = "\
        managed struct VectorF                      \n\
        {                                           \n\
            float x, y;                             \n\
        };                                          \n\
                                                    \n\
        protected void Func(this VectorF *)         \n\
        {                                           \n\
        }                                           \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Do1Wrong) {    

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        do                          \n\
            int i = 10;             \n\
    }                               \n\
   ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("sole body of"));

}

TEST_F(Compile0, Do2Wrong) { 

    // Should balk because the "while" clause is missing.

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int I;                      \n\
        do                          \n\
            I = 10;                 \n\
    }                               \n\
   ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("while"));
}

TEST_F(Compile0, Do3Wrong) {
    
    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int i;                      \n\
        do                          \n\
            i = 10;                 \n\
        while Holzschuh;            \n\
    }                               \n\
   ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("("));
}

TEST_F(Compile0, Do4Wrong) {    

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int i;                      \n\
        do                          \n\
            i = 10;                 \n\
        while (blah blah);          \n\
    }                               \n\
   ";
  
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("'blah'"));
}

TEST_F(Compile0, Protected0) {

    // Should fail, no modifying of protected components from the outside.
    
    char *inpl = "\
        struct Weapon {                        \n\
            protected int Damage;              \n\
            import int DoDamage();             \n\
        };                                     \n\
                                               \n\
        Weapon wp;                             \n\
                                               \n\
        int Weapon::DoDamage()                 \n\
        {                                      \n\
            wp.Damage = 7;                     \n\
        }                                      \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("rotected"));
}

TEST_F(Compile0, FuncHeader1) {
    
    // Can't have a specific array size in func parameters

    char *inpl = "\
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

TEST_F(Compile0, FuncHeader2) {  

    // Default for float parameter, an int value. Should fail

    char *inpl = "\
        void Foo(float Param = 7);              \n\
        {                                      \n\
             return;                           \n\
        }                                      \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("literal float"));
}

TEST_F(Compile0, FuncHeader3) {   

    // Integer default for managed parameter. Should fail
    char *inpl = "\
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

TEST_F(Compile0, ExtenderFuncHeaderFault1a) {    

    char *inpl = "\
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

TEST_F(Compile0, ExtenderFuncHeaderFault1b) {    

    // A comma or paren should follow 'Weapon'

    char *inpl = "\
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

TEST_F(Compile0, ExtenderFuncHeaderFault1c) {    

    char *inpl = "\
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

TEST_F(Compile0, ExtenderFuncHeaderFault2) {

    char *inpl = "\
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

TEST_F(Compile0, DoubleExtenderFunc) {  

    // Must not define a function with body twice.

    char *inpl = "\
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

TEST_F(Compile0, DoubleNonExtenderFunc) {    

    char *inpl = "\
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

TEST_F(Compile0, UndeclaredStructFunc1) {   

    // Should fail, Struct doesn't have Func

    char *inpl = "\
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

TEST_F(Compile0, UndeclaredStructFunc2) {   

    // Should succeed, Struct has Func

    char *inpl = "\
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

TEST_F(Compile0, ParamVoid) {   

    char *inpl = "\
        int Foo(void Bar)                      \n\
        {                                      \n\
            return 1;                          \n\
        }                                      \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("void"));
}

TEST_F(Compile0, LocalGlobalSeq2) {    

    // Should garner a warning for line 7 because the re-definition hides the func

    char *inpl = "\
        float Func(void) { return 7.7; }    \n\
        int Foo(void)                       \n\
        {                                   \n\
            float F1 = Func();              \n\
            {                               \n\
                float F2 = Func();          \n\
                int Func = 7;               \n\
                Func += Func;               \n\
            }                               \n\
            float F3 = Func();              \n\
        }                                   \n\
        ";

    std::vector<AGS::Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    AGS::SymbolTable sym;
    AGS::MessageHandler mh;
    AGS::FlagSet const options = ~SCOPT_NOIMPORTOVERRIDE | SCOPT_LINENUMBERS;

    ASSERT_LE(0, cc_scan(inpl, targ, scrip, sym, mh));  
    ASSERT_EQ(0, cc_parse(targ, options, scrip, sym, mh));

    ASSERT_EQ(1u, mh.GetMessages().size());
    EXPECT_EQ(7u, mh.GetMessages()[0].Lineno);
}

TEST_F(Compile0, VartypeLocalSeq1) {

    // Can't redefine a vartype as a local variable

    char *inpl = "\
        enum bool { false = 0, true, };     \n\
        int Foo(void)                       \n\
        {                                   \n\
            float bool;                     \n\
        }                                   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string const err = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_NE(std::string::npos, err.find("in use"));
}

TEST_F(Compile0, VartypeLocalSeq2) {   

    // Can't redefine an enum constant as a local variable

    char *inpl = "\
        enum bool { false = 0, true };      \n\
        int Foo(void)                       \n\
        {                                   \n\
            int false = 1;                  \n\
        }                                   \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    std::string const err = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : err.c_str());
    EXPECT_NE(std::string::npos, err.find("in use"));
}

TEST_F(Compile0, StructMemberImport) {    

    char *inpl = "\
        struct Parent                   \n\
        {                               \n\
            import int Payload;         \n\
        };                              \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("import"));
}

TEST_F(Compile0, StructExtend1) {    

    char *inpl = "\
        struct Parent                   \n\
        {                               \n\
            int Payload;                \n\
        };                              \n\
        struct Child extends Parent     \n\
        {                               \n\
            int Payload;                \n\
        };                              \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("Payload"));
}

TEST_F(Compile0, StructExtend2) {   

    char *inpl = "\
        struct Grandparent              \n\
        {                               \n\
            int Payload;                \n\
        };                              \n\
        struct Parent extends Grandparent \n\
        {                               \n\
            short Money;                \n\
        };                              \n\
        struct Child extends Parent     \n\
        {                               \n\
            int Payload;                \n\
        };                              \n\
        ";

    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("Payload"));
}

TEST_F(Compile0, StructExtend3) {   

    char *inpl = "\
        managed struct Parent           \n\
        {                               \n\
            int Wage;                   \n\
        };                              \n\
        managed struct Child extends Parent \n\
        {                               \n\
            int PocketMoney;            \n\
        };                              \n\
        void main()                     \n\
        {                               \n\
            Parent *Ptr = new Child;    \n\
        }                               \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, StructExtend4) { 

    // Can't assign Parent * to Child *: Parent doesn't necessarily have all the fields

    char *inpl = "\
        managed struct Parent           \n\
        {                               \n\
            int Wage;                   \n\
        };                              \n\
        managed struct Child extends Parent \n\
        {                               \n\
            int PocketMoney;            \n\
        };                              \n\
        void main()                     \n\
        {                               \n\
            Child *Ptr = new Parent;    \n\
        }                               \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("assign"));
}

TEST_F(Compile0, StructStaticFunc) {

    // Okay, a struct that is being defined is automatically forward-declared

    char *inpl = "\
        builtin managed struct GUI {                          \n\
            import static GUI* GetAtScreenXY(int x, int y);   \n\
        };                                                    \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, StructForwardDeclare1) {

    // GUI is forward-defined, but the definition will show up later so this is okay.

    char *inpl = "\
        managed struct GUI;     \n\
        GUI *Var;               \n\
        managed struct GUI {    \n\
        };                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, StructForwardDeclare2) {

    // Forward-declared structs must be "managed".

    char *inpl = "\
        struct GUI;     \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string msg = last_seen_cc_error();
    EXPECT_NE(std::string::npos, msg.find("managed"));
}

TEST_F(Compile0, StructForwardDeclare3) {

    // GUI only has a forward definition

    char *inpl = "\
        managed struct GUI;     \n\
        GUI *Var;               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string msg = last_seen_cc_error();
    EXPECT_NE(std::string::npos, msg.find("completely defined"));
}

TEST_F(Compile0, StructForwardDeclareNew) {

    // "new" on a forward-declared struct mustn't work
    // even when the struct is defined after the reference

    char *inpl = "\
        managed struct Bang;        \n\
        int main()                  \n\
        {                           \n\
            Bang sptr = new Bang;   \n\
        }                           \n\
        managed struct Bang         \n\
        {                           \n\
            int i;                  \n\
        };                          \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_NE(compileResult, 0);
    std::string lsce = last_seen_cc_error();
    EXPECT_NE(std::string::npos, lsce.find("Bang"));
}

TEST_F(Compile0, StructManaged1a)
{
    // Cannot have managed components in managed struct.
    // This is an Engine restriction.

    char *inpl = "\
        managed struct Managed1 \n\
        { };                    \n\
        managed struct Managed  \n\
        {                       \n\
            Managed1 compo;     \n\
        };                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, StructManaged1b)
{
    // Cannot have managed components in managed struct.
    // This is an Engine restriction.

    char *inpl = "\
        managed struct Managed1 \n\
        { };                    \n\
        managed struct Managed  \n\
        {                       \n\
            Managed1 compo[];   \n\
        };                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, StructManaged2)
{
    // Cannot have managed components in managed struct.
    // This is an Engine restriction.

    char *inpl = "\
        managed struct Managed  \n\
        {                       \n\
            int *compo;         \n\
        };                      \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, Undefined) {   

    char *inpl = "\
        Supercalifragilisticexpialidocious! \n\
        ";

    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, ImportOverride1) {

    char *inpl = "\
    import int Func(int i = 5);     \n\
    int Func(int i)                 \n\
    {                               \n\
        return 2 * i;               \n\
    }                               \n\
    ";

    ccSetOption(SCOPT_NOIMPORTOVERRIDE, true);
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, DynamicNonManaged1) {    

    // Dynamic array of non-managed struct not allowed

    char *inpl = "\
        struct Inner                                        \n\
        {                                                   \n\
            short Payload;                                  \n\
        };                                                  \n\
        managed struct Struct                               \n\
        {                                                   \n\
            Inner In[];                                     \n\
        };                                                  \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST_F(Compile0, DynamicNonManaged2) {    

    // Dynamic array of non-managed struct not allowed

    char *inpl = "\
        struct Inner                                        \n\
        {                                                   \n\
            short Payload;                                  \n\
        };                                                  \n\
        managed struct Struct                               \n\
        {                                                   \n\
            Inner *In;                                      \n\
        };                                                  \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST_F(Compile0, DynamicNonManaged3) {  

    // Dynamic array of non-managed struct not allowed

    char *inpl = "\
        struct Inner                                        \n\
        {                                                   \n\
            short Payload;                                  \n\
        };                                                  \n\
        Inner *In;                                          \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST_F(Compile0, BuiltinStructMember) {    

    // Builtin (non-managed) components not allowed 

    char *inpl = "\
        builtin struct Inner                                \n\
        {                                                   \n\
            short Fluff;                                    \n\
        };                                                  \n\
        managed struct Struct                               \n\
        {                                                   \n\
            Inner In;                                       \n\
        };                                                  \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST_F(Compile0, ImportOverride2) {    

    char *inpl = "\
        int Func(int i = 5);    \n\
        int Func(int i)         \n\
        {                       \n\
            return 2 * i;       \n\
        }                       \n\
        ";

    ccSetOption(SCOPT_NOIMPORTOVERRIDE, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ImportOverride3) {   

    char *inpl = "\
    int Func(int i)                 \n\
    {                               \n\
        return 2 * i;               \n\
    }                               \n\
    import int Func(int i = 5);     \n\
    ";

    ccSetOption(SCOPT_NOIMPORTOVERRIDE, true);
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, LocalSeq1) {    

    // The  { ... } must NOT invalidate Var1 but they MUST invalidate Var2.

    char *inpl = "\
        void Func()                 \n\
        {                           \n\
            int Var1 = 0;           \n\
            { short Var2 = 5; }     \n\
            float Var2 = 7.7;       \n\
            Var1 = 3;               \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, LocalSeq2) {

    // The  while() { ... } must NOT invalidate Var1 but MUST invalidate Var2.

    char *inpl = "\
        void Func()                     \n\
        {                               \n\
            int Var1 = 0;               \n\
            while (Var1 > 0) { short Var2 = 5; } \n\
            float Var2 = 7.7;           \n\
            Var1 = 3;                   \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, LocalSeq3) {
    
    // The  do { ... } while() must NOT invalidate Var1 but MUST invalidate Var2.

    char *inpl = "\
        void Func()                     \n\
        {                               \n\
            int Var1 = 0;               \n\
            do { short Var2 = 5; } while (Var1 > 0); \n\
            float Var2 = 7.7;           \n\
            Var1 = 3;                   \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, LocalSeq4) {
   
    // The  for() { ... } must NOT invalidate Var1 but MUST invalidate Var2 and Var3.

    char *inpl = "\
        void Func()                     \n\
        {                               \n\
            int Var1 = 0;               \n\
            for (int Var2 = 0; Var2 != Var2; Var2 = 1)  \n\
            {                           \n\
                short Var3 = 5;         \n\
            }                           \n\
            float Var2 = 7.7;           \n\
            float Var3 = 8.88;          \n\
            Var1 = 3;                   \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, LocalParameterSeq1) {  

    // Must fail because definitions of I collide

    char *inpl = "\
        void Func(int I)                \n\
        {                               \n\
            int I;                      \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, LocalParameterSeq2) { 

    // Fine

    char *inpl = "\
        void Func(int I)            \n\
        {                           \n\
            { int I; }              \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, LocalGlobalSeq1) {   

    char *inpl = "\
        void Func()                     \n\
        {                               \n\
            short Var = 5;              \n\
            return;                     \n\
        }                               \n\
        int Var;                        \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Void1) {

    char *inpl = "\
        builtin managed struct Parser {                             \n\
	        import static int    FindWordID(const string wordToFind);   \n\
	        import static void   ParseText(const string text);      \n\
	        import static bool   Said(const string text);           \n\
	        import static String SaidUnknownWord();                 \n\
        };                                                          \n\
        ";

    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, RetLengthNoMatch) { 

    char *inpl = "\
        builtin managed struct GUI {                                \n\
            import void Centre();                                   \n\
            import static GUI* GetAtScreenXY(int x, int y);         \n\
        };                                                          \n\
        ";

    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ImportVar1) {    

    char *inpl = "\
        import int Var;     \n\
        import int Var;     \n\
        int Var;            \n\
        export Var;         \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ImportVar2) {
    
    char *inpl = "\
        import int Var;     \n\
        import int Var;     \n\
        int Var;            \n\
        export Var;         \n\
        ";
    
    ccSetOption(SCOPT_NOIMPORTOVERRIDE, 1);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string msg = last_seen_cc_error();
    EXPECT_NE(std::string::npos, msg.find("import"));
}

TEST_F(Compile0, ImportVar3) {

    char *inpl = "\
        import int Var;     \n\
        import int Var;     \n\
        short Var;          \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string msg = last_seen_cc_error();
    EXPECT_NE(std::string::npos, msg.find("'short'"));
    EXPECT_NE(std::string::npos, msg.find("'int'"));
}

TEST_F(Compile0, ImportVar4) {

    char *inpl = "\
        int Var;            \n\
        import int Var;     \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string err = last_seen_cc_error();
    EXPECT_EQ(std::string::npos, err.find("xception"));
}

TEST_F(Compile0, ImportVar5) {
    
    // "import int Var" is treated as a forward declaration
    // for the "int Var" that follows, not as an import proper.

    char *inpl = "\
        import int Var;     \n\
        int main()          \n\
        {                   \n\
            return Var;     \n\
        }                   \n\
        int Var;            \n\
        export Var;         \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, ExtenderFuncDifference) {
    
    // Same func name, should be okay since they extend different structs

    char *inpl = "\
        struct A            \n\
        {                   \n\
            int A_Payload;  \n\
        };                  \n\
        struct B            \n\
        {                   \n\
            int B_Payload;  \n\
        };                  \n\
                            \n\
        int Func(this A *)  \n\
        {                   \n\
            return 0;       \n\
        }                   \n\
        int Func(this B *)  \n\
        {                   \n\
            return 0;       \n\
        }                   \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, StaticFuncCall) {
    
    // Static function call, should work.

    char *inpl = "\
        builtin managed struct GUI                                  \n\
        {                                                           \n\
            import static void ProcessClick(int x, int y, int z);   \n\
        };                                                          \n\
                                                                    \n\
        void main()                                                 \n\
        {                                                           \n\
            GUI.ProcessClick(1, 2, 3);                              \n\
        }                                                           \n\
    ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Import2GlobalAllocation) {
    
    // Imported var I becomes a global var; must be allocated only once.
    // This means that J ought to be allocated at 4.

    char *inpl = "\
        import int I;   \n\
        int I;          \n\
        int J;          \n\
    ";

    std::vector<AGS::Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    AGS::SymbolTable sym;
    AGS::MessageHandler mh;
    AGS::FlagSet const options = ~SCOPT_NOIMPORTOVERRIDE | SCOPT_LINENUMBERS;

    ASSERT_LE(0, cc_scan(inpl, targ, scrip, sym, mh));
    ASSERT_EQ(0, cc_parse(targ, options, scrip, sym, mh));

    AGS::Symbol const idx = sym.Find("J");
    ASSERT_LE(0, idx);
    AGS::SymbolTableEntry &entry = sym.entries.at(idx);
    ASSERT_EQ(4, entry.VariableD->SOffset);
}

TEST_F(Compile0, LocalImportVar) {
    
    char *inpl = "\
        import int Var;     \n\
        int Var;            \n\
        export Var;         \n\
        ";
    
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Recursive1) {

    char *agscode = "\
        import int Foo2 (int);    \n\
                                  \n\
        int Foo1(int a)           \n\
        {                         \n\
            if (a >= 0)           \n\
              return Foo2(a - 1); \n\
        }                         \n\
                                  \n\
        int Foo2(int a)           \n\
        {                         \n\
            return Foo1 (a - 2);  \n\
        }                         \n\
        ";

    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, GlobalFuncStructFunc) {

    char *agscode = "\
        import int Foo2 (int);      \n\
                                    \n\
        struct Struct               \n\
        {                           \n\
            import int Foo2(int);   \n\
        };                          \n\
                                    \n\
        int Struct::Foo2(int a)     \n\
        {                           \n\
            return 17;              \n\
        }                           \n\
        ";
   
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, VariadicFunc) {

    std::string agscode = "\
        String payload;                 \n\
                                        \n\
        void main()                     \n\
        {                               \n\
            payload = String.Format(    \n\
                \"%d bats are %s.\",    \n\
                17, \"fat\");           \n\
            return;                     \n\
        }                               \n\
        ";
    agscode = g_Input_String + agscode;
    agscode = g_Input_Bool + agscode;

    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, DynamicAndNull) {

    std::string agscode = "\
        int main()                          \n\
        {                                   \n\
            int DynArray[] = new int[10];   \n\
            if (DynArray == null)           \n\
                return 1;                   \n\
            else                            \n\
                return -77;                 \n\
        }                                   \n\
        ";

        
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, AssignPtr2ArrayOfPtr) {

    std::string agscode = "\
        managed struct DynamicSprite            \n\
        {                                       \n\
            import static DynamicSprite         \n\
                *Create(int width, int height, bool hasAlphaChannel = false);   \n\
        };                                      \n\
                                                \n\
        int main()                              \n\
        {                                       \n\
            DynamicSprite *sprites[] = new DynamicSprite[10];       \n\
            DynamicSprite *spr = DynamicSprite.Create(100, 100);    \n\
            sprites[0] = spr;                   \n\
        }                                       \n\
        ";
    agscode = g_Input_Bool + agscode;
    
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Attributes01) {
    
    // get_Flipped is implicitly declared with attribute Flipped so defns clash

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            float get_Flipped;                          \n\
            readonly import attribute bool Flipped;     \n\
        };                                              \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("ViewFrame::get_Flipped"));
}

TEST_F(Compile0, Attributes02) {   

    // get_Flipped is implicitly declared with attribute Flipped so defns clash

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            import bool get_Flipped(int Holzschuh);     \n\
            readonly import attribute bool Flipped;     \n\
        };                                              \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("ViewFrame::get_Flipped"));
}

TEST_F(Compile0, Attributes03) {

    // get_Flipped is implicitly declared with attribute Flipped so defns clash

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            readonly import attribute bool Flipped;     \n\
            import ViewFrame *get_Flipped();            \n\
        };                                              \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("ViewFrame::get_Flipped"));
}

TEST_F(Compile0, Attributes04) {

    // Components may have the same name as vartypes.

    char *inpl = "\
        builtin managed struct Character {  \n\
            readonly import attribute int Room;     \n\
        };                                  \n\
        builtin managed struct Room {       \n\
        };                                  \n\
        import readonly Character *player;  \n\
                                            \n\
        void main()                         \n\
        {                                   \n\
            if (player.Room == 1)           \n\
                return;                     \n\
        }                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Attributes05) {

    // Assignment to static attribute should call the setter.

    char *inpl = "\
        builtin managed struct Game {       \n\
            import static attribute int MinimumTextDisplayTimeMs;     \n\
        };                                  \n\
                                            \n\
        void main()                         \n\
        {                                   \n\
            Game.MinimumTextDisplayTimeMs = 3000;   \n\
        }                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Attributes06) {

    // Assignment to static indexed attribute

    char *inpl = "\
        builtin managed struct Dialog {                             \n\
            readonly import attribute int OptionCount;              \n\
        };                                                          \n\
                                                                    \n\
        builtin managed struct DialogOptionsRenderingInfo {         \n\
            readonly import attribute Dialog *DialogToRender;       \n\
        };                                                          \n\
                                                                    \n\
        int main()                                                  \n\
        {                                                           \n\
            DialogOptionsRenderingInfo *info;                       \n\
            int DialogOptionYPos[];                                 \n\
            DialogOptionYPos = new int[info.DialogToRender.OptionCount+2];  \n\
        }                                                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Attributes07) {

    // Reading an import static attribute should not trigger
    //  a Not Declared error since it is declared.

    char *inpl = "\
		enum bool { false = 0, true };                              \n\
		builtin managed struct Game {                               \n\
			readonly import static attribute bool Foo;              \n\
			readonly import static attribute bool SkippingCutscene; \n\
		};                                                          \n\
		int room_RepExec()                                          \n\
		{                                                           \n\
			int i = 0;                                              \n\
            if (Game.SkippingCutscene)                              \n\
                i = 1;                                              \n\
		}                                                           \n\
	";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Attributes08) {

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

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
}

TEST_F(Compile0, Attributes09) {

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

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());
    EXPECT_NE(std::string::npos, msg.find("static"));
}

TEST_F(Compile0, StructPtrFunc) {

    // Func is ptr to managed, but it is a function not a variable
    // so ought to be let through.

    char *inpl = "\
        managed struct MS {     \n\
            MS *Func();         \n\
        };                      \n\
        MS *MS::Func()          \n\
        {}                      \n\
        ";
   
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, StringOldstyle01) {    
    
    // Can't return a local string because it will be already de-allocated when
    // the function returns

    char *inpl = "\
        string MyFunction(int a)    \n\
        {                           \n\
            string x;               \n\
            return x;               \n\
        }                           \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string lerr = last_seen_cc_error();
    EXPECT_NE(std::string::npos, lerr.find("local string"));
}

TEST_F(Compile0, StringOldstyle02) {
    
    // If a function expects a non-const string, it mustn't be passed a const string

    char *inpl = "\
        void Func(string s)         \n\
        {                           \n\
            Func(\"Holzschuh\");    \n\
        }                           \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string lerr = last_seen_cc_error();
    EXPECT_NE(std::string::npos, lerr.find("ype mismatch"));
}

TEST_F(Compile0, StringOldstyle03) {
    
    // A string literal is a constant string, so you should not be able to
    // return it as a string.

    char *inpl = "\
        string Func()                   \n\
        {                               \n\
            return \"Parameter\";       \n\
        }                               \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string lerr = last_seen_cc_error();
    EXPECT_NE(std::string::npos, lerr.find("ype mismatch"));
}

TEST_F(Compile0, StructPointerAttribute) {

    // It's okay for a managed struct to have a pointer import attribute.

    char *inpl = "\
        builtin managed struct AudioClip {          \n\
            import void Stop();                     \n\
        };                                          \n\
        builtin managed struct ViewFrame {          \n\
            import attribute AudioClip *LinkedAudio;    \n\
        };                                          \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, StringNullCompare) {

    // It's okay to compare strings to null

    char inpl[] = "\
        void main()                         \n\
        {                                   \n\
            String SS;                      \n\
            int compare3 = (SS == null);    \n\
            int compare4 = (null == SS);    \n\
        }                                   \n\
    ";

    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += inpl;
    
    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Decl) {   

    // Should complain about the "+="
    // Note, there are many more legal possibilites than just "," ";" "=".

    char *inpl = "\
        int main()          \n\
        {                   \n\
            int Sum +=4;    \n\
        }                   \n\
    ";
  
    int compileResult = cc_compile(inpl, scrip);
    std::string lsce = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : lsce.c_str());
    EXPECT_NE(std::string::npos, lsce.find("+="));
    EXPECT_EQ(std::string::npos, lsce.find("xception"));
}

TEST_F(Compile0, DynamicArrayCompare) {

    // May compare DynArrays for equality.
    // The pointers, not the array components are compared.
    // May have a '*' after a struct defn.

    char *inpl = "\
        managed struct Struct               \n\
        {                                   \n\
        } *Arr1[];                          \n\
        Struct Arr2[];                      \n\
        int room_AfterFadeIn()              \n\
        {                                   \n\
            int Compare1 = (Arr1 != null);  \n\
            int Compare2 =                  \n\
                (Arr1 == new Struct[10]);   \n\
            int Compare3 = (Arr1 == Arr2);  \n\
        }                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, DoubleLocalDecl) {

    // A local definition may hide an outer local definition or a global definition;
    // those will be uncovered when the scope of the local definition ends.

    char *inpl = "\
        float Bang1 = 7.7;                              \n\
        int room_AfterFadeIn()                          \n\
        {                                               \n\
            float Bang2 = 24.0;                         \n\
            float Bang3 = 4.2;                          \n\
            for (int Bang3 = 0; Bang3 < 10; Bang3 += 3) \n\
            {                                           \n\
                int Bang1 = Bang3 + 1;                  \n\
                int Bang2 = Bang1;                      \n\
            }                                           \n\
            return (0.0 + Bang1 + Bang2 + Bang3) > 0.0; \n\
        }                                               \n\
    ";
  
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, NewEnumArray) {
    
    // dynamic array of enum should work

    char *inpl = "\
        enum bool                               \n\
        {                                       \n\
            false = 0,                          \n\
            true,                               \n\
        };                                      \n\
                                                \n\
        void Foo()                          \n\
        {                                       \n\
            bool Test[] = new bool[7];          \n\
        }                                       \n\
        ";
  
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Readonly01) {
    
    // Declaring a readonly variable with initialization is okay.

    char *inpl = "\
		int room_RepExec()                  \n\
        {                                   \n\
            readonly int Constant = 835;    \n\
		}                                   \n\
	";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Ternary01) {    

    // case labels accept expressions in AGS, so ternary expressions should work, too.

    char *inpl = "\
        void main()                     \n\
        {                               \n\
            int i = 15;                 \n\
            switch (i)                  \n\
            {                           \n\
                case i < 0 ? 1 : 2:     \n\
                    break;              \n\
                case i ?: 2:            \n\
                    return;             \n\
            }                           \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST_F(Compile0, Ternary02) {

    // Values of ternary must have compatible vartypes

    char *inpl = "\
        int main()                      \n\
        {                               \n\
            return 2 < 1 ? 1 : 2.0;     \n\
                    break;              \n\
        }                               \n\
        ";
  
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("int"));
    EXPECT_NE(std::string::npos, res.find("float"));
}

