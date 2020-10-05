#include <string>

#include "gtest/gtest.h"
#include "cc_bytecode_test_lib.h"
#include "cc_parser_test_lib.h"

#include "script/cc_options.h"

#include "script2/cs_parser.h"


/* This file is for Bytecode tests ONLY.
   See comments for cc_bytecode_test_0.cpp
*/


// The vars defined here are provided in each test that is in category "Bytecode0"
class Bytecode1 : public ::testing::Test
{
protected:
    ::ccCompiledScript scrip;

    Bytecode1()
    {
        // Initializations, will be done at the start of each test
        scrip.init();
        ccSetOption(SCOPT_NOIMPORTOVERRIDE, false);
        ccSetOption(SCOPT_LINENUMBERS, false);
        clear_error();
    }
};


TEST_F(Bytecode1, StringOldstyle03) {
    
    char *inpl = "\
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
    
    char *inpl = "\
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
    const size_t codesize = 89;
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
       3,    2,    3,   31,            3,    6,    3,    0,    // 87
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   12,  -999
    };
    char fixuptypes[] = {
      1,   2,  '\0'
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

TEST_F(Bytecode1, StringOldstyle05) {
    
    char *inpl = "\
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

TEST_F(Bytecode1, StringStandard05) {
    
    char inpl[] = "\
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
    const size_t codesize = 112;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,   48,    3,   31,    // 7
       3,    6,    3,    0,            5,   38,   13,   51,    // 15
       8,    7,    3,   50,            3,   51,    8,   48,    // 23
       3,   29,    3,   51,            4,   50,    3,   51,    // 31
      12,   49,   51,    4,           48,    3,   69,   30,    // 39
       4,   31,    6,   51,            8,   49,    6,    3,    // 47
       0,    5,   38,   50,            6,    3,    7,   64,    // 55
       3,   51,    0,   47,            3,    1,    1,    4,    // 63
      51,    4,   48,    3,           29,    3,   51,    4,    // 71
      50,    3,   51,    8,           49,   51,    4,   48,    // 79
       3,   69,   30,    4,            2,    1,    4,   31,    // 87
       9,   51,    4,   49,            2,    1,    4,    6,    // 95
       3,    0,    5,   38,           99,    6,    3,    7,    // 103
      64,    3,   31,    3,            6,    3,    0,    5,    // 111
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   54,  103,  -999
    };
    char fixuptypes[] = {
      1,   3,   3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 14;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'H',  'e',  'l',  'l',          'o',  '!',    0,  'H',     // 7
    'e',  'l',  'l',  'o',          '!',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StringStandardOldstyle) {

    char inpl[] = "\
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
        String Func4()                      \n\
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
    const size_t codesize = 114;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    3,    2,    3,    // 7
      64,    3,   31,    3,            6,    3,    0,    5,    // 15
      38,   16,   51,    8,            7,    3,   50,    3,    // 23
      51,    8,   48,    3,           29,    3,   51,    4,    // 31
      50,    3,   51,   12,           49,   51,    4,   48,    // 39
       3,   69,   30,    4,           31,    6,   51,    8,    // 47
      49,    6,    3,    0,            5,   38,   53,    6,    // 55
       3,    6,   64,    3,           29,    3,    6,    3,    // 63
      16,   23,    3,    2,            1,    4,    6,    3,    // 71
       0,    5,   38,   74,            6,    3,    6,   64,    // 79
       3,   51,    0,   47,            3,    1,    1,    4,    // 87
      51,    4,   48,    3,           67,    3,   29,    3,    // 95
       6,    3,   53,   23,            3,    2,    1,    4,    // 103
      51,    4,   49,    2,            1,    4,    6,    3,    // 111
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   57,   64,   78,         98,  -999
    };
    char fixuptypes[] = {
      1,   3,   2,   3,      2,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 12;
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

    char *inpl = "\
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

    char *inpl = "\
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

    char *inpl = "\
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

TEST_F(Bytecode1, Attributes04) {
    
    // Attribute func was not called properly

    char *inpl = "\
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

    char *inpl = "\
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

    char *inpl = "\
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
    const size_t codesize = 26;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,    6,    2,   22,    // 7
      29,    6,   34,    3,           45,    2,   39,    1,    // 15
       6,    3,   21,   33,            3,   35,    1,   30,    // 23
       6,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 3;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,    7,   18,  -999
    };
    char fixuptypes[] = {
      3,   4,   4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 2;
    std::string imports[] = {
    "Label::set_Text^1",          "lbl",          "[[SENTINEL]]"
    };

    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 1;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
      0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, Struct09) {
    
    // Should be able to find SetCharacter as a component of
    // VehicleBase as an extension of Vehicle Cars[5];
    // should generate call of VehicleBase::SetCharacter()

    char *inpl = "\
        enum CharacterDirection                                     \n\
        {                                                           \n\
            eDirectionUp = 3                                        \n\
        };                                                          \n\
                                                                    \n\
        builtin managed struct Character                            \n\
        {                                                           \n\
            readonly import attribute int ID;                       \n\
        };                                                          \n\
        import Character character[7];                              \n\
        import Character cAICar1;                                   \n\
                                                                    \n\
        struct VehicleBase                                          \n\
        {                                                           \n\
            import void SetCharacter(Character *c,                  \n\
                                int carSprite,                      \n\
                                CharacterDirection carSpriteDir,    \n\
                                int view = 0,                       \n\
                                int loop = 0,                       \n\
                                int frame = 0);                     \n\
        };                                                          \n\
                                                                    \n\
        struct Vehicle extends VehicleBase                          \n\
        {                                                           \n\
            float bodyMass;                                         \n\
        };                                                          \n\
        import Vehicle Cars[6];                                     \n\
                                                                    \n\
        int main()                                                  \n\
        {                                                           \n\
            int drivers[] = new int[6];                             \n\
            int i = 5;                                              \n\
            Cars[i].SetCharacter(                                   \n\
                character[cAICar1.ID + i],                          \n\
                7 + drivers[i],                                     \n\
                eDirectionUp,                                       \n\
                3 + i, 0, 0);                                       \n\
        }                                                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());
    // WriteOutput("Struct09", scrip);
    const size_t codesize = 193;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            6,   72,    3,    4,    // 7
       0,   51,    0,   47,            3,    1,    1,    4,    // 15
       6,    3,    5,   29,            3,    6,    2,    4,    // 23
      29,    2,   51,    8,            7,    3,   30,    2,    // 31
      46,    3,    6,   32,            3,    4,   11,    2,    // 39
       3,   29,    2,    6,            3,    0,   34,    3,    // 47
       6,    3,    0,   34,            3,    6,    3,    3,    // 55
      29,    3,   51,   12,            7,    3,   30,    4,    // 63
      11,    4,    3,    3,            4,    3,   34,    3,    // 71
       6,    3,    3,   34,            3,    6,    3,    7,    // 79
      29,    3,   51,   16,           48,    2,   52,   29,    // 87
       2,   51,   16,    7,            3,   30,    2,   32,    // 95
       3,    4,   71,    3,           11,    2,    3,    7,    // 103
       3,   30,    4,   11,            4,    3,    3,    4,    // 111
       3,   34,    3,    6,            2,    1,   29,    2,    // 119
       6,    2,    2,   29,            6,   45,    2,   39,    // 127
       0,    6,    3,    0,           33,    3,   30,    6,    // 135
      29,    3,   51,   16,            7,    3,   30,    4,    // 143
      11,    4,    3,    3,            4,    3,   30,    2,    // 151
      46,    3,    7,   32,            3,    0,   11,    2,    // 159
       3,    3,    2,    3,           34,    3,   51,    4,    // 167
       7,    2,   45,    2,           39,    6,    6,    3,    // 175
       3,   33,    3,   35,            6,   30,    2,   51,    // 183
       8,   49,    2,    1,            8,    6,    3,    0,    // 191
       5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      23,  117,  122,  131,        176,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 5;
    std::string imports[] = {
    "Character::get_ID^0",        "character",   "cAICar1",     "VehicleBase::SetCharacter^6",               // 3
    "Cars",         "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Struct10) {
    
    // When accessing a component of an import variable,
    // the import variable must be read first so that the fixup can be
    // applied. Only then may the offset be added to it.

    char *inpl = "\
        import struct Struct                                 \n\
        {                                                    \n\
            int fluff;                                       \n\
            int k;                                           \n\
        } ss;                                                \n\
                                                             \n\
        int main()                                           \n\
        {                                                    \n\
            return ss.k;                                     \n\
        }                                                    \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct10", scrip);
    const size_t codesize = 16;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    1,    2,    4,    // 7
       7,    3,   31,    3,            6,    3,    0,    5,    // 15
     -999
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

    const int numimports = 1;
    std::string imports[] = {
    "ss",           "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Struct11) {
    
    // Structs may contain variables that are structs themselves.
    // Since Inner1 is managed, In1 will convert into an Inner1 *.

    char *inpl = "\
        managed struct Inner1                               \n\
        {                                                   \n\
            short Fluff;                                    \n\
            int Payload;                                    \n\
        };                                                  \n\
        struct Inner2                                       \n\
        {                                                   \n\
            short Fluff;                                    \n\
            int Payload;                                    \n\
        };                                                  \n\
        import int Foo;                                     \n\
        import struct Struct                                \n\
        {                                                   \n\
            Inner1 In1;                                     \n\
            Inner2 In2;                                     \n\
        } SS;                                               \n\
                                                            \n\
        int main()                                          \n\
        {                                                   \n\
            SS.In1 = new Inner1;                            \n\
            SS.In1.Payload = 77;                            \n\
            SS.In2.Payload = 777;                           \n\
            return SS.In1.Payload + SS.In2.Payload;         \n\
        }                                                   \n\
    ";
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct11", scrip);
    const size_t codesize = 78;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            8,    6,    2,    1,    // 7
      47,    3,    6,    3,           77,   29,    3,    6,    // 15
       2,    1,   48,    2,           52,    1,    2,    2,    // 23
      30,    3,    8,    3,            6,    3,  777,   29,    // 31
       3,    6,    2,    1,            1,    2,    6,   30,    // 39
       3,    8,    3,    6,            2,    1,   48,    2,    // 47
      52,    1,    2,    2,            7,    3,   29,    3,    // 55
       6,    2,    1,    1,            2,    6,    7,    3,    // 63
      30,    4,   11,    4,            3,    3,    4,    3,    // 71
      31,    3,    6,    3,            0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   17,   35,   45,         58,  -999
    };
    char fixuptypes[] = {
      4,   4,   4,   4,      4,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 1;
    std::string imports[] = {
    "SS",           "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 0;
    EXPECT_EQ(stringssize, scrip.stringssize);
}

TEST_F(Bytecode1, Struct12) {
    
    // Managed structs may contain dynamic arrays.

    char *inpl = "\
        managed struct Inner                                \n\
        {                                                   \n\
            short Fluff;                                    \n\
            int Payload;                                    \n\
        };                                                  \n\
        short Fluff;                                        \n\
        managed struct Struct                               \n\
        {                                                   \n\
            Inner In[];                                     \n\
        } SS, TT[];                                         \n\
                                                            \n\
        int main()                                          \n\
        {                                                   \n\
            SS = new Struct;                                \n\
            SS.In = new Inner[7];                           \n\
            SS.In[3].Payload = 77;                          \n\
            TT = new Struct[5];                             \n\
            TT[2].In = new Inner[11];                       \n\
            TT[2].In[2].Payload = 777;                      \n\
            return SS.In[3].Payload + TT[2].In[2].Payload;  \n\
        }                                                   \n\
    ";
    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Struct12", scrip);
    const size_t codesize = 184;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            4,    6,    2,    2,    // 7
      47,    3,    6,    3,            7,   72,    3,    4,    // 15
       1,    6,    2,    2,           48,    2,   52,   47,    // 23
       3,    6,    3,   77,           29,    3,    6,    2,    // 31
       2,   48,    2,   52,           48,    2,   52,    1,    // 39
       2,   12,   48,    2,           52,    1,    2,    2,    // 47
      30,    3,    8,    3,            6,    3,    5,   72,    // 55
       3,    4,    1,    6,            2,    6,   47,    3,    // 63
       6,    3,   11,   72,            3,    4,    1,   29,    // 71
       3,    6,    2,    6,           48,    2,   52,    1,    // 79
       2,    8,   48,    2,           52,   30,    3,   47,    // 87
       3,    6,    3,  777,           29,    3,    6,    2,    // 95
       6,   48,    2,   52,            1,    2,    8,   48,    // 103
       2,   52,   48,    2,           52,    1,    2,    8,    // 111
      48,    2,   52,    1,            2,    2,   30,    3,    // 119
       8,    3,    6,    2,            2,   48,    2,   52,    // 127
      48,    2,   52,    1,            2,   12,   48,    2,    // 135
      52,    1,    2,    2,            7,    3,   29,    3,    // 143
       6,    2,    6,   48,            2,   52,    1,    2,    // 151
       8,   48,    2,   52,           48,    2,   52,    1,    // 159
       2,    8,   48,    2,           52,    1,    2,    2,    // 167
       7,    3,   30,    4,           11,    4,    3,    3,    // 175
       4,    3,   31,    3,            6,    3,    0,    5,    // 183
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 8;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       7,   19,   32,   61,         75,   96,  124,  146,    // 7
     -999
    };
    char fixuptypes[] = {
      1,   1,   1,   1,      1,   1,   1,   1,    // 7
     '\0'
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

TEST_F(Bytecode1, DynArrayOfPrimitives) {
    
    // Dynamic arrays of primitives are allowed.

    char *inpl = "\
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
    const size_t codesize = 67;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,           10,   72,    3,    2,    // 7
       0,   51,    0,   47,            3,    1,    1,    4,    // 15
       6,    3,    0,   29,            3,   51,    8,   48,    // 23
       2,   52,    1,    2,           14,   30,    3,   27,    // 31
       3,   51,    4,   48,            2,   52,    1,    2,    // 39
      14,   25,    3,   29,            3,   51,    8,   48,    // 47
       2,   52,    1,    2,            6,   30,    3,   27,    // 55
       3,   51,    4,   49,            2,    1,    4,    6,    // 63
       3,    0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

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

TEST_F(Bytecode1, ManagedDerefZerocheck) {
    
    // Bytecode ought to check that S isn't initialized yet
    char *inpl = "\
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
    const size_t codesize = 24;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            1,   29,    3,    6,    // 7
       2,    0,   48,    2,           52,    1,    2,   16,    // 15
      30,    3,    8,    3,            6,    3,    0,    5,    // 23
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       9,  -999
    };
    char fixuptypes[] = {
      1,  '\0'
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

TEST_F(Bytecode1, MemInitPtr1) {
    
    // Check that pointer vars are pushed correctly in func calls

    char *inpl = "\
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
    const size_t codesize = 116;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   73,    3,            4,   51,    0,   47,    // 7
       3,    1,    1,    4,            6,    3, 1060320051,   51,    // 15
       4,   48,    2,   52,            8,    3,   73,    3,    // 23
       4,   51,    0,   47,            3,    1,    1,    4,    // 31
       6,    3,    4,   51,            4,   48,    2,   52,    // 39
      26,    3,   51,    4,           48,    3,   29,    3,    // 47
      51,   12,   48,    3,           29,    3,    6,    3,    // 55
      77,   23,    3,    2,            1,    8,   29,    3,    // 63
      51,   12,   49,   51,            8,   49,    2,    1,    // 71
      12,    6,    3,    0,            5,   38,   77,   51,    // 79
       8,    7,    3,   50,            3,   51,   12,    7,    // 87
       3,   50,    3,   51,           12,   48,    2,   52,    // 95
      24,    3,   51,    8,           49,   51,   12,   49,    // 103
      31,    9,   51,    8,           49,   51,   12,   49,    // 111
       6,    3,    0,    5,          -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      56,  -999
    };
    char fixuptypes[] = {
      2,  '\0'
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

TEST_F(Bytecode1, Ternary1) {

    // Accept a simple ternary expression

    
    char *inpl = "\
    int Foo(int i)              \n\
    {                           \n\
        return i > 0 ? 1 : -1;  \n\
        return 9;               \n\
    }                           \n\
    ";

    int compileResult = cc_compile(inpl, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary1", scrip);
    const size_t codesize = 40;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,   51,    8,            7,    3,   29,    3,    // 7
       6,    3,    0,   30,            4,   17,    4,    3,    // 15
       3,    4,    3,   28,            5,    6,    3,    1,    // 23
      31,    3,    6,    3,           -1,   31,    8,    6,    // 31
       3,    9,   31,    3,            6,    3,    0,    5,    // 39
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

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

TEST_F(Bytecode1, Ternary2) {
    
    // Accept Elvis operator expression

    char *inpl = "\
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

    char *inpl = "\
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
    const size_t codesize = 77;
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
      99,    2,    1,    8,           31,    6,    2,    1,    // 71
       8,    6,    3,    0,            5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 0;
    EXPECT_EQ(numfixups, scrip.numfixups);

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

TEST_F(Bytecode1, Ternary4) {
    
    // String / literal string and conversion.

    char inpl[] = "\
        String main()                       \n\
        {                                   \n\
            String test = \"Test\";         \n\
            return 2 < 1 ? test : \"Foo\";  \n\
        }                                   \n\
        ";
    std::string input = g_Input_Bool;
    input += g_Input_String;
    input += inpl;

    int compileResult = cc_compile(input, scrip);
    ASSERT_STREQ("Ok", (compileResult >= 0) ? "Ok" : last_seen_cc_error());

    // WriteOutput("Ternary4", scrip);
    const size_t codesize = 74;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    3,            0,   64,    3,   51,    // 7
       0,   47,    3,    1,            1,    4,    6,    3,    // 15
       2,   29,    3,    6,            3,    1,   30,    4,    // 23
      18,    4,    3,    3,            4,    3,   28,    6,    // 31
      51,    4,   48,    3,           31,    5,    6,    3,    // 39
       5,   64,    3,   29,            3,   51,    4,   50,    // 47
       3,   51,    8,   49,           51,    4,   48,    3,    // 55
      69,   30,    4,    2,            1,    4,   31,    9,    // 63
      51,    4,   49,    2,            1,    4,    6,    3,    // 71
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   40,  -999
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

    const size_t stringssize = 9;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    'T',  'e',  's',  't',            0,  'F',  'o',  'o',     // 7
      0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, AssignToString) {
    
    // Definition of global string with assignment

    char inpl[] = "\
        string Payload = \"Holzschuh\";     \n\
        String main()                       \n\
        {                                   \n\
            String test = Payload;          \n\
            return (~~1 == 2) ? test : Payload;  \n\
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
    const size_t codesize = 98;
    EXPECT_EQ(codesize, scrip.codesize);

    int32_t code[] = {
      38,    0,    6,    2,            0,    3,    2,    3,    // 7
      64,    3,   51,    0,           47,    3,    1,    1,    // 15
       4,    6,    3,    1,            6,    4,   -1,   12,    // 23
       4,    3,    3,    4,            3,    6,    4,   -1,    // 31
      12,    4,    3,    3,            4,    3,   29,    3,    // 39
       6,    3,    2,   30,            4,   15,    4,    3,    // 47
       3,    4,    3,   28,            6,   51,    4,   48,    // 55
       3,   31,    8,    6,            2,    0,    3,    2,    // 63
       3,   64,    3,   29,            3,   51,    4,   50,    // 71
       3,   51,    8,   49,           51,    4,   48,    3,    // 79
      69,   30,    4,    2,            1,    4,   31,    9,    // 87
      51,    4,   49,    2,            1,    4,    6,    3,    // 95
       0,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 2;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,   61,  -999
    };
    char fixuptypes[] = {
      1,   1,  '\0'
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
    'H',  'o',  'l',  'z',          's',  'c',  'h',  'u',     // 7
    'h',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}

TEST_F(Bytecode1, StructWOldstyleString1) {
    
    // Unmanaged structs containing strings

    char inpl[] = "\
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
    const size_t codesize = 135;
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
     404,   29,    2,    6,            2, 1816,    7,    3,    // 71
      30,    2,   46,    3,            3,   32,    3,  404,    // 79
      11,    2,    3,    1,            2,    2,   30,    3,    // 87
       3,    3,    5,    3,            2,    4,    6,    7,    // 95
     199,    3,    4,    2,            7,    3,    3,    5,    // 103
       2,    8,    3,   28,           25,    1,    4,    1,    // 111
       1,    5,    1,    2,            7,    1,    3,    7,    // 119
       3,   70,  -26,    1,            5,    1,    3,    5,    // 127
       2,    6,    3,    0,            8,    3,    5,  -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 5;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
       4,    7,   56,   64,         69,  -999
    };
    char fixuptypes[] = {
      3,   1,   1,   1,      1,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 16;
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

    char inpl[] = "\
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
    const size_t codesize = 168;
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
       1,    2,    2,    3,            2,    3,   29,    3,    // 95
      51,    8,   48,    2,           52,    1,    2,    8,    // 103
      48,    2,   52,    1,            2,    2,   30,    3,    // 111
       3,    3,    5,    3,            2,    4,    6,    7,    // 119
     199,    3,    4,    2,            7,    3,    3,    5,    // 127
       2,    8,    3,   28,           25,    1,    4,    1,    // 135
       1,    5,    1,    2,            7,    1,    3,    7,    // 143
       3,   70,  -26,    1,            5,    1,    3,    5,    // 151
       2,    6,    3,    0,            8,    3,   51,    8,    // 159
      49,   51,    4,   49,            2,    1,    8,    5,    // 167
     -999
    };
    CompareCode(&scrip, codesize, code);

    const size_t numfixups = 1;
    EXPECT_EQ(numfixups, scrip.numfixups);

    int32_t fixups[] = {
      28,  -999
    };
    char fixuptypes[] = {
      3,  '\0'
    };
    CompareFixups(&scrip, numfixups, fixups, fixuptypes);

    const int numimports = 0;
    std::string imports[] = {
     "[[SENTINEL]]"
    };
    CompareImports(&scrip, numimports, imports);

    const size_t numexports = 0;
    EXPECT_EQ(numexports, scrip.numexports);

    const size_t stringssize = 7;
    EXPECT_EQ(stringssize, scrip.stringssize);

    char strings[] = {
    '-',  's',  'c',  'h',          'u',  'h',    0,  '\0'
    };
    CompareStrings(&scrip, stringssize, strings);
}
