//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <vector>
#include "gtest/gtest.h"
#include "script2/cc_internallist.h"
#include "script2/cs_parser_common.h"

// defined in script_common, modified by getnext
extern int currentline;

// The vars defined here are provided in each test
class SrcList : public ::testing::Test
{
protected:
    AGS::LineHandler lh = AGS::LineHandler();
    size_t cursor = 0u;
};

TEST_F(SrcList, GetNext) {
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh, cursor);

    EXPECT_EQ(1, src.GetNext());
    EXPECT_EQ(2, src.GetNext());

    src.SetCursor(1u);
    EXPECT_EQ(2, src.GetNext());

    src.SetCursor(4u);
    EXPECT_EQ(5, src.GetNext());
    EXPECT_EQ(AGS::SrcList::kEOF, src.GetNext());

    src.SetCursor(4711u);
    EXPECT_EQ(AGS::SrcList::kEOF, src.GetNext());
    EXPECT_TRUE(src.ReachedEOF());
}

TEST_F(SrcList, PeekNext) {
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh, cursor);

    src.SetCursor(1u);
    EXPECT_EQ(2, src.PeekNext());
    EXPECT_EQ(2, src.PeekNext());

    src.SetCursor(5u);
    EXPECT_EQ(AGS::SrcList::kEOF, src.PeekNext());
    EXPECT_TRUE(src.ReachedEOF());
}

TEST_F(SrcList, InRange) {
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh, cursor);

    EXPECT_TRUE(src.InRange(3u));
    EXPECT_FALSE(src.InRange(5u));
}

TEST_F(SrcList, Brackets) {
    std::vector<AGS::Symbol> script = { 1, 2, 3, 4, 5, };
    AGS::SrcList src(script, lh, cursor);

    EXPECT_EQ(1, src[0u]);
    EXPECT_EQ(4, src[3u]);

    EXPECT_EQ(AGS::SrcList::kEOF, src[12345u]);
}

TEST_F(SrcList, AppendLineno) {
    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh, cursor);
    src.Append(1);
    src.Append(1);
    src.NewLine(4711u);
    src.Append(3);
    src.NewLine(7u);
    src.NewLine(11u); // overwrites the previous 'NewLine()'
    src.Append(9);

    EXPECT_EQ(1u, src.GetLinenoAt(0u));
    EXPECT_EQ(1u, src.GetLinenoAt(1u));
    EXPECT_EQ(4711u, src.GetLinenoAt(2u));
    EXPECT_EQ(11u, src.GetLinenoAt(5u)); // last linenum is valid "forever"
    EXPECT_EQ(1u, src.GetLinenoAt(1u));
    src.SetCursor(4u);
    EXPECT_EQ(11u, src.GetLineno());
}

TEST_F(SrcList, AppendSections) {
    // A section change will only "stick" when it is followed by a 'NewLine()'
    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh, cursor);
    src.NewLine(10u);
    src.Append(1);
    src.NewSection("Apple");
    src.NewLine(10u);
    src.Append(1);
    src.Append(3);
    src.NewSection("Banana");
    src.NewLine(10u);
    src.NewSection("Cherry");
    src.NewLine(10u);
    src.Append(9);

    EXPECT_EQ(10u, src.GetLinenoAt(0u));
    EXPECT_EQ(10u, src.GetLinenoAt(1u));
    EXPECT_EQ(10u, src.GetLinenoAt(2u)); // NewSection doesn't automatically reset the line counter
    EXPECT_STREQ("Apple", src.SectionId2Section(src.GetSectionIdAt(2)).c_str());
    EXPECT_STREQ("Cherry", src.SectionId2Section(src.GetSectionIdAt(9)).c_str());
    src.SetCursor(0u);
    EXPECT_STREQ("", src.SectionId2Section(src.GetSectionId()).c_str());
}

TEST_F(SrcList, Reference0) {
    std::vector<AGS::Symbol> script;
    // Script 1  1   3  9
    // Line   10 10  10 77
    // Sect.     Ap  Ch
    AGS::SrcList src(script, lh, cursor);
    src.NewLine(10u);
    src.Append(1);
    src.NewSection("Apple");
    src.NewLine(10u);
    src.Append(1);
    src.Append(3);
    src.NewSection("Cherry");
    src.NewLine(10u);
    src.NewLine(77u);
    src.Append(9);

    // Script  3  9
    AGS::SrcList part = AGS::SrcList(src, 2, 7);
    EXPECT_EQ(3, part[0u]);
    EXPECT_EQ(9, part[1u]);
    EXPECT_EQ(AGS::SrcList::kEOF, part[2u]);
    EXPECT_TRUE(part.InRange(1u));
    EXPECT_FALSE(part.InRange(2u));
    src.SetCursor(0u); // ´'src' (!)
    EXPECT_EQ(AGS::SrcList::kEOF, part.GetNext());
    part.StartRead();
    EXPECT_EQ(3, part.GetNext());
    EXPECT_EQ(9, part.PeekNext());
    EXPECT_EQ(9, part.PeekNext());
    EXPECT_EQ(9, part.GetNext());
    EXPECT_EQ(AGS::SrcList::kEOF, part.GetNext());
    EXPECT_TRUE(part.ReachedEOF());
    EXPECT_EQ(10u, part.GetLinenoAt(0u));
    EXPECT_STREQ("Apple", part.SectionId2Section(part.GetSectionIdAt(0)).c_str());
    EXPECT_EQ(77u, part.GetLinenoAt(1u));
    EXPECT_STREQ("Cherry", part.SectionId2Section(part.GetSectionIdAt(1)).c_str());
}

TEST_F(SrcList, Reference1) {

    // cpy is set up as an excerpt of src.
    // GetCursor() should give the cursor position relative to the respective SrcList.
    // Moving the cursor in one SrcList should move the cursor in the other, too.

    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh, cursor);
    for (AGS::Symbol i = 0; i < 10; i++)
        src.Append(i);
    AGS::SrcList cpy(src, 3u, 3u);
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

TEST_F(SrcList, Reference2) {

    // cpy is set up as an excerpt of src.
    // Line numbers for a symbol should be the same
    // no matter the way the symbol is referenced.

    std::vector<AGS::Symbol> script;
    AGS::SrcList src(script, lh, cursor);
    src.NewLine(5u);
    for (AGS::Symbol i = 0; i < 5; i++)
        src.Append(i);
    src.NewLine(10u);
    for (AGS::Symbol i = 5; i < 10; i++)
        src.Append(i);
    AGS::SrcList cpy(src, 3, 3);
    EXPECT_EQ(5u, src.GetLinenoAt(4u));
    EXPECT_EQ(5u, cpy.GetLinenoAt(1u));
    EXPECT_EQ(10u, src.GetLinenoAt(5u));
    EXPECT_EQ(10u, cpy.GetLinenoAt(2u));
}
