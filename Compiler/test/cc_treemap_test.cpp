//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
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
