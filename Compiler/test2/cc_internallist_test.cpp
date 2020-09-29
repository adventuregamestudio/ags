#include <vector>
#include "gtest/gtest.h"
#include "script2/cc_internallist.h"
#include "script2/cs_parser_common.h"

// defined in script_common, modified by getnext
extern int currentline;

TEST(SrcList, GetNext) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);

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
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);

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
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);

    EXPECT_TRUE(src.InRange(3));
    EXPECT_FALSE(src.InRange(5));
}

TEST(SrcList, Brackets) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);

    EXPECT_EQ(1, src[0]);
    EXPECT_EQ(4, src[3]);

    EXPECT_EQ(AGS::SrcList::kEOF, src[12345]);
}

TEST(SrcList, AppendLineno) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);
    src.Append(1);
    src.Append(1);
    src.NewLine(4711);
    src.Append(3);
    src.NewLine(7);
    src.NewLine(11); // overwrites the previous NewLine()
    src.Append(9);

    EXPECT_EQ(1, src.GetLinenoAt(0));
    EXPECT_EQ(1, src.GetLinenoAt(1));
    EXPECT_EQ(4711, src.GetLinenoAt(2));
    EXPECT_EQ(11, src.GetLinenoAt(5)); // last linenum is valid "forever"
    EXPECT_EQ(1, src.GetLinenoAt(1));
    src.SetCursor(4);
    EXPECT_EQ(11, src.GetLineno());
}

TEST(SrcList, AppendSections) {
    // A section change will only "stick" when it is followed by a NewLine();
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);
    src.NewLine(10);
    src.Append(1);
    src.NewSection("Apple");
    src.NewLine(10);
    src.Append(1);
    src.Append(3);
    src.NewSection("Banana");
    src.NewLine(10);
    src.NewSection("Cherry");
    src.NewLine(10);
    src.Append(9);

    EXPECT_EQ(10, src.GetLinenoAt(0));
    EXPECT_EQ(10, src.GetLinenoAt(1));
    EXPECT_EQ(10, src.GetLinenoAt(2)); // NewSection doesn't automatically reset the line counter
    EXPECT_STREQ("Apple", src.SectionId2Section(src.GetSectionIdAt(2)).c_str());
    EXPECT_STREQ("Cherry", src.SectionId2Section(src.GetSectionIdAt(9)).c_str());
    src.SetCursor(0);
    EXPECT_STREQ("", src.SectionId2Section(src.GetSectionId()).c_str());
}

TEST(SrcList, Reference0) {
    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);
    src.NewLine(10);
    src.Append(1);
    src.NewSection("Apple");
    src.NewLine(10);
    src.Append(1);
    src.Append(3);
    src.NewSection("Cherry");
    src.NewLine(10);
    src.NewLine(77);
    src.Append(9);
    AGS::SrcList part = AGS::SrcList(src, 2, 7);
    EXPECT_EQ(3, part[0]);
    EXPECT_EQ(9, part[1]);
    EXPECT_EQ(AGS::SrcList::kEOF, part[2]);
    EXPECT_TRUE(part.InRange(1));
    EXPECT_FALSE(part.InRange(2));
    src.SetCursor(0);
    EXPECT_EQ(AGS::SrcList::kEOF, part.GetNext());
    part.StartRead();
    EXPECT_EQ(3, part.GetNext());
    EXPECT_EQ(9, part.PeekNext());
    EXPECT_EQ(9, part.PeekNext());
    EXPECT_EQ(9, part.GetNext());
    EXPECT_EQ(AGS::SrcList::kEOF, part.GetNext());
    EXPECT_TRUE(part.ReachedEOF());
    EXPECT_EQ(10, part.GetLinenoAt(0));
    EXPECT_STREQ("Apple", part.SectionId2Section(part.GetSectionIdAt(0)).c_str());
    EXPECT_EQ(77, part.GetLinenoAt(1));
    EXPECT_STREQ("Cherry", part.SectionId2Section(part.GetSectionIdAt(1)).c_str());
}

TEST(SrcList, Reference1) {

    // cpy is set up as an excerpt of src.
    // GetCursor() should give the cursor position relative to the respective SrcList.
    // Moving the cursor in one SrcList should move the cursor in the other, too.

    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);
    for (AGS::Symbol i = 0; i < 10; i++)
        src.Append(i);
    AGS::SrcList cpy(src, 3, 3);
    cpy.StartRead();
    EXPECT_EQ(3, src.GetCursor());
    EXPECT_EQ(0, cpy.GetCursor());

    EXPECT_EQ(3, cpy.GetNext());
    EXPECT_EQ(4, src.GetNext());
    EXPECT_EQ(2, cpy.GetCursor());

    EXPECT_EQ(5, cpy.GetNext());
    EXPECT_EQ(6, src.GetNext());
    EXPECT_EQ(true, cpy.ReachedEOF());
    EXPECT_EQ(false, src.ReachedEOF());
}

TEST(SrcList, Reference2) {

    // cpy is set up as an excerpt of src.
    // Line numbers for a symbol should be the same
    // no matter the way the symbol is referenced.

    AGS::LineHandler lh = AGS::LineHandler();
    std::vector<AGS::Symbol> script;
    size_t cursor = 0;
    AGS::SrcList src(script, lh, cursor);
    src.NewLine(5);
    for (AGS::Symbol i = 0; i < 5; i++)
        src.Append(i);
    src.NewLine(10);
    for (AGS::Symbol i = 5; i < 10; i++)
        src.Append(i);
    AGS::SrcList cpy(src, 3, 3);
    EXPECT_EQ(5, src.GetLinenoAt(4));
    EXPECT_EQ(5, cpy.GetLinenoAt(1));
    EXPECT_EQ(10, src.GetLinenoAt(5));
    EXPECT_EQ(10, cpy.GetLinenoAt(2));
}
