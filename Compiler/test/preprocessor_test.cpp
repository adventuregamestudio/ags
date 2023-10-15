#include "gtest/gtest.h"
#include "preproc/preprocessor.h"
#include <util/string_compat.h>
#include "test/cc_test_helper.h"

typedef AGS::Common::String AGSString;

namespace AGS {
namespace Preprocessor {


std::vector<AGSString> SplitLines(const AGSString& str)
{
    std::vector<AGSString> str_lines = str.Split('\n');
    for (int i = 0; i < str_lines.size(); i++) {
        if (str_lines[i].CompareRight("\r", 1) == 0) {
            str_lines[i].ClipRight(1);
        }
    }
    return str_lines;
}


TEST(Preprocess, Comments) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
// this is a comment
// this is another comment
// 1234
//#define invalid 5
// invalid
int i;
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl,"ScriptName");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(),9);
    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_ScriptName\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "");
    ASSERT_STREQ(lines[3].GetCStr(), "");
    ASSERT_STREQ(lines[4].GetCStr(), "");
    ASSERT_STREQ(lines[5].GetCStr(), "");
    ASSERT_STREQ(lines[6].GetCStr(), "");
    ASSERT_STREQ(lines[7].GetCStr(), "int i;");
    ASSERT_STREQ(lines[8].GetCStr(), "");
}


TEST(Preprocess, MultilineComments) {
    Preprocessor pp = Preprocessor();
    // this test is verifying multiline comments
    // and documents current behavior of Editor preprocessor
    const char* inpl = R"EOS(
int hey /* comment  */= 7;
// this  
"this prints";
//;
// /*
Display("this does display!");
// */
/* this
is a real
multiline comment*/
int k;
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl,"MultiLine");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(),14);
    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_MultiLine\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "int hey = 7;");
    ASSERT_STREQ(lines[3].GetCStr(), "");
    ASSERT_STREQ(lines[4].GetCStr(), "\"this prints\";");
    ASSERT_STREQ(lines[5].GetCStr(), "");
    ASSERT_STREQ(lines[6].GetCStr(), "");
    ASSERT_STREQ(lines[7].GetCStr(), "Display(\"this does display!\");");
    ASSERT_STREQ(lines[8].GetCStr(), "");
    ASSERT_STREQ(lines[9].GetCStr(), "");
    ASSERT_STREQ(lines[10].GetCStr(), "");
    ASSERT_STREQ(lines[11].GetCStr(), "");
    ASSERT_STREQ(lines[12].GetCStr(),"int k;");
}


TEST(Preprocess, Define) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#define MACRO2 MACRO3
#define MACRO3 9
#define TEST1 5
Display("a: %d", TEST1);
Display("b: TEST1");
Display("d: %d", MACRO3);
Display("e: %d", MACRO2);
#define MACRO4 MACRO3
Display("f: %d", MACRO4);
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ScriptDefine");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(), 12);
    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_ScriptDefine\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "");
    ASSERT_STREQ(lines[3].GetCStr(), "");
    ASSERT_STREQ(lines[4].GetCStr(), "");
    ASSERT_STREQ(lines[5].GetCStr(), "Display(\"a: %d\", 5);");
    ASSERT_STREQ(lines[6].GetCStr(), "Display(\"b: TEST1\");");
    ASSERT_STREQ(lines[7].GetCStr(), "Display(\"d: %d\", 9);");
    ASSERT_STREQ(lines[8].GetCStr(), "Display(\"e: %d\", 9);");
    ASSERT_STREQ(lines[9].GetCStr(), "");
    ASSERT_STREQ(lines[10].GetCStr(), "Display(\"f: %d\", 9);");
    ASSERT_STREQ(lines[11].GetCStr(), "");
}


TEST(Preprocess, ReDefine) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#define TEST1 5
Display("a: %d", TEST1);
#define TEST1 10
Display("c: %d", TEST1);
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ScriptReDefine");

    EXPECT_STREQ(last_seen_cc_error(), "Macro 'TEST1' is already defined");
}


TEST(Preprocess, MacroDoesNotExist) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#define BAR
#undef FOO
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "MacroDoesNotExist");

    EXPECT_STREQ(last_seen_cc_error(), "Macro 'FOO' is not defined");
}


TEST(Preprocess, MacroNameMissing) {
    Preprocessor pp = Preprocessor();
        const char* inpl = R"EOS(
#define BAR
#undef
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "MacroNameMissing");

    EXPECT_STREQ(last_seen_cc_error(), "Macro name expected");
}


TEST(Preprocess, MacroStartsWithDigit) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#define 1BAR
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "MacroStartsWithDigit");

    EXPECT_STREQ(last_seen_cc_error(), "Macro name '1BAR' cannot start with a digit");
}


TEST(Preprocess, UserError) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#error my error here
Display("this doesn't display");
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ScriptUserError");

    EXPECT_STREQ(last_seen_cc_error(), "User error: my error here");
}


TEST(Preprocess, RemoveEditorDirectives) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#region THIS_IS_DISPLAY
Display("Prints normally");
#endregion THIS_IS_DISPLAY
#sectionstart
Display("Prints normally too");
#sectionend
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "RemoveEditorDirectives");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(), 9);
    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_RemoveEditorDirectives\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "");
    ASSERT_STREQ(lines[3].GetCStr(), "Display(\"Prints normally\");");
    ASSERT_STREQ(lines[4].GetCStr(), "");
    ASSERT_STREQ(lines[5].GetCStr(), "");
    ASSERT_STREQ(lines[6].GetCStr(), "Display(\"Prints normally too\");");
    ASSERT_STREQ(lines[7].GetCStr(), "");
}


TEST(Preprocess, UnknownDirective) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#valhalla
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "UnknownDirective");

    EXPECT_STREQ(last_seen_cc_error(), "Unknown preprocessor directive 'valhalla'");
}


TEST(Preprocess, IfDef) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#define FOO
#ifdef FOO
Display("This displays!");
#endif
#undef FOO
#ifdef FOO
Display("This doesn't");
#endif
#ifdef BAR
Display("This doesn't too");
#endif
#define BAR
#ifdef BAR
Display("This displays dude");
Display("and this too");
#endif
#ifndef BAR
Display("This doesn't");
#endif
#ifndef BORK
Display("This does");
#endif
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ScriptIfDef");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(), 25);

    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_ScriptIfDef\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "");
    ASSERT_STREQ(lines[3].GetCStr(), "");
    ASSERT_STREQ(lines[4].GetCStr(), "Display(\"This displays!\");");
    ASSERT_STREQ(lines[5].GetCStr(), "");
    ASSERT_STREQ(lines[6].GetCStr(), "");
    ASSERT_STREQ(lines[7].GetCStr(), "");
    ASSERT_STREQ(lines[8].GetCStr(), "");
    ASSERT_STREQ(lines[9].GetCStr(), "");
    ASSERT_STREQ(lines[10].GetCStr(), "");
    ASSERT_STREQ(lines[11].GetCStr(), "");
    ASSERT_STREQ(lines[12].GetCStr(), "");
    ASSERT_STREQ(lines[13].GetCStr(), "");
    ASSERT_STREQ(lines[14].GetCStr(), "");
    ASSERT_STREQ(lines[15].GetCStr(), "Display(\"This displays dude\");");
    ASSERT_STREQ(lines[16].GetCStr(), "Display(\"and this too\");");
    ASSERT_STREQ(lines[17].GetCStr(), "");
    ASSERT_STREQ(lines[18].GetCStr(), "");
    ASSERT_STREQ(lines[19].GetCStr(), "");
    ASSERT_STREQ(lines[20].GetCStr(), "");
    ASSERT_STREQ(lines[21].GetCStr(), "");
    ASSERT_STREQ(lines[22].GetCStr(), "Display(\"This does\");");
    ASSERT_STREQ(lines[23].GetCStr(), "");
}


TEST(Preprocess, IfDefElse) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#define FOO
#ifdef FOO
Display("This displays!");
#else
Display("This doesn't");
#endif
#ifndef FOO
Display("This doesn't");
#else
Display("This displays!");
#endif
#undef FOO
#ifdef FOO
Display("This doesn't");
#else
Display("This displays!");
#endif
#ifndef FOO
Display("This displays!");
#else
Display("This doesn't");
#endif
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ScriptIfDefElse");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(), 25);

    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_ScriptIfDefElse\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "");
    ASSERT_STREQ(lines[3].GetCStr(), "");
    ASSERT_STREQ(lines[4].GetCStr(), "Display(\"This displays!\");");
    ASSERT_STREQ(lines[5].GetCStr(), "");
    ASSERT_STREQ(lines[6].GetCStr(), "");
    ASSERT_STREQ(lines[7].GetCStr(), "");
    ASSERT_STREQ(lines[8].GetCStr(), "");
    ASSERT_STREQ(lines[9].GetCStr(), "");
    ASSERT_STREQ(lines[10].GetCStr(), "");
    ASSERT_STREQ(lines[11].GetCStr(), "Display(\"This displays!\");");
    ASSERT_STREQ(lines[12].GetCStr(), "");
    ASSERT_STREQ(lines[13].GetCStr(), "");
    ASSERT_STREQ(lines[14].GetCStr(), "");
    ASSERT_STREQ(lines[15].GetCStr(), "");
    ASSERT_STREQ(lines[16].GetCStr(), "");
    ASSERT_STREQ(lines[17].GetCStr(), "Display(\"This displays!\");");
    ASSERT_STREQ(lines[18].GetCStr(), "");
    ASSERT_STREQ(lines[19].GetCStr(), "");
    ASSERT_STREQ(lines[20].GetCStr(), "Display(\"This displays!\");");
    ASSERT_STREQ(lines[21].GetCStr(), "");
    ASSERT_STREQ(lines[22].GetCStr(), "");
    ASSERT_STREQ(lines[23].GetCStr(), "");
}


TEST(Preprocess, IfVer) {
    Preprocessor pp = Preprocessor();
    pp.SetAppVersion("3.6.0.5");
    const char* inpl = R"EOS(
#ifver 3.6.0
Display("This displays!");
#endif
#ifver 3.7.0
Display("This doesn't");
#endif
#ifver 3.4.0
Display("This displays yey!");
#endif
#ifver 2.7.2
Display("This displays dude");
#endif
#ifver 3.6.0.6
Display("doesn't display");
#endif
#ifnver 3.6.0.6
Display("But this displays");
#endif
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ScriptIfVer");

    EXPECT_STREQ(last_seen_cc_error(), "");

    std::vector<AGSString> lines = SplitLines(res);
    ASSERT_EQ(lines.size(), 21);

    ASSERT_STREQ(lines[0].GetCStr(), "\"__NEWSCRIPTSTART_ScriptIfVer\"");
    ASSERT_STREQ(lines[1].GetCStr(), "");
    ASSERT_STREQ(lines[2].GetCStr(), "");
    ASSERT_STREQ(lines[3].GetCStr(), "Display(\"This displays!\");");
    ASSERT_STREQ(lines[4].GetCStr(), "");
    ASSERT_STREQ(lines[5].GetCStr(), "");
    ASSERT_STREQ(lines[6].GetCStr(), "");
    ASSERT_STREQ(lines[7].GetCStr(), "");
    ASSERT_STREQ(lines[8].GetCStr(), "");
    ASSERT_STREQ(lines[9].GetCStr(), "Display(\"This displays yey!\");");
    ASSERT_STREQ(lines[10].GetCStr(), "");
    ASSERT_STREQ(lines[11].GetCStr(), "");
    ASSERT_STREQ(lines[12].GetCStr(), "Display(\"This displays dude\");");
    ASSERT_STREQ(lines[13].GetCStr(), "");
    ASSERT_STREQ(lines[14].GetCStr(), "");
    ASSERT_STREQ(lines[15].GetCStr(), "");
    ASSERT_STREQ(lines[16].GetCStr(), "");
    ASSERT_STREQ(lines[17].GetCStr(), "");
    ASSERT_STREQ(lines[18].GetCStr(), "Display(\"But this displays\");");
    ASSERT_STREQ(lines[19].GetCStr(), "");
    ASSERT_STREQ(lines[20].GetCStr(), "");
}


TEST(Preprocess, IfWithoutEndIf) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#ifdef BAR
#endif
#ifdef FOO
Display("test");
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "IfWithoutEndIf");

    EXPECT_STREQ(last_seen_cc_error(), "Missing #endif");
}


TEST(Preprocess, EndIfWithoutIf) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#ifdef BAR
#endif
Display("test");
#endif
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "EndIfWithoutIf");

    EXPECT_STREQ(last_seen_cc_error(), "#endif has no matching #if");
}

TEST(Preprocess, ElseWithoutIf) {
    Preprocessor pp = Preprocessor();
    const char* inpl = R"EOS(
#ifdef BAR
#endif
Display("test");
#else
)EOS";

    clear_error();
    String res = pp.Preprocess(inpl, "ElseWithoutIf");

    EXPECT_STREQ(last_seen_cc_error(), "#else has no matching #if");
}


} // Preprocessor
} // AGS