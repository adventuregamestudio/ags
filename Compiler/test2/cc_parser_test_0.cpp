#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "script/cc_options.h"

#include "script2/cc_symboltable.h"
#include "script2/cc_internallist.h"
#include "script2/cs_parser.h"

#include "cc_parser_test_lib.h"

TEST(Compile0, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("int2"));
}

TEST(Compile0, DynamicArrayReturnValueErrorText) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct DynamicSprite { };   \n\
                                            \n\
        int[] Func()                        \n\
        {                                   \n\
          DynamicSprite *r[] = new DynamicSprite[10]; \n\
          return r;                         \n\
        }                                   \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
    EXPECT_STREQ("Type mismatch: cannot convert 'DynamicSprite *[]' to 'int[]'", last_seen_cc_error());
}

TEST(Compile0, StructMemberQualifierOrder) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, ParsingIntSuccess) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 1, int data2=2, int data3=3);\
        int testfunc(int x ) { int y = 42; } \
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, ParsingIntLimits) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
                 import int int_limits(int param_min = -2147483648, int param_max = 2147483647);\
                 int int_limits(int param_min, int param_max)\
                 {\
                 int var_min = - 2147483648;\
                 int var_max = 2147483647;\
                 }\
                 ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, ParsingIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 9999999999999999999999, int data2=2, int data3=3);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);

    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("9999999999999999999999"));

}

TEST(Compile0, ParsingNegIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = -9999999999999999999999, int data2=2, int data3=3);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("-9999999999999999999999"));
}

TEST(Compile0, ParsingIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int testfunc(int x ) { int y = 4200000000000000000000; } \
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("4200000000000000000000"));
}

TEST(Compile0, ParsingNegIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
                 int testfunc(int x ) { int y = -4200000000000000000000; } \
                 ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("-4200000000000000000000"));
}


TEST(Compile0, EnumNegative) {
    ccCompiledScript *scrip = newScriptFixture();
    std::vector<Symbol> tokens; AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    SymbolTable sym;

    char *inpl = "\
        enum TestMyEnums {\
            cat,\
            dog,\
            fish,\
            money=100,\
            death,\
            taxes,\
            popularity=-3,\
            x,\
            y,\
            z,\
            intmin=-2147483648,\
            intmax=2147483647\
        };\
        ";

    clear_error();
    ASSERT_LE(0, cc_scan(inpl, &targ, scrip, &sym));
    std::vector<AGS::Parser::WarningEntry> warnings;
    int compileResult = cc_parse(&targ, scrip, &sym, &warnings);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // C enums default to 0!
    EXPECT_EQ(1, sym.entries.at(sym.Find("cat")).SOffset);
    EXPECT_EQ(2, sym.entries.at(sym.Find("dog")).SOffset);
    EXPECT_EQ(3, sym.entries.at(sym.Find("fish")).SOffset);

    EXPECT_EQ(100, sym.entries.at(sym.Find("money")).SOffset);
    EXPECT_EQ(101, sym.entries.at(sym.Find("death")).SOffset);
    EXPECT_EQ(102, sym.entries.at(sym.Find("taxes")).SOffset);

    EXPECT_EQ(-3, sym.entries.at(sym.Find("popularity")).SOffset);
    EXPECT_EQ(-2, sym.entries.at(sym.Find("x")).SOffset);
    EXPECT_EQ(-1, sym.entries.at(sym.Find("y")).SOffset);
    EXPECT_EQ(0, sym.entries.at(sym.Find("z")).SOffset);

    // Note: -2147483648 gives an _unsigned_ int, not the lowest possible signed int
    // so it can't be used. Microsoft recomments using INT_MIN instead.
    EXPECT_EQ((INT_MIN), sym.entries.at(sym.Find("intmin")).SOffset);
    EXPECT_EQ((2147483647), sym.entries.at(sym.Find("intmax")).SOffset);
}


TEST(Compile0, DefaultParametersLargeInts) {
    ccCompiledScript *scrip = newScriptFixture();
    std::vector<Symbol> tokens; AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    SymbolTable sym;

    char *inpl = "\
        import int importedfunc(\
            int data0 = 0, \
            int data1 = 1, \
            int data2 = 2, \
            int data3 = -32000, \
            int data4 = 32001, \
            int data5 = 2147483647, \
            int data6 = -2147483648 , \
            int data7 = -1, \
            int data8 = -2  \
            );\
        ";

    clear_error();
    ASSERT_LE(0, cc_scan(inpl, &targ, scrip, &sym));

    std::vector<AGS::Parser::WarningEntry> warnings;
    int compileResult = cc_parse(&targ, scrip, &sym, &warnings);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    int funcidx;
    funcidx = sym.Find("importedfunc");

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(1));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[1].Type);
    EXPECT_EQ(0, sym.entries.at(funcidx).FuncParamDefaultValues[1].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(2));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[2].Type);
    EXPECT_EQ(1, sym.entries.at(funcidx).FuncParamDefaultValues[2].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(3));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[3].Type);
    EXPECT_EQ(2, sym.entries.at(funcidx).FuncParamDefaultValues[3].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(4));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[4].Type);
    EXPECT_EQ(-32000, sym.entries.at(funcidx).FuncParamDefaultValues[4].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(5));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[5].Type);
    EXPECT_EQ(32001, sym.entries.at(funcidx).FuncParamDefaultValues[5].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(6));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[6].Type);
    EXPECT_EQ((2147483647), sym.entries.at(funcidx).FuncParamDefaultValues[6].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(7));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[7].Type);
    // NOTE: It's not possible to write the lowest possible signed integer as -2147483648
    EXPECT_EQ(INT_MIN, sym.entries.at(funcidx).FuncParamDefaultValues[7].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(8));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[8].Type);
    EXPECT_EQ(-1, sym.entries.at(funcidx).FuncParamDefaultValues[8].IntDefault);

    EXPECT_EQ(true, sym.entries.at(funcidx).HasParamDefault(9));
    EXPECT_EQ(SymbolTableEntry::kDT_Int, sym.entries.at(funcidx).FuncParamDefaultValues[9].Type);
    EXPECT_EQ(-2, sym.entries.at(funcidx).FuncParamDefaultValues[9].IntDefault);
}

TEST(Compile0, ImportFunctionReturningDynamicArray) {
    ccCompiledScript *scrip = newScriptFixture();
    std::vector<Symbol> tokens; AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    SymbolTable sym;

    char *inpl = "\
        struct A                            \n\
        {                                   \n\
            import static int[] MyFunc();   \n\
        };                                  \n\
        ";

    clear_error();

    ASSERT_LE(0, cc_scan(inpl, &targ, scrip, &sym));
    std::vector<AGS::Parser::WarningEntry> warnings;
    int compileResult = cc_parse(&targ, scrip, &sym, &warnings);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    int funcidx;
    funcidx = sym.Find("A::MyFunc");

    ASSERT_TRUE(funcidx != -1);

    EXPECT_TRUE(sym.IsDynarray(sym.entries.at(funcidx).FuncParamVartypes[0]));
}

TEST(Compile0, DoubleNegatedConstant) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import int MyFunction(\
            int data0 = - -69\
            );\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    EXPECT_STREQ("Expected an integer literal or constant as parameter default", last_seen_cc_error());
}

TEST(Compile0, SubtractionWithoutSpaces) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int MyFunction()        \n\
        {                       \n\
            int data0 = 2-4;    \n\
        }                       \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, NegationLHSOfExpression) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, NegationRHSOfExpression) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}


TEST(Compile0, FuncDeclWrong1) {
    ccCompiledScript *scrip = newScriptFixture();

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
    int Func(Struct2 *S1, Struct1 *S2)  \n\
    {                               \n\
        return 0;                   \n\
    }                               \n\
                                    \n\
    int main()                      \n\
    {                               \n\
        return 0;                   \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("parameter"));

}

TEST(Compile0, FuncDeclWrong2) {
    ccCompiledScript *scrip = newScriptFixture();

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


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("parameter"));

}

TEST(Compile0, FuncDeclReturnVartype) {
    ccCompiledScript *scrip = newScriptFixture();

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


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}


TEST(Compile0, Writeprotected) {
    ccCompiledScript *scrip = newScriptFixture();

    // Directly taken from the doc on writeprotected, simplified.
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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    // Should fail, no modifying of writeprotected components from the outside.
    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Damage"));
}

TEST(Compile0, Protected1) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Damage"));
}

TEST(Compile0, Protected2) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Damage"));
}

TEST(Compile0, Protected3) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Protected4) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

}

TEST(Compile0, Protected5) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Do1Wrong) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        do                          \n\
            int i = 10;             \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("sole body of"));

}

TEST(Compile0, Do2Wrong) {
    ccCompiledScript *scrip = newScriptFixture();

    // Should balk because the "while" clause is missing.

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int I;                      \n\
        do                          \n\
            I = 10;                 \n\
    }                               \n\
   ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("while"));
}

TEST(Compile0, Do3Wrong) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int i;                      \n\
        do                          \n\
            i = 10;                 \n\
        while Holzschuh;            \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("("));
}

TEST(Compile0, Do4Wrong) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int i;                      \n\
        do                          \n\
            i = 10;                 \n\
        while (true true);          \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("true"));
}

TEST(Compile0, ProtectedFault1) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    // Should fail, no modifying of protected components from the outside.
    EXPECT_NE((char *)0, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("rotected"));
}

TEST(Compile0, FuncHeader1) {
    ccCompiledScript *scrip = newScriptFixture();

    // Can't have a specific array size in func parameters
    char *inpl = "\
        void main(int a[15])                   \n\
        {                                      \n\
             return;                           \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("rray size"));
}

TEST(Compile0, FuncHeader2) {
    ccCompiledScript *scrip = newScriptFixture();

    // Default for float parameter, an int value. Should fail
    char *inpl = "\
        void Foo(float Param = 7);              \n\
        {                                      \n\
             return;                           \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("float literal"));
}

TEST(Compile0, FuncHeader3) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("rameter default"));
}

TEST(Compile0, ExtenderFuncHeaderFault1a) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Holzschuh"));
}

TEST(Compile0, ExtenderFuncHeaderFault1b) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Weapon {                        \n\
            int Damage;                        \n\
        };                                     \n\
                                               \n\
        void Foo(static Weapon Of Destruction) \n\
        {                                      \n\
            return;                            \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("arameter name"));
}

TEST(Compile0, ExtenderFuncHeaderFault1c) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("tatic extender function"));
}

TEST(Compile0, ExtenderFuncHeaderFault2) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("struct"));
}

TEST(Compile0, DoubleExtenderFunc) {
    ccCompiledScript *scrip = newScriptFixture();

    // Must not define a function with body twice.

    char *inpl = "\
        struct Weapon {                         \n\
            int Damage;                         \n\
        };                                      \n\
                                                \n\
        int Weapon::Foo(void)                   \n\
        {                                       \n\
            return 1;                           \n\
        }                                       \n\
                                                \n\
        int Foo(this Weapon *)                  \n\
        {                                       \n\
            return 2;                           \n\
        }                                       \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    EXPECT_NE((char *)0, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("defined"));
}

TEST(Compile0, DoubleNonExtenderFunc) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    EXPECT_NE((char *)0, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    EXPECT_NE(std::string::npos, err.find("defined"));
}

TEST(Compile0, UndeclaredStructFunc1) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    std::string message = last_seen_cc_error();
    EXPECT_NE(std::string::npos, message.find("Func"));
}

TEST(Compile0, UndeclaredStructFunc2) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, ParamVoid) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Foo(void Bar)                      \n\
        {                                      \n\
            return 1;                          \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("void"));
}

TEST(Compile0, LocalGlobalSeq2) {
    ccCompiledScript *scrip = newScriptFixture();

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

    std::vector<Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    SymbolTable sym;
    ASSERT_LE(0, cc_scan(inpl, &targ, scrip, &sym));

    std::vector<AGS::Parser::WarningEntry> warnings;
    clear_error();
    int compileResult = cc_parse(&targ, scrip, &sym, &warnings);

    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    ASSERT_EQ(1u, warnings.size());
    EXPECT_EQ(7, warnings[0].Pos);
}

TEST(Compile0, VartypeLocalSeq1) {
    ccCompiledScript *scrip = newScriptFixture();

    // Can't redefine a vartype as a local variable

    char *inpl = "\
        enum bool { false = 0, true, };     \n\
        int Foo(void)                       \n\
        {                                   \n\
            float bool;                     \n\
        }                                   \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    std::string const err = last_seen_cc_error();
    EXPECT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    EXPECT_NE(std::string::npos, err.find("in use"));
}

TEST(Compile0, VartypeLocalSeq2) {
    ccCompiledScript *scrip = newScriptFixture();

    // Can't redefine an enum constant as a local variable

    char *inpl = "\
        enum bool { false = 0, true };      \n\
        int Foo(void)                       \n\
        {                                   \n\
            int false = 1;                  \n\
        }                                   \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    std::string const err = last_seen_cc_error();
    EXPECT_STRNE("Ok", (compileResult >= 0) ? "Ok" : err.c_str());
    EXPECT_NE(std::string::npos, err.find("in use"));
}

TEST(Compile0, StructExtend1) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Payload"));
}

TEST(Compile0, StructExtend2) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Payload"));
}

TEST(Compile0, StructExtend3) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, StructExtend4) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
}

TEST(Compile0, StructStaticFunc) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    builtin managed struct GUI {                          \n\
        import static GUI* GetAtScreenXY(int x, int y);   \n\
    }                                                     \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile0, Undefined) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        Supercalifragilisticexpialidocious! \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile0, ImportOverride1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    import int Func(int i = 5);     \n\
    int Func(int i)                 \n\
    {                               \n\
        return 2 * i;               \n\
    }                               \n\
    ";

    clear_error();

    ccSetOption(SCOPT_NOIMPORTOVERRIDE, true);
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile0, DynamicNonManaged1) {
    ccCompiledScript *scrip = newScriptFixture();

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
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST(Compile0, DynamicNonManaged2) {
    ccCompiledScript *scrip = newScriptFixture();

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
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST(Compile0, DynamicNonManaged3) {
    ccCompiledScript *scrip = newScriptFixture();

    // Dynamic array of non-managed struct not allowed

    char *inpl = "\
        struct Inner                                        \n\
        {                                                   \n\
            short Payload;                                  \n\
        };                                                  \n\
        Inner *In;                                          \n\
    ";
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}

TEST(Compile0, BuiltinStructMember) {
    ccCompiledScript *scrip = newScriptFixture();

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
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("Inner"));
}


TEST(Compile0, ImportOverride2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Func(int i = 5);            \n\
    int Func(int i)                 \n\
    {                               \n\
        return 2 * i;               \n\
    }                               \n\
    ";

    clear_error();

    ccSetOption(SCOPT_NOIMPORTOVERRIDE, true);
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, ImportOverride3) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Func(int i)                 \n\
    {                               \n\
        return 2 * i;               \n\
    }                               \n\
    import int Func(int i = 5);     \n\
    ";

    clear_error();

    ccSetOption(SCOPT_NOIMPORTOVERRIDE, true);
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile0, LocalSeq1) {
    ccCompiledScript *scrip = newScriptFixture();

    // The  { ... } must NOT invalidate Var1 but they MUST invalidate Var2.

    char *inpl = "\
    void Func()                     \n\
    {                               \n\
        int Var1 = 0;               \n\
        { short Var2 = 5; }         \n\
        float Var2 = 7.7;           \n\
        Var1 = 3;                   \n\
    }                               \n\
    ";

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, LocalSeq2) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, LocalSeq3) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, LocalSeq4) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, LocalParameterSeq1) {
    ccCompiledScript *scrip = newScriptFixture();

    // Must fail because definitions of I collide

    char *inpl = "\
    void Func(int I)                \n\
    {                               \n\
        int I;                      \n\
    }                               \n\
    ";

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, LocalParameterSeq2) {
    ccCompiledScript *scrip = newScriptFixture();

    // Fine

    char *inpl = "\
    void Func(int I)                \n\
    {                               \n\
        { int I; }                  \n\
    }                               \n\
    ";

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, LocalGlobalSeq1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    void Func()                     \n\
    {                               \n\
        short Var = 5;              \n\
        return;                     \n\
    }                               \n\
    int Var;                        \n\
    ";

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Void1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    builtin managed struct Parser {                             \n\
	    import static int    FindWordID(const string wordToFind);   \n\
	    import static void   ParseText(const string text);      \n\
	    import static bool   Said(const string text);           \n\
	    import static String SaidUnknownWord();                 \n\
    };                                                          \n\
    ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    clear_error();

    int compileResult = cc_compile(input.c_str(), scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, RetLengthNoMatch) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    builtin managed struct GUI {                                \n\
        import void Centre();                                   \n\
        import static GUI* GetAtScreenXY(int x, int y);         \n\
    };                                                          \n\
    ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    clear_error();

    int compileResult = cc_compile(input.c_str(), scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, GlobalImportVar1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import int Var;     \n\
        import int Var;     \n\
        int Var;            \n\
        export Var;         \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, GlobalImportVar2) {
    ccCompiledScript *scrip = newScriptFixture();
    ccSetOption(SCOPT_NOIMPORTOVERRIDE, 1);

    char *inpl = "\
        import int Var;     \n\
        import int Var;     \n\
        int Var;            \n\
        export Var;         \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, GlobalImportVar3) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import int Var;     \n\
        import int Var;     \n\
        short Var;          \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, GlobalImportVar4) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Var;            \n\
        import int Var;     \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, GlobalImportVar5) {
    ccCompiledScript *scrip = newScriptFixture();
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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, ExtenderFuncDifference)
{
    ccCompiledScript *scrip = newScriptFixture();
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
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, StaticFuncCall)
{
    ccCompiledScript *scrip = newScriptFixture();
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
    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Import2GlobalAllocation)
{
    ccCompiledScript *scrip = newScriptFixture();
    std::vector<Symbol> tokens;
    AGS::LineHandler lh;
    size_t cursor = 0;
    AGS::SrcList targ(tokens, lh, cursor);
    SymbolTable sym;

    // Imported var I becomes a global var; must be allocated only once.
    // This means that J ought to be allocated at 4.

    char *inpl = "\
        import int I;   \n\
        int I;          \n\
        int J;          \n\
    ";

    ASSERT_LE(0, cc_scan(inpl, &targ, scrip, &sym));

    std::vector<AGS::Parser::WarningEntry> warnings;
    int compileResult = cc_parse(&targ, scrip, &sym, &warnings);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    int idx = sym.Find("J");
    ASSERT_LE(0, idx);
    SymbolTableEntry &entry = sym.entries.at(idx);
    ASSERT_EQ(4, entry.SOffset);
}

TEST(Compile0, LocalImportVar) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import int Var;     \n\
        int Var;            \n\
        export Var;         \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Recursive1) {

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

    clear_error();
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, GlobalFuncStructFunc) {

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

    clear_error();
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}


TEST(Compile0, VariadicFunc) {

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

    clear_error();
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode.c_str(), scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, DynamicAndNull) {

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

    clear_error();
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode.c_str(), scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, AssignPtr2ArrayOfPtr) {

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
    clear_error();
    ccCompiledScript *scrip = newScriptFixture();
    int compileResult = cc_compile(agscode.c_str(), scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Attributes01) {
    ccCompiledScript *scrip = newScriptFixture();

    // get_Flipped is implicitly declared with attribute Flipped so defns clash

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            float get_Flipped;                          \n\
            readonly import attribute bool Flipped;     \n\
        };                                              \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("ViewFrame::get_Flipped"));
}

TEST(Compile0, Attributes02) {
    ccCompiledScript *scrip = newScriptFixture();

    // get_Flipped is implicitly declared with attribute Flipped so defns clash

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            import bool get_Flipped(int Holzschuh);     \n\
            readonly import attribute bool Flipped;     \n\
        };                                              \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("ViewFrame::get_Flipped"));
}

TEST(Compile0, Attributes03) {
    ccCompiledScript *scrip = newScriptFixture();

    // get_Flipped is implicitly declared with attribute Flipped so defns clash

    char *inpl = "\
        enum bool { false = 0, true = 1 };              \n\
        builtin managed struct ViewFrame {              \n\
            readonly import attribute bool Flipped;     \n\
            import ViewFrame *get_Flipped();            \n\
        };                                              \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Error message must name culprit
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("ViewFrame::get_Flipped"));
}

TEST(Compile0, StructPtrFunc) {
    ccCompiledScript *scrip = newScriptFixture();

    // Func is ptr to managed, but it is a function not a variable
    // so ought to be let through.

    char *inpl = "\
        managed struct MS {     \n\
            MS *Func();         \n\
        };                      \n\
        MS *MS::Func()          \n\
        {}                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, StringOldstyle01) {
    ccSetOption(SCOPT_OLDSTRINGS, true);
    ccCompiledScript *scrip = newScriptFixture();

    // Can't return a local string because it will be already de-allocated when
    // the function returns

    char *inpl = "\
        string MyFunction(int a)    \n\
        {                           \n\
            string x;               \n\
            return x;               \n\
        }                           \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string lerr = last_seen_cc_error();
    ASSERT_NE(std::string::npos, lerr.find("local string"));
}

TEST(Compile0, StringOldstyle02) {
    ccSetOption(SCOPT_OLDSTRINGS, true);
    ccCompiledScript *scrip = newScriptFixture();

    // If a function expects a non-const string, it mustn't be passed a const string

    char *inpl = "\
        void Func(string s)         \n\
        {                           \n\
            Func(\"Holzschuh\");    \n\
        }                           \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string lerr = last_seen_cc_error();
    ASSERT_NE(std::string::npos, lerr.find("ype mismatch"));
}

TEST(Compile0, StringOldstyle03) {
    ccSetOption(SCOPT_OLDSTRINGS, true);
    ccCompiledScript *scrip = newScriptFixture();

    // A string literal is a constant string, so you should not be able to
    // return it as a string.

    char *inpl = "\
        string Func()                   \n\
        {                               \n\
            return \"Parameter\";       \n\
        }                               \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    std::string lerr = last_seen_cc_error();
    ASSERT_NE(std::string::npos, lerr.find("ype mismatch"));
}

TEST(Compile0, StructPointerAttribute) {
    ccCompiledScript *scrip = newScriptFixture();

    // It's okay for a managed struct to have a pointer import attribute.

    char *inpl = "\
        builtin managed struct AudioClip {          \n\
            import void Stop();                     \n\
        };                                          \n\
        builtin managed struct ViewFrame {          \n\
            import attribute AudioClip *LinkedAudio;    \n\
        };                                          \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, StringNullCompare) {
    ccCompiledScript *scrip = newScriptFixture();

    // It's okay to compare strings to null

    char inpl[] = "\
        void main()                         \n\
        {                                   \n\
            String SS;                      \n\
            int compare3 = (SS == null);    \n\
            int compare4 = (null == SS);    \n\
        }                                   \n\
    ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    clear_error();
    int compileResult = cc_compile(input.c_str(), scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Attributes04) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Attributes05) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Attributes06) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Decl) {
    ccCompiledScript *scrip = newScriptFixture();

    // Should complain about the "+="
    // Note, there are many more legal possibilites than just "," ";" "=".

    char *inpl = "\
        int main()          \n\
        {                   \n\
            int Sum +=4;    \n\
        }                   \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    std::string lsce = last_seen_cc_error();
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : lsce.c_str());
    EXPECT_NE(std::string::npos, lsce.find("+="));
}

TEST(Compile0, DynamicArrayCompare)
{
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, DoubleLocalDecl)
{
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, NewForwardDeclStruct)
{
    ccCompiledScript *scrip = newScriptFixture();

    // "new" on a forward-declared struct mustn't work
    char *inpl = "\
        managed struct Bang;        \n\
        int main()                  \n\
        {                           \n\
            Bang sptr = new Bang;   \n\
        }                           \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_NE(compileResult, 0);
    std::string lsce = last_seen_cc_error();
    ASSERT_NE(std::string::npos, lsce.find("Bang"));
}

TEST(Compile0, NewEnumArray)
{
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Attributes07) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Readonly01) {
    ccCompiledScript *scrip = newScriptFixture();

    // Declaring a readonly variable with initialization is okay.

    char *inpl = "\
		int room_RepExec()                  \n\
        {                                   \n\
            readonly int Constant = 835;    \n\
		}                                   \n\
	";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Ternary01) {
    ccCompiledScript *scrip = newScriptFixture();

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

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile0, Ternary02) {
    ccCompiledScript *scrip = newScriptFixture();

    // Values of ternary must have compatible vartypes

    char *inpl = "\
        int main()                      \n\
        {                               \n\
            return 2 < 1 ? 1 : 2.0;     \n\
                    break;              \n\
        }                               \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // Error message must name culprits
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("int"));
    EXPECT_NE(std::string::npos, res.find("float"));
}
