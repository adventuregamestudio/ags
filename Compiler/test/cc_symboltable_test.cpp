#include "gtest/gtest.h"
#include "script/cc_symboltable.h"


TEST(SymbolTable, GetNameNonExistent) {
    SymbolTable testSym;

    // symbol must be >= 0. Max symbols 0x10000000 due to type flags
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(0));
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(1));
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(2));

    // check edge conditions. index immediately after 'c' should be null
    int a_sym = testSym.add_ex("a", kSYM_NoType, 0);
    int b_sym = testSym.add_ex("b", kSYM_NoType, 0);
    int c_sym = testSym.add_ex("c", kSYM_NoType, 0);
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(c_sym + 1));
}

TEST(SymbolTable, GetNameNormal) {
    SymbolTable testSym;

    int foo_sym = testSym.add_ex("foo", kSYM_NoType, 0);

    EXPECT_STREQ("foo", testSym.get_name(foo_sym));
}

TEST(SymbolTable, GetNameFlags) {
    SymbolTable testSym;

    int foo_sym = testSym.add_ex("foo", kSYM_NoType, 0);

    // const
    EXPECT_STREQ("const foo", testSym.get_name(foo_sym | kVTY_Const));

    // dynarray
    EXPECT_STREQ("foo[]", testSym.get_name(foo_sym | kVTY_DynArray));

   // pointer
    EXPECT_STREQ("foo*", testSym.get_name(foo_sym | kVTY_DynPointer));


    int bar_sym = testSym.add_ex("bar", kSYM_NoType, 0);

    // const dynarray
    EXPECT_STREQ("const bar[]", testSym.get_name(bar_sym | kVTY_Const | kVTY_DynArray));

    // const pointer
    EXPECT_STREQ("const bar*", testSym.get_name(bar_sym | kVTY_Const | kVTY_DynPointer));

    // const dynarray/pointer
    EXPECT_STREQ("const bar*[]", testSym.get_name(bar_sym | kVTY_Const | kVTY_DynArray | kVTY_DynPointer));
}


TEST(SymbolTable, GetNameNonExistentFlags) {
    SymbolTable testSym;

    int no_exist_sym = 5000;

    // on their own
    // -------------------

    // normal
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym));

    // const
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_Const));

    // dynarray
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_DynArray));

    // dynarray + pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_DynArray | kVTY_DynPointer));

    // pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_DynPointer));

    // combinations
    // -------------------

    // const dynarray
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_Const | kVTY_DynArray));

    // const pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_Const | kVTY_DynPointer));

    // const dynarray/pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | kVTY_Const | kVTY_DynArray | kVTY_DynPointer));
}

TEST(SymbolTable, AddExAlreadyExists) {
	SymbolTable testSym;

    int a_sym = testSym.add_ex("a", kSYM_NoType, 0);
    ASSERT_TRUE(testSym.add_ex("a", kSYM_NoType, 0) == -1);
}

TEST(SymbolTable, AddExUnique) {
	SymbolTable testSym;

    int a_sym = testSym.add_ex("a", kSYM_NoType, 0);
    int b_sym = testSym.add_ex("b", kSYM_NoType, 0);
    ASSERT_TRUE(a_sym != b_sym);
}

TEST(SymbolTable, AddExDefaultValues) {
	SymbolTable testSym;

    SymbolType stype = kSYM_Assign;
    int ssize = 2;
    int a_sym = testSym.add_ex("a", stype, ssize);

    ASSERT_TRUE(testSym.entries.at(a_sym).sname == std::string("a"));
    ASSERT_TRUE(testSym.entries.at(a_sym).stype == stype);
    ASSERT_TRUE(testSym.entries.at(a_sym).flags == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).vartype == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).soffs == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).ssize == ssize);
    ASSERT_TRUE(testSym.entries.at(a_sym).sscope == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).arrsize == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).extends == 0);
    ASSERT_TRUE(testSym.entries.at(a_sym).get_num_args() == 0);
}

TEST(SymbolTable, AddExAvailableAfterwards) {
	SymbolTable testSym;

    int a_sym = testSym.add_ex("x", kSYM_NoType, 0);

    // no test is available.. but we can try to get name.
    const char *name = testSym.get_name(a_sym);
    ASSERT_TRUE(name != 0);
}

TEST(SymbolTable, EntriesEnsureModifiable) {
	SymbolTable testSym;

    // ensure reading and writing to entries actually works!
    int a_sym = testSym.add_ex("x", kSYM_NoType, 0);
    testSym.entries.at(a_sym).flags = 10;
    ASSERT_TRUE(testSym.entries.at(a_sym).flags == 10);
}

TEST(SymbolTable, GetNumArgs) {
	SymbolTable testSym;
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
	SymbolTable testSym;
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
