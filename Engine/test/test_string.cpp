//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#ifdef _DEBUG

#include <vector>
#include <stdio.h>
#include <string.h>
#include "util/path.h"
#include "util/string.h"
#include "debug/assert.h"

using namespace AGS::Common;

void Test_Path()
{
     assert(Path::IsSameOrSubDir(".", "dir1/") == true);
     assert(Path::IsSameOrSubDir(".", "dir1/dir2/dir3/") == true);
     assert(Path::IsSameOrSubDir(".", "dir1/../") == true);
     assert(Path::IsSameOrSubDir(".", "dir1/dir2/../../") == true);
     assert(Path::IsSameOrSubDir(".", "dir1/../dir2/../dir3/") == true);
     assert(Path::IsSameOrSubDir(".", "..dir/") == true);

     assert(Path::IsSameOrSubDir(".", "../") == false);
     assert(Path::IsSameOrSubDir(".", "../") == false);
     assert(Path::IsSameOrSubDir(".", "/dir1/") == false);
     assert(Path::IsSameOrSubDir(".", "dir1/../../") == false);
//     assert(Path::IsSameOrSubDir(".", "dir1/../dir2/../../dir3/") == false);
}

void Test_String()
{

     // Test Compare
     {
         String s1 = "abcdabcdabcd";
         String s2 = "abcdbfghijklmn";
         int cmp1 = s1.Compare(s2);
         auto cmp2 = s1.StartsWith("abcd");
         auto cmp3 = s1.StartsWith("abcdxxx");
         auto cmp4 = s1.StartsWith("abcdxxx");
         auto cmp8 = s1.StartsWith("abcdabcdabcdxxxx");
       
         assert(cmp1 < 0);
         assert(cmp2);
         assert(!cmp3);
         assert(!cmp4);
         assert(!cmp8);

     }

     // Test FindChar
     {
         String s1 = "findsomethinginhere";
         String s2 = "stringtofindsomethinginside";
         String s3 = "findsomethinginherex";
         String s4 = "xstringtofindsomethinginside";
         String s5;
         size_t find1 = s1.FindChar('o');
         size_t find2 = s2.FindCharReverse('o');
         size_t find3 = s1.FindChar('x');
         size_t find4 = s2.FindCharReverse('x');
         size_t find5 = s3.FindChar('x');
         size_t find6 = s4.FindCharReverse('x');
         size_t find7 = s5.FindChar('x');
         size_t find8 = s5.FindCharReverse('x');
         size_t find9 = s1.FindChar('i', 2);
         size_t find10 = s1.FindCharReverse('i', 12);
         assert(find1 == 5);
         assert(find2 == 13);
         assert(find3 == -1);
         assert(find4 == -1);
         assert(find5 == 19);
         assert(find6 == 0);
         assert(find7 == -1);
         assert(find8 == -1);
         assert(find9 == 10);
         assert(find10 == 10);
     }

     // Test GetAt
     {
         String s1 = "abcdefghijklmnop";
         String s2;
         char c1 = s1.GetAt(0);
         char c2 = s1.GetAt(15);
         char c3 = s1.GetAt(-10);
         char c4 = s1.GetAt(16);
         char c5 = s2.GetAt(0);
         assert(c1 == 'a');
         assert(c2 == 'p');
         assert(c3 == 0);
         assert(c4 == 0);
         assert(c5 == 0);
     }

     // Test ToInt
     {
         String s1;
         String s2 = "100";
         String s3 = "202aaa";
         String s4 = "aaa333";
         int i1 = s1.ToInt();
         int i2 = s2.ToInt();
         int i3 = s3.ToInt();
         int i4 = s4.ToInt();
         assert(i1 == 0);
         assert(i2 == 100);
         assert(i3 == 202);
         assert(i4 == 0);
     }

     // Test Left/Right/Mid
     {
         String s1 = "this is a string to be split";
         String s2 = s1.Left(4);
         String s3 = s1.Left(100);
         String s4 = s1.Mid(10);
         String s5 = s1.Mid(10, 6);
         String s6 = s1.Mid(0, 200);
         String s9 = s1.Left(0);
         String s10 = s1.Mid(-1, 0);

         assert(strcmp(s2.GetCStr(), "this") == 0);
         assert(strcmp(s3.GetCStr(), "this is a string to be split") == 0);
         assert(strcmp(s4.GetCStr(), "string to be split") == 0);
         assert(strcmp(s5.GetCStr(), "string") == 0);
         assert(strcmp(s6.GetCStr(), "this is a string to be split") == 0);
         assert(strcmp(s9.GetCStr(), "") == 0);
         assert(strcmp(s10.GetCStr(), "") == 0);
     }

     // Test Append
     {
         String s1 = "a string to enlarge - ";
         s1.Append("make it bigger");
         assert(strcmp(s1.GetCStr(), "a string to enlarge - make it bigger") == 0);
         s1.AppendChar('!');
         assert(strcmp(s1.GetCStr(), "a string to enlarge - make it bigger!") == 0);
         s1.AppendChar(' ');
         assert(strcmp(s1.GetCStr(), "a string to enlarge - make it bigger! ") == 0);
         s1.Append("much much bigger!");
         assert(strcmp(s1.GetCStr(), "a string to enlarge - make it bigger! much much bigger!") == 0);
     }

     // Test Clip
     {
         String str1 = "long truncateable string";
         String str2 = str1;
         String str3 = str1;
         String str4 = str1;
         String str5 = str1;

         str1.ClipLeft(4);
         str2.ClipRight(6);
         str3.ClipMid(5, 12);
         str4.ClipMid(5, 0);
         str5.ClipMid(0);
         assert(strcmp(str1.GetCStr(), " truncateable string") == 0);
         assert(strcmp(str2.GetCStr(), "long truncateable ") == 0);
         assert(strcmp(str3.GetCStr(), "long  string") == 0);
         assert(strcmp(str4.GetCStr(), "long truncateable string") == 0);
         assert(strcmp(str5.GetCStr(), "") == 0);
     }

     // Test making new string
     {
         String s1 = "we have some string here";
         assert(strcmp(s1.GetCStr(), "we have some string here") == 0);
         s1.Empty();
         assert(strcmp(s1.GetCStr(), "") == 0);
         s1.Format("this %d is %9ld a %x formatted %0.2f string %s", 1,2,100,22.55F,"abcd");
         assert(strcmp(s1.GetCStr(), "this 1 is         2 a 64 formatted 22.55 string abcd") == 0);
         s1.SetString("some string");
         assert(strcmp(s1.GetCStr(), "some string") == 0);
         s1.SetString("some string", 4);
         assert(strcmp(s1.GetCStr(), "some") == 0);
     }

     // Test Upper/Lower case
     {
         String s1 = "ThIs StRiNg Is TwIsTeD";
         String s2 = s1;
         String s3 = s1;
         s2.MakeLower();
         assert(strcmp(s2.GetCStr(), "this string is twisted") == 0);
     }

     // Test Prepend
     {
         String s1 = "- a string to enlarge";
         s1.Prepend("make it bigger ");
         assert(strcmp(s1.GetCStr(), "make it bigger - a string to enlarge") == 0);
         s1.PrependChar('!');
         assert(strcmp(s1.GetCStr(), "!make it bigger - a string to enlarge") == 0);
         s1.PrependChar(' ');
         assert(strcmp(s1.GetCStr(), " !make it bigger - a string to enlarge") == 0);
         s1.Prepend("much much bigger!");
         assert(strcmp(s1.GetCStr(), "much much bigger! !make it bigger - a string to enlarge") == 0);
     }

     // Test ReplaceMid
     {
         String s1 = "we need to replace PRECISELY THIS PART in this string";
         String s2 = s1;
         String new_long = "WITH A NEW TAD LONGER SUBSTRING";
         String new_short = "SMALL STRING";
         s1.ReplaceMid(19, 19, new_long);
         assert(strcmp(s1.GetCStr(), "we need to replace WITH A NEW TAD LONGER SUBSTRING in this string") == 0);
         s2.ReplaceMid(19, 19, new_short);
         assert(strcmp(s2.GetCStr(), "we need to replace SMALL STRING in this string") == 0);
         String s3 = "insert new string here: ";
         s3.ReplaceMid(s3.GetLength(), 0, "NEW STRING");
         assert(strcmp(s3.GetCStr(), "insert new string here: NEW STRING") == 0);
     }

     // Test SetAt
     {
         String s1 = "strimg wiyh typos";
         s1.SetAt(-1, 'a');
         assert(strcmp(s1.GetCStr(), "strimg wiyh typos") == 0);
         s1.SetAt(100, 'a');
         assert(strcmp(s1.GetCStr(), "strimg wiyh typos") == 0);
         s1.SetAt(1, 0);
         assert(strcmp(s1.GetCStr(), "strimg wiyh typos") == 0);
         s1.SetAt(4, 'n');
         s1.SetAt(9, 't');
         assert(strcmp(s1.GetCStr(), "string with typos") == 0);
     }

     // Test Trim
     {
         String str1 = "\t   This string is quite long and should be cut a little bit\r\n    ";
         String str2 = str1;

         str2.TrimRight();

         assert(strcmp(str2.GetCStr(), "\t   This string is quite long and should be cut a little bit") == 0);
     }

     // Test Truncate
     {
         String str1 = "long truncateable string";
         String str2 = str1;

         str1.TruncateToLeft(4);
         assert(strcmp(str1.GetCStr(), "long") == 0);
     }

    
    // test split
    {
        
        {
            auto result = String("a;b;c").Split(";");
            auto expected = std::vector<String>{String("a"), String("b"),String("c") };
            assert (result == expected);
        }
        
        {
            auto result = String("a;b;c").Split(";", 1);
            auto expected = std::vector<String>{String("a"), String("b;c") };
            assert (result == expected);
        }
        
        {
            auto result = String(";;").Split(";");
            auto expected = std::vector<String>{String(""), String(""),String("") };
            assert (result == expected);
        }

    }
}

#endif // _DEBUG
