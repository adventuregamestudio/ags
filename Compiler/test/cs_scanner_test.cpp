#include "gtest/gtest.h" 

#include <string>
#include <iostream>
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

#include "script/cs_scanner.h"

extern ccCompiledScript *newScriptFixture(); // cs_parser_test.cpp

TEST(Scan, ShortInputBackslash1)
{
    SymbolTable sym;
    bool eofe;
    bool errore;
    std::string estr;

    std::string Input1 = "Test\\";
    struct ccInternalList TokenList1;

    AGS::Scanner scanner1(Input1, 3, &TokenList1);
    std::string symstring;
    AGS::Scanner::ScanType sct;

    // Test
    scanner1.GetNextSymstring(symstring, sct, eofe, errore);
    // EXPECT_EQ(0, symstring.compare("Test"));

    // Backslash
    scanner1.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);
}

TEST(Scan, ShortInputBackslash2)
{
    SymbolTable sym;
    bool eofe;
    bool errore;
    std::string symstring;
    AGS::Scanner::ScanType sct;
    std::string Input2 = "int i = '\\";
    struct ccInternalList TokenList2;

    AGS::Scanner scanner2(Input2, 3, &TokenList2);
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

TEST(Scan, ShortInputBackslash3)
{
    SymbolTable sym;
    bool eofe;
    bool errore;
    std::string symstring;
    AGS::Scanner::ScanType sct;

    std::string Input3 = "String s = \"a\\";
    struct ccInternalList TokenList3;
    AGS::Scanner scanner3(Input3, 3, &TokenList3);
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

TEST(Scan, ShortInputSimple1)
{
    SymbolTable sym;
    bool eofe;
    bool errore;
    std::string estr;

    std::string Input2 = "int i = ' ";
    struct ccInternalList TokenList2;
    AGS::Scanner::ScanType sct;
    std::string symstring;
    AGS::Scanner scanner2(Input2, 3, &TokenList2);
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

TEST(Scan, ShortInputSimple2)
{
    SymbolTable sym;
    bool eofe;
    bool errore;
    std::string estr;
    std::string symstring;
    AGS::Scanner::ScanType sct;

    std::string Input3 = "String s = \"a";
    struct ccInternalList TokenList3;
    AGS::Scanner scanner3(Input3, 3, &TokenList3);
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


TEST(Scan, TwoByteSymbols)
{
    SymbolTable sym;
    bool eofe;
    bool errore;
    std::string estr;

    std::string input = "i++<=j"; // Should be i ++ <= j
    struct ccInternalList TokenList;
    AGS::Scanner::ScanType sct;
    std::string symstring;
    AGS::Scanner scanner(input, 3, &TokenList);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("i"));
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("++"));
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("<="));
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_EQ(0, symstring.compare("j"));

}

TEST(Scan, IdentifiersElementary)
{
    SymbolTable sym;
    std::string Input = "\nIdentifier\r\nIden2tifier\r\r iden_ti_9f9_ier3";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    int lno = scanner.GetLineno();
    EXPECT_EQ(4, lno);
    estr = "Identifier";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(5, lno);
    estr = "Iden2tifier";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(5, lno);
    estr = "iden_ti_9f9_ier3";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);
}

TEST(Scan, IdentifiersNumbers)
{
    SymbolTable sym;
    std::string Input = "Ident 4ify5er; _4 6.5 6996";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 1, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    int lno = scanner.GetLineno();
    EXPECT_EQ(1, lno);
    estr = "Ident";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);


    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "4";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "ify5er";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = ";";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_NonChar, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "_4";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_Identifier, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "6.5";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_FloatLiteral, sct);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "6996";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);
}

TEST(Scan, Strings)
{
    SymbolTable sym;
    std::string Input =
        "\"ABC\"\n'G' \
         \"\nH\" flurp";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 1, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;
    int lno;
    std::string errorstring;

    // Standard string, should be passed back normally.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(false, errore);
    EXPECT_EQ(1, lno);
    EXPECT_STREQ("\"ABC\"", symstring.c_str());
    EXPECT_EQ(AGS::Scanner::kSct_StringLiteral, sct);

    // Character literal, should not be a string, but an integer.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(false, errore);
    EXPECT_EQ(2, lno);
    estr = "71";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_IntLiteral, sct);

    // String containing a newline. This should be marked as erroneous.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(true, errore);
    EXPECT_EQ(2, lno);
    errorstring = scanner.GetLastError();
}

TEST(Scan, CharLit1)
{
    SymbolTable sym;
    std::string Input = "foo \'";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "foo";
    EXPECT_EQ(estr, symstring);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(eofe);
    EXPECT_TRUE(errore);
    estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("input"));

}

TEST(Scan, CharLit2)
{
    SymbolTable sym;
    std::string Input = "foo '\\";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "foo";
    EXPECT_EQ(estr, symstring);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(eofe);
    EXPECT_TRUE(errore);
    estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("input"));

}

TEST(Scan, CharLit3)
{
    SymbolTable sym;
    std::string Input = "foo \'A$";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "foo";
    EXPECT_EQ(estr, symstring);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(errore);
    estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("$"));
}

TEST(Scan, CharLit4)
{
    SymbolTable sym;
    std::string Input = "foo '\\A$";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    estr = "foo";
    EXPECT_EQ(estr, symstring);

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    EXPECT_TRUE(errore);
    estr = scanner.GetLastError();
    EXPECT_NE(std::string::npos, estr.find("nknown"));
}

TEST(Scan, CharLit5)
{
    SymbolTable sym;
    std::string Input = "'\\n'";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_FALSE(errore);
    EXPECT_STREQ("10", symstring.c_str());
}

TEST(Scan, String1)
{
    SymbolTable sym;
    std::string Input = "\"Oh, \\the \\brow\\n \\fo\\x5e jumps \\[ove\\r] the \\100\\azy dog.\"";
    struct ccInternalList TokenList;

    AGS::Scanner scanner(Input, 3, &TokenList);
    std::string symstring;
    AGS::Scanner::ScanType sct;
    bool eofe;
    bool errore;
    std::string estr;

    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_FALSE(errore);
    EXPECT_STREQ("\"Oh, \the \brow\n \fo^ jumps \\[ove\r] the @\azy dog.\"", symstring.c_str());
}
