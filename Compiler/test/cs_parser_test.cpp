#include <string>
#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"
#include "util/string.h"

typedef AGS::Common::String AGSString;

extern int cc_tokenize(const char*inpl, ccInternalList*targ, ccCompiledScript*scrip);
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
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();
    sym.reset();  // <-- global
    return scrip;
}

TEST(Compile, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("int2"));
}

TEST(Compile, DynamicArrayReturnValueErrorText) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct DynamicSprite { };\
        \
        int[] Func()\
        {\
          DynamicSprite *r[] = new DynamicSprite[10];\
          return r;\
        }";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Type mismatch: cannot convert 'DynamicSprite*[]' to 'int[]'", last_seen_cc_error());
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

    ASSERT_EQ(-1, compileResult);
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
    ASSERT_EQ(-1, compileResult);
    
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("'9999999999999999999999'"));
}

TEST(Compile, ParsingNegIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = -9999999999999999999999, int data2=2, int data3=3);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    // Offer some leeway in the error message, but insist that the culprit is named
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("'-9999999999999999999999'"));
}

TEST(Compile, ParsingIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int testfunc(int x ) { int y = 4200000000000000000000; } \
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
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
    ASSERT_EQ(-1, compileResult);
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
    ASSERT_EQ(0, compileResult);

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

    EXPECT_EQ((-2147483648), sym.entries[sym.find("intmin")].soffs);
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
    ASSERT_EQ(0, compileResult);

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
    EXPECT_EQ((-2147483648), sym.entries[funcidx].funcParamDefaultValues[7]);

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

    EXPECT_EQ(STYPE_DYNARRAY, sym.entries[funcidx].funcparamtypes[0] & STYPE_DYNARRAY);
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
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default value must be literal", last_seen_cc_error());
}

TEST(Compile, SubtractionWithoutSpaces) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int MyFunction()\
        {\
            int data0 = 2-4;\
        }\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, NegationLHSOfExpression) {
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
            int data0 = - 4 * 4;\
            int data1 = - MyFunctionA() * 4;\
            int data2 = -cat * 4;\
            \
            return 0;\
        }\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    printf("Error: %s\n", last_seen_cc_error());
    ASSERT_EQ(0, compileResult);
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
    printf("Error: %s\n", last_seen_cc_error());
    ASSERT_EQ(0, compileResult);
}


TEST(Compile, FuncDeclWrong) {
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
    import  int Func(Struct1 *S1, Struct2 *S2);  \n\
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
    ASSERT_EQ(-1, compileResult);
    // Offer some leeway in the error message
    std::string res(last_seen_cc_error());
    EXPECT_NE(std::string::npos, res.find("parameter"));

}

TEST(Compile, EnumType) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
    enum bool          \n\
    {                  \n\
        false = 0,     \n\
        true = 1       \n\
    };                 \n\
                       \n\
    bool V1 = false;   \n\
    bool Test(bool V2) \n\
    {                  \n\
        return true;   \n\
    }                  \n\
   ";


    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_GE(0, compileResult);
}
