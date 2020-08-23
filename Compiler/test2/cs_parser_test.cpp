#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"
#include "script2/cs_parser.h"
#include "script2/cc_symboltable.h"
#include "script2/cc_internallist.h"
#include "script/cc_error.h"
#include "script/cc_options.h"
#include "script/script_common.h"

#include "util/string.h"

typedef AGS::Common::String AGSString;
std::string last_cc_error_buf;

void clear_error()
{
    last_cc_error_buf.clear();
}

const char *last_seen_cc_error()
{
    return last_cc_error_buf.c_str();
}

// IMPORTANT: the last_seen_cc_error must contain unformatted error message.
// It is being used in test and compared to hard-coded strings.
std::pair<AGSString, AGSString> cc_error_at_line(const char *error_msg)
{
    // printf("error: %s\n", error_msg);
    last_cc_error_buf = _strdup(error_msg);
    return std::make_pair(AGSString::FromFormat("Error (line %d): %s", currentline, error_msg), AGSString());
}

AGSString cc_error_without_line(const char *error_msg)
{
    last_cc_error_buf = _strdup(error_msg);
    return AGSString::FromFormat("Error (line unknown): %s", error_msg);
}

ccCompiledScript *newScriptFixture() {
    ccSetOption(SCOPT_NOIMPORTOVERRIDE, 0);
    ccSetOption(SCOPT_LINENUMBERS, false);
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();
    return scrip;
}

char g_Input_String[] = "\
\"__NEWSCRIPTSTART_StringDefn\"                             \n\
internalstring autoptr builtin managed struct String        \n\
{                                                           \n\
	import static String Format(const string format, ...);  \n\
	import static bool IsNullOrEmpty(String stringToCheck); \n\
	import String  Append(const string appendText);         \n\
	import String  AppendChar(char extraChar);              \n\
	import int     CompareTo(const string otherString,      \n\
					bool caseSensitive = false);            \n\
	import int     Contains(const string needle);           \n\
	import String  Copy();                                  \n\
	import bool    EndsWith(const string endsWithText,      \n\
					bool caseSensitive = false);            \n\
	import int     IndexOf(const string needle);            \n\
	import String  LowerCase();                             \n\
	import String  Replace(const string lookForText,        \n\
					const string replaceWithText,           \n\
					bool caseSensitive = false);            \n\
	import String  ReplaceCharAt(int index, char newChar);  \n\
	import bool    StartsWith(const string startsWithText,  \n\
					bool caseSensitive = false);            \n\
	import String  Substring(int index, int length);        \n\
	import String  Truncate(int length);                    \n\
	import String  UpperCase();                             \n\
	readonly import attribute float AsFloat;                \n\
	readonly import attribute int AsInt;                    \n\
	readonly import attribute char Chars[];                 \n\
	readonly import attribute int Length;                   \n\
};                                                          \n\
";

char g_Input_Bool[] = "\
\"__NEWSCRIPTSTART_BoolDefn\"             \n\
    enum bool { false = 0, true = 1 };";

TEST(Compile, UnknownKeywordAfterReadonly) {
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

TEST(Compile, DynamicArrayReturnValueErrorText) {
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

TEST(Compile, StructMemberQualifierOrder) {
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

TEST(Compile, ParsingIntSuccess) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 1, int data2=2, int data3=3);\
        int testfunc(int x ) { int y = 42; } \
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile, ParsingIntLimits) {
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

TEST(Compile, ParsingIntDefaultOverflow) {
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

TEST(Compile, ParsingNegIntDefaultOverflow) {
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

TEST(Compile, ParsingIntOverflow) {
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

TEST(Compile, ParsingNegIntOverflow) {
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


TEST(Compile, EnumNegative) {
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
    int compileResult = cc_parse(&targ, scrip, &sym);
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


TEST(Compile, DefaultParametersLargeInts) {
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
    int compileResult = cc_parse(&targ, scrip, &sym);
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

TEST(Compile, ImportFunctionReturningDynamicArray) {
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
    int compileResult = cc_parse(&targ, scrip, &sym);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    int funcidx;
    funcidx = sym.Find("A::MyFunc");

    ASSERT_TRUE(funcidx != -1);

    EXPECT_TRUE(sym.IsDynarray(sym.entries.at(funcidx).FuncParamTypes[0]));
}

TEST(Compile, DoubleNegatedConstant) {
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

TEST(Compile, SubtractionWithoutSpaces) {
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

TEST(Compile, NegationLHSOfExpression) {
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

TEST(Compile, NegationRHSOfExpression) {
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


TEST(Compile, FuncDeclWrong1) {
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

TEST(Compile, FuncDeclWrong2) {
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

TEST(Compile, FuncDeclReturnVartype) {
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


TEST(Compile, Writeprotected) {
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

TEST(Compile, Protected1) {
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

TEST(Compile, Protected2) {
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

TEST(Compile, Protected3) {
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

TEST(Compile, Protected4) {
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

TEST(Compile, Protected5) {
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

TEST(Compile, Do1Wrong) {
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

TEST(Compile, Do2Wrong) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    void main()                     \n\
    {                               \n\
        int i;                      \n\
        do                          \n\
            i = 10;                 \n\
    }                               \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GT(0, compileResult);
    // Offer some leeway in the error message
    // Should balk because the "while" clause is missing.
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("while"));
}

TEST(Compile, Do3Wrong) {
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

TEST(Compile, Do4Wrong) {
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

TEST(Compile, ProtectedFault1) {
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

TEST(Compile, FuncHeader1) {
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

TEST(Compile, FuncHeader2) {
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

TEST(Compile, FuncHeader3) {
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

TEST(Compile, ExtenderFuncHeaderFault1a) {
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

TEST(Compile, ExtenderFuncHeaderFault1b) {
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

TEST(Compile, ExtenderFuncHeaderFault1c) {
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

TEST(Compile, ExtenderFuncHeaderFault2) {
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

TEST(Compile, DoubleExtenderFunc) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct Weapon {                        \n\
            int Damage;                        \n\
        };                                     \n\
                                               \n\
        int Foo(this Weapon *)                 \n\
        {                                      \n\
            return 1;                          \n\
        }                                      \n\
        int Foo(this Weapon *)                 \n\
        {                                      \n\
            return 2;                          \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    EXPECT_NE((char *)0, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("already been def"));
}

TEST(Compile, DoubleNonExtenderFunc) {
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
    ASSERT_NE(std::string::npos, err.find("already been def"));
}

TEST(Compile, UndeclaredStructFunc1) {
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
}

TEST(Compile, UndeclaredStructFunc2) {
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

TEST(Compile, ParamVoid) {
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

TEST(Compile, ParamGlobal) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        short Bermudas;                        \n\
        int Foo(int Bermudas)                  \n\
        {                                      \n\
            return 1;                          \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GT(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Bermudas"));
}

TEST(Compile, StructExtend1) {
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

TEST(Compile, StructExtend2) {
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

TEST(Compile, StructExtend3) {
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

TEST(Compile, StructExtend4) {
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

TEST(Compile, StructStaticFunc) {
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

TEST(Compile, Undefined) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        Supercalifragilisticexpialidocious! \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile, ImportOverride1) {
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

TEST(Compile, DynamicNonManaged1) {
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

TEST(Compile, DynamicNonManaged2) {
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

TEST(Compile, DynamicNonManaged3) {
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

TEST(Compile, BuiltinStructMember) {
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


TEST(Compile, ImportOverride2) {
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

TEST(Compile, ImportOverride3) {
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

TEST(Compile, LocalGlobalSeq1) {
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

TEST(Compile, LocalGlobalSeq2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    int Var;                        \n\
    void Func()                     \n\
    {                               \n\
        short Var = 5;              \n\
        return;                     \n\
    }                               \n\
    ";

    clear_error();

    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile, Void1) {
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

TEST(Compile, RetLengthNoMatch) {
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

TEST(Compile, GlobalImportVar1) {
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

TEST(Compile, GlobalImportVar2) {
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

TEST(Compile, GlobalImportVar3) {
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

TEST(Compile, GlobalImportVar4) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int Var;            \n\
        import int Var;     \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STRNE("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
}

TEST(Compile, GlobalImportVar5) {
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

TEST(Compile, ExtenderFuncDifference)
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

TEST(Compile, StaticFuncCall)
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

TEST(Compile, Import2GlobalAllocation)
{
    ccCompiledScript *scrip = newScriptFixture();
    std::vector<Symbol> tokens; AGS::LineHandler lh;
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
    int compileResult = cc_parse(&targ, scrip, &sym);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    int idx = sym.Find("J");
    ASSERT_LE(0, idx);
    SymbolTableEntry &entry = sym.entries.at(idx);
    ASSERT_EQ(4, entry.SOffset);
}

TEST(Compile, LocalImportVar) {
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

TEST(Compile, Recursive1) {

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

TEST(Compile, GlobalFuncStructFunc) {

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


TEST(Compile, VariadicFunc) {

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

TEST(Compile, DynamicAndNull) {

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

TEST(Compile, AssignPtr2ArrayOfPtr) {

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

TEST(Compile, Attributes01) {
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

TEST(Compile, Attributes02) {
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

TEST(Compile, Attributes03) {
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

TEST(Compile, StructPtrFunc) {
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

TEST(Compile, StringOldstyle01) {
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

TEST(Compile, StringOldstyle02) {
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

TEST(Compile, StringOldstyle03) {
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

TEST(Compile, StructPointerAttribute) {
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

TEST(Compile, StringNullCompare) {
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

TEST(Compile, Attributes04) {
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

TEST(Compile, Attributes05) {
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

TEST(Compile, Attributes06) {
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

TEST(Compile, Decl) {
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

TEST(Compile, DynamicArrayCompare)
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

TEST(Compile, DoubleLocalDecl)
{
    ccCompiledScript *scrip = newScriptFixture();

    // Even within { ... } a local definition must not re-define another
    // local definition

    char *inpl = "\
        int room_AfterFadeIn()                          \n\
        {                                               \n\
            int Bang = 24;                              \n\
            for (int Loop = 0; Loop < 10; Loop += 3)    \n\
            {                                           \n\
                int Bang = Loop - 4;                    \n\
            }                                           \n\
        }                                               \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_NE(compileResult, 0);
    std::string lsce = last_seen_cc_error();
    ASSERT_NE(std::string::npos, lsce.find("Bang"));
}

TEST(Compile, NewForwardDeclStruct)
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

TEST(Compile, NewEnumArray)
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

TEST(Compile, Attributes07) {
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

TEST(Compile, Readonly01) {
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

TEST(Compile, Ternary01) {
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

TEST(Compile, Ternary02) {
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

TEST(Compile, Sections)
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

TEST(Compile, Autoptr)
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

TEST(Compile, BinaryNot)
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

TEST(Compile, UnaryDivideBy)
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

TEST(Compile, FloatInt1)
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

TEST(Compile, FloatInt2)
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

TEST(Compile, StringInt1)
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

TEST(Compile, ExpressionVoid)
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

TEST(Compile, ExpressionLoneUnary1)
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

TEST(Compile, ExpressionLoneUnary2)
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

TEST(Compile, ExpressionBinaryWithoutRHS)
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

TEST(Compile, StaticArrayIndex1)
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

TEST(Compile, StaticArrayIndex2)
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

TEST(Compile, ExpressionArray1)
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

TEST(Compile, ExpressionArray2)
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

TEST(Compile, FuncTypeClash1)
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

TEST(Compile, FloatOutOfRange)
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

TEST(Compile, DoWhileSemicolon)
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

TEST(Compile, ExtenderExtender)
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
    EXPECT_NE(std::string::npos, msg.find("xtender"));
}

TEST(Compile, NonManagedStructParameter)
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

TEST(Compile, StrangeParameterName)
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

TEST(Compile, DoubleParameterName)
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

TEST(Compile, FuncParamDefaults1)
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

TEST(Compile, FuncParamDefaults2)
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

TEST(Compile, FuncParamDefaults3)
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

TEST(Compile, FuncParamNumber)
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

TEST(Compile, FuncVarargsCollision)
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

TEST(Compile, FuncReturnTypes)
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

TEST(Compile, FuncReturnStruct1)
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

TEST(Compile, FuncReturnStruct2)
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

TEST(Compile, FuncReturnStruct3)
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

TEST(Compile, FuncDouble)
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
    EXPECT_NE(std::string::npos, msg.find("with a body"));
}

TEST(Compile, FuncProtected)
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

TEST(Compile, FuncNameClash1)
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

TEST(Compile, TypeEqComponent)
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

TEST(Compile, ExtenderFuncClash)
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
