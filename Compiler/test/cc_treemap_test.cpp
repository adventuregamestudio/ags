#include "catch.hpp"

#include "script/cc_treemap.h"

TEST_CASE( "Initial Symbol Tree Test", "[treemap]" ) {
	ccTreeMap symbolTree;
	REQUIRE (symbolTree.findValue("hello") == -1);
}