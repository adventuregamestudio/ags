#include <string>

#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"
#include "script/cc_options.h"

#include "util/string.h"

typedef AGS::Common::String AGSString;

extern int cc_tokenize(const char *inpl, ccInternalList *targ, ccCompiledScript *scrip);
extern int currentline; // in script/script_common

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
    // TODO: investigate proper google test fixtures.
    ccSetOption(SCOPT_NOIMPORTOVERRIDE, 0);
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();
    sym.reset();  // <-- global
    return scrip;
}

char g_Input_String[] = "\
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
                                                            \n\
"; // 30 Zeilen

char g_Input_Bool[] = "\
    enum bool { false = 0, true = 1 };"; // 1 Zeile

TEST(Compile, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GE(0, compileResult);
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

    ASSERT_GE(0, compileResult);
    EXPECT_STREQ("Type mismatch: cannot convert 'DynamicSprite[]' to 'int[]'", last_seen_cc_error());
}

TEST(Compile, DynamicTypeReturnNonPointerManaged) {
    // "DynamicSprite[]" type in the function declaration should cause error on its own, because managed types are
    // only allowed to be referenced by pointers.

    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct DynamicSprite { };\
        \
        DynamicSprite[] Func()\
        {\
        }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GE(0, compileResult);
    EXPECT_STREQ("Cannot pass non-pointer struct array", last_seen_cc_error());
}

TEST(Compile, StructMemberQualifierOrder) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct BothOrders { \
            protected readonly import _tryimport static attribute int something; \
            attribute static _tryimport import readonly writeprotected int another; \
            readonly import attribute int MyAttrib; \
            import readonly attribute int YourAttrib; \
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
    ASSERT_GE(0, compileResult);

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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("-4200000000000000000000"));
}


TEST(Compile, EnumNegative) {
    ccCompiledScript *scrip = newScriptFixture();

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
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // C enums default to 0!
    EXPECT_EQ(1, sym.entries[sym.find("cat")].soffs);
    EXPECT_EQ(2, sym.entries[sym.find("dog")].soffs);
    EXPECT_EQ(3, sym.entries[sym.find("fish")].soffs);

    EXPECT_EQ(100, sym.entries[sym.find("money")].soffs);
    EXPECT_EQ(101, sym.entries[sym.find("death")].soffs);
    EXPECT_EQ(102, sym.entries[sym.find("taxes")].soffs);

    EXPECT_EQ(-3, sym.entries[sym.find("popularity")].soffs);
    EXPECT_EQ(-2, sym.entries[sym.find("x")].soffs);
    EXPECT_EQ(-1, sym.entries[sym.find("y")].soffs);
    EXPECT_EQ(0, sym.entries[sym.find("z")].soffs);

    // Note: -2147483648 gives an _unsigned_ int, not the lowest possible signed int
    // so it can't be used. Microsoft recomments using INT_MIN instead.
    EXPECT_EQ((INT_MIN), sym.entries[sym.find("intmin")].soffs);
    EXPECT_EQ((2147483647), sym.entries[sym.find("intmax")].soffs);
}


TEST(Compile, DefaultParametersLargeInts) {
    ccCompiledScript *scrip = newScriptFixture();

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
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    int funcidx;
    funcidx = sym.find("importedfunc");

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[1]);
    EXPECT_EQ(0, sym.entries[funcidx].funcParamDefaultValues[1]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[2]);
    EXPECT_EQ(1, sym.entries[funcidx].funcParamDefaultValues[2]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[3]);
    EXPECT_EQ(2, sym.entries[funcidx].funcParamDefaultValues[3]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[4]);
    EXPECT_EQ(-32000, sym.entries[funcidx].funcParamDefaultValues[4]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[5]);
    EXPECT_EQ(32001, sym.entries[funcidx].funcParamDefaultValues[5]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[6]);
    EXPECT_EQ((2147483647), sym.entries[funcidx].funcParamDefaultValues[6]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[7]);
    // NOTE: It's not possible to write the lowest possible signed integer as
    // -2147483648
    EXPECT_EQ(INT_MIN, sym.entries[funcidx].funcParamDefaultValues[7]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[8]);
    EXPECT_EQ(-1, sym.entries[funcidx].funcParamDefaultValues[8]);

    EXPECT_EQ(true, sym.entries[funcidx].funcParamHasDefaultValues[9]);
    EXPECT_EQ(-2, sym.entries[funcidx].funcParamDefaultValues[9]);
}

TEST(Compile, ImportFunctionReturningDynamicArray) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct A\
        {\
            import static int[] MyFunc();\
        };\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    int funcidx;
    funcidx = sym.find("A::MyFunc");

    ASSERT_TRUE(funcidx != -1);

    EXPECT_EQ(kVTY_DynArray, sym.entries[funcidx].funcparamtypes[0] & kVTY_DynArray);
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
    ASSERT_GE(0, compileResult);
    EXPECT_STREQ("Parameter default value must be literal", last_seen_cc_error());
}

TEST(Compile, SubtractionWithoutSpaces) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int MyFunction()\n\
        {\n\
            int data0 = 2-4;\n\
        }\n\
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("parameter"));

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
    ASSERT_GE(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Damage"));
}

TEST(Compile, Protected1) {
    ccCompiledScript *scrip = newScriptFixture();

    // Directly taken from the doc on protected, simplified.
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

    // Should fail, no modifying of protected components from the outside.
    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GE(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Damage"));
}

TEST(Compile, Protected2) {
    ccCompiledScript *scrip = newScriptFixture();

    // Directly taken from the doc on protected, simplified.
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

    // Should fail, no modifying of protected components from the outside.
    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GE(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("Damage"));
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("rotected"));
}

TEST(Compile, FuncHeader1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        void main(int a[15])                   \n\
        {                                      \n\
             return;                           \n\
        }                                      \n\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    // Should fail, no modifying of protected components from the outside.
    ASSERT_NE(nullptr, last_seen_cc_error());
    ASSERT_GE(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("rray size"));
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
    std::string err = last_seen_cc_error();
    ASSERT_NE(std::string::npos, err.find("already been def"));
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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
    ASSERT_GE(0, compileResult);
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

    ASSERT_GE(0, compileResult);
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

    ASSERT_GE(0, compileResult);
}

TEST(Compile, Undefined) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        Supercalifragilisticexpialidocious! \n\
    ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GE(0, compileResult);
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

    ASSERT_GE(0, compileResult);
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

    ASSERT_GE(0, compileResult);
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

    ASSERT_GE(0, compileResult);
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
    // Imported var I becomes a global var; must be allocated only once.
    // This means that J ought to be allocated at 4.
    char *inpl = "\
        import int I;   \n\
        int I;          \n\
        int J;          \n\
    ";
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    int idx = sym.find("J");
    ASSERT_LE(0, idx);
    SymbolTableEntry &entry = sym.entries[idx];
    ASSERT_EQ(4, entry.soffs);
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
        builtin managed struct DynamicSprite    \n\
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
        managed struct ManagedStruct {                  \n\
            ManagedStruct *Func();                      \n\
        };                                              \n\
        ManagedStruct *ManagedStruct::Func()            \n\
        {}                                              \n\
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
            import attribute AudioClip* LinkedAudio;    \n\
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

