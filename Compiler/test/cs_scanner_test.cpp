#include "gtest/gtest.h" 

#include <string>
#include <iostream>
#include "script/cs_parser.h"
#include "script/cc_symboltable.h"
#include "script/cc_internallist.h"

TEST(Scanner, ShortInputBackslash)
{
    bool eofe;
    bool errore;
    std::string estr;

    std::string Input1 = "Test\\";
    struct ccInternalList TokenList1;
    sym.reset();

    AGS::Scanner scanner1(Input1, 3, &TokenList1);
    std::string symstring;
    AGS::Scanner::ScanType sct;

    // Test
    scanner1.GetNextSymstring(symstring, sct, eofe, errore);
    // EXPECT_EQ(0, symstring.compare("Test"));

    // Backslash
    scanner1.GetNextSymstring(symstring, sct, eofe, errore);
    ASSERT_TRUE(errore);

    std::string Input2 = "int i = '\\";
    struct ccInternalList TokenList2;
    sym.reset();
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


    std::string Input3 = "String s = \"a\\";
    struct ccInternalList TokenList3;
    sym.reset();
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

TEST(Scanner, ShortInputSimple)
{
    bool eofe;
    bool errore;
    std::string estr;

    std::string Input2 = "int i = ' ";
    struct ccInternalList TokenList2;
    sym.reset();
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


    std::string Input3 = "String s = \"a";
    struct ccInternalList TokenList3;
    sym.reset();
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


TEST(Scanner, TwoByteSymbols)
{
    

    bool eofe;
    bool errore;
    std::string estr;

    std::string input = "i++<=j"; // Should be i ++ <= j
    struct ccInternalList TokenList;
    sym.reset();
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


TEST(Scanner, IdentifiersElementary)
{
    sym.reset();
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

TEST(Scanner, IdentifiersNumbers)
{
    sym.reset();
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

TEST(Scanner, Strings)
{
    sym.reset();
    std::string Input =
        "\"ABC\"\n\"D\\E;\\nF\" 'G' \
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
    estr = "\"ABC\"";
    EXPECT_EQ(estr, symstring);
    EXPECT_EQ(AGS::Scanner::kSct_StringLiteral, sct);

    // Standard string, should be passed back normally.
    // "\\E" should be equivalent to "E". "\\n" shoult NOT be a newline char.
    scanner.GetNextSymstring(symstring, sct, eofe, errore);
    lno = scanner.GetLineno();
    EXPECT_EQ(false, eofe);
    EXPECT_EQ(false, errore);
    EXPECT_EQ(2, lno);
    estr = "\"D\\E;\\nF\"";
    EXPECT_EQ(estr, symstring);
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


TEST(Scanner, TokenizerBasicStruct)
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

TEST(Scanner, TokenizerMatchBraceParen)
{
    // The tokenizer checks that nested (), [], {} match.
    {
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
    {
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
}
