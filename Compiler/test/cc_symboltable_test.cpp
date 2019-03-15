#include "gtest/gtest.h"
#include "script/cc_symboldef.h"
#include "script/cc_symboltable.h"


TEST(SymbolTable, GetNameNonExistent) {
    SymbolTable testSym;

    // symbol must be >= 0. Max symbols 0x10000000 due to type flags
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(0));
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(1));
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(2));

    // check edge conditions. index immediately after 'c' should be null
    int a_sym = testSym.add_ex("a", 0, 0);
    int b_sym = testSym.add_ex("b", 0, 0);
    int c_sym = testSym.add_ex("c", 0, 0);
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(c_sym + 1));
}

TEST(SymbolTable, GetNameNormal) {
    SymbolTable testSym;

    int foo_sym = testSym.add_ex("foo", 0, 0);

    EXPECT_STREQ("foo", testSym.get_name(foo_sym));
}

TEST(SymbolTable, GetNameFlags) {
    SymbolTable testSym;

    int foo_sym = testSym.add_ex("foo", 0, 0);

    // const
    EXPECT_STREQ("const foo", testSym.get_name(foo_sym | STYPE_CONST));

    // dynarray
    EXPECT_STREQ("foo[]", testSym.get_name(foo_sym | STYPE_DYNARRAY));

   // pointer
    EXPECT_STREQ("foo*", testSym.get_name(foo_sym | STYPE_POINTER));


    int bar_sym = testSym.add_ex("bar", 0, 0);

    // const dynarray
    EXPECT_STREQ("const bar[]", testSym.get_name(bar_sym | STYPE_CONST | STYPE_DYNARRAY));

    // const pointer
    EXPECT_STREQ("const bar*", testSym.get_name(bar_sym | STYPE_CONST | STYPE_POINTER));

    // const dynarray/pointer
    EXPECT_STREQ("const bar*[]", testSym.get_name(bar_sym | STYPE_CONST | STYPE_DYNARRAY | STYPE_POINTER));
}


TEST(SymbolTable, GetNameNonExistentFlags) {
    SymbolTable testSym;

    int no_exist_sym = 5000;

    // on their own
    // -------------------

    // normal
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym));

    // const
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_CONST));

    // dynarray
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_DYNARRAY));

    // dynarray + pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_DYNARRAY | STYPE_POINTER));

    // pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_POINTER));

    // combinations
    // -------------------

    // const dynarray
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_DYNARRAY));

    // const pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_POINTER));

    // const dynarray/pointer
    EXPECT_STREQ("(invalid symbol)", testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_DYNARRAY | STYPE_POINTER));
}

TEST(SymbolTable, AddExAlreadyExists) {
	SymbolTable testSym;

    int a_sym = testSym.add_ex("a", 0, 0);
    ASSERT_TRUE(testSym.add_ex("a", 0, 0) == -1);
}

TEST(SymbolTable, AddExUnique) {
	SymbolTable testSym;

    int a_sym = testSym.add_ex("a", 0, 0);
    int b_sym = testSym.add_ex("b", 0, 0);
    ASSERT_TRUE(a_sym != b_sym);
}

TEST(SymbolTable, AddExDefaultValues) {
	SymbolTable testSym;

    int typo = 1;
    int sizee = 2;
    int a_sym = testSym.add_ex("a", typo, sizee);

    ASSERT_TRUE(testSym.entries[a_sym].sname == std::string("a"));
    ASSERT_TRUE(testSym.entries[a_sym].stype == typo);
    ASSERT_TRUE(testSym.entries[a_sym].flags == 0);
    ASSERT_TRUE(testSym.entries[a_sym].vartype == 0);
    ASSERT_TRUE(testSym.entries[a_sym].soffs == 0);
    ASSERT_TRUE(testSym.entries[a_sym].ssize == sizee);
    ASSERT_TRUE(testSym.entries[a_sym].sscope == 0);
    ASSERT_TRUE(testSym.entries[a_sym].arrsize == 0);
    ASSERT_TRUE(testSym.entries[a_sym].extends == 0);
    ASSERT_TRUE(testSym.entries[a_sym].get_num_args() == 0);
}

TEST(SymbolTable, AddExAvailableAfterwards) {
	SymbolTable testSym;

    int a_sym = testSym.add_ex("x", 0, 0);

    // no test is available.. but we can try to get name.
    const char *name = testSym.get_name(a_sym);
    ASSERT_TRUE(name != 0);
}

TEST(SymbolTable, EntriesEnsureModifiable) {
	SymbolTable testSym;

    // ensure reading and writing to entries actually works!
    int a_sym = testSym.add_ex("x", 0, 0);
    testSym.entries[a_sym].flags = 10;
    ASSERT_TRUE(testSym.entries[a_sym].flags == 10);
}

TEST(SymbolTable, GetNumArgs) {
	SymbolTable testSym;
	int sym_01 = testSym.add("yellow");

    testSym.entries[sym_01].sscope = 0;
    ASSERT_TRUE(testSym.entries[sym_01].get_num_args() == 0);
    testSym.entries[sym_01].sscope = 1;
    ASSERT_TRUE(testSym.entries[sym_01].get_num_args() == 1);
    testSym.entries[sym_01].sscope = 2;
    ASSERT_TRUE(testSym.entries[sym_01].get_num_args() == 2);

    testSym.entries[sym_01].sscope = 100;
    ASSERT_TRUE(testSym.entries[sym_01].get_num_args() == 0);
    testSym.entries[sym_01].sscope = 101;
    ASSERT_TRUE(testSym.entries[sym_01].get_num_args() == 1);
    testSym.entries[sym_01].sscope = 102;
    ASSERT_TRUE(testSym.entries[sym_01].get_num_args() == 2);
}

TEST(SymbolTable, IsLoadableVariable) {
	SymbolTable testSym;
	int sym_01 = testSym.add("supergreen");

    ASSERT_TRUE(!testSym.entries[sym_01].is_loadable_variable());

    testSym.entries[sym_01].stype = SYM_GLOBALVAR;
    ASSERT_TRUE(testSym.entries[sym_01].is_loadable_variable());
    testSym.entries[sym_01].stype = SYM_LOCALVAR;
    ASSERT_TRUE(testSym.entries[sym_01].is_loadable_variable());
    testSym.entries[sym_01].stype = SYM_CONSTANT;
    ASSERT_TRUE(testSym.entries[sym_01].is_loadable_variable());
}

TEST(SymbolTable, AttrFuncs) {

	SymbolTable testSym;
	int sym_01 = testSym.add("cup");

	testSym.entries[sym_01].set_attrfuncs(0, 0);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrget() == 0);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrset() == 0);

	testSym.entries[sym_01].set_attrfuncs(1, 2);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrget() == 1);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrset() == 2);

	testSym.entries[sym_01].set_attrfuncs(100, 200);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrget() == 100);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrset() == 200);

	testSym.entries[sym_01].set_attrfuncs(0xFFFF, 0xFFFF);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrget() == -1);
	ASSERT_TRUE(testSym.entries[sym_01].get_attrset() == -1);
}

TEST(SymbolTable, OperatorToVCPUCmd) {
	SymbolTable testSym;
	int sym_01 = testSym.add("grassgreen");

    testSym.entries[sym_01].vartype = 0;
    ASSERT_TRUE(testSym.entries[sym_01].operatorToVCPUCmd() == 0);
    testSym.entries[sym_01].vartype = 1;
    ASSERT_TRUE(testSym.entries[sym_01].operatorToVCPUCmd() == 1);
    testSym.entries[sym_01].vartype = 10;
    ASSERT_TRUE(testSym.entries[sym_01].operatorToVCPUCmd() == 10);
    testSym.entries[sym_01].vartype = 100;
    ASSERT_TRUE(testSym.entries[sym_01].operatorToVCPUCmd() == 100);
}
