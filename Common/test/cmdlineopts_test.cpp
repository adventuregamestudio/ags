#include "gtest/gtest.h"
#include "util/cmdlineopts.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;



//// Basic tests : only positional parameters or parameters that are simple flags.
//TEST(CmdLineOpts, ParserBasics) {
//
//    ParseResult parseResult = {};
//    std::set<String> optParamsWith1Values;
//
//    const char * argv_a[] = {"program","-a","b"};
//    int argc_a = 3;
//
//    parseResult = CmdLineOptsParser::Parse(argc_a, argv_a, {}, {}, {}, {});
//
//    ASSERT_EQ(parseResult.HelpRequested,false);
//    ASSERT_EQ(parseResult.Opt.size(),1);
//    ASSERT_TRUE(parseResult.Opt.count("-a"));
//    ASSERT_EQ(parseResult.PosArgs.size(),1);
//    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(),"b");
//
//
//    const char * argv_b[] = {"program","b","-a"};
//    int argc_b = 3;
//
//    parseResult = CmdLineOptsParser::Parse(argc_b, argv_b, {}, {}, {}, {});
//
//    ASSERT_EQ(parseResult.HelpRequested,false);
//    ASSERT_EQ(parseResult.Opt.size(),1);
//    ASSERT_TRUE(parseResult.Opt.count("-a"));
//    ASSERT_EQ(parseResult.PosArgs.size(),1);
//    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(),"b");
//
//
//    const char * argv_c[] = {"program","a","-b","--long","pos","-longsingle","--long-double"};
//    int argc_c = 7;
//
//    parseResult = CmdLineOptsParser::Parse(argc_c, argv_c, {}, {}, {}, {});
//
//    ASSERT_EQ(parseResult.HelpRequested,false);
//    ASSERT_EQ(parseResult.Opt.size(),4);
//    ASSERT_TRUE(parseResult.Opt.count("-b"));
//    ASSERT_TRUE(parseResult.Opt.count("--long"));
//    ASSERT_TRUE(parseResult.Opt.count("-longsingle"));
//    ASSERT_TRUE(parseResult.Opt.count("--long-double"));
//    ASSERT_EQ(parseResult.PosArgs.size(),2);
//    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(),"a");
//    ASSERT_STREQ(parseResult.PosArgs[1].GetCStr(),"pos");
//}
//
//
//// Test that the parser picks up on the Help flag (flags -h or --help are present)
//TEST(CmdLineOpts, ParserRaisedHelp) {
//    ParseResult parseResult = {};
//    std::set<String> optParamsWith1Values;
//
//    optParamsWith1Values = {};
//    const char * argv_a[] = {"program","-h","b"};
//    int argc_a = 3;
//
//    parseResult = CmdLineOptsParser::Parse(argc_a, argv_a, optParamsWith1Values, {}, {}, {});
//
//    ASSERT_TRUE(parseResult.HelpRequested);
//    ASSERT_EQ(parseResult.Opt.size(),1);
//    ASSERT_TRUE(parseResult.Opt.count("-h"));
//    ASSERT_EQ(parseResult.PosArgs.size(),1);
//    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(),"b");
//
//
//    optParamsWith1Values = {};
//    const char * argv_b[] = {"program","--help","b"};
//    int argc_b = 3;
//
//    parseResult = CmdLineOptsParser::Parse(argc_b, argv_b, optParamsWith1Values, {}, {}, {});
//
//    ASSERT_TRUE(parseResult.HelpRequested);
//    ASSERT_EQ(parseResult.Opt.size(),1);
//    ASSERT_TRUE(parseResult.Opt.count("--help"));
//    ASSERT_EQ(parseResult.PosArgs.size(),1);
//    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(),"b");
//}
//
//// Test that the parser is able to read the argument
//// immediately after a parameter that takes axactly one value.
//TEST(CmdLineOpts, ParserOptWith1Values) {
//    ParseResult parseResult = {};
//    std::set<String> optParamsWith1Values;
//
//    // Test 1 : -a, followed by b
//
//    optParamsWith1Values = {"-a"};
//    const char * argv_a[] = {"program","-a","b"};
//    int argc_a = 3;
//
//    parseResult = CmdLineOptsParser::Parse(argc_a, argv_a, optParamsWith1Values, {}, {}, {});
//
//    ASSERT_EQ(parseResult.HelpRequested,false);
//    ASSERT_EQ(parseResult.Opt.size(),0);
//    ASSERT_EQ(parseResult.OptWith1Value.size(),1);
//    ASSERT_STREQ(parseResult.OptWith1Value[0].first.GetCStr(),"-a");
//    ASSERT_STREQ(parseResult.OptWith1Value[0].second.GetCStr(),"b");
//    ASSERT_EQ(parseResult.PosArgs.size(),0);
//
//    // Test 2 : -a, followed by b (but no white space inbetween)
//
//    optParamsWith1Values = {"-a"};
//    const char * argv_b[] = {"program","-ab"};
//    int argc_b = 2;
//
//    parseResult = CmdLineOptsParser::Parse(argc_b, argv_b, optParamsWith1Values, {}, {}, {});
//
//    ASSERT_EQ(parseResult.HelpRequested,false);
//    ASSERT_EQ(parseResult.Opt.size(),0);
//    ASSERT_EQ(parseResult.OptWith1Value.size(),1);
//    ASSERT_STREQ(parseResult.OptWith1Value[0].first.GetCStr(),"-a");
//    ASSERT_STREQ(parseResult.OptWith1Value[0].second.GetCStr(),"b");
//    ASSERT_EQ(parseResult.PosArgs.size(),0);
//
//    // Test 3 : -D present several times, followed by b, ccc, ddd, and eee
//
//    optParamsWith1Values = {"-D"};
//    const char * argv_c[] = {"program","-Db", "-Dccc", "-Dddd", "-D","eee"};
//    int argc_c = 6;
//
//    parseResult = CmdLineOptsParser::Parse(argc_c, argv_c, optParamsWith1Values, {}, {}, {});
//
//    ASSERT_EQ(parseResult.HelpRequested,false);
//    ASSERT_EQ(parseResult.Opt.size(),0);
//    ASSERT_EQ(parseResult.OptWith1Value.size(),4);
//    ASSERT_STREQ(parseResult.OptWith1Value[0].first.GetCStr(),"-D");
//    ASSERT_STREQ(parseResult.OptWith1Value[0].second.GetCStr(),"b");
//    ASSERT_STREQ(parseResult.OptWith1Value[1].first.GetCStr(),"-D");
//    ASSERT_STREQ(parseResult.OptWith1Value[1].second.GetCStr(),"ccc");
//    ASSERT_STREQ(parseResult.OptWith1Value[2].first.GetCStr(),"-D");
//    ASSERT_STREQ(parseResult.OptWith1Value[2].second.GetCStr(),"ddd");
//    ASSERT_STREQ(parseResult.OptWith1Value[3].first.GetCStr(),"-D");
//    ASSERT_STREQ(parseResult.OptWith1Value[3].second.GetCStr(),"eee");
//    ASSERT_EQ(parseResult.PosArgs.size(),0);
//
//}


// Test that the parser is able to read the 2 arguments
// immediately after a parameter that takes axactly 2 values.
TEST(CmdLineOpts, ParserOptWith2Values) {
    ParseResult parseResult = {};
    std::set<String> optParamsWith2Values;

    // Test 1 : -a, followed by b and c

    optParamsWith2Values = { "-a" };
    const char* argv_a[] = { "program","-a","b","c"};
    int argc_a = 4;

    parseResult = CmdLineOptsParser::Parse(argc_a, argv_a, {}, optParamsWith2Values, {}, {});

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.IsBadlyFormed, false);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.OptWith2Values.size(), 1);
    ASSERT_STREQ(parseResult.OptWith2Values[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWith2Values[0].second[0].GetCStr(), "b");
    ASSERT_STREQ(parseResult.OptWith2Values[0].second[1].GetCStr(), "c");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);

    // Test 2 : -a, followed by b and c but no space between -a and b

    optParamsWith2Values = { "-a" };
    const char* argv_b[] = { "program","-ab","c" };
    int argc_b = 3;

    parseResult = CmdLineOptsParser::Parse(argc_b, argv_b, {}, optParamsWith2Values, {}, {});

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.IsBadlyFormed, false);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.OptWith2Values.size(), 1);
    ASSERT_STREQ(parseResult.OptWith2Values[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWith2Values[0].second[0].GetCStr(), "b");
    ASSERT_STREQ(parseResult.OptWith2Values[0].second[1].GetCStr(), "c");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);


    //// Test 3 : Several occurrences of -a with its parameters

    optParamsWith2Values = { "-a" };
    const char* argv_c[] = { "program","-ab","c", "-ae", "f", "-a", "g", "h"};
    int argc_c = 8;

    parseResult = CmdLineOptsParser::Parse(argc_c, argv_c, {}, optParamsWith2Values, {}, {});

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.IsBadlyFormed, false);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.OptWith2Values.size(), 3);
    ASSERT_STREQ(parseResult.OptWith2Values[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWith2Values[0].second[0].GetCStr(), "b");
    ASSERT_STREQ(parseResult.OptWith2Values[0].second[1].GetCStr(), "c");
    ASSERT_STREQ(parseResult.OptWith2Values[1].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWith2Values[1].second[0].GetCStr(), "e");
    ASSERT_STREQ(parseResult.OptWith2Values[1].second[1].GetCStr(), "f");
    ASSERT_STREQ(parseResult.OptWith2Values[2].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWith2Values[2].second[0].GetCStr(), "g");
    ASSERT_STREQ(parseResult.OptWith2Values[2].second[1].GetCStr(), "h");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);

    // Test 4 : badly formed. -a, followed by b but c is missing

    optParamsWith2Values = { "-a" };
    const char* argv_d[] = { "program","-a","b" };
    int argc_d = 3;

    parseResult = CmdLineOptsParser::Parse(argc_d, argv_d, {}, optParamsWith2Values, {}, {});

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.IsBadlyFormed, true);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.PosArgs.size(), 0);

    // Test 4 : badly formed. Same as previous, but without the white space.

    optParamsWith2Values = { "-a" };
    const char* argv_e[] = { "program","-ab" };
    int argc_e = 2;

    parseResult = CmdLineOptsParser::Parse(argc_e, argv_e, {}, optParamsWith2Values, {}, {});

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.IsBadlyFormed, true);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.PosArgs.size(), 0);
}

