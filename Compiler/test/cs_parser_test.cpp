#include "gtest/gtest.h"
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

extern int cc_tokenize(const char*inpl, ccInternalList*targ, ccCompiledScript*scrip);

char *last_seen_cc_error = 0;

void cc_error_at_line(char *buffer, const char *error_msg)
{
    last_seen_cc_error = _strdup(error_msg);
}

TEST(Tokenize, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();

    sym.reset();  // <-- global

    ccInternalList targ;

    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    int tokenizeResult = cc_tokenize(inpl, &targ, scrip);

    ASSERT_EQ(0, tokenizeResult);
}

TEST(Compile, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();

    sym.reset();  // <-- global

    char *inpl = "struct MyStruct \
        {\
          readonly int2 a; \
          readonly int2 b; \
        };";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    ASSERT_STREQ("Syntax error at 'MyStruct::int2'; expected variable type", last_seen_cc_error);
}

TEST(Compile, DynamicArrayReturnValueErrorText) {
    ccCompiledScript *scrip = new ccCompiledScript();
    scrip->init();

    sym.reset();  // <-- global

    char *inpl = "\
        managed struct DynamicSprite { };\
        \
        DynamicSprite[] Func()\
        {\
          DynamicSprite *r[] = new DynamicSprite[10];\
          return r;\
        }";

    last_seen_cc_error = 0;
    int compileResult = cc_compile(inpl, scrip);

    ASSERT_EQ(-1, compileResult);
    ASSERT_STREQ("Type mismatch: cannot convert 'DynamicSprite*[]' to 'DynamicSprite[]'", last_seen_cc_error);
}

// TODO: "DynamicSprite[]" type in the function declaration should cause error on its own, because managed types are
// only allowed to be referenced by pointers.
