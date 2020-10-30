#include "gtest/gtest.h" 

#include <string>
#include <iostream>
#include "script2/cs_parser.h"
#include "script2/cc_symboltable.h"
#include "script2/cc_internallist.h"

#include "script2/cs_scanner.h"

// The vars defined here are provided in each test that is in category "Scan"
class Scan : public ::testing::Test
{
protected:
    SymbolTable sym;
    std::vector<Symbol> script;
    LineHandler lh;
    size_t cursor = 0;
    SrcList token_list = AGS::SrcList(script, lh, cursor);
    struct ::ccCompiledScript string_collector;
    MessageHandler mh;
    Scanner::ScanType sct;
    std::string symstring;
};

TEST_F(Scan, ShortInputBackslash1)
{
    // Should read in an identifier and an escaped backslash.

    std::string Input1 = "Test\\";
    AGS::Scanner scanner1(Input1, token_list, string_collector, sym, mh);

    // Test
    EXPECT_EQ(0, scanner1.GetNextSymstring(symstring, sct));
    EXPECT_EQ(0, symstring.compare("Test"));

    // Backslash
    EXPECT_GT(0, scanner1.GetNextSymstring(symstring, sct));
}

TEST_F(Scan, ShortInputBackslash2)
{
    // Should read in common symbols.
    // Should detect unclosed char constant.

    std::string Input = "int i = '\\";

    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    for (size_t loop = 0; loop < 3; loop++)
    {
        ASSERT_LE(0, scanner.GetNextSymstring(symstring, sct));
        ASSERT_FALSE(scanner.EOFReached());
    }
    ASSERT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string errmsg = mh.GetError().Message;
    ASSERT_NE(std::string::npos, errmsg.find("nded instead"));
}

TEST_F(Scan, ShortInputBackslash3)
{
    // Should detect unclosed string.

    std::string Input = "String s = \"a\\";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    for (size_t loop = 0; loop < 3; loop++)
    {
        EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
        EXPECT_FALSE(scanner.EOFReached());
    }
    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    ASSERT_TRUE(scanner.EOFReached());
}

TEST_F(Scan, ShortInputSimple1)
{
    // Should detect unclosed quote mark.

    std::string Input = "int i = ' ";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    for (size_t loop = 0; loop < 3; loop++)
    {
        EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
        EXPECT_FALSE(scanner.EOFReached());
    }
    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("nded instead"));
}

TEST_F(Scan, ShortInputSimple2)
{
    // Should detect unclosed quote mark (the second one)
    std::string Input = "String s = \"a";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    for (size_t loop = 0; loop < 3; loop++)
    {
        EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
        EXPECT_FALSE(scanner.EOFReached());
    }
    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    ASSERT_TRUE(scanner.EOFReached());
}

TEST_F(Scan, ShortInputString1) {

    // String literal isn't ended

    char *Input = "\"Supercalifragilisticexpialidocious";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("nd of input"));
}

TEST_F(Scan, ShortInputString2) {

    // String literal isn't ended

    char *Input = "\"Donaudampfschiffahrtskapitaen\\";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("nd of input"));
}

TEST_F(Scan, ShortInputString3) {

    // String literal isn't ended

    char *Input = "\"Aldiborontiphoscophornio!\nWhere left you...";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("nd of line"));
}

TEST_F(Scan, TwoByteSymbols1)
{
    // Should recognize the two-byte symbols ++ and <=

    std::string Input = "i++<=j";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_EQ(0, symstring.compare("i"));
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_EQ(0, symstring.compare("++"));
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_EQ(0, symstring.compare("<="));
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_EQ(0, symstring.compare("j"));
}

TEST_F(Scan, TwoByteSymbols2)
{
    // Mustn't use '..'; it's either '.' or '...'

    std::string Input = "i .. ";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_EQ(0, symstring.compare("i"));
    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
}

TEST_F(Scan, IdentifiersElementary)
{
    // Should scan common forms of identifier.

    std::string Input = "\nIdentifier\r\nIden2tifier\r\r iden_ti_9f9_ier3";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    int lno = scanner.GetLineno();
    EXPECT_EQ(2, lno);
    EXPECT_STREQ("Identifier", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    lno = scanner.GetLineno();
    EXPECT_EQ(3, lno);
    EXPECT_STREQ("Iden2tifier", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    lno = scanner.GetLineno();
    EXPECT_EQ(3, lno);
    EXPECT_STREQ("iden_ti_9f9_ier3", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);
}

TEST_F(Scan, IdentifiersNumbers)
{
    // Should scan common forms of numbers and identifiers

    std::string Input = "Ident 4ify5er; _4 6.5 6996";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));

    int lno = scanner.GetLineno();
    EXPECT_EQ(1, lno);
    EXPECT_STREQ("Ident", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("4", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("ify5er", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ(";", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_NonChar, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("_4", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("6.5", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_FloatLiteral, sct);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("6996", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);
}

TEST_F(Scan, Strings)
{
    // Should scan tokens as noted in comments below.

    std::string Input =
        "\"ABC\"\n'G' \
         \"\nH\" flurp";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    size_t lno;
    std::string errorstring;

    // Standard string, should be passed back normally.
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    lno = scanner.GetLineno();
    EXPECT_EQ(false, scanner.EOFReached());
    EXPECT_EQ(1u, lno);
    EXPECT_STREQ("ABC", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_StringLiteral, sct);

    // Character literal, should not be a string, but an integer.
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    lno = scanner.GetLineno();
    EXPECT_EQ(false, scanner.EOFReached());
    EXPECT_EQ(2u, lno);
    EXPECT_STREQ("71", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);

    // String containing a newline. This should be marked as erroneous.
    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    lno = scanner.GetLineno();
    EXPECT_EQ(false, scanner.EOFReached());
    EXPECT_EQ(2u, lno);
}

TEST_F(Scan, StringCollect)
{
    // String literal should be copied into the strings space.
    // Names of String literals have quote marks and are mangled.
    // In the string space, they are as is, without quote marks

    std::string Input = "String s = \"Zwiebelkuchen\"; s = \"Holz\7schuh\";";

    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    EXPECT_LE(0, scanner.Scan());

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
    
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("foo", symstring.c_str());

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_TRUE(scanner.EOFReached());
    std::string estr = mh.GetError().Message;
    EXPECT_NE(std::string::npos, estr.find("input"));
}

TEST_F(Scan, CharLit2)
{
    // Should detect unclosed char literal.

    std::string Input = "foo '\\";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);
    
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("foo", symstring.c_str());

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string estr = mh.GetError().Message;
    EXPECT_NE(std::string::npos, estr.find("nded instead"));
}

TEST_F(Scan, CharLit3)
{
    // Should detect over long char literal.

    std::string Input = "foo \'A$";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("foo", symstring.c_str());

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string estr = mh.GetError().Message;
    EXPECT_NE(std::string::npos, estr.find("$"));
}

TEST_F(Scan, CharLit4)
{
    // Should complain about escape sequence \A

    std::string Input = "foo '\\A'";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("foo", symstring.c_str());

    EXPECT_GT(0, scanner.GetNextSymstring(symstring, sct));
    std::string estr = mh.GetError().Message;
    EXPECT_NE(std::string::npos, estr.find("nrecognized"));
}

TEST_F(Scan, CharLit5)
{
    // Should convert backslash combination to 10

    std::string Input = "'\\n'";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("10", symstring.c_str());
}

TEST_F(Scan, BackslashBracketInChar) {

    // Character literal '\[' is forbidden ('[' is okay)

    char *Input = "int i = '\\[';";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_GT(0, scanner.Scan());
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("'\\['"));
}

TEST_F(Scan, BackslashOctal1) {

    // "\19" is equivalent to "\1" + "9" because 9 isn't an octal digit

    char *Input = "String s = \"Boom\\19 Box\";";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("String", symstring.c_str());
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("s", symstring.c_str());
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("=", symstring.c_str());
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("Boom\1" "9 Box", symstring.c_str());
}

TEST_F(Scan, BackslashOctal2) {

    // '/' is just below the lowest digit '0'; "\7/" is equivalent to "\7" + "/"
    // Octal 444 is too large for a character, so this is equivalent to "\44" + "4"

    char *Input = "String s = \"Boom\\7/Box\\444/Borg\";";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    for (size_t symbol_idx = 4; symbol_idx --> 1 ;) // note! smiley ";)" needed
        EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("Boom\7" "/Box" "\44" "4/Borg", symstring.c_str());
}

TEST_F(Scan, BackslashOctal3) {

    // '\102' is 66 corresponds to 'B'; '\234' is 156u is -100

    char *Input = "\"b\\102b\" '\\234'";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("bBb", symstring.c_str());
    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("-100", symstring.c_str());
}

TEST_F(Scan, BackslashHex1) {

    // Expect a hex digit after '\x'

    char *Input = "\"Le\\xicon\"";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_GT(0, scanner.Scan());
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("hex digit"));
}

TEST_F(Scan, BackslashHex2) {

    // End hex when '/' is encountered; that's directly before '0'
    // End hex when '@' is encountered; that's between '9' and 'A'
    // End hex when 'g' is encountered; that's directly after 'F'
    // End hex after two hex digits

    char *Input = "\"He\\xA/meter \\xC@fe Nicolas C\\xAGE \\xFACE \"";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("He\12/meter \14@fe Nicolas C\12GE \372CE ", symstring.c_str());
}

TEST_F(Scan, BackslashOctHex) {

    
    // Test all combinations of upper and lower letters and numbers

    char *Input =
        "\" \\x19 \\x2a \\x3A \\xb4 \\xcd \\xeB \\xC5 \\xDf \\xEF \""
        "\" \\31 \\52 \\72 \\264 \\315 \\353 \\305 \\337 \\357 \"";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    std::string symstring2;
    EXPECT_LE(0, scanner.GetNextSymstring(symstring2, sct));

    EXPECT_STREQ(symstring2.c_str(), symstring.c_str());
}

TEST_F(Scan, BackslashCSym) {
    
    // Test different symbol characters after '\'

    char *Input = "\" Is \\'Java\\' \\equal to \\\"Ja\\va\\\" \\? \"";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ(" Is 'Java' \x1bqual to \"Ja\va\" ? ", symstring.c_str());
}

TEST_F(Scan, String1)
{
    // Should scan advanced escape sequences within string.

    std::string Input = "\"Oh, \\the \\brow\\n \\fo\\x5e jumps \\[ove\\r] the \\100\\azy dog.\"";
    AGS::Scanner scanner(Input, token_list, string_collector, sym, mh);

    EXPECT_LE(0, scanner.GetNextSymstring(symstring, sct));
    EXPECT_STREQ("Oh, \the \brow\n \fo^ jumps \\[ove\r] the @\azy dog.", symstring.c_str());
}

TEST_F(Scan, UnknownKeywordAfterReadonly) {
    // This incorrect code would crash the scanner.
    // Now, semantic struct parsing has been completely relocated into the parser,
    // and thus this sequence should not pose problems.

    char *inpl =   "struct MyStruct \
                    {\
                      readonly int2 a; \
                      readonly int2 b; \
                    };";

    AGS::Scanner scanner = { inpl, token_list, string_collector, sym, mh };
    ASSERT_LE(0, scanner.Scan());
}

TEST_F(Scan, SectionChange)
{
    // Should handle the section change.
    // Should not return the string token since it is a section change.

    std::string Input = "\
        \n\
        String A = \"__NEWSCRIPTSTART_Foo\"; \n\
     ";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };
    ASSERT_LE(0, scanner.Scan());
    token_list.SetCursor(0u);
    
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
    ASSERT_EQ(2u, token_list.GetLineno());
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

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };
    ASSERT_GT(0, scanner.Scan());

    std::string err = mh.GetError().Message;
    EXPECT_EQ(5u, scanner.GetLineno());
    EXPECT_NE(std::string::npos, err.find("ine 2"));
}

TEST_F(Scan, MatchBraceParen2)
{
    // The scanner checks that nested (), [], {} match.
    // "This closing ')' does not match the '[' on this line"

    std::string Input = "f(a[bb.ccc * (d + e - ( f - g)))";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };
    ASSERT_GE(0, scanner.Scan());
    EXPECT_EQ(1u, scanner.GetLineno());
    EXPECT_NE(std::string::npos, mh.GetError().Message.find("this line"));
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

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };
    ASSERT_GT(0, scanner.Scan());
    EXPECT_EQ(5u, scanner.GetLineno());
    EXPECT_NE(std::string::npos, mh.GetError().Message.find("ine 2"));
}

TEST_F(Scan, MatchBraceParen4)
{
    // The scanner checks that nested (), [], {} match.
    // Closer without opener

    std::string Input = "struct B );";

    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };
    ASSERT_GT(0, scanner.Scan());
    EXPECT_EQ(1u, scanner.GetLineno());
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("matches the closing"));
}

TEST_F(Scan, MatchBraceParen5)
{
    // The scanner checks that nested (), [], {} match.
    // Opener without closer

    char *Input = "\
            struct MyStruct \n\
            {               \n\
                int i;      \n\
            } S;            \n\
            void Test()     \n\
            {               \n\
                S.          \n\
        ";
    AGS::Scanner scanner = { Input, token_list, string_collector, sym, mh };
    ASSERT_GT(0, scanner.Scan());
    EXPECT_LE(7u, scanner.GetLineno());
    std::string errmsg = mh.GetError().Message;
    EXPECT_NE(std::string::npos, errmsg.find("not been closed"));
    EXPECT_NE(std::string::npos, errmsg.find("ine 6"));
}
