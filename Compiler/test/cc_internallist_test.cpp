#include "gtest/gtest.h"
#include "script/cc_internallist.h"

// defined in script_common, modified by getnext
extern int currentline; 


TEST(InternalList, Constructor) {
	ccInternalList tlist;
	ASSERT_TRUE (tlist.pos == -1);
	ASSERT_TRUE (tlist.length == 0);
	ASSERT_TRUE (tlist.cancelCurrentLine == 1);
	ASSERT_TRUE (tlist.lineAtEnd == -1);
}

TEST(InternalList, Write) {
	ccInternalList tlist;

	tlist.write(1);
	ASSERT_TRUE (tlist.length == 1);
	ASSERT_TRUE (tlist.script[0] == 1);

	tlist.write(1000);
	ASSERT_TRUE (tlist.length == 2);
	ASSERT_TRUE (tlist.script[1] == 1000);
}


TEST(InternalList, WriteMeta) {
	ccInternalList tlist;

	tlist.write_meta(1, 2);
	ASSERT_TRUE (tlist.length == 3);
	ASSERT_TRUE (tlist.script[0] == SCODE_META);
	ASSERT_TRUE (tlist.script[1] == 1);
	ASSERT_TRUE (tlist.script[2] == 2);

	tlist.write_meta(1000, 2000);
	ASSERT_TRUE (tlist.length == 6);
	ASSERT_TRUE (tlist.script[3] == SCODE_META);
	ASSERT_TRUE (tlist.script[4] == 1000);
	ASSERT_TRUE (tlist.script[5] == 2000);
}

TEST(InternalList, StartRead) {
	ccInternalList tlist;

	tlist.startread();
	ASSERT_TRUE (tlist.pos == 0);
}

TEST(InternalList, PeekNext) {

	// normal usage
	{
	ccInternalList tlist;

	tlist.write(1);
	tlist.write(2);
	tlist.write(3);

	tlist.startread();
	ASSERT_TRUE (tlist.peeknext() == 1);
	ASSERT_TRUE (tlist.getnext() == 1);
	ASSERT_TRUE (tlist.peeknext() == 2);
	ASSERT_TRUE (tlist.getnext() == 2);
	ASSERT_TRUE (tlist.peeknext() == 3);
	ASSERT_TRUE (tlist.getnext() == 3);
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);
	}

	// empty
	{
	ccInternalList tlist;

	tlist.startread();
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);
	}

	// no startread
	{
	ccInternalList tlist;
	tlist.write(1);
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);
	}

	// skip meta
	{
	ccInternalList tlist;

	tlist.write_meta(1000, 2000);
	tlist.write_meta(1001, 2002);
	tlist.write(200);

	tlist.startread();
	ASSERT_TRUE (tlist.peeknext() == 200);
	}

	// skip meta to eof
	{
	ccInternalList tlist;

	tlist.write_meta(1000, 2000);
	tlist.write_meta(1001, 2001);

	tlist.startread();
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);
	}

	// skip truncated meta
	{
	ccInternalList tlist;
	tlist.startread();

	tlist.write(SCODE_META);
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);

	tlist.write(1);
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);

	tlist.write(2);
	ASSERT_TRUE (tlist.peeknext() == SCODE_INVALID);

	tlist.write(9000);
	ASSERT_TRUE (tlist.peeknext() == 9000);
	}
}

TEST(InternalList, GetNext) {

	// normal case
	{
	ccInternalList tlist;
	tlist.write(2);
	tlist.write(4);
	tlist.write(6);
	tlist.write(8);

	currentline = 42;
	tlist.startread();
	ASSERT_TRUE (tlist.getnext() == 2);
	ASSERT_TRUE (currentline == 42);  // no line meta sym
	ASSERT_TRUE (tlist.getnext() == 4);
	ASSERT_TRUE (currentline == 42);
	ASSERT_TRUE (tlist.getnext() == 6);
	ASSERT_TRUE (currentline == 42);
	ASSERT_TRUE (tlist.getnext() == 8);
	ASSERT_TRUE (currentline == 42);
	ASSERT_TRUE (tlist.getnext() == SCODE_INVALID);
	ASSERT_TRUE (currentline == -10);
	}
	
	// cancelCurrentLine == false
	{
	ccInternalList tlist;
	tlist.write(3);

	currentline = 74;
	tlist.startread();
	tlist.cancelCurrentLine = 0;
	ASSERT_TRUE (tlist.getnext() == 3);
	ASSERT_TRUE (currentline == 74); 
	ASSERT_TRUE (tlist.getnext() == SCODE_INVALID);
	ASSERT_TRUE (currentline == 74);
	}

	// set current line
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 101);
	tlist.write(7);

	currentline = 100;
	tlist.startread();
	ASSERT_TRUE (currentline == 100); 
	ASSERT_TRUE (tlist.getnext() == 7);
	ASSERT_TRUE (currentline == 101); 
	}

	// set lineAtEnd
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 13);
	tlist.write_meta(SMETA_END, 0); // value ignored
	tlist.write(7);

	currentline = 100;
	tlist.startread();
	ASSERT_TRUE (tlist.lineAtEnd == -1); 
	ASSERT_TRUE (tlist.getnext() == SCODE_META);//<-- weird!  we return the start of the meta code.
	ASSERT_TRUE (tlist.lineAtEnd == 13);  
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
	ASSERT_TRUE (currentline == 100); 
	ASSERT_TRUE (tlist.getnext() == 7);
	ASSERT_TRUE (currentline == 104); 
	}

	// meta , no data
	{
	ccInternalList tlist;
	tlist.write_meta(SMETA_LINENUM, 101);
	tlist.write_meta(SMETA_LINENUM, 102);

	currentline = 100;
	tlist.startread();
	tlist.cancelCurrentLine = 0;
	ASSERT_TRUE (currentline == 100); 
	ASSERT_TRUE (tlist.getnext() == SCODE_INVALID);
	ASSERT_TRUE (currentline == 102); 
	}
}
