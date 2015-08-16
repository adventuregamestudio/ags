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









	/*


	  int numsymbols;

	// index for predefined symbols
    int normalIntSym;
    int normalStringSym;
    int normalFloatSym;
    int normalVoidSym;
    int nullSym;
    int stringStructSym;

	// properties for symbols, size is numsymbols
    std::vector<short> stype;
    std::vector<long> flags;
    std::vector<short> vartype;
    std::vector<int> soffs;
    std::vector<long> ssize; // or return type size for function
    std::vector<short> sscope; // or num arguments for function
    std::vector<long> arrsize;
    std::vector<short> extends; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    std::vector<std::vector<unsigned long> > funcparamtypes;
    std::vector<std::vector<short> > funcParamDefaultValues;

    symbolTable();
    void reset();    // clears table
    int  find(const char*);  // returns ID of symbol, or -1
    int  add_ex(char*,int,char);  // adds new symbol of type and size
    int  add(char*);   // adds new symbol, returns -1 if already exists
    int  get_num_args(int funcSym);
    char*get_name(int); // gets symbol name of index
    int  get_type(int ii);
    int  operatorToVCPUCmd(int opprec);
    int  is_loadable_variable(int symm);

    void set_propfuncs(int symb, int propget, int propset);
    int get_propget(int symb);
    int get_propset(int symb);
*/