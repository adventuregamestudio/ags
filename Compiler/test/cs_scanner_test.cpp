#include "gtest/gtest.h" 

#include <string>
#include <iostream>
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

#include "script/cs_scanner.h"

extern ccCompiledScript *newScriptFixture(); // cs_parser_test.cpp

// The vars defined here are provided in each test that is in category "Scan"
class Scan : public ::testing::Test
{
protected:
    SymbolTable sym;
    std::vector<Symbol> script;
    AGS::LineHandler lh;
    AGS::SrcList token_list = { script, lh };
    struct ::ccCompiledScript string_collector;
    AGS::Scanner::ScanType sct;
    std::string symstring;
    bool eofe;
    bool errore;
};

TEST_F(Scan, ShortInputBackslash1)
{
    // Should read in an identifier and an escaped backslash.

    std::string Input1 = "Test\\";
    AGS::Scanner scanner1(Input1, token_list, string_collector, sym);

    // Test
    scanner1.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("Test"));

    // Backslash
    scanner1.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);
}

TEST_F(Scan, ShortInputBackslash2)
{
    // Should read in common symbols.
    // Should detect unclosed char constant.

    std::string Input2 = "int i = '\\";

    AGS::Scanner scanner2(Input2, token_list, string_collector, sym);
    for (size_t loop = 0; loop < 3; loop++)
    {
        scanner2.GetNextSymstring(symstring, sct, eofe, errore);
        ASSERT_FALSE(errore);
        ASSERT_FALSE(eofe);
    }
    scanner2.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);
    ASSERT_TRUE(eofe);
}

TEST_F(Scan, ShortInputBackslash3)
{
    // Should detect unclosed string.

    std::string Input3 = "String s = \"a\\";
    struct ccInternalList TokenList3;
    AGS::Scanner scanner3(Input3, token_list, string_collector, sym);
    for (size_t loop = 0; loop < 3; loop++)
    {
        scanner3.GetNextSymstring(symstring, sct, eofe, errore);
        EXPECT_FALSE(errore);
        EXPECT_FALSE(eofe);
    }
    scanner3.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);
    ASSERT_TRUE(eofe);
}

TEST_F(Scan, ShortInputSimple1)
{
    // Should detect unclosed quote mark.

    std::string Input2 = "int i = ' ";
    AGS::Scanner scanner2(Input2, token_list, string_collector, sym);
    for (size_t loop = 0; loop < 3; loop++)
    {
        scanner2.GetNextSymstring(symstring, sct, eofe, errore);
        EXPECT_FALSE(errore);
        EXPECT_FALSE(eofe);
    }
    scanner2.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);
    ASSERT_TRUE(eofe);
}

TEST_F(Scan, ShortInputSimple2)
{
    // Should detect unclosed quote mark (the second one)
    std::string Input3 = "String s = \"a";
    AGS::Scanner scanner3(Input3, token_list, string_collector, sym);
    for (size_t loop = 0; loop < 3; loop++)
    {
        scanner3.GetNextSymstring(symstring, sct, eofe, errore);
        EXPECT_FALSE(errore);
        EXPECT_FALSE(eofe);
    }
    scanner3.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);
    ASSERT_TRUE(eofe);
}

TEST_F(Scan, TwoByteSymbols)
{
    // Should recognize the two-byte symbols ++ and <=

    std::string Input3 = "i++<=j";
    AGS::Scanner scanner(Input3, token_list, string_collector, sym);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("i"));
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("++"));
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("<="));
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("j"));
}

TEST_F(Scan, IdentifiersElementary)
{
    // Should scan common forms of identifier.

    std::string Input = "\nIdentifier\r\nIden2tifier\r\r iden_ti_9f9_ier3";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    int lno = scanner.GetLineno();
    EXPECT_EQ(2, lno);
    EXPECT_STREQ("Identifier", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(3, lno);
    EXPECT_STREQ("Iden2tifier", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(3, lno);
    EXPECT_STREQ("iden_ti_9f9_ier3", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);
}

TEST_F(Scan, IdentifiersNumbers)
{
    // Should scan common forms of numbers and identifiers

    std::string Input = "Ident 4ify5er; _4 6.5 6996";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);
    scanner.GetNextSymstring(symstring, sct, eofe, errore);

    int lno = scanner.GetLineno();
    EXPECT_EQ(1, lno);
    EXPECT_STREQ("Ident", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("4", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("ify5er", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ(";", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_NonChar, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("_4", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("6.5", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_FloatLiteral, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("6996", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);
}

TEST_F(Scan, Strings)
{
    // Should scan tokens as noted in comments below.

    std::string Input =
        "\"ABC\"\n'G' \
         \"\nH\" flurp";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);
    int lno;
    std::string errorstring;

    // Standard string, should be passed back normally.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(false, errore);
    EXPECT_EQ(1, lno);
    EXPECT_STREQ("ABC", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_StringLiteral, sct);

    // Character literal, should not be a string, but an integer.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(false, errore);
    EXPECT_EQ(2, lno);
    EXPECT_STREQ("71", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);

    // String containing a newline. This should be marked as erroneous.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(true, errore);
    EXPECT_EQ(2, lno);
    errorstring = scanner.GetLastError();
}

TEST_F(Scan, StringCollect)
{
    // String literal should be copied into the strings space.
    // Names of String literals have quote marks and are mangled.
    // In the string space, they are as is, without quote marks

    std::string Input = "String s = \"Zwiebelkuchen\"; s = \"Holz\7schuh\";";

    AGS::Scanner scanner(Input, token_list, string_collector, sym);
    scanner.Scan(errore);
    ASSERT_FALSE(errore);

    std::string text_in_buffer;
    
    Symbol const s1 = sym.Find("\"Zwiebelkuchen\"");
    ASSERT_LT(0, s1);
    int const pos1 = sym[s1].SOffset;
    ASSERT_LE(0, pos1);
    text_in_buffer.assign(string_collector.strings + pos1);
    ASSERT_EQ("Zwiebelkuchen", text_in_buffer);

    Symbol const s2 = sym.Find("\"Holz\\x07schuh\"");
    ASSERT_LT(0, s2);
    int const pos2 = sym[s2].SOffset;
    ASSERT_LE(0, pos2);
    text_in_buffer.assign(string_collector.strings + pos2);
    ASSERT_EQ("Holz\x07schuh", text_in_buffer);
}

TEST_F(Scan, CharLit1)
{
    // Should detect unclosed char literal.

    std::string Input = "foo \'";
    
    AGS::Scanner scanner(Input, token_list, string_collector, sym);
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("foo", symstring.c_str());

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(eofe);
    EXPECT_TRUE(errore);
    std::string estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("input"));
}

TEST_F(Scan, CharLit2)
{
    // Should detect unclosed char literal.

    std::string Input = "foo '\\";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);
    
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("foo", symstring.c_str());

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(eofe);
    EXPECT_TRUE(errore);
    std::string estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("input"));
}

TEST_F(Scan, CharLit3)
{
    // Should detect over long char literal.

    std::string Input = "foo \'A$";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("foo", symstring.c_str());

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(errore);
    std::string estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("$"));
}

TEST_F(Scan, CharLit4)
{
    // Should detect overlong char literal

    std::string Input = "foo '\\A$";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_STREQ("foo", symstring.c_str());

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(errore);
    std::string estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("nknown"));
}

TEST_F(Scan, CharLit5)
{
    // Should convert backslash combination to 10

    std::string Input = "'\\n'";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym };

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_FALSE(errore);
    EXPECT_STREQ("10", symstring.c_str());
}

TEST_F(Scan, String1)
{
    // Should scan advanced escape sequences within string.

    std::string Input = "\"Oh, \\the \\brow\\n \\fo\\x5e jumps \\[ove\\r] the \\100\\azy dog.\"";
    AGS::Scanner scanner(Input, token_list, string_collector, sym);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_FALSE(errore);
    EXPECT_STREQ("Oh, \the \brow\n \fo^ jumps \\[ove\r] the @\azy dog.", symstring.c_str());
}

TEST_F(Scan, UnknownKeywordAfterReadonly) {
    ccCompiledScript *scrip = newScriptFixture();
    SymbolTable sym;

    // This incorrect code would crash the scanner.
    // Now, semantic struct parsing has been completely relocated into the parser,
    // and thus this sequence should not pose problems.

    char *inpl =   "struct MyStruct \
                    {\
                      readonly int2 a; \
                      readonly int2 b; \
                    };";

    AGS::Scanner scanner = { inpl, token_list, string_collector, sym };
    scanner.Scan(errore);

    ASSERT_EQ(false, errore);
}

TEST_F(Scan, SectionChange)
{
    // Should handle the section change.
    // Should not return the string token since it is a section change.

    std::string Input = "\
        \n\
        String A = \"__NEWSCRIPTSTART_Foo\"; \n\
     ";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym };
    scanner.Scan(errore);
    token_list.SetCursor(0);
    
    // String
    ASSERT_FALSE(token_list.ReachedEOF());
    AGS::Symbol s = token_list.GetNext();
    std::string token_str = sym.GetName(s);
    ASSERT_EQ(0, token_str.compare("String"));

    // A
    ASSERT_FALSE(token_list.ReachedEOF());
    s = token_list.GetNext();
    token_str = sym.GetName(s);
    ASSERT_EQ(0, token_str.compare("A"));

    // =
    ASSERT_FALSE(token_list.ReachedEOF());
    s = token_list.GetNext();
    token_str = sym.GetName(s);
    ASSERT_EQ(0, token_str.compare("="));

    // ((Section change)) ;
    ASSERT_FALSE(token_list.ReachedEOF());
    ASSERT_EQ(0, token_list.GetLineno());
    s = token_list.GetNext();
}

TEST_F(Scan, MatchBraceParen1)
{
    // The scanner checks that nested (), [], {} match.
    // "This closing ']' does not match the '{' on line"

    std::string Input = "   \r\n\
    struct B {          \r\n\
        String B;       \r\n\
        float A;        \r\n\
    ];                  \r\n\
    ";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym };
    scanner.Scan(errore);
    ASSERT_TRUE(errore);

    EXPECT_EQ(5, scanner.GetLineno());
    EXPECT_NE(std::string::npos, scanner.GetLastError().find("ine 2"));
}

TEST_F(Scan, MatchBraceParen2)
{
    // The scanner checks that nested (), [], {} match.
    // "This closing ')' does not match the '[' on this line"

    std::string Input = "f(a[bb.ccc * (d + e - ( f - g)))";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym };
    scanner.Scan(errore);
    ASSERT_TRUE(errore);
    EXPECT_EQ(1, scanner.GetLineno());
    EXPECT_NE(std::string::npos, scanner.GetLastError().find("this line"));
}

TEST_F(Scan, MatchBraceParen3)
{
    // The scanner checks that nested (), [], {} match.
    // "This closing ']' does not match the '{' on line 2"

    std::string Input = "   \r\n\
    struct B [          \r\n\
        String B;       \r\n\
        float A;        \r\n\
    };";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym };
    scanner.Scan(errore);
    ASSERT_TRUE(errore);
    EXPECT_EQ(5, scanner.GetLineno());
    EXPECT_NE(std::string::npos, scanner.GetLastError().find("ine 2"));
}
