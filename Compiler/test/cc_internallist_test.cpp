#include <vector>
#include "gtest/gtest.h"
#include "script/cc_internallist.h"
#include "script/cs_parser_common.h"

// defined in script_common, modified by getnext
extern int currentline;

TEST(SrcList, GetNext) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh);

    EXPECT_EQ(1, src.GetNext());
    EXPECT_EQ(2, src.GetNext());

    src.SetCursor(1);
    EXPECT_EQ(2, src.GetNext());

    src.SetCursor(4);
    EXPECT_EQ(5, src.GetNext());
    EXPECT_EQ(AGS::SrcList::kEOF, src.GetNext());

    src.SetCursor(4711);
    EXPECT_EQ(AGS::SrcList::kEOF, src.GetNext());
    EXPECT_TRUE(src.ReachedEOF());
}

TEST(SrcList, PeekNext) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh);

    src.SetCursor(1);
    EXPECT_EQ(2, src.PeekNext());
    EXPECT_EQ(2, src.PeekNext());

    src.SetCursor(5);
    EXPECT_EQ(AGS::SrcList::kEOF, src.PeekNext());
    EXPECT_TRUE(src.ReachedEOF());
}

TEST(SrcList, InRange) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh);

    EXPECT_TRUE(src.InRange(3));
    EXPECT_FALSE(src.InRange(5));
}

TEST(SrcList, Brackets) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh);

    EXPECT_EQ(1, src[0]);
    EXPECT_EQ(4, src[3]);

    EXPECT_EQ(AGS::SrcList::kEOF, src[12345]);
}

TEST(SrcList, AppendLineno) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh);
    src.Append(1);
    src.Append(1);
    src.NewLine(4711);
    src.Append(3);
    src.NewLine(7);
    src.NewLine(11); // overwrites the previous NewLine()
    src.Append(9);

    EXPECT_EQ(0, src.GetLineno(0));
    EXPECT_EQ(0, src.GetLineno(1));
    EXPECT_EQ(4711, src.GetLineno(2));
    EXPECT_EQ(11, src.GetLineno(5)); // last linenum is valid "forever"
    EXPECT_EQ(0, src.GetLineno(1));
    src.SetCursor(4);
    EXPECT_EQ(11, src.GetLineno());
}

TEST(SrcList, AppendSections) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh);
    src.NewLine(10);
    src.Append(1);
    src.NewSection("Apple");
    src.Append(1);
    src.Append(3);
    src.NewSection("Banana");
    src.NewSection("Cherry"); // overwrites the previous NewSection()
    src.Append(9);

    EXPECT_EQ(10, src.GetLineno(0));
    EXPECT_EQ(10, src.GetLineno(1));
    EXPECT_EQ(10, src.GetLineno(2)); // NewSection doesn't automatically reset the line counter
    EXPECT_STREQ("Apple", src.Id2Section(src.GetSectionId(2)).c_str());
    EXPECT_STREQ("Cherry", src.Id2Section(src.GetSectionId(9)).c_str());
    src.SetCursor(0);
    EXPECT_STREQ("", src.Id2Section(src.GetSectionId()).c_str());
}

TEST(SrcList, PartCopy) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh);
    src.NewLine(10);
    src.Append(1);
    src.NewSection("Apple");
    src.Append(1);
    src.Append(3);
    src.NewSection("Banana");
    src.NewSection("Cherry"); // overwrites the previous NewSection()
    src.NewLine(77);
    src.Append(9);
    AGS::SrcList part = AGS::SrcList(src, 2, 7);
    EXPECT_EQ(3, part[0]);
    EXPECT_EQ(9, part[1]);
    EXPECT_EQ(AGS::SrcList::kEOF, part[2]);
    EXPECT_TRUE(part.InRange(1));
    EXPECT_FALSE(part.InRange(2));
    src.SetCursor(0);
    EXPECT_EQ(3, part.GetNext());
    EXPECT_EQ(9, part.PeekNext());
    EXPECT_EQ(9, part.PeekNext());
    EXPECT_EQ(9, part.GetNext());
    EXPECT_EQ(AGS::SrcList::kEOF, part.GetNext());
    EXPECT_TRUE(part.ReachedEOF());
    EXPECT_EQ(10, part.GetLineno(0));
    EXPECT_STREQ("Apple", part.Id2Section(part.GetSectionId(0)).c_str());
    EXPECT_EQ(77, part.GetLineno(1));
    EXPECT_STREQ("Cherry", part.Id2Section(part.GetSectionId(1)).c_str());
}

TEST(InternalList, Constructor) {
    ccInternalList tlist;
    ASSERT_TRUE(tlist.pos == -1);
    ASSERT_TRUE(tlist.length == 0);
    ASSERT_TRUE(tlist.cancelCurrentLine == 1);
    ASSERT_TRUE(tlist.lineAtEnd == -1);
}

TEST(InternalList, Write) {
    ccInternalList tlist;

    tlist.write(1);
    ASSERT_TRUE(tlist.length == 1);
    ASSERT_TRUE(tlist.script[0] == 1);

    tlist.write(1000);
    ASSERT_TRUE(tlist.length == 2);
    ASSERT_TRUE(tlist.script[1] == 1000);
}


TEST(InternalList, WriteMeta) {
    ccInternalList tlist;

    tlist.write_meta(1, 2);
    ASSERT_TRUE(tlist.length == 3);
    ASSERT_TRUE(tlist.script[0] == SCODE_META);
    ASSERT_TRUE(tlist.script[1] == 1);
    ASSERT_TRUE(tlist.script[2] == 2);

    tlist.write_meta(1000, 2000);
    ASSERT_TRUE(tlist.length == 6);
    ASSERT_TRUE(tlist.script[3] == SCODE_META);
    ASSERT_TRUE(tlist.script[4] == 1000);
    ASSERT_TRUE(tlist.script[5] == 2000);
}

TEST(InternalList, StartRead) {
    ccInternalList tlist;

    tlist.startread();
    ASSERT_TRUE(tlist.pos == 0);
}

TEST(InternalList, PeekNext) {

    // normal usage
    {
        ccInternalList tlist;

        tlist.write(1);
        tlist.write(2);
        tlist.write(3);

        tlist.startread();
        ASSERT_TRUE(tlist.peeknext() == 1);
        ASSERT_TRUE(tlist.getnext() == 1);
        ASSERT_TRUE(tlist.peeknext() == 2);
        ASSERT_TRUE(tlist.getnext() == 2);
        ASSERT_TRUE(tlist.peeknext() == 3);
        ASSERT_TRUE(tlist.getnext() == 3);
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);
    }

    // empty
    {
        ccInternalList tlist;

        tlist.startread();
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);
    }

    // no startread
    {
        ccInternalList tlist;
        tlist.write(1);
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);
    }

    // skip meta
    {
        ccInternalList tlist;

        tlist.write_meta(1000, 2000);
        tlist.write_meta(1001, 2002);
        tlist.write(200);

        tlist.startread();
        ASSERT_TRUE(tlist.peeknext() == 200);
    }

    // skip meta to eof
    {
        ccInternalList tlist;

        tlist.write_meta(1000, 2000);
        tlist.write_meta(1001, 2001);

        tlist.startread();
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);
    }

    // skip truncated meta
    {
        ccInternalList tlist;
        tlist.startread();

        tlist.write(SCODE_META);
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);

        tlist.write(1);
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);

        tlist.write(2);
        ASSERT_TRUE(tlist.peeknext() == SCODE_INVALID);

        tlist.write(9000);
        ASSERT_TRUE(tlist.peeknext() == 9000);
    }
}

