#include <string>

#include "gtest/gtest.h"
#include "cc_bytecode_test_lib.h"
#include "cc_parser_test_lib.h"

#include "script/cc_common.h"

#include "script2/cs_parser.h"


/* This file is for Bytecode tests ONLY.
   See comments for cc_bytecode_test_0.cpp
*/


// The vars defined here are provided in each test that is in category "Bytecode0"
class Bytecode1 : public ::testing::Test
{
protected:
    AGS::ccCompiledScript scrip{ true }; // enable LINUM directives

    Bytecode1()
    {
        // Initializations, will be done at the start of each test
        // Note, the parser doesn't react to SCOPT_LINENUMBERS, that's on ccCompiledScript
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        clear_error();
    }
};

TEST_F(Bytecode1, StringOldstyle01) {
    ccSetOption(SCOPT_OLDSTRINGS, true);

    char const *inpl = "\
        int Sentinel1;              \n\
        string GLOBAL;              \n\
        int Sentinel2;              \n\
                                    \n\
        string MyFunction(int a)    \n\
        {                           \n\
            string x;               \n\
            char   Sentinel3;       \n\
            return GLOBAL;          \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle01", scrip);

    size_t const codesize = 34;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    6,   38,    0,           36,    7,   51,    0,    // 7
      63,  200,    1,    1,          200,   36,    8,   51,    // 15
       0,   63,    1,    1,            1,    1,   36,    9,    // 23
       6,    2,    4,    3,            2,    3,    2,    1,    // 31
     201,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      26,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, StringOldstyle02) {

    char const *inpl = "\
        int sub(const string s) \n\
        {                       \n\
            return;             \n\
        }                       \n\
                                \n\
        int main()              \n\
        {                       \n\
            sub(\"Foo\");       \n\
        }                       \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle02", scrip);

    size_t const codesize = 35;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,    5,   36,    7,           38,   10,   36,    8,    // 15
       6,    3,    0,   29,            3,    6,    3,    0,    // 23
      23,    3,    2,    1,            4,   36,    9,    6,    // 31
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,   23,  -999
    };
    char fixuptypes[] = {
      3,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 4;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'F',  'o',  'o',    0,          '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringOldstyle03) {
    
    char const *inpl = "\
        int Sentinel1;                  \n\
        string Global;                  \n\
        int Sentinel2;                  \n\
                                        \n\
        void ModifyString(string Parm)  \n\
        {                               \n\
            Parm = \"Parameter\";       \n\
        }                               \n\
                                        \n\
        int main()                      \n\
        {                               \n\
            ModifyString(Global);       \n\
        }                               \n\
        ";
	
	ccSetOption(SCOPT_OLDSTRINGS, true);
	
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle03", scrip);

    size_t const codesize = 88;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    6,   38,    0,           36,    7,    6,    3,    // 7
       0,   51,    8,    3,            3,    5,    3,    2,    // 15
       4,    6,    7,  199,            3,    4,    2,    7,    // 23
       3,    3,    5,    2,            8,    3,   28,   25,    // 31
       1,    4,    1,    1,            5,    1,    2,    7,    // 39
       1,    3,    7,    3,           70,  -26,    1,    5,    // 47
       1,    3,    5,    2,            6,    3,    0,    8,    // 55
       3,   36,    8,    5,           36,   11,   38,   60,    // 63
      36,   12,    6,    2,            4,    3,    2,    3,    // 71
      29,    3,    6,    3,            0,   23,    3,    2,    // 79
       1,    4,   36,   13,            6,    3,    0,    5,    // 87
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   68,   76,  -999
    };
    char fixuptypes[] = {
      3,   1,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 10;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'P',  'a',  'r',  'a',          'm',  'e',  't',  'e',     // 7
    'r',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringOldstyle04) {
    
    char const *inpl = "\
        int Sentinel;                   \n\
        string Global;                  \n\
        int main()                      \n\
        {                               \n\
            string Local = Func(Global); \n\
        }                               \n\
        string Func(string Param)       \n\
        {                               \n\
            return Param;               \n\
        }                               \n\
        ";
		
	ccSetOption(SCOPT_OLDSTRINGS, true);

	int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle04", scrip);

    size_t const codesize = 94;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,    6,    2,    // 7
       4,    3,    2,    3,           29,    3,    6,    3,    // 15
      82,   23,    3,    2,            1,    4,   51,    0,    // 23
       3,    3,    5,    3,            2,    4,    6,    7,    // 31
     199,    3,    4,    2,            7,    3,    3,    5,    // 39
       2,    8,    3,   28,           25,    1,    4,    1,    // 47
       1,    5,    1,    2,            7,    1,    3,    7,    // 55
       3,   70,  -26,    1,            5,    1,    3,    5,    // 63
       2,    6,    3,    0,            8,    3,    1,    1,    // 71
     200,   36,    6,    2,            1,  200,    6,    3,    // 79
       0,    5,   36,    8,           38,   82,   36,    9,    // 87
      51,    8,    3,    2,            3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   16,  -999
    };
    char fixuptypes[] = {
      1,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, StringOldstyle05) {
    
    char const *inpl = "\
        int main()                  \n\
        {                           \n\
            string S3 = \"Holz-\";  \n\
            string S4;              \n\
            S4 = \"-Schuh\";        \n\
        }                           \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

	int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringOldstyle05", scrip);

    size_t const codesize = 131;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   51,    0,    3,            3,    5,    3,    2,    // 15
       4,    6,    7,  199,            3,    4,    2,    7,    // 23
       3,    3,    5,    2,            8,    3,   28,   25,    // 31
       1,    4,    1,    1,            5,    1,    2,    7,    // 39
       1,    3,    7,    3,           70,  -26,    1,    5,    // 47
       1,    3,    5,    2,            6,    3,    0,    8,    // 55
       3,    1,    1,  200,           36,    4,   51,    0,    // 63
      63,  200,    1,    1,          200,   36,    5,    6,    // 71
       3,    6,   51,  200,            3,    3,    5,    3,    // 79
       2,    4,    6,    7,          199,    3,    4,    2,    // 87
       7,    3,    3,    5,            2,    8,    3,   28,    // 95
      25,    1,    4,    1,            1,    5,    1,    2,    // 103
       7,    1,    3,    7,            3,   70,  -26,    1,    // 111
       5,    1,    3,    5,            2,    6,    3,    0,    // 119
       8,    3,   36,    6,            2,    1,  400,    6,    // 127
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   73,  -999
    };
    char fixuptypes[] = {
      3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 13;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'o',  'l',  'z',          '-',    0,  '-',  'S',     // 7
    'c',  'h',  'u',  'h',            0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringStandard01) {

    char const *inpl = "\
        int main()                         \n\
        {                                  \n\
            String s = \"Hello, world!\";  \n\
            if (s != \"Bye\")              \n\
                return 1;                  \n\
            return 0;                      \n\
        }                                  \n\
        ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;


    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringStandard01", scrip);

    size_t const codesize = 63;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   64,    3,   51,            0,   47,    3,    1,    // 15
       1,    4,   36,    4,           51,    4,   48,    3,    // 23
      29,    3,    6,    3,           14,   30,    4,   66,    // 31
       4,    3,    3,    4,            3,   28,   12,   36,    // 39
       5,    6,    3,    1,           51,    4,   49,    2,    // 47
       1,    4,    5,   36,            6,    6,    3,    0,    // 55
      51,    4,   49,    2,            1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   28,  -999
    };
    char fixuptypes[] = {
      3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 18;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'e',  'l',  'l',          'o',  ',',  ' ',  'w',     // 7
    'o',  'r',  'l',  'd',          '!',    0,  'B',  'y',     // 15
    'e',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringStandard02) {
    
    char const *inpl = "\
        String S;                           \n\
        import String I;                    \n\
        String Func1()                      \n\
        {                                   \n\
            return S;                       \n\
        }                                   \n\
        String Func2(String P)              \n\
        {                                   \n\
            return P;                       \n\
        }                                   \n\
        String Func3()                      \n\
        {                                   \n\
            String L = \"Hello!\";          \n\
            return L;                       \n\
        }                                   \n\
        String Func4()                      \n\
        {                                   \n\
            return \"Hello!\";              \n\
        }                                   \n\
        ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

	int compileResult = cc_compile(input, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringStandard02", scrip);

    size_t const codesize = 101;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,    6,    2,    // 7
       0,   48,    3,    5,           36,    8,   38,   12,    // 15
      51,    8,    7,    3,           50,    3,   36,    9,    // 23
      51,    8,   48,    3,           29,    3,   51,    4,    // 31
      50,    3,   51,   12,           49,   51,    4,   48,    // 39
       3,   69,   30,    4,            5,   36,   12,   38,    // 47
      45,   36,   13,    6,            3,    7,   64,    3,    // 55
      51,    0,   47,    3,            1,    1,    4,   36,    // 63
      14,   51,    4,   48,            3,   29,    3,   51,    // 71
       4,   50,    3,   51,            8,   49,   51,    4,    // 79
      48,    3,   69,   30,            4,    2,    1,    4,    // 87
       5,   36,   17,   38,           89,   36,   18,    6,    // 95
       3,    7,   64,    3,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   53,   97,  -999
    };
    char fixuptypes[] = {
      1,   3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 14;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'e',  'l',  'l',          'o',  '!',    0,  'H',     // 7
    'e',  'l',  'l',  'o',          '!',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringStandardOldstyle) {

    char const *inpl = "\
        string OS;                          \n\
        String Func1()                      \n\
        {                                   \n\
            return OS;                      \n\
        }                                   \n\
        String Func2(String P)              \n\
        {                                   \n\
            return P;                       \n\
        }                                   \n\
        int Func3(const string OP)          \n\
        {                                   \n\
            Func2(\"Hello\");               \n\
        }                                   \n\
        void Func4()                        \n\
        {                                   \n\
            String S = \"Hello\";           \n\
            Func3(S);                       \n\
        }                                   \n\
        ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringStandardOldstyle", scrip);

    size_t const codesize = 120;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    2,    // 7
       0,    3,    2,    3,           64,    3,    5,   36,    // 15
       7,   38,   15,   51,            8,    7,    3,   50,    // 23
       3,   36,    8,   51,            8,   48,    3,   29,    // 31
       3,   51,    4,   50,            3,   51,   12,   49,    // 39
      51,    4,   48,    3,           69,   30,    4,    5,    // 47
      36,   11,   38,   48,           36,   12,    6,    3,    // 55
       6,   64,    3,   29,            3,    6,    3,   15,    // 63
      23,    3,    2,    1,            4,   36,   13,    6,    // 71
       3,    0,    5,   36,           15,   38,   75,   36,    // 79
      16,    6,    3,    6,           64,    3,   51,    0,    // 87
      47,    3,    1,    1,            4,   36,   17,   51,    // 95
       4,   48,    3,   67,            3,   29,    3,    6,    // 103
       3,   48,   23,    3,            2,    1,    4,   36,    // 111
      18,   51,    4,   49,            2,    1,    4,    5,    // 119
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   56,   63,   83,        105,  -999
    };
    char fixuptypes[] = {
      1,   3,   2,   3,      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 12;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'e',  'l',  'l',          'o',    0,  'H',  'e',     // 7
    'l',  'l',  'o',    0,          '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, AccessStructAsPointer01) {
    
    // Managed structs can be declared without (implicit) pointer iff:
    // - they are "import" globals
    // - the struct is "builtin" as well as "managed".
    // Such structs can be used as a parameter of a function that expects a pointered struct

    char const *inpl = "\
        builtin managed struct Object {                 \n\
        };                                              \n\
        import Object oCleaningCabinetDoor;             \n\
                                                        \n\
        builtin managed struct Character                \n\
        {                                               \n\
            import int FaceObject(Object *);            \n\
        };                                              \n\
        import readonly Character *player;              \n\
                                                        \n\
        int oCleaningCabinetDoor_Interact()             \n\
        {                                               \n\
            player.FaceObject(oCleaningCabinetDoor);    \n\
        }                                               \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("AccessStructAsPointer01", scrip);

    size_t const codesize = 45;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   12,   38,    0,           36,   13,    6,    2,    // 7
       2,   48,    2,   52,           29,    2,    6,    2,    // 15
       0,    3,    2,    3,           34,    3,   51,    4,    // 23
       7,    2,   45,    2,           39,    1,    6,    3,    // 31
       1,   33,    3,   35,            1,   30,    2,   36,    // 39
      14,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   16,   32,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 3;
    std::string imports[] = {
    "oCleaningCabinetDoor",       "Character::FaceObject^1",    "player",      // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, AccessStructAsPointer02) {
    
    // Managed structs can be declared without (implicit) pointer in certain circumstances.
    // Such structs can be assigned to a variable that is a pointered struct

    char const *inpl = "\
        builtin managed struct Object { };              \n\
        import Object oCleaningCabinetDoor;             \n\
                                                        \n\
        builtin managed struct Character                \n\
        {                                               \n\
            import int FaceObject(Object *);            \n\
        };                                              \n\
        import readonly Character *player;              \n\
                                                        \n\
        int oCleaningCabinetDoor_Interact()             \n\
        {                                               \n\
            Object o1 = oCleaningCabinetDoor;           \n\
            player.FaceObject(o1);                      \n\
        }                                               \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("AccessStructAsPointer02", scrip);

    size_t const codesize = 64;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   11,   38,    0,           36,   12,    6,    2,    // 7
       0,    3,    2,    3,           51,    0,   47,    3,    // 15
       1,    1,    4,   36,           13,    6,    2,    2,    // 23
      48,    2,   52,   29,            2,   51,    8,   48,    // 31
       3,   34,    3,   51,            4,    7,    2,   45,    // 39
       2,   39,    1,    6,            3,    1,   33,    3,    // 47
      35,    1,   30,    2,           36,   14,   51,    4,    // 55
      49,    2,    1,    4,            6,    3,    0,    5,    // 63
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   23,   45,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 3;
    std::string imports[] = {
    "oCleaningCabinetDoor",       "Character::FaceObject^1",    "player",      // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, AccessStructAsPointer03) {
    
    // Managed structs can be declared without (implicit) pointer in certain circumstances.
    // Such structs can be assigned to a variable that is a pointered struct

    char const *inpl = "\
        builtin managed struct Object {                 \n\
            readonly int Reserved;                      \n\
        };                                              \n\
        import Object object[40];                       \n\
                                                        \n\
        builtin managed struct Character                \n\
        {                                               \n\
            import int FaceObject(Object *);            \n\
        };                                              \n\
        import readonly Character *player;              \n\
                                                        \n\
        int oCleaningCabinetDoor_Interact()             \n\
        {                                               \n\
            Object o2 = object[5];                      \n\
        }                                               \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("AccessStructAsPointer03", scrip);

    size_t const codesize = 34;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   13,   38,    0,           36,   14,    6,    2,    // 7
       0,    1,    2,   20,            3,    2,    3,   51,    // 15
       0,   47,    3,    1,            1,    4,   36,   15,    // 23
      51,    4,   49,    2,            1,    4,    6,    3,    // 31
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "object",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes01) {

    char const *inpl = "\
        enum bool { false = 0, true = 1, };              \n\
        builtin managed struct ViewFrame {              \n\
            readonly import attribute bool Flipped;     \n\
            import attribute int Graphic;               \n\
            readonly import attribute float AsFloat;    \n\
        };                                              \n\
                                                        \n\
        int main()                                      \n\
        {                                               \n\
            ViewFrame *VF;                              \n\
            if (VF.Flipped)                             \n\
            {                                           \n\
                VF.Graphic = 17;                        \n\
                float f = VF.AsFloat + VF.AsFloat;      \n\
                return VF.Graphic;                      \n\
            }                                           \n\
            return VF.Flipped;                          \n\
        }                                               \n\
        ";


    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes01", scrip);

    size_t const codesize = 170;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    9,   38,    0,           36,   10,   51,    0,    // 7
      49,    1,    1,    4,           36,   11,   51,    4,    // 15
      48,    2,   52,   29,            6,   45,    2,   39,    // 23
       0,    6,    3,    0,           33,    3,   30,    6,    // 31
      28,  109,   36,   13,           51,    4,   48,    2,    // 39
      52,    6,    3,   17,           29,    6,   34,    3,    // 47
      45,    2,   39,    1,            6,    3,    2,   33,    // 55
       3,   35,    1,   30,            6,   36,   14,   51,    // 63
       4,   48,    2,   52,           29,    6,   45,    2,    // 71
      39,    0,    6,    3,            3,   33,    3,   30,    // 79
       6,   29,    3,   51,            8,   48,    2,   52,    // 87
      29,    6,   45,    2,           39,    0,    6,    3,    // 95
       3,   33,    3,   30,            6,   30,    4,   57,    // 103
       4,    3,    3,    4,            3,   29,    3,   36,    // 111
      15,   51,    8,   48,            2,   52,   29,    6,    // 119
      45,    2,   39,    0,            6,    3,    1,   33,    // 127
       3,   30,    6,   51,            8,   49,    2,    1,    // 135
       8,    5,   36,   16,            2,    1,    4,   36,    // 143
      17,   51,    4,   48,            2,   52,   29,    6,    // 151
      45,    2,   39,    0,            6,    3,    0,   33,    // 159
       3,   30,    6,   51,            4,   49,    2,    1,    // 167
       4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      27,   54,   76,   96,        126,  158,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 4;
    std::string imports[] = {
    "ViewFrame::get_Flipped^0",   "ViewFrame::get_Graphic^0",   "ViewFrame::set_Graphic^1",   // 2
    "ViewFrame::get_AsFloat^0",    "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes02) {

    // The getter and setter functions are defined locally, so
    // they ought to be exported instead of imported.
    // Assigning to the attribute should generate the same call
    // as calling the setter; reading the same as calling the getter.
    // 'Armor::' functions should be allowed to access '_Damage'.

    char const *inpl = "\
        managed struct Armor {                          \n\
            attribute int Damage;                       \n\
            writeprotected short _Aura;                 \n\
            protected int _Damage;                      \n\
        };                                              \n\
                                                        \n\
        int main()                                      \n\
        {                                               \n\
            Armor *armor = new Armor;                   \n\
            armor.Damage = 17;                          \n\
            return (armor.Damage > 10);                 \n\
        }                                               \n\
                                                        \n\
        void Armor::set_Damage(int damage)              \n\
        {                                               \n\
            if (damage >= 0)                            \n\
                _Damage = damage;                       \n\
        }                                               \n\
                                                        \n\
        int Armor::get_Damage()                         \n\
        {                                               \n\
            return _Damage;                             \n\
        }                                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes02", scrip);

    size_t const codesize = 139;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,   73,    3,    // 7
       8,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,   10,   51,    4,           48,    2,   52,    6,    // 23
       3,   17,   29,    6,           29,    3,   45,    2,    // 31
       6,    3,   80,   23,            3,    2,    1,    4,    // 39
      30,    6,   36,   11,           51,    4,   48,    2,    // 47
      52,   29,    6,   45,            2,    6,    3,  123,    // 55
      23,    3,   30,    6,           29,    3,    6,    3,    // 63
      10,   30,    4,   17,            4,    3,    3,    4,    // 71
       3,   51,    4,   49,            2,    1,    4,    5,    // 79
      36,   15,   38,   80,           36,   16,   51,    8,    // 87
       7,    3,   29,    3,            6,    3,    0,   30,    // 95
       4,   19,    4,    3,            3,    4,    3,   28,    // 103
      15,   36,   17,   51,            8,    7,    3,    3,    // 111
       6,    2,   52,    1,            2,    2,    8,    3,    // 119
      36,   18,    5,   36,           21,   38,  123,   36,    // 127
      22,    3,    6,    2,           52,    1,    2,    2,    // 135
       7,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      34,   55,  -999
    };
    char fixuptypes[] = {
      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes03) {

    // The getters and setters are NOT defined locally here, 
    // so import decls should be generated for them.
    // The getters and setters should be called as import funcs.

    char const *inpl = "\
        managed struct Armor {                          \n\
            attribute int Damage;                       \n\
            writeprotected short _aura;                 \n\
            protected int _damage;                      \n\
        };                                              \n\
                                                        \n\
        int main()                                      \n\
        {                                               \n\
            Armor *armor = new Armor;                   \n\
            armor.Damage = 17;                          \n\
            return (armor.Damage > 10);                 \n\
        }";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes03", scrip);

    size_t const codesize = 83;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,   73,    3,    // 7
       8,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,   10,   51,    4,           48,    2,   52,    6,    // 23
       3,   17,   29,    6,           34,    3,   45,    2,    // 31
      39,    1,    6,    3,            1,   33,    3,   35,    // 39
       1,   30,    6,   36,           11,   51,    4,   48,    // 47
       2,   52,   29,    6,           45,    2,   39,    0,    // 55
       6,    3,    0,   33,            3,   30,    6,   29,    // 63
       3,    6,    3,   10,           30,    4,   17,    4,    // 71
       3,    3,    4,    3,           51,    4,   49,    2,    // 79
       1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      36,   58,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 2;
    std::string imports[] = {
    "Armor::get_Damage^0",        "Armor::set_Damage^1",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes04) {
    
    // Attribute func was not called properly

    char const *inpl = "\
        builtin managed struct Character {      \n\
            import attribute int  x;            \n\
        };                                      \n\
        import readonly Character *player;      \n\
                                                \n\
        int LinkattusStoop(int x, int y)        \n\
        {                                       \n\
            player.x += 77;                     \n\
        }                                       \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes04", scrip);

    size_t const codesize = 64;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    3,    // 7
      77,   29,    3,    6,            2,    2,   48,    2,    // 15
      52,   29,    6,   45,            2,   39,    0,    6,    // 23
       3,    0,   33,    3,           30,    6,   30,    4,    // 31
      11,    3,    4,    6,            2,    2,   48,    2,    // 39
      52,   29,    6,   34,            3,   45,    2,   39,    // 47
       1,    6,    3,    1,           33,    3,   35,    1,    // 55
      30,    6,   36,    9,            6,    3,    0,    5,    // 63
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      13,   25,   37,   51,        -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 3;
    std::string imports[] = {
    "Character::get_x^0",         "Character::set_x^1",         "player",      // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes05) {
    
    // Test static attribute

    char const *inpl = "\
        enum bool                               \n\
        {                                       \n\
            false = 0,                          \n\
            true = 1                            \n\
        };                                      \n\
                                                \n\
        builtin managed struct Game             \n\
        {                                       \n\
            readonly import static attribute    \n\
                bool SkippingCutscene;          \n\
        };                                      \n\
                                                \n\
        void Hook3()                            \n\
        {                                       \n\
            if (Game.SkippingCutscene)          \n\
            {                                   \n\
                int i = 99;                     \n\
            }                                   \n\
        }                                       \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes05", scrip);

    size_t const codesize = 30;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   14,   38,    0,           36,   15,   39,    0,    // 7
       6,    3,    0,   33,            3,   28,   12,   36,    // 15
      17,    6,    3,   99,           29,    3,   36,   18,    // 23
       2,    1,    4,   36,           19,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      10,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Game::get_SkippingCutscene^0",               "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes06) {
    
    // Indexed static attribute -- must return an int

    char const *inpl = "\
        builtin managed struct Game             \n\
        {                                       \n\
            readonly import static attribute    \n\
                int SpriteWidth[];              \n\
        };                                      \n\
                                                \n\
        void main()                             \n\
        {                                       \n\
            int I = Game.SpriteWidth[9];        \n\
        }                                       \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes06", scrip);

    size_t const codesize = 28;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,    6,    3,    // 7
       9,   34,    3,   39,            1,    6,    3,    0,    // 15
      33,    3,   35,    1,           29,    3,   36,   10,    // 23
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "Game::geti_SpriteWidth^1",    "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes07) {
    
    // Assignment to attribute -- should not generate null dereference error

    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += "\
        builtin managed struct Label {      \n\
            attribute String Text;          \n\
        };                                  \n\
        import Label lbl;                   \n\
                                            \n\
        void main()                         \n\
        {                                   \n\
            lbl.Text = \"\";                \n\
        }                                   \n\
    ";

    int compileResult = cc_compile(input, scrip);

    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Attributes07", scrip);

    size_t const codesize = 34;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    3,    // 7
       0,    6,    2,   22,           64,    3,   29,    6,    // 15
      34,    3,   45,    2,           39,    1,    6,    3,    // 23
      21,   33,    3,   35,            1,   30,    6,   36,    // 31
       9,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   11,   24,  -999
    };
    char fixuptypes[] = {
      3,   4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 2;
    std::string imports[] = {
    "Label::set_Text^1",          "lbl",          "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 1;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
      0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, Attributes08) {

    char const *inpl = "\
        builtin managed autoptr struct String           \n\
        {                                               \n\
            char Payload;                               \n\
        };                                              \n\
                                                        \n\
        builtin managed struct ListBox                  \n\
        {                                               \n\
            import attribute String Items[];            \n\
        };                                              \n\
                                                        \n\
        void ListBox::seti_Items(int idx, String val)   \n\
        {                                               \n\
        }                                               \n\
                                                        \n\
        String ListBox::geti_Items(int idx)             \n\
        {                                               \n\
            return null;                                \n\
        }                                               \n\
                                                        \n\
        void main()                                     \n\
        {                                               \n\
            ListBox L;                                  \n\
            String S = L.Items[7];                      \n\
            L.Items[8] = S;                             \n\
        }                                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // WriteOutput("Attributes08", scrip);

    size_t const codesize = 123;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   12,   38,    0,           51,   12,    7,    3,    // 7
      50,    3,   36,   13,           51,   12,   49,    5,    // 15
      36,   16,   38,   16,           36,   17,    6,    3,    // 23
       0,    5,   36,   21,           38,   26,   36,   22,    // 31
      51,    0,   49,    1,            1,    4,   36,   23,    // 39
      51,    4,   48,    2,           52,   29,    6,   29,    // 47
       2,    6,    3,    7,           30,    2,   29,    3,    // 55
      45,    2,    6,    3,           16,   23,    3,    2,    // 63
       1,    4,   30,    6,           51,    0,   47,    3,    // 71
       1,    1,    4,   36,           24,   51,    4,   48,    // 79
       3,   51,    8,   48,            2,   52,   29,    6,    // 87
      29,    3,   29,    2,            6,    3,    8,   30,    // 95
       2,   29,    3,   45,            2,    6,    3,    0,    // 103
      23,    3,    2,    1,            8,   30,    6,   36,    // 111
      25,   51,    8,   49,           51,    4,   49,    2,    // 119
       1,    8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      60,  103,  -999
    };
    char fixuptypes[] = {
      2,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes09) {

    // Function call to 'set_Visible()' mustn't go awry
    // After loading 'true' to AX´, must protect AX from being clobbered (push it)

    char const *inpl = "\
        enum bool { false = 0, true };                  \n\
        builtin managed struct GUIControl               \n\
        {                                               \n\
            import attribute bool Visible;              \n\
        };                                              \n\
        builtin managed struct Label extends GUIControl \n\
        { };                                            \n\
        struct TwoClickHandler                          \n\
        {                                               \n\
            import static attribute Label *ActionLabel; \n\
        };                                              \n\
                                                        \n\
        int room_RepExec()                              \n\
        {                                               \n\
            TwoClickHandler.ActionLabel.Visible = true; \n\
        }                                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // WriteOutput("Attributes09", scrip);

    size_t const codesize = 43;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   14,   38,    0,           36,   15,   39,    0,    // 7
       6,    3,    2,   33,            3,    3,    3,    2,    // 15
      52,    6,    3,    1,           29,    6,   34,    3,    // 23
      45,    2,   39,    1,            6,    3,    1,   33,    // 31
       3,   35,    1,   30,            6,   36,   16,    6,    // 39
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      10,   30,  -999
    };
    char fixuptypes[] = {
      4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 2;
    std::string imports[] = {
    "GUIControl::set_Visible^1",  "TwoClickHandler::get_ActionLabel^0",        // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes10) {

    // Accept extender attributes

    std::string inpl = g_Input_Bool;
    inpl += g_Input_String;

    inpl += "\
        builtin managed struct Character                    \n\
        {                                                   \n\
            int payload;                                    \n\
        };                                                  \n\
        import attribute float Weight(this Character *);    \n\
        import attribute String Weapon[](this Character);   \n\
        Character *player;                                  \n\
                                                            \n\
        int game_start()                                    \n\
        {                                                   \n\
            float weight = player.Weight;                   \n\
            player.Weight                                   \n\
                          =                                 \n\
                            weight                          \n\
                                   ?:                       \n\
                                      9.9;                  \n\
            String weapon = player.Weapon[3];               \n\
            player.Weapon[9] = \"Rotten tomato\";           \n\
        }                                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // WriteOutput("Attributes10", scrip);

    size_t const codesize = 155;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   10,   38,    0,           36,   11,    6,    2,    // 7
       0,   48,    2,   52,           29,    6,   45,    2,    // 15
      39,    0,    6,    3,           20,   33,    3,   30,    // 23
       6,   29,    3,   36,           14,   51,    4,    7,    // 31
       3,   70,    5,   36,           16,    6,    3, 1092511334,    // 39
      36,   12,    6,    2,            0,   48,    2,   52,    // 47
      29,    6,   34,    3,           45,    2,   39,    1,    // 55
       6,    3,   21,   33,            3,   35,    1,   30,    // 63
       6,   36,   17,    6,            2,    0,   48,    2,    // 71
      52,   29,    6,   29,            2,    6,    3,    3,    // 79
      30,    2,   34,    3,           45,    2,   39,    1,    // 87
       6,    3,   22,   33,            3,   35,    1,   30,    // 95
       6,   51,    0,   47,            3,    1,    1,    4,    // 103
      36,   18,    6,    3,            0,    6,    2,    0,    // 111
      48,    2,   52,   64,            3,   29,    6,   34,    // 119
       3,   29,    2,    6,            3,    9,   30,    2,    // 127
      34,    3,   45,    2,           39,    2,    6,    3,    // 135
      23,   33,    3,   35,            2,   30,    6,   36,    // 143
      19,   51,    4,   49,            2,    1,    8,    6,    // 151
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 9;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   20,   44,   58,         69,   90,  108,  111,    // 7
     136,  -999
    };
    char fixuptypes[] = {
      1,   4,   1,   4,      1,   4,   3,   1,    // 7
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 4;
    std::string imports[] = {
    "Character::get_Weight^0",    "Character::set_Weight^1",    "Character::geti_Weapon^1",   // 22
    "Character::seti_Weapon^2",    "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 14;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'R',  'o',  't',  't',          'e',  'n',  ' ',  't',     // 7
    'o',  'm',  'a',  't',          'o',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, Attributes11) {

    // Accept static extender attributes

    char const *inpl = "\
    builtin managed struct Character                    \n\
    {                                                   \n\
        int payload;                                    \n\
    };                                                  \n\
    import attribute float Foo(static Character);       \n\
    import attribute int Bar[](static Character);       \n\
                                                        \n\
    int game_start()                                    \n\
    {                                                   \n\
        float F = Character.Foo;                        \n\
        Character.Foo = 77.7;                           \n\
        int I = Character.Bar[3];                       \n\
        Character.Bar[33] = I;                          \n\
    }                                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // WriteOutput("Attributes11", scrip);

    size_t const codesize = 80;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    9,   38,    0,           36,   10,   39,    0,    // 7
       6,    3,    0,   33,            3,   29,    3,   36,    // 15
      11,    6,    3, 1117480550,           34,    3,   39,    1,    // 23
       6,    3,    1,   33,            3,   35,    1,   36,    // 31
      12,    6,    3,    3,           34,    3,   39,    1,    // 39
       6,    3,    2,   33,            3,   35,    1,   29,    // 47
       3,   36,   13,   51,            4,    7,    3,   34,    // 55
       3,    6,    3,   33,           34,    3,   39,    2,    // 63
       6,    3,    3,   33,            3,   35,    2,   36,    // 71
      14,    2,    1,    8,            6,    3,    0,    5,    // 79
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      10,   26,   42,   66,        -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 4;
    std::string imports[] = {
    "Character::get_Foo^0",       "Character::set_Foo^1",       "Character::geti_Bar^1",      // 2
    "Character::seti_Bar^2",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}
        
TEST_F(Bytecode1, ManagedDerefZerocheck) {
    
    // Bytecode ought to check that S isn't initialized yet

    char const *inpl = "\
        managed struct Struct           \n\
        {                               \n\
            int Int[10];                \n\
        } S;                            \n\
                                        \n\
        int room_AfterFadeIn()          \n\
        {                               \n\
            S.Int[4] = 1;               \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("ManagedDerefZerocheck", scrip);

    size_t const codesize = 26;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    2,    // 7
       0,   48,    2,   52,            6,    3,    1,    1,    // 15
       2,   16,    8,    3,           36,    9,    6,    3,    // 23
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, MemInitPtr1) {
    
    // Check that pointer vars are pushed correctly in func calls

    char const *inpl = "\
        managed struct Struct1              \n\
        {                                   \n\
            float Payload1;                 \n\
        };                                  \n\
        managed struct Struct2              \n\
        {                                   \n\
            char Payload2;                  \n\
        };                                  \n\
                                            \n\
        int main()                          \n\
        {                                   \n\
            Struct1 SS1 = new Struct1;      \n\
            SS1.Payload1 = 0.7;             \n\
            Struct2 SS2 = new Struct2;      \n\
            SS2.Payload2 = 4;               \n\
            int Val = Func(SS1, SS2);       \n\
        }                                   \n\
                                            \n\
        int Func(Struct1 S1, Struct2 S2)    \n\
        {                                   \n\
            return S2.Payload2;             \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    WriteOutput("MemInitPtr1", scrip);

    size_t const codesize = 123;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   11,   38,    0,           36,   12,   73,    3,    // 7
       4,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,   13,   51,    4,           48,    2,   52,    6,    // 23
       3, 1060320051,    8,    3,           36,   14,   73,    3,    // 31
       4,   51,    0,   47,            3,    1,    1,    4,    // 39
      36,   15,   51,    4,           48,    2,   52,    6,    // 47
       3,    4,   26,    3,           36,   16,   51,    4,    // 55
      48,    3,   29,    3,           51,   12,   48,    3,    // 63
      29,    3,    6,    3,           91,   23,    3,    2,    // 71
       1,    8,   29,    3,           36,   17,   51,   12,    // 79
      49,   51,    8,   49,            2,    1,   12,    6,    // 87
       3,    0,    5,   36,           20,   38,   91,   51,    // 95
       8,    7,    3,   50,            3,   51,   12,    7,    // 103
       3,   50,    3,   36,           21,   51,   12,   48,    // 111
       2,   52,   24,    3,           51,    8,   49,   51,    // 119
      12,   49,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      68,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Ternary1) {

    // Accept a simple ternary expression
    // The 'return' in line 8 isn't reachable
    
    char const *inpl = "\
    int Foo(int i)              \n\
    {                           \n\
        return i > 0            \n\
                     ?          \n\
                       1        \n\
                         :      \n\
                           -1;  \n\
        return 9;               \n\
    }                           \n\
    ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_EQ(8, mh.GetMessages().at(0).Lineno);
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("reach"));

    // WriteOutput("Ternary1", scrip);

    size_t const codesize = 46;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,   51,    8,    // 7
       7,    3,   29,    3,            6,    3,    0,   30,    // 15
       4,   17,    4,    3,            3,    4,    3,   28,    // 23
       7,   36,    5,    6,            3,    1,   31,    5,    // 31
      36,    7,    6,    3,           -1,   36,    7,    5,    // 39
      36,    8,    6,    3,            9,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Ternary2) {
    
    // Accept Elvis operator expression

    char const *inpl = "\
    managed struct Struct       \n\
    {                           \n\
        int Payload;            \n\
    } S, T;                     \n\
                                \n\
    void main()                 \n\
    {                           \n\
        S = null;               \n\
        T = new Struct;         \n\
        Struct Res = S          \n\
                       ?:       \n\
                          T;    \n\
    }                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary2", scrip);

    size_t const codesize = 58;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    3,    // 7
       0,    6,    2,    0,           47,    3,   36,    9,    // 15
      73,    3,    4,    6,            2,    4,   47,    3,    // 23
      36,   10,    6,    2,            0,   48,    3,   70,    // 31
       7,   36,   12,    6,            2,    4,   48,    3,    // 39
      36,   12,   51,    0,           47,    3,    1,    1,    // 47
       4,   36,   13,   51,            4,   49,    2,    1,    // 55
       4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   21,   28,   37,        -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Ternary3) {
    
    // Accept nested expression

    char const *inpl = "\
    int main()                  \n\
    {                           \n\
        int t1 = 15;            \n\
        int t2 = 16;            \n\
        return t1 < 0 ? (t1 > 15 ? t2 : t1) : 99;     \n\
    }                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary3", scrip);

    size_t const codesize = 77;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
      15,   29,    3,   36,            4,    6,    3,   16,    // 15
      29,    3,   36,    5,           51,    8,    7,    3,    // 23
      29,    3,    6,    3,            0,   30,    4,   18,    // 31
       4,    3,    3,    4,            3,   28,   31,   51,    // 39
       8,    7,    3,   29,            3,    6,    3,   15,    // 47
      30,    4,   17,    4,            3,    3,    4,    3,    // 55
      28,    6,   51,    4,            7,    3,   31,    4,    // 63
      51,    8,    7,    3,           31,    3,    6,    3,    // 71
      99,    2,    1,    8,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Ternary4) {
    
    // String / literal string and conversion.

    char const *inpl = "\
        String main()                       \n\
        {                                   \n\
            String test = \"Test\";         \n\
            readonly int zajin = 7;         \n\
            return zajin ? test : \"Foo\";  \n\
        }                                   \n\
        ";

    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary4", scrip);

    size_t const codesize = 64;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   64,    3,   51,            0,   47,    3,    1,    // 15
       1,    4,   36,    4,            6,    3,    7,   29,    // 23
       3,   36,    5,   51,            4,    7,    3,   28,    // 31
       6,   51,    8,   48,            3,   31,    5,    6,    // 39
       3,    5,   64,    3,           29,    3,   51,    4,    // 47
      50,    3,   51,   12,           49,   51,    4,   48,    // 55
       3,   69,   30,    4,            2,    1,    8,    5,    // 63
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   41,  -999
    };
    char fixuptypes[] = {
      3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 9;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'T',  'e',  's',  't',            0,  'F',  'o',  'o',     // 7
      0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, Ternary5) {

    // Compile-time evaluation

    char const *inpl = "\
        float main()                        \n\
        {                                   \n\
            int I1a = 0 ? 10 : 20;          \n\
            int I1b = 2 ? 30 : 40;          \n\
            int I2a = 0 ?: 50;              \n\
            int I2b = 3 ?: 60;              \n\
            int I3a = 0 ? I1a : (7 + I1b);  \n\
            int I3b = 4 ? I2a : (7 + I2b);  \n\
            int I4a = 0 ? 70 : I3a;         \n\
            int I4b = 4 ? 80 : I3b;         \n\
            int I5a = 0 ? I4a : 90;         \n\
            int I5b = 5 ? I4b : 100;        \n\
            int I6 = 0 ? : I5a;             \n\
            return 0.;                      \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary5", scrip);

    size_t const codesize = 108;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
      20,   29,    3,   36,            4,    6,    3,   30,    // 15
      29,    3,   36,    5,            6,    3,   50,   29,    // 23
       3,   36,    6,    6,            3,    3,   29,    3,    // 31
      36,    7,    6,    3,            7,   29,    3,   51,    // 39
      16,    7,    3,   30,            4,   11,    4,    3,    // 47
       3,    4,    3,   29,            3,   36,    8,   51,    // 55
      12,    7,    3,   29,            3,   36,    9,   51,    // 63
       8,    7,    3,   29,            3,   36,   10,    6,    // 71
       3,   80,   29,    3,           36,   11,    6,    3,    // 79
      90,   29,    3,   36,           12,   51,    8,    7,    // 87
       3,   29,    3,   36,           13,   51,    8,    7,    // 95
       3,   29,    3,   36,           14,    6,    3,    0,    // 103
       2,    1,   44,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, AssignToString) {
    
    // Definition of global string with assignment

    char const *inpl = "\
        string Payload = \"Holzschuh\";     \n\
        readonly int una = 1;               \n\
        String main()                       \n\
        {                                   \n\
            String test = Payload;          \n\
            return (~~una == 2) ? test : Payload;  \n\
        }                                   \n\
        ";

    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += "\n\"__NEWSCRIPTSTART_MAIN\"\n";
    input += inpl;

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("AssignToString", scrip);

    size_t const codesize = 95;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,    6,    2,    // 7
       0,    3,    2,    3,           64,    3,   51,    0,    // 15
      47,    3,    1,    1,            4,   36,    6,    6,    // 23
       2,  200,    7,    3,            6,    4,   -1,   12,    // 31
       4,    3,    3,    4,            3,    6,    4,   -1,    // 39
      12,    4,    3,    3,            4,    3,   29,    3,    // 47
       6,    3,    2,   30,            4,   15,    4,    3,    // 55
       3,    4,    3,   28,            6,   51,    4,   48,    // 63
       3,   31,    8,    6,            2,    0,    3,    2,    // 71
       3,   64,    3,   29,            3,   51,    4,   50,    // 79
       3,   51,    8,   49,           51,    4,   48,    3,    // 87
      69,   30,    4,    2,            1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   25,   69,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 10;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'o',  'l',  'z',          's',  'c',  'h',  'u',     // 7
    'h',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StructWOldstyleString1) {
    
    // Unmanaged structs containing strings

    char const *inpl = "\
        struct Struct               \n\
        {                           \n\
            short Pad1;             \n\
            string ST1;             \n\
            short Pad2;             \n\
            string ST2;             \n\
        } S1, S2[3];                \n\
        string S3 = \"Holzschuh\";  \n\
        readonly int Int = 2;       \n\
                                    \n\
        void main()                 \n\
        {                           \n\
            S1.ST1 = \"Holz-\";     \n\
            S2[Int].ST1 = S1.ST1;   \n\
        }                           \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StructWOldstyleString1", scrip);

    size_t const codesize = 139;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   12,   38,    0,           36,   13,    6,    3,    // 7
      10,    6,    2,    2,            3,    3,    5,    3,    // 15
       2,    4,    6,    7,          199,    3,    4,    2,    // 23
       7,    3,    3,    5,            2,    8,    3,   28,    // 31
      25,    1,    4,    1,            1,    5,    1,    2,    // 39
       7,    1,    3,    7,            3,   70,  -26,    1,    // 47
       5,    1,    3,    5,            2,    6,    3,    0,    // 55
       8,    3,   36,   14,            6,    2,    2,    3,    // 63
       2,    3,   29,    3,            6,    2, 1816,    7,    // 71
       3,   46,    3,    3,           32,    3,  404,    6,    // 79
       2,  404,   11,    2,            3,   30,    3,    1,    // 87
       2,    2,    3,    3,            5,    3,    2,    4,    // 95
       6,    7,  199,    3,            4,    2,    7,    3,    // 103
       3,    5,    2,    8,            3,   28,   25,    1,    // 111
       4,    1,    1,    5,            1,    2,    7,    1,    // 119
       3,    7,    3,   70,          -26,    1,    5,    1,    // 127
       3,    5,    2,    6,            3,    0,    8,    3,    // 135
      36,   15,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   11,   62,   70,         81,  -999
    };
    char fixuptypes[] = {
      3,   1,   1,   1,      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 16;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'o',  'l',  'z',          's',  'c',  'h',  'u',     // 7
    'h',    0,  'H',  'o',          'l',  'z',  '-',    0,     // 15
    '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StructWOldstyleString2) {
    
    // Managed structs containing strings

    char const *inpl = "\
        managed struct Struct               \n\
        {                                   \n\
            short Pad1;                     \n\
            string ST1;                     \n\
            short Pad2;                     \n\
            string ST2;                     \n\
        };                                  \n\
                                            \n\
        void main()                         \n\
        {                                   \n\
            Struct S1 = new Struct;         \n\
            Struct S2[] = new Struct[3];    \n\
            S1.ST1 = \"-schuh\";            \n\
            S2[2].ST1 = S1.ST1;             \n\
        }                                   \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StructWOldstyleString2", scrip);

    size_t const codesize = 176;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   10,   38,    0,           36,   11,   73,    3,    // 7
     404,   51,    0,   47,            3,    1,    1,    4,    // 15
      36,   12,    6,    3,            3,   72,    3,    4,    // 23
       1,   51,    0,   47,            3,    1,    1,    4,    // 31
      36,   13,    6,    3,            0,   51,    8,   48,    // 39
       2,   52,    1,    2,            2,    3,    3,    5,    // 47
       3,    2,    4,    6,            7,  199,    3,    4,    // 55
       2,    7,    3,    3,            5,    2,    8,    3,    // 63
      28,   25,    1,    4,            1,    1,    5,    1,    // 71
       2,    7,    1,    3,            7,    3,   70,  -26,    // 79
       1,    5,    1,    3,            5,    2,    6,    3,    // 87
       0,    8,    3,   36,           14,   51,    8,   48,    // 95
       2,   52,    1,    2,            2,    3,    2,    3,    // 103
      51,    4,   48,    2,           52,    1,    2,    8,    // 111
      48,    2,   52,    1,            2,    2,    3,    3,    // 119
       5,    3,    2,    4,            6,    7,  199,    3,    // 127
       4,    2,    7,    3,            3,    5,    2,    8,    // 135
       3,   28,   25,    1,            4,    1,    1,    5,    // 143
       1,    2,    7,    1,            3,    7,    3,   70,    // 151
     -26,    1,    5,    1,            3,    5,    2,    6,    // 159
       3,    0,    8,    3,           36,   15,   51,    8,    // 167
      49,   51,    4,   49,            2,    1,    8,    5,    // 175
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      36,  -999
    };
    char fixuptypes[] = {
      3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 7;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    '-',  's',  'c',  'h',          'u',  'h',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, ThisExpression1) {

    // "this" must be handled correctly as an expression term

    char const *inpl = "\
        builtin managed struct Character    \n\
        {                                   \n\
        };                                  \n\
                                            \n\
        import readonly Character *player;  \n\
                                            \n\
        int TestCharacter(this Character *) \n\
        {                                   \n\
            Character *c = this;            \n\
            if (this == player)             \n\
                return 1;                   \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("ThisExpression1", scrip);

    size_t const codesize = 70;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    8,   38,    0,           36,    9,    3,    6,    // 7
       2,   52,    3,    2,            3,   51,    0,   47,    // 15
       3,    1,    1,    4,           36,   10,    3,    6,    // 23
       2,   52,    3,    2,            3,   29,    3,    6,    // 31
       2,    0,   48,    3,           30,    4,   15,    4,    // 39
       3,    3,    4,    3,           28,   12,   36,   11,    // 47
       6,    3,    1,   51,            4,   49,    2,    1,    // 55
       4,    5,   36,   12,           51,    4,   49,    2,    // 63
       1,    4,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      33,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "player",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, CrementAttribute1) {

    char const *inpl = "\
        builtin managed struct Object       \n\
        {                                   \n\
            import attribute int Graphic;   \n\
        } obj;                              \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            obj.Graphic++;                  \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CrementAttribute1", scrip);

    size_t const codesize = 57;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    2,    // 7
       0,   48,    2,   52,           29,    6,   45,    2,    // 15
      39,    0,    6,    3,            0,   33,    3,   30,    // 23
       6,    1,    3,    1,            6,    2,    0,   48,    // 31
       2,   52,   29,    6,           34,    3,   45,    2,    // 39
      39,    1,    6,    3,            1,   33,    3,   35,    // 47
       1,   30,    6,   36,            9,    6,    3,    0,    // 55
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   20,   30,   44,        -999
    };
    char fixuptypes[] = {
      1,   4,   1,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 2;
    std::string imports[] = {
    "Object::get_Graphic^0",      "Object::set_Graphic^1",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, CrementAttribute2) {

    char const *inpl = "\
        builtin managed struct Object       \n\
        {                                   \n\
            import attribute int Graphic;   \n\
        } obj;                              \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            return obj.Graphic++;           \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CrementAttribute2", scrip);

    size_t const codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    2,    // 7
       0,   48,    2,   52,           29,    6,   45,    2,    // 15
      39,    0,    6,    3,            0,   33,    3,   30,    // 23
       6,   29,    3,    1,            3,    1,    6,    2,    // 31
       0,   48,    2,   52,           29,    6,   34,    3,    // 39
      45,    2,   39,    1,            6,    3,    1,   33,    // 47
       3,   35,    1,   30,            6,   30,    3,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   20,   32,   46,        -999
    };
    char fixuptypes[] = {
      1,   4,   1,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 2;
    std::string imports[] = {
    "Object::get_Graphic^0",      "Object::set_Graphic^1",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, CrementInExpression1) {

    char const *inpl = "\
        int foo ()                          \n\
        {                                   \n\
            int I;                          \n\
            return 1 + --I;                 \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CementInExpression1", scrip);

    size_t const codesize = 41;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,   36,            4,    6,    3,    1,    // 15
      29,    3,   51,    8,            7,    3,    2,    3,    // 23
       1,    8,    3,    7,            3,   30,    4,   11,    // 31
       4,    3,    3,    4,            3,    2,    1,    4,    // 39
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, CrementInExpression2) {

    char const *inpl = "\
        int foo ()                          \n\
        {                                   \n\
            char Ch;                        \n\
            return Ch-- - 1;                \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CrementInExpression2", scrip);

    size_t const codesize = 44;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,   51,    0,    // 7
      63,    1,    1,    1,            1,   36,    4,   51,    // 15
       1,   24,    3,    2,            3,    1,   26,    3,    // 23
       1,    3,    1,   29,            3,    6,    3,    1,    // 31
      30,    4,   12,    4,            3,    3,    4,    3,    // 39
       2,    1,    1,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, CrementInExpression3) {

    char const *inpl = "\
        int foo ()                          \n\
        {                                   \n\
            int I = 7;                      \n\
            short J = 9;                    \n\
            if (++I == (J)--)               \n\
                --J;                        \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CrementInExpression3", scrip);

    size_t const codesize = 80;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       7,   29,    3,   36,            4,    6,    3,    9,    // 15
      51,    0,   27,    3,            1,    1,    2,   36,    // 23
       5,   51,    6,    7,            3,    1,    3,    1,    // 31
       8,    3,    7,    3,           29,    3,   51,    6,    // 39
      25,    3,    2,    3,            1,   27,    3,    1,    // 47
       3,    1,   30,    4,           15,    4,    3,    3,    // 55
       4,    3,   28,   11,           36,    6,   51,    2,    // 63
      25,    3,    2,    3,            1,   27,    3,   36,    // 71
       7,    2,    1,    6,            6,    3,    0,    5,    // 79
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, CompareStringToNull) {

    // If a String is compared to 'null', the pointer opcodes must be used,
    // not the String opcodes.

    char const *inpl = "\
        String S;                           \n\
        bool func()                         \n\
        {                                   \n\
            bool b1 = S != null;            \n\
            bool b2 = S == null;            \n\
            bool b3 = null != S;            \n\
        }                                   \n\
        ";

    std::string input = "";
    input += g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    int compileResult = cc_compile(input, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CompareStringToNull", scrip);

    size_t const codesize = 79;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    3,   38,    0,           36,    4,    6,    2,    // 7
       0,   48,    3,   29,            3,    6,    3,    0,    // 15
      30,    4,   16,    4,            3,    3,    4,    3,    // 23
      29,    3,   36,    5,            6,    2,    0,   48,    // 31
       3,   29,    3,    6,            3,    0,   30,    4,    // 39
      15,    4,    3,    3,            4,    3,   29,    3,    // 47
      36,    6,    6,    3,            0,   29,    3,    6,    // 55
       2,    0,   48,    3,           30,    4,   16,    4,    // 63
       3,    3,    4,    3,           29,    3,   36,    7,    // 71
       2,    1,   12,    6,            3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       8,   30,   57,  -999
    };
    char fixuptypes[] = {
      1,   1,   1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, DynarrayLength1) {

    char const *inpl = "\
        managed struct Struct               \n\
        {                                   \n\
            int Payload;                    \n\
        } Dynarray[];                       \n\
                                            \n\
        int foo ()                          \n\
        {                                   \n\
            Dynarray = new Struct[5];       \n\
            return Dynarray.Length;         \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    WriteOutput("DynarrayLength1", scrip);

    size_t const codesize = 38;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    7,   38,    0,           36,    8,    6,    3,    // 7
       5,   72,    3,    4,            1,    6,    2,    0,    // 15
      47,    3,   36,    9,            6,    2,    0,   48,    // 23
       2,   52,   34,    2,           39,    1,    6,    3,    // 31
       0,   33,    3,   35,            1,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,   22,   32,  -999
    };
    char fixuptypes[] = {
      1,   1,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "__Builtin_DynamicArrayLength^1",             "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, DynarrayLength2) {

    char const *inpl = "\
        int foo ()                          \n\
        {                                   \n\
            int Dynarray[] = new int[7];    \n\
            int len = Dynarray.Length;      \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    WriteOutput("DynarrayLength2", scrip);

    size_t const codesize = 52;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       7,   72,    3,    4,            0,   51,    0,   47,    // 15
       3,    1,    1,    4,           36,    4,   51,    4,    // 23
      48,    2,   52,   34,            2,   39,    1,    6,    // 31
       3,    0,   33,    3,           35,    1,   29,    3,    // 39
      36,    5,   51,    8,           49,    2,    1,    8,    // 47
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      33,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 1;
    std::string imports[] = {
    "__Builtin_DynamicArrayLength^1",             "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, DynarrayOfPrimitives) {

    // Dynamic arrays of primitives are allowed.

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            short PrmArray[] = new short[10];   \n\
            PrmArray[7] = 0;                    \n\
            PrmArray[3] = PrmArray[7];          \n\
        }                                       \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("DynarrayOfPrimitives", scrip);

    size_t const codesize = 69;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
      10,   72,    3,    2,            0,   51,    0,   47,    // 15
       3,    1,    1,    4,           36,    4,   51,    4,    // 23
      48,    2,   52,    6,            3,    0,    1,    2,    // 31
      14,   27,    3,   36,            5,   51,    4,   48,    // 39
       2,   52,    1,    2,           14,   25,    3,   51,    // 47
       4,   48,    2,   52,            1,    2,    6,   27,    // 55
       3,   36,    6,   51,            4,   49,    2,    1,    // 63
       4,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, StringLiteral2String) {

    char const *inpl = "\
        internalstring autoptr builtin managed struct String    \n\
        {};                         \n\
                                    \n\
        struct StructWithString     \n\
        {                           \n\
            String Txt;             \n\
        };                          \n\
                                    \n\
        int func1()                 \n\
        {                           \n\
            StructWithString a;     \n\
            a.Txt = \"Cause bug!\"; \n\
        }                           \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringLiteral2String", scrip);

    size_t const codesize = 34;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,   10,   38,    0,           36,   11,    6,    3,    // 7
       0,   29,    3,   36,           12,    6,    3,    0,    // 15
      51,    4,   64,    3,           47,    3,   36,   13,    // 23
      51,    4,   49,    2,            1,    4,    6,    3,    // 31
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      15,  -999
    };
    char fixuptypes[] = {
      3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 11;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'C',  'a',  'u',  's',          'e',  ' ',  'b',  'u',     // 7
    'g',  '!',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, LongMin1) {

    // Accept LONG_MIN written in decimal, generate appropriate code

    char const *inpl = "\
        int i = - 2147483648;                   \n\
                                                \n\
        int test(int foo = -2147483648)         \n\
        {                                       \n\
            int i1 = - 2147483648;              \n\
            int i2 = -1 - -2147483648;          \n\
            return test() + (2 + -2147483648);  \n\
        }                                       \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("LongMin1", scrip);

    size_t const codesize = 50;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    4,   38,    0,           36,    5,    6,    3,    // 7
    LONG_MIN,   29,    3,   36,            6,    6,    3, 2147483647,    // 15
      29,    3,   36,    7,            6,    3, LONG_MIN,   29,    // 23
       3,    6,    3,    0,           23,    3,    2,    1,    // 31
       4,   29,    3,    6,            3, -2147483646,   30,    4,    // 39
      11,    4,    3,    3,            4,    3,    2,    1,    // 47
       8,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      27,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Linenum01)
{
    // Linenum directive must be generated for each declaration

    char const *inpl = "\
    int game_start()            \n\
    {                           \n\
        int a = 1;              \n\
        int b = 1 + 1;          \n\
    }                           \n\
    ";


    MessageHandler mh;
    AGS::ccCompiledScript scrip{ true };
    int compileResult = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());

    // WriteOutput("Linenum01", scrip);

    size_t const codesize = 27;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       1,   29,    3,   36,            4,    6,    3,    2,    // 15
      29,    3,   36,    5,            2,    1,    8,    6,    // 23
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Linenum02)
{
    // Linenum directive must be generated for 'c +=' line

    char const *inpl = "\
    int game_start()            \n\
    {                           \n\
        int c = 0;              \n\
        c += 1 + 1;             \n\
        return 0;               \n\
    }                           \n\
    ";

    MessageHandler mh;
    AGS::ccCompiledScript scrip{ true };
    int compileResult = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());

    // WriteOutput("Linenum02", scrip);

    size_t const codesize = 38;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      36,    2,   38,    0,           36,    3,    6,    3,    // 7
       0,   29,    3,   36,            4,    6,    3,    2,    // 15
      29,    3,   51,    8,            7,    3,   30,    4,    // 23
      11,    3,    4,    8,            3,   36,    5,    6,    // 31
       3,    0,    2,    1,            4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int const numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    size_t const numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    size_t const stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}
