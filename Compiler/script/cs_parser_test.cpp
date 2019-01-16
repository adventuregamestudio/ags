#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"


// int ConvertCharacterConstantToIntegerLiteral(char  thissymbol[500], bool &retflag);

extern int cc_tokenize(const char*inpl, ccInternalList*targ, ccCompiledScript*scrip);

char *last_seen_cc_error = 0;

void cc_error_at_line(char *buffer, const char *error_msg)
{
    // printf("error: %s\n", error_msg);
    last_seen_cc_error = _strdup(error_msg);
}

void cc_error_without_line(char *buffer, const char *error_msg)
{
    last_seen_cc_error = _strdup(error_msg);
}

ccCompiledScript *newScriptFixture() {
    // TODO: investigate proper google test fixtures.
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();
    sym.reset();  // <-- global
    return scrip;
}

TEST(Tokenize, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    // This incorrect code would crash the tokenizer.
    char *inpl = "struct MyStruct \n\
        {\n\
          readonly int2 a; \n\
          readonly int2 b; \n\
        };";

    ccInternalList targ;
    int tokenizeResult = cc_tokenize(inpl, &targ, scrip);

    ASSERT_EQ(0, tokenizeResult);
}

TEST(Compile, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "struct MyStruct \
        {\
          int2 a; \
          int2 b; \
        };";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
    EXPECT_STREQ("Expected a variable type instead of 'int2'", last_seen_cc_error);

    scrip = newScriptFixture();

    inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    last_seen_cc_error = 0;
    compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
    EXPECT_STREQ("Expected a variable type instead of 'int2'", last_seen_cc_error);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Type mismatch: cannot convert 'DynamicSprite*[]' to 'int[]'", last_seen_cc_error);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("cannot pass non-pointer struct array", last_seen_cc_error);
}

TEST(Compile, SimpleStruct1) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct TestIt { \
            int Something; \
            float Another; \
            }; \
            float Something; \
            int Outside; \
        ";

    // The snippet must compile.
    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    // 'TestIt' and 'Something' must have mangled in the symbol table.
    int token = sym.find("TestIt::Something");
    ASSERT_LE(0, token);

    // 'TestIt::Something' must have base type 'int'.
    int stype = sym.entries[token].vartype;
    ASSERT_EQ(sym.normalIntSym, stype);

    // 'Something' on its own must by in the symbol table, too.
    token = sym.find("Something");
    ASSERT_LE(0, token);

    // 'Something' must have base type 'float'.
    stype = sym.entries[token].vartype;
    ASSERT_EQ(sym.normalFloatSym, stype);

    // Closing brace must have stopped Struct Mode, so 'TestIt::Outside' must NOT exist.
    token = sym.find("TestIt::Outside");
    ASSERT_GT(0, token);
}


TEST(Compile, SimpleStruct2) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct TestIt; \
            int Outside; \
        ";

    // The snippet must compile.
    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    // Closing brace must have stopped Struct Mode, so 'TestIt::Outside' must NOT exist.
    int token = sym.find("TestIt::Outside");
    ASSERT_GT(0, token);

    // 'Outside' on its own must by in the symbol table
    token = sym.find("Outside");
    ASSERT_LE(0, token);

    // 'Outside' must have base type 'int'.
    int stype = sym.entries[token].vartype;
    ASSERT_EQ(sym.normalIntSym, stype);
}

TEST(Compile, TokenizerBraceMatchTest) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        struct TestIt{ \n\
            int Outside; \n\
        ]";

    // The snippet must not compile.
    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_NE(0, compileResult);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);
}

TEST(Compile, EnumDeclarations) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "      \
        enum Foo {      \
            Bar,        \
            Baz = 5,    \
            Wup,        \
            Zok = -5,   \
            Bazz = Baz, \
        };              \
        ";

    // Snippet must compile.
    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    // Bar is 2
    int bar_token = sym.find("Bar");
    ASSERT_LE(0, bar_token);
    int bar_val = sym.entries[bar_token].soffs;
    ASSERT_EQ(1, bar_val);

    // Wup is 6
    int wup_token = sym.find("Wup");
    ASSERT_LE(0, wup_token);
    int wup_val = sym.entries[wup_token].soffs;
    ASSERT_EQ(6, wup_val);

    // Zok is -5
    int zok_token = sym.find("Zok");
    ASSERT_LE(0, zok_token);
    int zok_val = sym.entries[zok_token].soffs;
    ASSERT_EQ(-5, zok_val);

    // Bazz is 5
    int bazz_token = sym.find("Bazz");
    ASSERT_LE(0, bazz_token);
    int bazz_val = sym.entries[bazz_token].soffs;
    ASSERT_EQ(5, bazz_val);
}


TEST(Compile, ParsingIntSuccess) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 1, int data2=2, int data3=3);\
        int testfunc(int x ) { int y = 42; } \
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);
}


TEST(Compile, ParsingIntLimits1Off) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
                 import int int_limits(int param_min = -2147483649, int param_max = 2147483647);\
                 ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);

    scrip = newScriptFixture();

    inpl =       "\
                 import int int_limits(int param_min = -2147483648, int param_max = 2147483648);\
                 ";

    last_seen_cc_error = 0;
    compileResult = cc_compile(inpl, scrip);

    ASSERT_GT(0, compileResult);
}

TEST(Compile, ParsingIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 9999999999999999999999, int data2=2, int data3=3);\
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Literal value 9999999999999999999999 is too large (max. is 2147483647)", last_seen_cc_error);
}

TEST(Compile, ParsingNegIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = -9999999999999999999999, int data2=2, int data3=3);\
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Literal value '-9999999999999999999999' is too small (min. is '-2147483648')", last_seen_cc_error);
}

TEST(Compile, ParsingIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int testfunc(int x ) { int y = 4200000000000000000000; } \
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '4200000000000000000000' because of overflow.", last_seen_cc_error);
}

TEST(Compile, ParsingNegIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
                 int testfunc(int x ) { int y = -4200000000000000000000; } \
                 ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '-4200000000000000000000' because of overflow.", last_seen_cc_error);
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

    last_seen_cc_error = 0;
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
            int data3 = - 32000, \
            int data4 =32001, \
            int data5 =  2147483647, \
            int data6 = -2147483648 , \
            int data7 = -1, \
            int data8 = -2  \
            );\
        ";

    last_seen_cc_error = 0;
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

    last_seen_cc_error = 0;
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default value must be literal", last_seen_cc_error);
}

TEST(Compile, SubtractionWithoutSpaces) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int MyFunction()\
        {\
            int data0 = 2-4;\
        }\
        ";

    last_seen_cc_error = 0;
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    printf("Error: %s\n", last_seen_cc_error);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    printf("Error: %s\n", last_seen_cc_error);
    ASSERT_EQ(0, compileResult);
}
