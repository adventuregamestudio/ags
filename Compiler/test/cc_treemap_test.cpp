#include "gtest/gtest.h"
#include "script/cc_treemap.h"

TEST(TreeMap, AddEntrySingleValue) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 1);
	ASSERT_TRUE (symbolTree.findValue("a") == 1);
}

TEST(TreeMap, AddEntryOverwriteValue) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 1);
	symbolTree.addEntry("a", 2);
	ASSERT_TRUE (symbolTree.findValue("a") == 2);
}

TEST(TreeMap, AddEntryDoNothingIfNullOrString) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("", 100);
	ASSERT_TRUE (symbolTree.findValue("") == -1);
	symbolTree.addEntry(NULL, 101);
	ASSERT_TRUE (symbolTree.findValue(NULL) == -1);
}

TEST(TreeMap, AddEntryCaseSensitivity) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 200);
	ASSERT_TRUE (symbolTree.findValue("a") == 200);
	ASSERT_TRUE (symbolTree.findValue("A") == -1);

	symbolTree.addEntry("A", 201);
	ASSERT_TRUE (symbolTree.findValue("a") == 200);
	ASSERT_TRUE (symbolTree.findValue("A") == 201);
}

TEST(TreeMap, FindValueNullOrEmpty) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 300);
	symbolTree.addEntry("b", 301);
	ASSERT_TRUE (symbolTree.findValue("") == -1);
	ASSERT_TRUE (symbolTree.findValue(NULL) == -1);
}

TEST(TreeMap, FindValueSuccess) {
	ccTreeMap symbolTree;
	ASSERT_TRUE (symbolTree.findValue("a") == -1);
	symbolTree.addEntry("a", 400);
	ASSERT_TRUE (symbolTree.findValue("a") == 400);
}

TEST(TreeMap, Clear) {
	ccTreeMap symbolTree;
	symbolTree.addEntry("a", 400);
	ASSERT_TRUE (symbolTree.findValue("a") == 400);
	symbolTree.clear();
	ASSERT_TRUE (symbolTree.findValue("a") == -1);
}
