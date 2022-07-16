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
    AGS::ccCompiledScript scrip{ false };

    Bytecode1()
    {
        // Initializations, will be done at the start of each test
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        ccSetOption(SCOPT_LINENUMBERS, false);
        clear_error();
    }
};

TEST_F(Bytecode1, StringOldstyle01) {
    ccSetOption(SCOPT_OLDSTRINGS, true);

    const char *inpl = "\
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
    size_t const codesize = 26;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,  200,    1,    1,    // 7
     200,   51,    0,   63,            1,    1,    1,    1,    // 15
       6,    2,    4,    3,            2,    3,    2,    1,    // 23
     201,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      18,  -999
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

    const char *inpl = "\
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
    size_t const codesize = 25;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,    5,   38,    6,    // 7
       6,    3,    0,   29,            3,    6,    3,    0,    // 15
      23,    3,    2,    1,            4,    6,    3,    0,    // 23
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      10,   15,  -999
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
    
    const char *inpl = "\
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
    const size_t codesize = 76;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   51,    8,    3,    // 7
       3,    5,    3,    2,            4,    6,    7,  199,    // 15
       3,    4,    2,    7,            3,    3,    5,    2,    // 23
       8,    3,   28,   25,            1,    4,    1,    1,    // 31
       5,    1,    2,    7,            1,    3,    7,    3,    // 39
      70,  -26,    1,    5,            1,    3,    5,    2,    // 47
       6,    3,    0,    8,            3,    5,   38,   54,    // 55
       6,    2,    4,    3,            2,    3,   29,    3,    // 63
       6,    3,    0,   23,            3,    2,    1,    4,    // 71
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   58,   66,  -999
    };
    char fixuptypes[] = {
      3,   1,   2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 10;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'P',  'a',  'r',  'a',          'm',  'e',  't',  'e',     // 7
    'r',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringOldstyle04) {
    
    const char *inpl = "\
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
    size_t const codesize = 84;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            4,    3,    2,    3,    // 7
      29,    3,    6,    3,           76,   23,    3,    2,    // 15
       1,    4,   51,    0,            3,    3,    5,    3,    // 23
       2,    4,    6,    7,          199,    3,    4,    2,    // 31
       7,    3,    3,    5,            2,    8,    3,   28,    // 39
      25,    1,    4,    1,            1,    5,    1,    2,    // 47
       7,    1,    3,    7,            3,   70,  -26,    1,    // 55
       5,    1,    3,    5,            2,    6,    3,    0,    // 63
       8,    3,    1,    1,          200,    2,    1,  200,    // 71
       6,    3,    0,    5,           38,   76,   51,    8,    // 79
       3,    2,    3,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   12,  -999
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
    
    const char *inpl = "\
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
    const size_t codesize = 121;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   51,    0,    3,    // 7
       3,    5,    3,    2,            4,    6,    7,  199,    // 15
       3,    4,    2,    7,            3,    3,    5,    2,    // 23
       8,    3,   28,   25,            1,    4,    1,    1,    // 31
       5,    1,    2,    7,            1,    3,    7,    3,    // 39
      70,  -26,    1,    5,            1,    3,    5,    2,    // 47
       6,    3,    0,    8,            3,    1,    1,  200,    // 55
      51,    0,   63,  200,            1,    1,  200,    6,    // 63
       3,    6,   51,  200,            3,    3,    5,    3,    // 71
       2,    4,    6,    7,          199,    3,    4,    2,    // 79
       7,    3,    3,    5,            2,    8,    3,   28,    // 87
      25,    1,    4,    1,            1,    5,    1,    2,    // 95
       7,    1,    3,    7,            3,   70,  -26,    1,    // 103
       5,    1,    3,    5,            2,    6,    3,    0,    // 111
       8,    3,    2,    1,          400,    6,    3,    0,    // 119
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   65,  -999
    };
    char fixuptypes[] = {
      3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 13;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'o',  'l',  'z',          '-',    0,  '-',  'S',     // 7
    'c',  'h',  'u',  'h',            0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringStandard01) {

    const char inpl[] = "\
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
    size_t const codesize = 53;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   64,    3,   51,    // 7
       0,   47,    3,    1,            1,    4,   51,    4,    // 15
      48,    3,   29,    3,            6,    3,   14,   30,    // 23
       4,   66,    4,    3,            3,    4,    3,   28,    // 31
      10,    6,    3,    1,           51,    4,   49,    2,    // 39
       1,    4,    5,    6,            3,    0,   51,    4,    // 47
      49,    2,    1,    4,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   22,  -999
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
    
    const char inpl[] = "\
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
    size_t const codesize = 83;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    3,    5,    // 7
      38,    8,   51,    8,            7,    3,   50,    3,    // 15
      51,    8,   48,    3,           29,    3,   51,    4,    // 23
      50,    3,   51,   12,           49,   51,    4,   48,    // 31
       3,   69,   30,    4,            5,   38,   37,    6,    // 39
       3,    7,   64,    3,           51,    0,   47,    3,    // 47
       1,    1,    4,   51,            4,   48,    3,   29,    // 55
       3,   51,    4,   50,            3,   51,    8,   49,    // 63
      51,    4,   48,    3,           69,   30,    4,    2,    // 71
       1,    4,    5,   38,           75,    6,    3,    7,    // 79
      64,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   41,   79,  -999
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

    const char inpl[] = "\
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
    size_t const codesize = 98;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    3,    2,    3,    // 7
      64,    3,    5,   38,           11,   51,    8,    7,    // 15
       3,   50,    3,   51,            8,   48,    3,   29,    // 23
       3,   51,    4,   50,            3,   51,   12,   49,    // 31
      51,    4,   48,    3,           69,   30,    4,    5,    // 39
      38,   40,    6,    3,            6,   64,    3,   29,    // 47
       3,    6,    3,   11,           23,    3,    2,    1,    // 55
       4,    6,    3,    0,            5,   38,   61,    6,    // 63
       3,    6,   64,    3,           51,    0,   47,    3,    // 71
       1,    1,    4,   51,            4,   48,    3,   67,    // 79
       3,   29,    3,    6,            3,   40,   23,    3,    // 87
       2,    1,    4,   51,            4,   49,    2,    1,    // 95
       4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   44,   51,   65,         85,  -999
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

    const char *inpl = "\
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
    const size_t codesize = 39;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            2,   48,    2,   52,    // 7
      29,    2,    6,    2,            0,    3,    2,    3,    // 15
      34,    3,   51,    4,            7,    2,   45,    2,    // 23
      39,    1,    6,    3,            1,   33,    3,   35,    // 31
       1,   30,    2,    6,            3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   12,   28,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 3;
    std::string imports[] = {
    "oCleaningCabinetDoor",       "Character::FaceObject^1",    "player",      // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, AccessStructAsPointer02) {
    
    // Managed structs can be declared without (implicit) pointer in certain circumstances.
    // Such structs can be assigned to a variable that is a pointered struct

    const char *inpl = "\
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
    const size_t codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    3,    2,    3,    // 7
      51,    0,   47,    3,            1,    1,    4,    6,    // 15
       2,    2,   48,    2,           52,   29,    2,   51,    // 23
       8,   48,    3,   34,            3,   51,    4,    7,    // 31
       2,   45,    2,   39,            1,    6,    3,    1,    // 39
      33,    3,   35,    1,           30,    2,   51,    4,    // 47
      49,    2,    1,    4,            6,    3,    0,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   17,   39,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 3;
    std::string imports[] = {
    "oCleaningCabinetDoor",       "Character::FaceObject^1",    "player",      // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, AccessStructAsPointer03) {
    
    // Managed structs can be declared without (implicit) pointer in certain circumstances.
    // Such structs can be assigned to a variable that is a pointered struct

    const char *inpl = "\
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
    const size_t codesize = 28;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    1,    2,   20,    // 7
       3,    2,    3,   51,            0,   47,    3,    1,    // 15
       1,    4,   51,    4,           49,    2,    1,    4,    // 23
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 3;
    std::string imports[] = {
    "object",      "Character::FaceObject^1",    "player",       "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes01) {

    const char *inpl = "\
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
    size_t const codesize = 154;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           49,    1,    1,    4,    // 7
      51,    4,   48,    2,           52,   29,    6,   45,    // 15
       2,   39,    0,    6,            3,    0,   33,    3,    // 23
      30,    6,   28,  101,           51,    4,   48,    2,    // 31
      52,    6,    3,   17,           29,    6,   34,    3,    // 39
      45,    2,   39,    1,            6,    3,    2,   33,    // 47
       3,   35,    1,   30,            6,   51,    4,   48,    // 55
       2,   52,   29,    6,           45,    2,   39,    0,    // 63
       6,    3,    3,   33,            3,   30,    6,   29,    // 71
       3,   51,    8,   48,            2,   52,   29,    6,    // 79
      45,    2,   39,    0,            6,    3,    3,   33,    // 87
       3,   30,    6,   30,            4,   57,    4,    3,    // 95
       3,    4,    3,   29,            3,   51,    8,   48,    // 103
       2,   52,   29,    6,           45,    2,   39,    0,    // 111
       6,    3,    1,   33,            3,   30,    6,   51,    // 119
       8,   49,    2,    1,            8,    5,    2,    1,    // 127
       4,   51,    4,   48,            2,   52,   29,    6,    // 135
      45,    2,   39,    0,            6,    3,    0,   33,    // 143
       3,   30,    6,   51,            4,   49,    2,    1,    // 151
       4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 6;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      21,   46,   66,   86,        114,  142,  -999
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
    // Armor:: functions should be allowed to access _Damage.

    const char *inpl = "\
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
    size_t const codesize = 119;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            8,   51,    0,   47,    // 7
       3,    1,    1,    4,           51,    4,   48,    2,    // 15
      52,    6,    3,   17,           29,    6,   29,    3,    // 23
      45,    2,    6,    3,           72,   23,    3,    2,    // 31
       1,    4,   30,    6,           51,    4,   48,    2,    // 39
      52,   29,    6,   45,            2,    6,    3,  107,    // 47
      23,    3,   30,    6,           29,    3,    6,    3,    // 55
      10,   30,    4,   17,            4,    3,    3,    4,    // 63
       3,   51,    4,   49,            2,    1,    4,    5,    // 71
      38,   72,   51,    8,            7,    3,   29,    3,    // 79
       6,    3,    0,   30,            4,   19,    4,    3,    // 87
       3,    4,    3,   28,           13,   51,    8,    7,    // 95
       3,    3,    6,    2,           52,    1,    2,    2,    // 103
       8,    3,    5,   38,          107,    3,    6,    2,    // 111
      52,    1,    2,    2,            7,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      28,   47,  -999
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

    const char *inpl = "\
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
    size_t const codesize = 75;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            8,   51,    0,   47,    // 7
       3,    1,    1,    4,           51,    4,   48,    2,    // 15
      52,    6,    3,   17,           29,    6,   34,    3,    // 23
      45,    2,   39,    1,            6,    3,    1,   33,    // 31
       3,   35,    1,   30,            6,   51,    4,   48,    // 39
       2,   52,   29,    6,           45,    2,   39,    0,    // 47
       6,    3,    0,   33,            3,   30,    6,   29,    // 55
       3,    6,    3,   10,           30,    4,   17,    4,    // 63
       3,    3,    4,    3,           51,    4,   49,    2,    // 71
       1,    4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      30,   50,  -999
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

    const char *inpl = "\
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
    const size_t codesize = 58;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           77,   29,    3,    6,    // 7
       2,    2,   48,    2,           52,   29,    6,   45,    // 15
       2,   39,    0,    6,            3,    0,   33,    3,    // 23
      30,    6,   30,    4,           11,    3,    4,    6,    // 31
       2,    2,   48,    2,           52,   29,    6,   34,    // 39
       3,   45,    2,   39,            1,    6,    3,    1,    // 47
      33,    3,   35,    1,           30,    6,    6,    3,    // 55
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,   21,   33,   47,        -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 3;
    std::string imports[] = {
    "Character::get_x^0",         "Character::set_x^1",         "player",      // 2
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes05) {
    
    // Test static attribute

    const char *inpl = "\
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
    const size_t codesize = 20;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   39,    0,            6,    3,    0,   33,    // 7
       3,   28,    8,    6,            3,   99,   29,    3,    // 15
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       6,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "Game::get_SkippingCutscene^0",               "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Attributes06) {
    
    // Indexed static attribute -- must return an int

    const char *inpl = "\
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
    const size_t codesize = 22;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            9,   34,    3,   39,    // 7
       1,    6,    3,    0,           33,    3,   35,    1,    // 15
      29,    3,    2,    1,            4,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,  -999
    };
    char fixuptypes[] = {
      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "Game::geti_SpriteWidth^1",    "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
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
    size_t const codesize = 28;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,    6,    2,   22,    // 7
      64,    3,   29,    6,           34,    3,   45,    2,    // 15
      39,    1,    6,    3,           21,   33,    3,   35,    // 23
       1,   30,    6,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,    7,   20,  -999
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

    const char *inpl = "\
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
    size_t const codesize = 105;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,   12,            7,    3,   50,    3,    // 7
      51,   12,   49,    5,           38,   12,    6,    3,    // 15
       0,    5,   38,   18,           51,    0,   49,    1,    // 23
       1,    4,   51,    4,           48,    2,   52,   29,    // 31
       6,   29,    2,    6,            3,    7,   30,    2,    // 39
      29,    3,   45,    2,            6,    3,   12,   23,    // 47
       3,    2,    1,    4,           30,    6,   51,    0,    // 55
      47,    3,    1,    1,            4,   51,    4,   48,    // 63
       3,   51,    8,   48,            2,   52,   29,    6,    // 71
      29,    3,   29,    2,            6,    3,    8,   30,    // 79
       2,   29,    3,   45,            2,    6,    3,    0,    // 87
      23,    3,    2,    1,            8,   30,    6,   51,    // 95
       8,   49,   51,    4,           49,    2,    1,    8,    // 103
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      46,   87,  -999
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

    const char *inpl = "\
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
    size_t const codesize = 37;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   39,    0,            6,    3,    2,   33,    // 7
       3,    3,    3,    2,           52,    6,    3,    1,    // 15
      29,    6,   34,    3,           45,    2,   39,    1,    // 23
       6,    3,    1,   33,            3,   35,    1,   30,    // 31
       6,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       6,   26,  -999
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
        player.Weight = weight ?: 9.9;                  \n\
        String weapon = player.Weapon[3];               \n\
        player.Weapon[9] = \"Rotten tomato\";           \n\
    }                                                   \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    std::string msg = last_seen_cc_error();
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : msg.c_str());

    // WriteOutput("Attributes10", scrip);
    size_t const codesize = 139;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    2,   52,    // 7
      29,    6,   45,    2,           39,    0,    6,    3,    // 15
      20,   33,    3,   30,            6,   29,    3,   51,    // 23
       4,    7,    3,   70,            3,    6,    3, 1092511334,    // 31
       6,    2,    0,   48,            2,   52,   29,    6,    // 39
      34,    3,   45,    2,           39,    1,    6,    3,    // 47
      21,   33,    3,   35,            1,   30,    6,    6,    // 55
       2,    0,   48,    2,           52,   29,    6,   29,    // 63
       2,    6,    3,    3,           30,    2,   34,    3,    // 71
      45,    2,   39,    1,            6,    3,   22,   33,    // 79
       3,   35,    1,   30,            6,   51,    0,   47,    // 87
       3,    1,    1,    4,            6,    3,    0,    6,    // 95
       2,    0,   48,    2,           52,   64,    3,   29,    // 103
       6,   34,    3,   29,            2,    6,    3,    9,    // 111
      30,    2,   34,    3,           45,    2,   39,    2,    // 119
       6,    3,   23,   33,            3,   35,    2,   30,    // 127
       6,   51,    4,   49,            2,    1,    8,    6,    // 135
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 9;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   16,   34,   48,         57,   78,   94,   97,    // 7
     122,  -999
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

    const char inpl[] = "\
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
    size_t const codesize = 68;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   39,    0,            6,    3,    0,   33,    // 7
       3,   29,    3,    6,            3, 1117480550,   34,    3,    // 15
      39,    1,    6,    3,            1,   33,    3,   35,    // 23
       1,    6,    3,    3,           34,    3,   39,    1,    // 31
       6,    3,    2,   33,            3,   35,    1,   29,    // 39
       3,   51,    4,    7,            3,   34,    3,    6,    // 47
       3,   33,   34,    3,           39,    2,    6,    3,    // 55
       3,   33,    3,   35,            2,    2,    1,    8,    // 63
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       6,   20,   34,   56,        -999
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
        
TEST_F(Bytecode1, DynArrayOfPrimitives) {
    
    // Dynamic arrays of primitives are allowed.

    const char *inpl = "\
        int main()                              \n\
        {                                       \n\
            short PrmArray[] = new short[10];   \n\
            PrmArray[7] = 0;                    \n\
            PrmArray[3] = PrmArray[7];          \n\
        }                                       \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("DynArrayOfPrimitives", scrip);
    size_t const codesize = 59;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           10,   72,    3,    2,    // 7
       0,   51,    0,   47,            3,    1,    1,    4,    // 15
      51,    4,   48,    2,           52,    6,    3,    0,    // 23
       1,    2,   14,   27,            3,   51,    4,   48,    // 31
       2,   52,    1,    2,           14,   25,    3,   51,    // 39
       4,   48,    2,   52,            1,    2,    6,   27,    // 47
       3,   51,    4,   49,            2,    1,    4,    6,    // 55
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

TEST_F(Bytecode1, ManagedDerefZerocheck) {
    
    // Bytecode ought to check that S isn't initialized yet
    const char *inpl = "\
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
    size_t const codesize = 20;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    2,   52,    // 7
       6,    3,    1,    1,            2,   16,    8,    3,    // 15
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,  -999
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

    const char *inpl = "\
        managed struct Struct1          \n\
        {                               \n\
            float Payload1;             \n\
        };                              \n\
        managed struct Struct2          \n\
        {                               \n\
            char Payload2;              \n\
        };                              \n\
                                        \n\
        int main()                      \n\
        {                               \n\
            Struct1 SS1 = new Struct1;  \n\
            SS1.Payload1 = 0.7;         \n\
            Struct2 SS2 = new Struct2;  \n\
            SS2.Payload2 = 4;           \n\
            int Val = Func(SS1, SS2);   \n\
        }                               \n\
                                        \n\
        int Func(Struct1 S1, Struct2 S2) \n\
        {                               \n\
            return S2.Payload2;         \n\
        }                               \n\
        ";
    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("MemInitPtr1", scrip);
    size_t const codesize = 105;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            4,   51,    0,   47,    // 7
       3,    1,    1,    4,           51,    4,   48,    2,    // 15
      52,    6,    3, 1060320051,            8,    3,   73,    3,    // 23
       4,   51,    0,   47,            3,    1,    1,    4,    // 31
      51,    4,   48,    2,           52,    6,    3,    4,    // 39
      26,    3,   51,    4,           48,    3,   29,    3,    // 47
      51,   12,   48,    3,           29,    3,    6,    3,    // 55
      77,   23,    3,    2,            1,    8,   29,    3,    // 63
      51,   12,   49,   51,            8,   49,    2,    1,    // 71
      12,    6,    3,    0,            5,   38,   77,   51,    // 79
       8,    7,    3,   50,            3,   51,   12,    7,    // 87
       3,   50,    3,   51,           12,   48,    2,   52,    // 95
      24,    3,   51,    8,           49,   51,   12,   49,    // 103
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      56,  -999
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
    // The 'return' in line 4 isn't reachable
    
    const char *inpl = "\
    int Foo(int i)              \n\
    {                           \n\
        return i > 0 ? 1 : -1;  \n\
        return 9;               \n\
    }                           \n\
    ";

    MessageHandler mh;
    int compileResult = cc_compile(inpl, 0, scrip, mh);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : mh.GetError().Message.c_str());
    ASSERT_LE(1u, mh.GetMessages().size());
    EXPECT_EQ(4, mh.GetMessages().at(0).Lineno);
    EXPECT_NE(std::string::npos, mh.GetMessages().at(0).Message.find("reach"));

    // WriteOutput("Ternary1", scrip);
    size_t const codesize = 34;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
       6,    3,    0,   30,            4,   17,    4,    3,    // 15
       3,    4,    3,   28,            5,    6,    3,    1,    // 23
      31,    3,    6,    3,           -1,    5,    6,    3,    // 31
       9,    5,  -999
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

    const char *inpl = "\
    managed struct Struct       \n\
    {                           \n\
        int Payload;            \n\
    } S, T;                     \n\
                                \n\
    void main()                 \n\
    {                           \n\
        S = null;               \n\
        T = new Struct;         \n\
        Struct Res = S ?: T;    \n\
    }                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary2", scrip);
    const size_t codesize = 44;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,    6,    2,    0,    // 7
      47,    3,   73,    3,            4,    6,    2,    4,    // 15
      47,    3,    6,    2,            0,   48,    3,   70,    // 23
       5,    6,    2,    4,           48,    3,   51,    0,    // 31
      47,    3,    1,    1,            4,   51,    4,   49,    // 39
       2,    1,    4,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   15,   20,   27,        -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,     '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Ternary3) {
    
    // Accept nested expression

    const char *inpl = "\
    int main()                  \n\
    {                           \n\
        int t1 = 15;            \n\
        int t2 = 16;            \n\
        return t1 < 0 ? (t1 > 15 ? t2 : t1) : 99;     \n\
    }                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    //WriteOutput("Ternary3", scrip);
    size_t const codesize = 69;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           15,   29,    3,    6,    // 7
       3,   16,   29,    3,           51,    8,    7,    3,    // 15
      29,    3,    6,    3,            0,   30,    4,   18,    // 23
       4,    3,    3,    4,            3,   28,   31,   51,    // 31
       8,    7,    3,   29,            3,    6,    3,   15,    // 39
      30,    4,   17,    4,            3,    3,    4,    3,    // 47
      28,    6,   51,    4,            7,    3,   31,    4,    // 55
      51,    8,    7,    3,           31,    3,    6,    3,    // 63
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

    char inpl[] = "\
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
    size_t const codesize = 56;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   64,    3,   51,    // 7
       0,   47,    3,    1,            1,    4,    6,    3,    // 15
       7,   29,    3,   51,            4,    7,    3,   28,    // 23
       6,   51,    8,   48,            3,   31,    5,    6,    // 31
       3,    5,   64,    3,           29,    3,   51,    4,    // 39
      50,    3,   51,   12,           49,   51,    4,   48,    // 47
       3,   69,   30,    4,            2,    1,    8,    5,    // 55
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   33,  -999
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

    const char inpl[] = "\
        float main()                    \n\
        {                               \n\
            int I1a = 0 ? 10 : 20;      \n\
            int I1b = 2 ? 30 : 40;      \n\
            int I2a = 0 ?: 50;          \n\
            int I2b = 3 ?: 60;          \n\
            int I3a = 0 ? I1a : (7 + I1b);    \n\
            int I3b = 4 ? I2a : (7 + I2b);    \n\
            int I4a = 0 ? 70 : I3a;     \n\
            int I4b = 4 ? 80 : I3b;     \n\
            int I5a = 0 ? I4a : 90;     \n\
            int I5b = 5 ? I4b : 100;    \n\
            int I6 = 0 ? : I5a;         \n\
            return 0.;                  \n\
        }                               \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary5", scrip);
    size_t const codesize = 82;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           20,   29,    3,    6,    // 7
       3,   30,   29,    3,            6,    3,   50,   29,    // 15
       3,    6,    3,    3,           29,    3,    6,    3,    // 23
       7,   29,    3,   51,           16,    7,    3,   30,    // 31
       4,   11,    4,    3,            3,    4,    3,   29,    // 39
       3,   51,   12,    7,            3,   29,    3,   51,    // 47
       8,    7,    3,   29,            3,    6,    3,   80,    // 55
      29,    3,    6,    3,           90,   29,    3,   51,    // 63
       8,    7,    3,   29,            3,   51,    8,    7,    // 71
       3,   29,    3,    6,            3,    0,    2,    1,    // 79
      44,    5,  -999
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

    const char inpl[] = "\
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
    size_t const codesize = 89;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    3,    2,    3,    // 7
      64,    3,   51,    0,           47,    3,    1,    1,    // 15
       4,    6,    2,  200,            7,    3,    6,    4,    // 23
      -1,   12,    4,    3,            3,    4,    3,    6,    // 31
       4,   -1,   12,    4,            3,    3,    4,    3,    // 39
      29,    3,    6,    3,            2,   30,    4,   15,    // 47
       4,    3,    3,    4,            3,   28,    6,   51,    // 55
       4,   48,    3,   31,            8,    6,    2,    0,    // 63
       3,    2,    3,   64,            3,   29,    3,   51,    // 71
       4,   50,    3,   51,            8,   49,   51,    4,    // 79
      48,    3,   69,   30,            4,    2,    1,    4,    // 87
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   19,   63,  -999
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

    const char inpl[] = "\
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
    size_t const codesize = 131;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           10,    6,    2,    2,    // 7
       3,    3,    5,    3,            2,    4,    6,    7,    // 15
     199,    3,    4,    2,            7,    3,    3,    5,    // 23
       2,    8,    3,   28,           25,    1,    4,    1,    // 31
       1,    5,    1,    2,            7,    1,    3,    7,    // 39
       3,   70,  -26,    1,            5,    1,    3,    5,    // 47
       2,    6,    3,    0,            8,    3,    6,    2,    // 55
       2,    3,    2,    3,           29,    3,    6,    2,    // 63
    1816,    7,    3,   46,            3,    3,   32,    3,    // 71
     404,    6,    2,  404,           11,    2,    3,   30,    // 79
       3,    1,    2,    2,            3,    3,    5,    3,    // 87
       2,    4,    6,    7,          199,    3,    4,    2,    // 95
       7,    3,    3,    5,            2,    8,    3,   28,    // 103
      25,    1,    4,    1,            1,    5,    1,    2,    // 111
       7,    1,    3,    7,            3,   70,  -26,    1,    // 119
       5,    1,    3,    5,            2,    6,    3,    0,    // 127
       8,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,    7,   56,   64,         75,  -999
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

    const char inpl[] = "\
        managed struct Struct       \n\
        {                           \n\
            short Pad1;             \n\
            string ST1;             \n\
            short Pad2;             \n\
            string ST2;             \n\
        };                          \n\
                                    \n\
        void main()                 \n\
        {                           \n\
            Struct S1 = new Struct; \n\
            Struct S2[] = new Struct[3];     \n\
            S1.ST1 = \"-schuh\";    \n\
            S2[2].ST1 = S1.ST1;     \n\
        }                           \n\
        ";

    ccSetOption(SCOPT_OLDSTRINGS, true);

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StructWOldstyleString2", scrip);
    size_t const codesize = 164;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,          404,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3,    3,   72,    // 15
       3,    4,    1,   51,            0,   47,    3,    1,    // 23
       1,    4,    6,    3,            0,   51,    8,   48,    // 31
       2,   52,    1,    2,            2,    3,    3,    5,    // 39
       3,    2,    4,    6,            7,  199,    3,    4,    // 47
       2,    7,    3,    3,            5,    2,    8,    3,    // 55
      28,   25,    1,    4,            1,    1,    5,    1,    // 63
       2,    7,    1,    3,            7,    3,   70,  -26,    // 71
       1,    5,    1,    3,            5,    2,    6,    3,    // 79
       0,    8,    3,   51,            8,   48,    2,   52,    // 87
       1,    2,    2,    3,            2,    3,   51,    4,    // 95
      48,    2,   52,    1,            2,    8,   48,    2,    // 103
      52,    1,    2,    2,            3,    3,    5,    3,    // 111
       2,    4,    6,    7,          199,    3,    4,    2,    // 119
       7,    3,    3,    5,            2,    8,    3,   28,    // 127
      25,    1,    4,    1,            1,    5,    1,    2,    // 135
       7,    1,    3,    7,            3,   70,  -26,    1,    // 143
       5,    1,    3,    5,            2,    6,    3,    0,    // 151
       8,    3,   51,    8,           49,   51,    4,   49,    // 159
       2,    1,    8,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      28,  -999
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

    const char inpl[] = "\
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
    size_t const codesize = 60;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    3,    6,            2,   52,    3,    2,    // 7
       3,   51,    0,   47,            3,    1,    1,    4,    // 15
       3,    6,    2,   52,            3,    2,    3,   29,    // 23
       3,    6,    2,    0,           48,    3,   30,    4,    // 31
      15,    4,    3,    3,            4,    3,   28,   10,    // 39
       6,    3,    1,   51,            4,   49,    2,    1,    // 47
       4,    5,   51,    4,           49,    2,    1,    4,    // 55
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      27,  -999
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

    const char inpl[] = "\
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

    size_t const codesize = 51;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    2,   52,    // 7
      29,    6,   45,    2,           39,    0,    6,    3,    // 15
       0,   33,    3,   30,            6,    1,    3,    1,    // 23
       6,    2,    0,   48,            2,   52,   29,    6,    // 31
      34,    3,   45,    2,           39,    1,    6,    3,    // 39
       1,   33,    3,   35,            1,   30,    6,    6,    // 47
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   16,   26,   40,        -999
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

    const char inpl[] = "\
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
    size_t const codesize = 52;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    2,   52,    // 7
      29,    6,   45,    2,           39,    0,    6,    3,    // 15
       0,   33,    3,   30,            6,   29,    3,    1,    // 23
       3,    1,    6,    2,            0,   48,    2,   52,    // 31
      29,    6,   34,    3,           45,    2,   39,    1,    // 39
       6,    3,    1,   33,            3,   35,    1,   30,    // 47
       6,   30,    3,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 4;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   16,   28,   42,        -999
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

    const char inpl[] = "\
        int foo ()                          \n\
        {                                   \n\
            int I;                          \n\
            return 1 + --I;                 \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CementInExpression1", scrip);
    size_t const codesize = 35;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,    1,   29,    3,           51,    8,    7,    3,    // 15
       2,    3,    1,    8,            3,    7,    3,   30,    // 23
       4,   11,    4,    3,            3,    4,    3,    2,    // 31
       1,    4,    5,  -999
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

    const char inpl[] = "\
        int foo ()                          \n\
        {                                   \n\
            char Ch;                        \n\
            return Ch-- - 1;                \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CrementInExpression2", scrip);
    size_t const codesize = 38;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    0,           63,    1,    1,    1,    // 7
       1,   51,    1,   24,            3,    2,    3,    1,    // 15
      26,    3,    1,    3,            1,   29,    3,    6,    // 23
       3,    1,   30,    4,           12,    4,    3,    3,    // 31
       4,    3,    2,    1,            1,    5,  -999
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

    const char inpl[] = "\
        int foo ()                          \n\
        {                                   \n\
            int I = 7;                      \n\
            short J = 9;                    \n\
                                            \n\
            if (++I == (J)--)               \n\
                --J;                        \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("CrementInExpression3", scrip);
    size_t const codesize = 68;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            7,   29,    3,    6,    // 7
       3,    9,   51,    0,           27,    3,    1,    1,    // 15
       2,   51,    6,    7,            3,    1,    3,    1,    // 23
       8,    3,    7,    3,           29,    3,   51,    6,    // 31
      25,    3,    2,    3,            1,   27,    3,    1,    // 39
       3,    1,   30,    4,           15,    4,    3,    3,    // 47
       4,    3,   28,    9,           51,    2,   25,    3,    // 55
       2,    3,    1,   27,            3,    2,    1,    6,    // 63
       6,    3,    0,    5,          -999
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

    const char inpl[] = "\
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
    size_t const codesize = 69;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    3,   29,    // 7
       3,    6,    3,    0,           30,    4,   16,    4,    // 15
       3,    3,    4,    3,           29,    3,    6,    2,    // 23
       0,   48,    3,   29,            3,    6,    3,    0,    // 31
      30,    4,   15,    4,            3,    3,    4,    3,    // 39
      29,    3,    6,    3,            0,   29,    3,    6,    // 47
       2,    0,   48,    3,           30,    4,   16,    4,    // 55
       3,    3,    4,    3,           29,    3,    2,    1,    // 63
      12,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   24,   49,  -999
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

    const char inpl[] = "\
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

    // WriteOutput("DynarrayLength1", scrip);
    size_t const codesize = 32;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            5,   72,    3,    4,    // 7
       1,    6,    2,    0,           47,    3,    6,    2,    // 15
       0,   48,    2,   52,           34,    2,   39,    1,    // 23
       6,    3,    0,   33,            3,   35,    1,    5,    // 31
     -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      11,   16,   26,  -999
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

    const char inpl[] = "\
        int foo ()                          \n\
        {                                   \n\
            int Dynarray[] = new int[7];    \n\
            int len = Dynarray.Length;      \n\
        }                                   \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("DynarrayLength2", scrip);

    size_t const codesize = 44;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            7,   72,    3,    4,    // 7
       0,   51,    0,   47,            3,    1,    1,    4,    // 15
      51,    4,   48,    2,           52,   34,    2,   39,    // 23
       1,    6,    3,    0,           33,    3,   35,    1,    // 31
      29,    3,   51,    8,           49,    2,    1,    8,    // 39
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      27,  -999
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

TEST_F(Bytecode1, StringLiteral2String) {

    const char inpl[] = "\
        internalstring autoptr builtin managed struct String    \n\
        {};                     \n\
                                \n\
        struct StructWithString \n\
        {                       \n\
            String Txt;         \n\
        };                      \n\
                                \n\
        int func1()             \n\
        {                       \n\
            StructWithString a; \n\
            a.Txt = \"Cause bug!\"; \n\
        }                       \n\
        ";

    int compileResult = cc_compile(inpl, scrip);
    EXPECT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("StringLiteral2String", scrip);

    size_t const codesize = 26;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   29,    3,    6,    // 7
       3,    0,   51,    4,           64,    3,   47,    3,    // 15
      51,    4,   49,    2,            1,    4,    6,    3,    // 23
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,  -999
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
    char *inpl = "\
        int I = - 2147483648;                   \n\
                                                \n\
        int test(int foo = -2147483648)         \n\
        {                                       \n\
            int i2 = -1 - -2147483648;          \n\
            return test() + (2 + -2147483648);  \n\
        }\n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // WriteOutput("LongMin1", scrip);
    size_t const codesize = 37;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,         2147483647,   29,    3,    6,    // 7
       3, -2147483648,   29,    3,            6,    3,    0,   23,    // 15
       3,    2,    1,    4,           29,    3,    6,    3,    // 23
    -2147483646,   30,    4,   11,            4,    3,    3,    4,    // 31
       3,    2,    1,    4,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    size_t const numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      14,  -999
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
