#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

extern int cc_tokenize(const char*inpl, ccInternalList*targ, ccCompiledScript*scrip);

char *last_seen_cc_error = 0;

void cc_error_at_line(char *buffer, const char *error_msg)
{
    // printf("error: %s\n", error_msg);
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
    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    ccInternalList targ;
    int tokenizeResult = cc_tokenize(inpl, &targ, scrip);

    ASSERT_EQ(0, tokenizeResult);
}

TEST(Compile, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Syntax error at 'MyStruct::int2'; expected variable type", last_seen_cc_error);
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


TEST(Compile, ParsingIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 9999999999999999999999, int data2=2, int data3=3);\
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '9999999999999999999999' because of overflow.", last_seen_cc_error);
}

TEST(Compile, ParsingNegIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = -9999999999999999999999, int data2=2, int data3=3);\
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '-9999999999999999999999' because of overflow.", last_seen_cc_error);
}

TEST(Compile, ParsingIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        int testfunc(int x ) { int y = 4200000000000000000000; } \
        ";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Error while parsing integer symbol '4200000000000000000000'.", last_seen_cc_error);
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
    EXPECT_EQ(1, sym.soffs[sym.find("cat")]);
    EXPECT_EQ(2, sym.soffs[sym.find("dog")]);
    EXPECT_EQ(3, sym.soffs[sym.find("fish")]);

    EXPECT_EQ(100, sym.soffs[sym.find("money")]);
    EXPECT_EQ(101, sym.soffs[sym.find("death")]);
    EXPECT_EQ(102, sym.soffs[sym.find("taxes")]);

    EXPECT_EQ(-3, sym.soffs[sym.find("popularity")]);
    EXPECT_EQ(-2, sym.soffs[sym.find("x")]);
    EXPECT_EQ(-1, sym.soffs[sym.find("y")]);
    EXPECT_EQ(0, sym.soffs[sym.find("z")]);

    EXPECT_EQ((-2147483648), sym.soffs[sym.find("intmin")]);
    EXPECT_EQ((2147483647), sym.soffs[sym.find("intmax")]);
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

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    int funcidx;
    funcidx = sym.find("importedfunc");

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][1]);
    EXPECT_EQ(0, sym.funcParamDefaultValues[funcidx][1]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][2]);
    EXPECT_EQ(1, sym.funcParamDefaultValues[funcidx][2]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][3]);
    EXPECT_EQ(2, sym.funcParamDefaultValues[funcidx][3]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][4]);
    EXPECT_EQ(-32000, sym.funcParamDefaultValues[funcidx][4]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][5]);
    EXPECT_EQ(32001, sym.funcParamDefaultValues[funcidx][5]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][6]);
    EXPECT_EQ((2147483647), sym.funcParamDefaultValues[funcidx][6]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][7]);
    EXPECT_EQ((-2147483648), sym.funcParamDefaultValues[funcidx][7]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][8]);
    EXPECT_EQ(-1, sym.funcParamDefaultValues[funcidx][8]);

    EXPECT_EQ(true, sym.funcParamHasDefaultValues[funcidx][9]);
    EXPECT_EQ(-2, sym.funcParamDefaultValues[funcidx][9]);
}
