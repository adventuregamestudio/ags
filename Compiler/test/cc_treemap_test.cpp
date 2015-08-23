#include "catch.hpp"
#include "script/cc_treemap.h"

TEST_CASE( "addEntry - single value", "[treemap]" ) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 1);
	REQUIRE (symbolTree.findValue("a") == 1);
}

TEST_CASE( "addEntry - overwrite value", "[treemap]" ) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 1);
	symbolTree.addEntry("a", 2);
	REQUIRE (symbolTree.findValue("a") == 2);
}

TEST_CASE( "addEntry - do nothing if null or string", "[treemap]" ) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("", 100);
	REQUIRE (symbolTree.findValue("") == -1);
	symbolTree.addEntry(NULL, 101);
	REQUIRE (symbolTree.findValue(NULL) == -1);
}

TEST_CASE( "addEntry - case sensitivity", "[treemap]" ) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 200);
	REQUIRE (symbolTree.findValue("a") == 200);
	REQUIRE (symbolTree.findValue("A") == -1);

	symbolTree.addEntry("A", 201);
	REQUIRE (symbolTree.findValue("a") == 200);
	REQUIRE (symbolTree.findValue("A") == 201);
}

TEST_CASE( "findValue - null or empty", "[treemap]" ) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 300);
	symbolTree.addEntry("b", 301);
	REQUIRE (symbolTree.findValue("") == -1);
	REQUIRE (symbolTree.findValue(NULL) == -1);
}

TEST_CASE( "findValue - success", "[treemap]" ) {
	ccTreeMap symbolTree;
	REQUIRE (symbolTree.findValue("a") == -1);
	symbolTree.addEntry("a", 400);
	REQUIRE (symbolTree.findValue("a") == 400);
}

TEST_CASE( "clear", "[treemap]" ) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 400);
	REQUIRE (symbolTree.findValue("a") == 400);
	symbolTree.clear();
	REQUIRE (symbolTree.findValue("a") == -1);
}
