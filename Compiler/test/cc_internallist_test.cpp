#include "catch.hpp"
#include "script/cc_internallist.h"

// defined in script_common, modified by getnext
extern int currentline; 


TEST_CASE("constructor", "[cc_internallist]") {
	ccInternalList tlist;
	REQUIRE (tlist.pos == -1);
	REQUIRE (tlist.length == 0);
	REQUIRE (tlist.cancelCurrentLine == 1);
	REQUIRE (tlist.lineAtEnd == -1);
}

TEST_CASE("write", "[cc_internallist]") {
	ccInternalList tlist;

	tlist.write(1);
	REQUIRE (tlist.length == 1);
	REQUIRE (tlist.script[0] == 1);

	tlist.write(1000);
	REQUIRE (tlist.length == 2);
	REQUIRE (tlist.script[1] == 1000);
}


TEST_CASE("write_meta", "[cc_internallist]") {
	ccInternalList tlist;

	tlist.write_meta(1, 2);
	REQUIRE (tlist.length == 3);
	REQUIRE (tlist.script[0] == SCODE_META);
	REQUIRE (tlist.script[1] == 1);
	REQUIRE (tlist.script[2] == 2);

	tlist.write_meta(1000, 2000);
	REQUIRE (tlist.length == 6);
	REQUIRE (tlist.script[3] == SCODE_META);
	REQUIRE (tlist.script[4] == 1000);
	REQUIRE (tlist.script[5] == 2000);
}

TEST_CASE("startread", "[cc_internallist]") {
	ccInternalList tlist;

	tlist.startread();
	REQUIRE (tlist.pos == 0);
}

TEST_CASE("peeknext", "[cc_internallist]") {

	// normal usage
	{
	ccInternalList tlist;

	tlist.write(1);
	tlist.write(2);
	tlist.write(3);

	tlist.startread();
	REQUIRE (tlist.peeknext() == 1);
	REQUIRE (tlist.getnext() == 1);
	REQUIRE (tlist.peeknext() == 2);
	REQUIRE (tlist.getnext() == 2);
	REQUIRE (tlist.peeknext() == 3);
	REQUIRE (tlist.getnext() == 3);
	REQUIRE (tlist.peeknext() == SCODE_INVALID);
	}

	// empty
	{
	ccInternalList tlist;

	tlist.startread();
	REQUIRE (tlist.peeknext() == SCODE_INVALID);
	}

	// no startread
	{
	ccInternalList tlist;
	tlist.write(1);
	REQUIRE (tlist.peeknext() == SCODE_INVALID);
	}

	// skip meta
	{
	ccInternalList tlist;

	tlist.write_meta(1000, 2000);
	tlist.write_meta(1001, 2002);
	tlist.write(200);

	tlist.startread();
	REQUIRE (tlist.peeknext() == 200);
	}

	// skip meta to eof
	{
	ccInternalList tlist;

	tlist.write_meta(1000, 2000);
	tlist.write_meta(1001, 2001);

	tlist.startread();
	REQUIRE (tlist.peeknext() == SCODE_INVALID);
	}

	// skip truncated meta
	{
	ccInternalList tlist;
	tlist.startread();

	tlist.write(SCODE_META);
	REQUIRE (tlist.peeknext() == SCODE_INVALID);

	tlist.write(1);
	REQUIRE (tlist.peeknext() == SCODE_INVALID);

	tlist.write(2);
	REQUIRE (tlist.peeknext() == SCODE_INVALID);

	tlist.write(9000);
	REQUIRE (tlist.peeknext() == 9000);
	}
}

TEST_CASE("getnext", "[cc_internallist]") {

	// normal case
	{
	ccInternalList tlist;
	tlist.write(2);
	tlist.write(4);
	tlist.write(6);
	tlist.write(8);

	currentline = 42;
	tlist.startread();
	REQUIRE (tlist.getnext() == 2);
	REQUIRE (currentline == 42);  // no line meta sym
	REQUIRE (tlist.getnext() == 4);
	REQUIRE (currentline == 42);
	REQUIRE (tlist.getnext() == 6);
	REQUIRE (currentline == 42);
	REQUIRE (tlist.getnext() == 8);
	REQUIRE (currentline == 42);
	REQUIRE (tlist.getnext() == SCODE_INVALID);
	REQUIRE (currentline == -10);
	}
	
	// cancelCurrentLine == false
	{
	ccInternalList tlist;
	tlist.write(3);

	currentline = 74;
	tlist.startread();
	tlist.cancelCurrentLine = 0;
	REQUIRE (tlist.getnext() == 3);
	REQUIRE (currentline == 74); 
	REQUIRE (tlist.getnext() == SCODE_INVALID);
	REQUIRE (currentline == 74);
	}

	// set current line
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 101);
	tlist.write(7);

	currentline = 100;
	tlist.startread();
	REQUIRE (currentline == 100); 
	REQUIRE (tlist.getnext() == 7);
	REQUIRE (currentline == 101); 
	}

	// set lineAtEnd
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 13);
	tlist.write_meta(SMETA_END, 0); // value ignored
	tlist.write(7);

	currentline = 100;
	tlist.startread();
	REQUIRE (tlist.lineAtEnd == -1); 
	REQUIRE (tlist.getnext() == SCODE_META);//<-- weird!  we return the start of the meta code.
	REQUIRE (tlist.lineAtEnd == 13);  
	}

	// multiple metas
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 101);
	tlist.write_meta(SMETA_LINENUM, 102);
	tlist.write_meta(SMETA_LINENUM, 103);
	tlist.write_meta(SMETA_LINENUM, 104);
	tlist.write(7);

	currentline = 100;
	tlist.startread();
	REQUIRE (currentline == 100); 
	REQUIRE (tlist.getnext() == 7);
	REQUIRE (currentline == 104); 
	}

	// meta , no data
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 101);
	tlist.write_meta(SMETA_LINENUM, 102);

	currentline = 100;
	tlist.startread();
	tlist.cancelCurrentLine = 0;
	REQUIRE (currentline == 100); 
	REQUIRE (tlist.getnext() == SCODE_INVALID);
	REQUIRE (currentline == 102); 
	}
}