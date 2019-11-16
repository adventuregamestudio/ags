#include "gtest/gtest.h"
#include "script/cc_symboltable.h"


TEST(SymbolTable, GetNameNonExistent) {
    AGS::SymbolTable testSym;

    // symbol must be >= 0. Max symbols 0x10000000 due to type flags
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(100));
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(200));

    // check edge conditions. index immediately after 'c' should be null
    int a_sym = testSym.add_ex("a", AGS::kSYM_NoType, 0);
    int b_sym = testSym.add_ex("b", AGS::kSYM_NoType, 0);
    int c_sym = testSym.add_ex("c", AGS::kSYM_NoType, 0);
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(c_sym + 1));
}

TEST(SymbolTable, GetNameNormal) {
    AGS::SymbolTable testSym;

    int foo_sym = testSym.add_ex("foo", AGS::kSYM_NoType, 0);

    EXPECT_STREQ("foo", testSym.get_name(foo_sym));
}

TEST(SymbolTable, GetNameConverted) {
    AGS::SymbolTable testSym;

    AGS::Vartype const foo_vartype = testSym.add_ex("foo", AGS::kSYM_NoType, 0);
    testSym[foo_vartype].SType = AGS::kSYM_Vartype;
    testSym[foo_vartype].VartypeType = AGS::kVTT_Atomic;
    AGS::Vartype foo_conv_vartype = foo_vartype;
    EXPECT_STREQ(
        "foo[]",
        testSym.get_name_string(testSym.VartypeWith(AGS::kVTT_Dynarray, foo_vartype)).c_str());

    EXPECT_STREQ(
        "foo *",
        testSym.get_name_string(testSym.VartypeWith(AGS::kVTT_Dynpointer, foo_vartype)).c_str());


    std::vector<size_t> const dims = { 3, 5, 7 };
    EXPECT_STREQ(
        "foo[3, 5, 7]",
        testSym.get_name_string(testSym.VartypeWithArray(dims, foo_vartype)).c_str());
}

TEST(SymbolTable, AddExAlreadyExists) {
    AGS::SymbolTable testSym;

    int a_sym = testSym.add_ex("a", AGS::kSYM_NoType, 0);
    ASSERT_TRUE(testSym.add_ex("a", AGS::kSYM_NoType, 0) == -1);
}

TEST(SymbolTable, AddExUnique) {
    AGS::SymbolTable testSym;

    int a_sym = testSym.add_ex("a", AGS::kSYM_NoType, 0);
    int b_sym = testSym.add_ex("b", AGS::kSYM_NoType, 0);
    ASSERT_TRUE(a_sym != b_sym);
}

TEST(SymbolTable, AddExDefaultValues) {
    AGS::SymbolTable testSym;

    AGS::SymbolType stype = AGS::kSYM_Assign;
    int ssize = 2;
    int a_sym = testSym.add_ex("a", stype, ssize);

    ASSERT_TRUE(testSym.entries.at(a_sym).SName == std::string("a"));
    ASSERT_TRUE(testSym.entries.at(a_sym).SType == stype);
    ASSERT_TRUE(testSym.entries.at(a_sym).Flags == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).vartype == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).SOffset == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).SSize == ssize);
    ASSERT_TRUE(testSym.entries.at(a_sym).sscope == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).extends == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).get_num_args() == 0);
}

TEST(SymbolTable, AddExAvailableAfterwards) {
    AGS::SymbolTable testSym;

    int a_sym = testSym.add_ex("x", AGS::kSYM_NoType, 0);

    // no test is available.. but we can try to get name.
    const char *name = testSym.get_name(a_sym);
    ASSERT_TRUE(name != 0);
}

TEST(SymbolTable, EntriesEnsureModifiable) {
    AGS::SymbolTable testSym;

    // ensure reading and writing to entries actually works!
    int a_sym = testSym.add_ex("x", AGS::kSYM_NoType, 0);
    testSym.entries.at(a_sym).Flags = 10;
    ASSERT_TRUE(testSym.entries.at(a_sym).Flags == 10);
}

TEST(SymbolTable, GetNumArgs) {
    AGS::SymbolTable testSym;
	int sym_01 = testSym.add("yellow");

    testSym.entries.at(sym_01).sscope = 0;
    ASSERT_TRUE(testSym.entries.at(sym_01).get_num_args() == 0);
    testSym.entries.at(sym_01).sscope = 1;
    ASSERT_TRUE(testSym.entries.at(sym_01).get_num_args() == 1);
    testSym.entries.at(sym_01).sscope = 2;
    ASSERT_TRUE(testSym.entries.at(sym_01).get_num_args() == 2);

    testSym.entries.at(sym_01).sscope = 100;
    ASSERT_TRUE(testSym.entries.at(sym_01).get_num_args() == 0);
    testSym.entries.at(sym_01).sscope = 101;
    ASSERT_TRUE(testSym.entries.at(sym_01).get_num_args() == 1);
    testSym.entries.at(sym_01).sscope = 102;
    ASSERT_TRUE(testSym.entries.at(sym_01).get_num_args() == 2);
}

TEST(SymbolTable, OperatorToVCPUCmd) {
    AGS::SymbolTable testSym;
	int sym_01 = testSym.add("grassgreen");

    testSym.entries.at(sym_01).vartype = 0;
    ASSERT_TRUE(testSym.entries.at(sym_01).operatorToVCPUCmd() == 0);
    testSym.entries.at(sym_01).vartype = 1;
    ASSERT_TRUE(testSym.entries.at(sym_01).operatorToVCPUCmd() == 1);
    testSym.entries.at(sym_01).vartype = 10;
    ASSERT_TRUE(testSym.entries.at(sym_01).operatorToVCPUCmd() == 10);
    testSym.entries.at(sym_01).vartype = 100;
    ASSERT_TRUE(testSym.entries.at(sym_01).operatorToVCPUCmd() == 100);
}
