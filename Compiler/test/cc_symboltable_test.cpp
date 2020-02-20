#include "gtest/gtest.h"
#include "script/cc_symboltable.h"


TEST(SymbolTable, GetNameNonExistent) {
    AGS::SymbolTable testSym;

    // symbol must be >= 0. Max symbols 0x10000000 due to type flags
    EXPECT_STREQ("(invalid symbol)", testSym.GetName(100).c_str());
    EXPECT_STREQ("(invalid symbol)", testSym.GetName(200).c_str());

    // check edge conditions. index immediately after 'c' should be null
    testSym.Add("a");
    testSym.Add("b");
    int const c_sym = testSym.Add("c");
    EXPECT_STREQ("(invalid symbol)", testSym.GetName(c_sym + 1).c_str());
}

TEST(SymbolTable, GetNameNormal) {
    AGS::SymbolTable testSym;

    int const foo_sym = testSym.Add("foo", AGS::kSYM_NoType, 0);

    EXPECT_STREQ("foo", testSym.GetName(foo_sym).c_str());
}

TEST(SymbolTable, GetNameConverted) {
    AGS::SymbolTable testSym;

    AGS::Vartype const foo_vartype = testSym.Add("foo", AGS::kSYM_NoType, 0);
    testSym[foo_vartype].SType = AGS::kSYM_Vartype;
    testSym[foo_vartype].VartypeType = AGS::kVTT_Atomic;
    AGS::Vartype foo_conv_vartype = foo_vartype;
    EXPECT_STREQ(
        "foo[]",
        testSym.GetName(testSym.VartypeWith(AGS::kVTT_Dynarray, foo_vartype)).c_str());

    EXPECT_STREQ(
        "foo *",
        testSym.GetName(testSym.VartypeWith(AGS::kVTT_Dynpointer, foo_vartype)).c_str());


    std::vector<size_t> const dims = { 3, 5, 7 };
    EXPECT_STREQ(
        "foo[3, 5, 7]",
        testSym.GetName(testSym.VartypeWithArray(dims, foo_vartype)).c_str());
}

TEST(SymbolTable, AddExAlreadyExists) {
    AGS::SymbolTable testSym;

    int const a_sym = testSym.Add("a", AGS::kSYM_NoType, 0);
    ASSERT_EQ(-1, testSym.Add("a", AGS::kSYM_NoType, 0));
}

TEST(SymbolTable, AddExUnique) {
    AGS::SymbolTable testSym;

    int const a_sym = testSym.Add("a", AGS::kSYM_NoType, 0);
    int const b_sym = testSym.Add("b", AGS::kSYM_NoType, 0);
    ASSERT_NE(a_sym, b_sym);
}

TEST(SymbolTable, AddExDefaultValues) {
    AGS::SymbolTable testSym;

    AGS::SymbolType const stype = AGS::kSYM_Assign;
    int const ssize = 2;
    int const a_sym = testSym.Add("a", stype, ssize);

    EXPECT_STREQ("a", testSym.entries.at(a_sym).SName.c_str());
    EXPECT_EQ(stype, testSym.entries.at(a_sym).SType);
    EXPECT_EQ(0, testSym.entries.at(a_sym).Flags);
    EXPECT_EQ(0, testSym.entries.at(a_sym).vartype);
    EXPECT_EQ(0, testSym.entries.at(a_sym).SOffset);
    EXPECT_EQ(ssize, testSym.entries.at(a_sym).SSize);
    EXPECT_EQ(0, testSym.entries.at(a_sym).SScope);
    EXPECT_EQ(0, testSym.entries.at(a_sym).GetNumOfFuncParams());
}

TEST(SymbolTable, EntriesEnsureModifiable) {
    AGS::SymbolTable testSym;

    // ensure reading and writing to entries actually works!
    int const a_sym = testSym.Add("x", AGS::kSYM_NoType, 0);
    testSym.entries.at(a_sym).Flags = 10;
    EXPECT_EQ(10, testSym.entries.at(a_sym).Flags);
    testSym[a_sym].Flags = 11;
    EXPECT_EQ(11, testSym.entries.at(a_sym).Flags);
    EXPECT_EQ(11, testSym[a_sym].Flags);
}

TEST(SymbolTable, Operators) {
    AGS::SymbolTable testSym;
    int  sym_01 = testSym.AddOp("Antiatomkraftprotestplakat", AGS::kSYM_Operator, 7, 77);
    EXPECT_EQ(7, testSym.GetOperatorOpcode(sym_01));
    EXPECT_EQ(77, testSym.BinaryOpPrio(sym_01));

    int const sym_02 = testSym.AddOp("Betriebsgenehmigung", AGS::kSYM_Operator, 8, 88, 888);
    EXPECT_EQ(8, testSym.GetOperatorOpcode(sym_02));
    EXPECT_EQ(88, testSym.BinaryOpPrio(sym_02));
    EXPECT_EQ(888, testSym.UnaryOpPrio(sym_02));

    int const sym_03 = testSym.AddOp("Charaktereignungstest", AGS::kSYM_Assign, 9, 99, 999);
    EXPECT_EQ(AGS::kSYM_Assign, testSym.GetSymbolType(sym_03));
    EXPECT_EQ(9, testSym.GetOperatorOpcode(sym_03));
    EXPECT_EQ(99, testSym.BinaryOpPrio(sym_03));
    EXPECT_EQ(999, testSym.UnaryOpPrio(sym_03));
}
