//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <string>

#include "gtest/gtest.h"

#include "script/cc_common.h"

#include "script2/cc_symboltable.h"
#include "script2/cc_internallist.h"
#include "script2/cs_parser.h"

#include "cc_parser_test_lib.h"


// The vars defined here are provided in each test that is in category "Compile1"
class Compile2 : public ::testing::Test
{
public:
    FlagSet const kNoOptions = 0u;

protected:
    ccCompiledScript scrip = ccCompiledScript(); // Note: calls Init();
    MessageHandler mh;

    Compile2()
    {
		// Initialisations for each test
        // Note: Don't use 'ccSetOption()', 'ccResetOptions()' etc.
        // googletests are often called in parallel, and these clobber each other
    }
};

TEST_F(Compile2, CompileTimeConstant1)
{
    char const *inpl = "\
        const int CI = 4711;                    \n\
        const float Euler = 2.718281828459045;  \n\
        const float AroundOne = Euler / Euler;  \n\
        float Array[CI];                        \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}

TEST_F(Compile2, CompileTimeConstant2)
{
    char const *inpl = "\
        int main() {                            \n\
            while (1)                           \n\
            {                                   \n\
                const int CI2 = 4712;           \n\
            }                                   \n\
            float CI2;                          \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}

TEST_F(Compile2, CompileTimeConstant3)
{
    char const *inpl = "\
        struct Str                          \n\
        {                                   \n\
            int stuff;                      \n\
            const int foo = 17;             \n\
            static const int foo_squared =  \n\
                Str.foo * Str.foo;          \n\
        } s;                                \n\
                                            \n\
        int main() {                        \n\
            return s.foo;                   \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}

TEST_F(Compile2, CompileTimeConstant4a)
{
    char const *inpl = "\
        import const int C = 42; \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("import"));
}

TEST_F(Compile2, CompileTimeConstant4b)
{
    char const *inpl = "\
        readonly const int C = 42; \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("readonly"));
}

TEST_F(Compile2, CompileTimeConstant5a)
{
    // Cannot define a compile-time constant of type 'short'

    char const *inpl = "\
        const short S = 42; \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'short'"));  
}

TEST_F(Compile2, CompileTimeConstant5b)
{
    // Cannot define a compile-time constant array

    char const *inpl = "\
        const int C[]; \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("array"));
}

TEST_F(Compile2, CompileTimeConstant5c)
{
    // Misplaced '[]'

    char const *inpl = "\
        const int[] C; \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'['"));
}

TEST_F(Compile2, CompileTimeConstant6)
{
    // Can't re-use a compile-time constant that is in use

    char const *inpl = "\
            const float pi = 3.14;  \n\
        int main() {                \n\
            float pi = 3.141;       \n\
        }                           \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("in use"));
}

TEST_F(Compile2, BinaryCompileTimeEval1) {

    // Checks binary compile time evaluations for integers.

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (4 + 3) / 0;                 \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'7 /"));
}

TEST_F(Compile2, BinaryCompileTimeEval2) {

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073741824 + 1073741824);   \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("Overflow"));

}

TEST_F(Compile2, BinaryCompileTimeEval3) {

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073741824 + 1073741823);   \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}

TEST_F(Compile2, CTEvalIntPlus1) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (4 + 3) / 0;                 \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'7 /"));
}

TEST_F(Compile2, CTEvalIntPlus2) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073741824 + 1073741824);   \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("Overflow"));
}

TEST_F(Compile2, CTEvalIntPlus3) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073741824 + -1073741823); \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}


TEST_F(Compile2, CTEvalIntMinus1) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (83 - 95) / 0;               \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'-12 /"));
}

TEST_F(Compile2, CTEvalIntMinus2) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073741824 - 1073741824);  \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("Overflow"));

}

TEST_F(Compile2, CTEvalIntMinus3) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073741824 - 1073741823);  \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}

TEST_F(Compile2, CTEvalIntMultiply1) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (33 * -39) / 0;              \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'-1287 /"));
}

TEST_F(Compile2, CTEvalIntMultiply2) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (46341 * 46341);             \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("Overflow"));

}

TEST_F(Compile2, CTEvalIntMultiply3) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (46341 * 46340);             \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STREQ("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
}

TEST_F(Compile2, CTEvalIntDivide) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (52 / 8) / 0;                \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'6 /"));
}

TEST_F(Compile2, CTEvalIntModulo1) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (95 % 17) / 0;               \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'10 /"));

}

TEST_F(Compile2, CTEvalIntModulo2) {


    char const *inpl = inpl = "\
        int main()                              \n\
        {                                       \n\
            return (46341 % -0);                \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("Modulo zero"));
}

TEST_F(Compile2, CTEvalIntShiftLeft1) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (60 << 3) / 0;               \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'480 /"));
}

TEST_F(Compile2, CTEvalIntShiftLeft2) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return 536870912 << 2;              \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("Overflow"));
}

TEST_F(Compile2, CTEvalIntShiftLeft3) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-1073 << 4) / 0; \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'-17168 /"));
}

TEST_F(Compile2, CTEvalIntShiftLeft4) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073 << 0) / 0; \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'1073 /"));
}

TEST_F(Compile2, CTEvalIntShiftLeft5) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return 1073 << -5;                  \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("egative shift"));
}

TEST_F(Compile2, CTEvalIntShiftRight1) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (60 >> 3) / 0;               \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'7 /"));
 }

TEST_F(Compile2, CTEvalIntShiftRight2) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (-10730 >> 4) / 0; \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'-671 /"));

}

TEST_F(Compile2, CTEvalIntShiftRight3) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (1073 >> 0) / 0; \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'1073 /"));
}

TEST_F(Compile2, CTEvalIntShiftRight4) {


    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return 1073 >> -5;                  \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("egative shift"));
}

TEST_F(Compile2, CTEvalIntComparisons) {

    // Will fail as soon as any one of those comparisons go awry

    char const *inpl = "\
        int main()                          \n\
        {                                   \n\
            return                          \n\
                (          (7 == 77)        \n\
                + 2 *      (7 >= 77)        \n\
                + 4 *      (7 >  77)        \n\
                + 8 *      (7 <= 77)        \n\
                + 16 *     (7 <  77)        \n\
                + 32 *     (7 != 77)        \n\
                + 64 *     (77 == 7)        \n\
                + 128 *    (77 >= 7)        \n\
                + 256 *    (77 >  7)        \n\
                + 512 *    (77 <= 7)        \n\
                + 1024 *   (77 <  7)        \n\
                + 2048 *   (77 != 7)        \n\
                + 4096 *   (77 == 77)       \n\
                + 81928 *  (77 >= 77)       \n\
                + 16384 *  (77 >  77)       \n\
                + 32768 *  (77 <= 77)       \n\
                + 65536 *  (77 <  77)       \n\
                + 131072 * (77 != 77)) / 0; \n\
        }                                   \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'121280 /"));
}

TEST_F(Compile2, CTEvalBitOps) {

    char const *inpl = "\
        int main()                        \n\
        {                                 \n\
            return                        \n\
                (          (0 & 0)        \n\
                + 2 *      (0 | 0)        \n\
                + 4 *      (0 ^ 0)        \n\
                + 8 *      (0 & 3)        \n\
                + 16 *     (0 | 3)        \n\
                + 32 *     (0 ^ 3)        \n\
                + 64 *     (3 & 0)        \n\
                + 128 *    (3 | 0)        \n\
                + 256 *    (3 ^ 0)        \n\
                + 512 *    (3 & 3)        \n\
                + 1024 *   (3 | 3)        \n\
                + 2048 *   (3 ^ 3)) / 0;  \n\
        }                                 \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'5904 /"));
}

TEST_F(Compile2, CTEvalBitNeg) {

    char const *inpl = "\
        int main()                        \n\
        {                                 \n\
            return (~660753869) / 0;      \n\
        }                                 \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'-660753870 /"));
}

TEST_F(Compile2, CTEvalLogicalOps) {

    char const *inpl = "\
        int main()                              \n\
        {                                       \n\
            return (  100000000 *   (!!7) +     \n\
                      10000000 *    (! 7) +     \n\
                      1000000 *     (! 0) +     \n\
                      100000 *      (5 || 7) +  \n\
                      10000 *       (7 || 0) +  \n\
                      1000 *        (0 || 7) +  \n\
                      100 *         (5 && 7) +  \n\
                      10 *          (7 && 0) +  \n\
                                    (0 && 7) ) / 0;   \n\
        }                                       \n\
        ";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;
    size_t err_line = mh.GetError().Lineno;
    EXPECT_EQ(0u, mh.WarningsCount());
    
    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'101577700 /"));
}

TEST_F(Compile2, MultiDimCharArrayToString) {

    // A character array with more than 1 dimension cannot be
    // converted to 'const string'.

    char const *inpl = R"%&/(
        int test(const string s)
        {
            char arr[5][7];
            return test(arr);
        }
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("convert"));
}

TEST_F(Compile2, ArrayInitLiteralOverflow) {

    // Can't initialize the array because the literal
    // including the terminal '\0' is too long

    char const *inpl = R"%&/(
        char arr[5] = "Super";
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("too long"));
}

TEST_F(Compile2, ArrayInitSequenceTooLong) {

    // Too many values to initialize an array of 5 elements

    char const *inpl = R"%&/(
        short arr[5] = { 1, 2, 3, 4, 5, 6 };
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("at most 5"));
}

TEST_F(Compile2, ArrayInitNamedUnnamedMix01) {

    // Can't mix unnamed and named entries in
    // array initialization sequence

    char const *inpl = R"%&/(
        short arr[5] = { 1, [2]: 5 };
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'['"));
}

TEST_F(Compile2, ArrayInitNamedUnnamedMix02) {

    // Can't mix unnamed and named entries in
    // array initialization sequence

    char const *inpl = R"%&/(
        short arr[5] = { [0]: 1, 2, };
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'2'"));
}

TEST_F(Compile2, ArrayInitNamedOutOfRange01) {

    // Can't mix unnamed and named entries in
    // array initialization sequence

    char const *inpl = R"%&/(
        short arr[5] = { [(3 - 5) / 2]: 1 };
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("too low"));
}

TEST_F(Compile2, ArrayInitNamedOutOfRange02) {

    // Can't mix unnamed and named entries in
    // array initialization sequence

    char const *inpl = R"%&/(
        short arr[5] = { [5]: 1 };
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("too high"));
}

TEST_F(Compile2, ArrayInitNamedDouble) {

    // Can't mix unnamed and named entries in
    // array initialization sequence

    char const *inpl = R"%&/(
        short arr[5] = { [3]: 1,
                         [3]: 2, };
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("[3]"));
}

TEST_F(Compile2, InitManaged01)
{
    
    char const *inpl = R"%&/(
        managed struct Foo
        {
            int Payload;
        } Var = new Foo;
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'null'"));
}
TEST_F(Compile2, InitManaged02)
{
    
    char const *inpl = R"%&/(
        int List[] = new int;
        )%&/";

    int compile_result = cc_compile(inpl, kNoOptions, scrip, mh);
    std::string const &err_msg = mh.GetError().Message;

    ASSERT_STRNE("Ok", mh.HasError() ? err_msg.c_str() : "Ok");
    EXPECT_NE(std::string::npos, err_msg.find("'null'"));
}
