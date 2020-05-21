#include <string>
#include <util/string_compat.h>
#include "gtest/gtest.h"
#include "test/cc_test_helper.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"
#include "util/string.h"

extern int cc_tokenize(const char*inpl, ccInternalList*targ, ccCompiledScript*scrip);

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
    const char *inpl = ""
        "struct MyStruct "
        "{"
        "  readonly int2 a; "
        "  readonly int2 b; "
        "};";

    ccInternalList targ;
    int tokenizeResult = cc_tokenize(inpl, &targ, scrip);

    ASSERT_EQ(0, tokenizeResult);
}

TEST(Compile, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "struct MyStruct "
        "{ "
        "  readonly int2 a; "
        "  readonly int2 b; "
        "}; ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Syntax error at 'MyStruct::int2'; expected variable type", last_seen_cc_error());
}

TEST(Compile, DynamicArrayReturnValueErrorText) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "managed struct DynamicSprite { };"
        ""
        "int[] Func()"
        "{"
        "DynamicSprite *r[] = new DynamicSprite[10];"
        "return r;"
        "}";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Type mismatch: cannot convert 'DynamicSprite*[]' to 'int[]'", last_seen_cc_error());
}

TEST(Compile, DynamicTypeReturnNonPointerManaged) {
    // "DynamicSprite[]" type in the function declaration should cause error on its own, because managed types are
    // only allowed to be referenced by pointers.

    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "managed struct DynamicSprite { };"
        ""
        "DynamicSprite[] Func()"
        "{"
        "}";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("cannot pass non-pointer struct array", last_seen_cc_error());
}

TEST(Compile, StructMemberQualifierOrder) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "struct BothOrders { "
        "  protected readonly import _tryimport static attribute int something; "
        "  attribute static _tryimport import readonly writeprotected int another; "
        "  readonly import attribute int MyAttrib; "
        "  import readonly attribute int YourAttrib; "
        "};";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);
}

TEST(Compile, ParsingIntSuccess) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "import  int  importedfunc(int data1 = 1, int data2=2, int data3=3);"
        "int testfunc(int x ) { int y = 42; } "
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);
}

TEST(Compile, ParsingIntLimits) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "import int int_limits(int param_min = -2147483648, int param_max = 2147483647);"
        "int int_limits(int param_min, int param_max)"
        "{"
        "int var_min = - 2147483648;"
        "int var_max = 2147483647;"
        "}";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(0, compileResult);
}

TEST(Compile, ParsingIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "import  int  importedfunc(int data1 = 9999999999999999999999, int data2=2, int data3=3);"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '9999999999999999999999' because of overflow.", last_seen_cc_error());
}

TEST(Compile, ParsingNegIntDefaultOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "import  int  importedfunc(int data1 = -9999999999999999999999, int data2=2, int data3=3);"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '-9999999999999999999999' because of overflow.", last_seen_cc_error());
}

TEST(Compile, ParsingIntDefaultFloatMismatch) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(int data1 = 156.15);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default value must be int or 0", last_seen_cc_error());
}

TEST(Compile, ParsingFloatDefault) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(float data1 = 0, float data2=12.5, float data3=-35654.156);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, ParsingFloatDefaultIntMismatch) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        import  int  importedfunc(float data1 = 1);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default value must be float or 0", last_seen_cc_error());
}

TEST(Compile, ParsingStringDefaultNull) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct String { };\
        import  int  importedfunc(String data1 = 0);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, ParsingStringDefaultInvalid) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct String { };\
        import  int  importedfunc(String data1 = 1);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default value for type String must be 0", last_seen_cc_error());
}

TEST(Compile, ParsingPointerDefaultNull) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct Character { };\
        import  int  importedfunc(Character *data1 = 0);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, ParsingPointerDefaultInvalid) {
    ccCompiledScript *scrip = newScriptFixture();

    char *inpl = "\
        managed struct Character { };\
        import  int  importedfunc(Character *data1 = 10);\
        ";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default pointer value can only be 0", last_seen_cc_error());
}

TEST(Compile, ParsingIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "int testfunc(int x ) { int y = 4200000000000000000000; } "
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '4200000000000000000000' because of overflow.", last_seen_cc_error());
}

TEST(Compile, ParsingNegIntOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "int testfunc(int x ) { int y = -4200000000000000000000; } "
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '-4200000000000000000000' because of overflow.", last_seen_cc_error());
}


TEST(Compile, EnumNegative) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "enum TestMyEnums {"
        "cat,"
        "dog,"
        "fish,"
        "money=100,"
        "death,"
        "taxes,"
        "popularity=-3,"
        "x,"
        "y,"
        "z,"
        "intmin=-2147483648,"
        "intmax=2147483647"
        "};"
        "";

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

    const char *inpl = ""
        "import int importedfunc("
        "    int data0 = 0,"
        "    int data1 = 1, "
        "    int data2 = 2, "
        "    int data3 = -32000, "
        "    int data4 = 32001, "
        "    int data5 = 2147483647, "
        "    int data6 = -2147483648 , "
        "    int data7 = -1, "
        "    int data8 = -2  "
        ");"
        "";

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

TEST(Compile, HexLiterals) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "enum TestHex {"
        "a=0x12345,"
        "b=0xFF,"
        "c=0x7FFFFFFF,"
        "d=0xFFFFFFFF,"
        "};"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);

    EXPECT_EQ((0x12345), sym.entries[sym.find("a")].soffs);
    EXPECT_EQ((0xFF), sym.entries[sym.find("b")].soffs);
    EXPECT_EQ((0x7FFFFFFF), sym.entries[sym.find("c")].soffs);
    EXPECT_EQ((0xFFFFFFFF), sym.entries[sym.find("d")].soffs);
}

TEST(Compile, ParsingHexOverflow) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "int testfunc(int x ) { int y = 0x1FFFFFFFF; } "
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Could not parse integer symbol '0x1FFFFFFFF' because of overflow.", last_seen_cc_error());
}

TEST(Compile, ImportFunctionReturningDynamicArray) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "struct A"
        "{"
        "import static int[] MyFunc();"
        "};"
        "";

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

    const char *inpl = ""
        "import int MyFunction("
        "    int data0 = - -69"
        ");"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("Parameter default value must be literal", last_seen_cc_error());
}

TEST(Compile, SubtractionWithoutSpaces) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "int MyFunction()"
        "{"
        "    int data0 = 2-4;"
        "}"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, NegationLHSOfExpression) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "enum MyEnum"
        "{"
        "    cat"
        "};"
        ""
        "int MyFunctionA()"
        "{"
        "    return 0;"
        "}"
        ""
        "int MyFunctionB()"
        "{"
        "    int data0 = - 4 * 4;"
        "    int data1 = - MyFunctionA() * 4;"
        "    int data2 = -cat * 4;"
        "    "
        "    return 0;"
        "}"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    //printf("Error: %s\n", last_seen_cc_error());
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, NegationRHSOfExpression) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "enum MyEnum"
        "{"
        "    cat"
        "};"
        ""
        "int MyFunctionA()"
        "{"
        "    return 0;"
        "}"
        ""
        "int MyFunctionB()"
        "{"
        "    int data0 = 3 - - 4 * 4;"
        "    int data1 = 3 - - MyFunctionA() * 4;"
        "    int data2 = 3 - -cat * 4;"
        "    "
        "    return 0;"
        "}"
        "";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    //printf("Error: %s\n", last_seen_cc_error()); fails because it's interpreted as error, but left here for debug
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, CheckPropertyHandlersAreInPlace) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "managed struct A {"
        "    readonly import attribute int x;"
        "};"
        ""
        "managed struct B {"
        "    import attribute A *a;"
        "};"
        ""
        "managed struct C {"
        "    readonly import attribute B *b;"
        "};";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    //printf("Error: %s\n", last_seen_cc_error());
    ASSERT_EQ(0, compileResult);

    EXPECT_EQ(0, sym.entries[sym.find("A::x")].get_propget());
    EXPECT_EQ(-1, sym.entries[sym.find("A::x")].get_propset());
    EXPECT_EQ(1, sym.entries[sym.find("B::a")].get_propget());
    EXPECT_EQ(2, sym.entries[sym.find("B::a")].get_propset());
    EXPECT_EQ(3, sym.entries[sym.find("C::b")].get_propget());
    EXPECT_EQ(-1, sym.entries[sym.find("C::b")].get_propset());
}

TEST(Compile, AccessMembersInSequence) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "managed struct A {"
        "    import attribute int X;"
        "};"
        ""
        "managed struct B {"
        "    import attribute A *a;"
        "};"
        ""
        "managed struct C {"
        "    import attribute B *b;"
        "};"
        ""
        "int get_X(this A*)"
        "{"
        "    return 0;"
        "}"
        ""
        "A* get_a(this B*)"
        "{"
        "    return null;"
        "}"
        ""
        "B* get_b(this C*)"
        "{"
        "    return null;"
        "}"
        ""
        "void Func() {"
        "    C *c;"
        "    int a = c.b.a.X;"
        "}";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    //printf("Error: %s\n", last_seen_cc_error());
    ASSERT_EQ(0, compileResult);
}

TEST(Compile, AccessNonStaticMemberOfAType) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "managed struct A {"
        "    import attribute int x;"
        "};"
        ""
        "builtin struct B {"
        "    import readonly attribute A *a;"
        "};"
        ""
        "void Func() {"
        "    int a = B.a.x;"
        "}";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_EQ(-1, compileResult);
    EXPECT_STREQ("must have an instance of the struct to access a non-static member", last_seen_cc_error());
}

TEST(Compile, AccessNonStaticMemberOfAStaticMember) {
    ccCompiledScript *scrip = newScriptFixture();

    const char *inpl = ""
        "managed struct A {"
        "    import attribute int x;"
        "};"
        ""
        "builtin struct B {"
        "    import static readonly attribute A *a;"
        "};"
        ""
        "void Func() {"
        "    int a = B.a.x;"
        "}";

    clear_error();
    int compileResult = cc_compile(inpl, scrip);
    //printf("Error: %s\n", last_seen_cc_error());
    ASSERT_EQ(0, compileResult);
}
