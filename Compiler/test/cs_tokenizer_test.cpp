#include "gtest/gtest.h" 

#include <string>
#include <iostream>
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

#include "script/cs_tokenizer.h"

extern int cc_tokenize(
    const char * inpl,         // preprocessed text to be tokenized
    ccInternalList * targ,     // store for the tokenized text
    ccCompiledScript * scrip); // store for the strings in the text

extern ccCompiledScript *newScriptFixture(); // cs_parser_test.cpp

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

TEST(Tokenize, BasicStruct)
{
    sym.reset();

    // Basic function of the Tokenizer
    // Jibbling the struct name with the struct components
    std::string Input = "\
        int A; \
        struct B { \
            String B; \
            float A; \
        }; \
        int C; \
";

    struct ccInternalList TokenList;
    struct ccCompiledScript StringCollect;

    AGS::Scanner scanner(Input, 1, &TokenList);
    AGS::Tokenizer tokenizer(&scanner, &TokenList, &sym, &StringCollect);

    int token;
    bool eof_encountered;
    bool error_encountered;
    std::string token_str;
    int token_type;

    // int
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("int"));

    // A
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("A"));

    // ;
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare(";"));


    // struct 
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("struct"));
    token_type = sym.get_type(token);
    ASSERT_EQ(SYM_STRUCT, token_type);

    // B 
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("B"));
    token_type = sym.get_type(token);
    ASSERT_EQ(0, token_type); // new, unknown token

    // {
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_type = sym.get_type(token);
    ASSERT_EQ(SYM_OPENBRACE, token_type);

    // String 
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("String"));


    // B
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("B::B"));  // Important: B must be jibbled to B

    // ;
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_type = sym.get_type(token);
    ASSERT_EQ(SYM_SEMICOLON, token_type);

    // float
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("float"));

    // A
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("B::A"));  // Important: B must be jibbled to A

    // ;
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_type = sym.get_type(token);
    ASSERT_EQ(SYM_SEMICOLON, token_type);

    // }
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_type = sym.get_type(token);
    ASSERT_EQ(SYM_CLOSEBRACE, token_type);

    // ;
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);

    // int 
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);

    // C
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("C"));
    token_type = sym.get_type(token);

    // ;
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_type = sym.get_type(token);
    ASSERT_EQ(SYM_SEMICOLON, token_type);

    // EOF
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_TRUE(eof_encountered);
}

TEST(Tokenize, SectionChange)
{
    sym.reset();

    // Basic function of the Tokenizer
    // Jibbling the struct name with the struct components
    std::string Input = "\
        \n\
        String A = \"__NEWSCRIPTSTART_Foo\"; \n\
     ";

    struct ccInternalList TokenList;
    struct ccCompiledScript StringCollect;

    AGS::Scanner scanner(Input, 1, &TokenList);
    AGS::Tokenizer tokenizer(&scanner, &TokenList, &sym, &StringCollect);

    int token;
    bool eof_encountered;
    bool error_encountered;
    std::string token_str;
    int token_type;

    // String
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("String"));

    // A
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("A"));

    // =
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    token_str = sym.get_name_string(token);
    ASSERT_EQ(0, token_str.compare("="));


    // ((Section change))
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    ASSERT_FALSE(eof_encountered);
    ASSERT_FALSE(error_encountered);
    ASSERT_EQ(0, scanner.GetLineno());
}



TEST(Tokenize, MatchBraceParen1)
{
    // The tokenizer checks that nested (), [], {} match.

    // This closing ']' does not match the '{' on line 99
    std::string Input = "   \r\n\
    struct B {          \r\n\
        String B;       \r\n\
        float A;        \r\n\
    ];                  \r\n\
";

    struct ccInternalList TokenList;
    struct ccCompiledScript StringCollect;
    sym.reset();
    AGS::Scanner scanner(Input, 1, &TokenList);
    AGS::Tokenizer tokenizer(&scanner, &TokenList, &sym, &StringCollect);

    int token;
    bool eof_encountered;
    bool error_encountered;

    for (size_t count = 0; count < 9; count++)
    {
        tokenizer.GetNextToken(token, eof_encountered, error_encountered);
        if (eof_encountered || error_encountered) break;
    }

    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    EXPECT_TRUE(error_encountered);
    std::string errorstring = tokenizer.GetLastError();
    size_t ret = errorstring.find("line 2");
    EXPECT_NE(std::string::npos, ret);
}

TEST(Tokenize, MatchBraceParen2)
{
    // The tokenizer checks that nested (), [], {} match.

    struct ccInternalList TokenList;
    struct ccCompiledScript StringCollect;

    // This closing ')' does not match the '[' on this line
    std::string Input = "f(a[bb.ccc * (d + e - ( f - g)))";
    sym.reset();
    AGS::Scanner scanner(Input, 1, &TokenList);
    AGS::Tokenizer tokenizer(&scanner, &TokenList, &sym, &StringCollect);
    bool eof_encountered;
    bool error_encountered;
    int token;

    for (size_t count = 0; count < 19; count++)
    {
        tokenizer.GetNextToken(token, eof_encountered, error_encountered);
        if (eof_encountered || error_encountered) break;
    }
    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    EXPECT_TRUE(error_encountered);
    std::string errorstring = tokenizer.GetLastError();
    size_t ret = errorstring.find("this line");
}

TEST(Tokenize, MatchBraceParen3)
{
    // The tokenizer checks that nested (), [], {} match.

    // This closing ']' does not match the '{' on line 99
    std::string Input = "   \r\n\
    struct B [          \r\n\
        String B;       \r\n\
        float A;        \r\n\
    };                  \r\n\
";

    struct ccInternalList TokenList;
    struct ccCompiledScript StringCollect;
    sym.reset();
    AGS::Scanner scanner(Input, 1, &TokenList);
    AGS::Tokenizer tokenizer(&scanner, &TokenList, &sym, &StringCollect);

    int token;
    bool eof_encountered;
    bool error_encountered;

    for (size_t count = 0; count < 9; count++)
    {
        tokenizer.GetNextToken(token, eof_encountered, error_encountered);
        if (eof_encountered || error_encountered) break;
    }

    tokenizer.GetNextToken(token, eof_encountered, error_encountered);
    EXPECT_TRUE(error_encountered);
    std::string errorstring = tokenizer.GetLastError();
    size_t ret = errorstring.find("line 2");
    EXPECT_NE(std::string::npos, ret);
}
