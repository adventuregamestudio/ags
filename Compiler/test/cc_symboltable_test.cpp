#include "catch.hpp"
#include "script/cc_symboltable.h"
#include "script/cc_symboldef.h"

TEST_CASE("get_name - non-existent", "[cc_symboltable]") {
	symbolTable testSym;

	// symbol must be >= 0. Max symbols 0x10000000 due to type flags
	REQUIRE (testSym.get_name(0) == NULL);
	REQUIRE (testSym.get_name(1) == NULL);
	REQUIRE (testSym.get_name(2) == NULL);

	// check edge conditions. index immediately after 'c' should be null
	int a_sym = testSym.add_ex("a",0,0);
	int b_sym = testSym.add_ex("b",0,0);
	int c_sym = testSym.add_ex("c",0,0);
	REQUIRE (testSym.get_name(c_sym + 1) == NULL);
}

TEST_CASE("get_name - normal", "[cc_symboltable]") {
	symbolTable testSym;

	int foo_sym = testSym.add_ex("foo",0,0);

	REQUIRE (strcmp(testSym.get_name(foo_sym), "foo")== 0);
}

TEST_CASE("get_name - flags", "[cc_symboltable]") {
	symbolTable testSym;

	int foo_sym = testSym.add_ex("foo",0,0);

	// const
	REQUIRE (strcmp(testSym.get_name(foo_sym | STYPE_CONST), "const foo")== 0);
	
	// dynarray
	REQUIRE (strcmp(testSym.get_name(foo_sym | STYPE_DYNARRAY), "foo[]")== 0);

	// dynarray + pointer is just a dynarray
	REQUIRE (strcmp(testSym.get_name(foo_sym | STYPE_DYNARRAY | STYPE_POINTER), "foo[]")== 0);

	// pointer
	REQUIRE (strcmp(testSym.get_name(foo_sym | STYPE_POINTER), "foo*")== 0);


	int bar_sym = testSym.add_ex("bar",0,0);

	// const dynarray
	REQUIRE (strcmp(testSym.get_name(bar_sym | STYPE_CONST | STYPE_DYNARRAY), "const bar[]")== 0);

	// const pointer
	REQUIRE (strcmp(testSym.get_name(bar_sym | STYPE_CONST | STYPE_POINTER), "const bar*")== 0);

	// const dynarray/pointer
	REQUIRE (strcmp(testSym.get_name(bar_sym | STYPE_CONST | STYPE_DYNARRAY | STYPE_POINTER), "const bar[]")== 0);
}


TEST_CASE("get_name - flags of non-existent", "[cc_symboltable]") {
	symbolTable testSym;

	int no_exist_sym = 5000;

	// on their own
	// -------------------

	// normal
	REQUIRE (testSym.get_name(no_exist_sym) == NULL);

	// const
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_CONST) == NULL);
	
	// dynarray
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_DYNARRAY) == NULL);

	// dynarray + pointer is just a dynarray
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_DYNARRAY | STYPE_POINTER) == NULL);

	// pointer
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_POINTER) == NULL);

	// combinations
	// -------------------

	// const dynarray
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_DYNARRAY) == NULL);

	// const pointer
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_POINTER) == NULL);

	// const dynarray/pointer
	REQUIRE (testSym.get_name(no_exist_sym | STYPE_CONST | STYPE_DYNARRAY | STYPE_POINTER) == NULL);
}

TEST_CASE("add_ex - already exists", "[cc_symboltable]") {
	symbolTable testSym;

	int a_sym = testSym.add_ex("a",0,0);
	REQUIRE(testSym.add_ex("a",0,0) == -1);
}

TEST_CASE("add_ex - unique", "[cc_symboltable]") {
	symbolTable testSym;

	int a_sym = testSym.add_ex("a",0,0);
	int b_sym = testSym.add_ex("b",0,0);
	REQUIRE(a_sym != b_sym);
}
	
TEST_CASE("add_ex - default values", "[cc_symboltable]") {
	symbolTable testSym;

	int typo = 1;
	int sizee = 2;
	int a_sym = testSym.add_ex("a", typo, sizee);

	REQUIRE(testSym.entries[a_sym].sname == std::string("a"));
	REQUIRE(testSym.stype[a_sym] == typo);
	REQUIRE(testSym.flags[a_sym] == 0);
	REQUIRE(testSym.vartype[a_sym] == 0);
	REQUIRE(testSym.soffs[a_sym] == 0);
	REQUIRE(testSym.ssize[a_sym] == sizee);
	REQUIRE(testSym.sscope[a_sym] == 0);
	REQUIRE(testSym.arrsize[a_sym] == 0);
	REQUIRE(testSym.extends[a_sym] == 0);
	REQUIRE(testSym.get_num_args(a_sym) == 0);
}

TEST_CASE("add_ex - available afterwards", "[cc_symboltable]") {
	symbolTable testSym;

	int a_sym = testSym.add_ex("x",0,0);

	// no test is available.. but we can try to get name.
	char *name = testSym.get_name(a_sym);
	REQUIRE(name != 0);
}