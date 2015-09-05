#include "gtest/gtest.h"

#include "script/cc_symboltable.h"
#include "script/cc_symboldef.h"

TEST(SymbolTable, GetNameNonExistent) {
    symbolTable testSym;

    // symbol must be >= 0. Max symbols 0x10000000 due to type flags
    EXPECT_EQ (NULL, testSym.get_name(0));
    EXPECT_EQ (NULL, testSym.get_name(1));
    EXPECT_EQ (NULL, testSym.get_name(2));

    // check edge conditions. index immediately after 'c' should be null
    int a_sym = testSym.add_ex("a",0,0);
    int b_sym = testSym.add_ex("b",0,0);
    int c_sym = testSym.add_ex("c",0,0);
    EXPECT_EQ (NULL, testSym.get_name(c_sym + 1));
}

TEST(SymbolTable, GetNameNormal) {
    symbolTable testSym;

    int foo_sym = testSym.add_ex("foo",0,0);

    EXPECT_STREQ("foo", testSym.get_name(foo_sym));
}

TEST(SymbolTable, GetNameFlags) {
    symbolTable testSym;

    int foo_sym = testSym.add_ex("foo",0,0);

    // const
    EXPECT_STREQ("const foo", testSym.get_name(foo_sym | STYPE_CONST));

    // dynarray
    EXPECT_STREQ("foo[]", testSym.get_name(foo_sym | STYPE_DYNARRAY));

    // dynarray + pointer is just a dynarray
    EXPECT_STREQ("foo[]", testSym.get_name(foo_sym | STYPE_DYNARRAY | STYPE_POINTER));

    // pointer
    EXPECT_STREQ("foo*", testSym.get_name(foo_sym | STYPE_POINTER));


    int bar_sym = testSym.add_ex("bar",0,0);

    // const dynarray
    EXPECT_STREQ("const bar[]", testSym.get_name(bar_sym | STYPE_CONST | STYPE_DYNARRAY));

    // const pointer
    EXPECT_STREQ("const bar*", testSym.get_name(bar_sym | STYPE_CONST | STYPE_POINTER));

    // const dynarray/pointer
    EXPECT_STREQ("const bar[]", testSym.get_name(bar_sym | STYPE_CONST | STYPE_DYNARRAY | STYPE_POINTER));
}


TEST(SymbolTable, GetNameNonExistentFlags) {
    symbolTable testSym;

    int no_exist_sym = 5000;

    // on their own
    // -------------------

    // normal
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym));

    // const
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_CONST));

    // dynarray
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_DYNARRAY));

    // dynarray + pointer is just a dynarray
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_DYNARRAY | STYPE_POINTER));

    // pointer
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_POINTER));

    // combinations
    // -------------------

    // const dynarray
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_DYNARRAY));

    // const pointer
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_POINTER));

    // const dynarray/pointer
    EXPECT_EQ (NULL, testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_DYNARRAY | STYPE_POINTER));
}
