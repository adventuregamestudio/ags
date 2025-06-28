using NUnit.Framework;

namespace AGS.Editor
{
    [TestFixture]
    public class IncludeUtilsTests
    {
        [Test]
        public void CreatePatternList_ReadPatternsFromString()
        {
            string patternInput = @"
*.asc
!ignoreme.asc
folder/*
";
            var patterns = IncludeUtils.CreatePatternList(patternInput);

            Assert.That(patterns.Length, Is.EqualTo(3));
            Assert.That(patterns[0].OriginalPattern, Is.EqualTo("*.asc"));
            Assert.That(patterns[1].OriginalPattern, Is.EqualTo("ignoreme.asc"));
            Assert.That(patterns[2].OriginalPattern, Is.EqualTo("folder/*"));
        }

        [Test]
        public void CreatePatternList_ParsePatterns()
        {
            string[] patternInput =
            {
                "*.asc",
                "!ignoreme.asc",
                "folder/*"
            };

            var patterns = IncludeUtils.CreatePatternList(patternInput);

            Assert.That(patterns.Length, Is.EqualTo(3));
            Assert.That(patterns[0].Type, Is.EqualTo(IncludeUtils.PatternType.Include));
            Assert.That(patterns[1].Type, Is.EqualTo(IncludeUtils.PatternType.Exclude));
            Assert.That(patterns[2].Type, Is.EqualTo(IncludeUtils.PatternType.Include));

            Assert.That(patterns[0].OriginalPattern, Is.EqualTo("*.asc"));
            Assert.That(patterns[1].OriginalPattern, Is.EqualTo("ignoreme.asc"));
            Assert.That(patterns[2].OriginalPattern, Is.EqualTo("folder/*"));

            // NOTE: IncludeUtils encloses the pattern into (^|/)...($|/) pair
            Assert.That(patterns[0].RegexPattern, Is.EqualTo("(^|/).*\\.asc($|/)"));
            Assert.That(patterns[1].RegexPattern, Is.EqualTo("(^|/)ignoreme\\.asc($|/)"));
            Assert.That(patterns[2].RegexPattern, Is.EqualTo("(^|/)folder\\/.*($|/)"));
        }

        [Test]
        public void FilterItemList_CaseInsensitiveMatch()
        {
            string[] items = new string[]
            {
                "file1.txt",
                "file2.TXT",
                "file3.TxT"
            };

            string[] patternInput =
            {
                "*.TXT"
            };

            var patterns = IncludeUtils.CreatePatternList(patternInput, IncludeUtils.MatchOption.CaseInsensitive);
            var result = IncludeUtils.FilterItemList(items, patterns, IncludeUtils.MatchOption.CaseInsensitive);

            Assert.That(result.Length, Is.EqualTo(3));
        }

        [Test]
        public void FilterItemList_InclusionAndExclusion()
        {
            string[] items =
            {
                "a.txt",
                "b.txt",
                "include_me/c.txt",
                "include_me/d.txt",
                "include_me/exclude_me_anyway.dat",
                "exclude_me/file1.dat",
                "exclude_me/file2.dat",
                "exclude_me/e.txt",
                "exclude_me/f.txt",
                "exclude_me/include_me_anyway.dat",
            };

            string[] patternInput =
            {
                "*.txt",
                "!exclude_me_anyway.dat",
                "!exclude_me/",
                "include_me_anyway.dat"
            };

            var patterns = IncludeUtils.CreatePatternList(patternInput);
            var result = IncludeUtils.FilterItemList(items, patterns, IncludeUtils.MatchOption.CaseInsensitive);

            Assert.That(result.Length, Is.EqualTo(5));
            Assert.That(result, Does.Contain("a.txt"));
            Assert.That(result, Does.Contain("b.txt"));
            Assert.That(result, Does.Contain("include_me/c.txt"));
            Assert.That(result, Does.Contain("include_me/d.txt"));
            Assert.That(result, Does.Contain("exclude_me/include_me_anyway.dat"));
            Assert.That(result, Does.Not.Contain("include_me/exclude_me_anyway.dat"));
            Assert.That(result, Does.Not.Contain("exclude_me/file1.dat"));
            Assert.That(result, Does.Not.Contain("exclude_me/file2.dat"));
            Assert.That(result, Does.Not.Contain("exclude_me/e.txt"));
            Assert.That(result, Does.Not.Contain("exclude_me/f.txt"));
        }

        [Test]
        public void FilterItemList_PatternOrderMatters()
        {
            string[] items = new string[]
            {
                "file.txt",
                "exclude.txt"
            };

            string[] patternInput =
            {
                "*.txt",
                "!exclude.txt",
                "*.txt"
            };

            var patterns = IncludeUtils.CreatePatternList(patternInput);
            var result = IncludeUtils.FilterItemList(items, patterns);

            Assert.That(result, Does.Contain("file.txt"));
            Assert.That(result, Does.Contain("exclude.txt"));
        }

        [Test]
        public void FilterItemList_IgnoreEmptyOrCommentLines()
        {
            string patternInput = @"
# This is a comment
*.dat

!ignore.dat
";

            var patterns = IncludeUtils.CreatePatternList(patternInput);

            Assert.That(patterns.Length, Is.EqualTo(2));
            Assert.That(patterns[0].OriginalPattern, Is.EqualTo("*.dat"));
            Assert.That(patterns[1].OriginalPattern, Is.EqualTo("ignore.dat"));
        }

        [Test]
        public void FilterItemList_MatchAnySectionOfThePath()
        {
            string[] files =
            {
                "name",
                "name/file1.dat",
                "name/file2.dat",
                "name/file3.dat",
                "name.dir/file4.dat",
                "dir/name",
                "dir/name/file5.dat",
                "dir/name.dat"
            };

            string[] patternInput =
            {
                "name"
            };

            var patterns = IncludeUtils.CreatePatternList(patternInput);
            var outFiles = IncludeUtils.FilterItemList(files, patterns, IncludeUtils.MatchOption.CaseInsensitive);

            Assert.That(outFiles.Length, Is.EqualTo(6));
            Assert.That(outFiles, Does.Contain("name"));
            Assert.That(outFiles, Does.Contain("name/file1.dat"));
            Assert.That(outFiles, Does.Contain("name/file2.dat"));
            Assert.That(outFiles, Does.Contain("name/file3.dat"));
            Assert.That(outFiles, Does.Contain("dir/name"));
            Assert.That(outFiles, Does.Contain("dir/name/file5.dat"));
            Assert.That(outFiles, Does.Not.Contain("name.dir/file4.dat"));
            Assert.That(outFiles, Does.Not.Contain("dir/name.dat"));
        }

        [Test]
        public void FilterItemList_ExampleTemplate()
        {
            string[] exampleGameFiles = 
            {
                "acsprset.spr",
                "AGSFNT0.WFN",
                "AGSFNT1.WFN",
                "AGSFNT2.WFN",
                "AudioCache",
                "Compiled",
                "Compiled/Data",
                "Game.agf",
                "Game.agf.bak",
                "Game.agf.user",
                "GlobalScript.asc",
                "GlobalScript.ash",
                "KeyboardMovement.asc",
                "KeyboardMovement.ash",
                "KeyboardMovement.txt",
                "room1.asc",
                "room1.crm",
                "Speech",
                "sprindex.dat",
                "Sprites",
                "Sprites/bluecup0.png",
                "Sprites/Defaults",
                "Sprites/Defaults/Cursors",
                "Sprites/Defaults/Cursors/cursor_interact.png",
                "Sprites/Defaults/ico_bluecup.png",
                "Sprites/Defaults/ico_key.png",
                "Sprites/Defaults/ico_x.png",
                "Sprites/Defaults/UI",
                "Sprites/Defaults/UI/background_inventory.png",
                "Sprites/Defaults/Verbs",
                "Sprites/Defaults/Verbs/btn_arrowdown_normal.png",
                "Sprites/Defaults/Verbs/btn_arrowdown_over.png",
                "template.files",
                "template.ico",
                "template.txt",
                "_Debug",
                "_Debug/acsetup.cfg",
                "_Debug/SDL2.dll",
                "_Debug/Sierra-style.exe"
            };

            string[] patternInput =
            {
                "*.asc",
                "*.ash",
                "*.crm",
                "*.ttf",
                "*.wfn",
                "*.txt",
                "Game.agf",
                "template.files",
                "template.ico",
                "Speech/",
                "Sprites/",
                "!_Debug/",
                "!*.bak",
                "!*.user",
            };

            var patterns = IncludeUtils.CreatePatternList(patternInput, IncludeUtils.MatchOption.CaseInsensitive);
            var result = IncludeUtils.FilterItemList(exampleGameFiles, patterns, IncludeUtils.MatchOption.CaseInsensitive);

            Assert.That(result, Does.Contain("AGSFNT0.WFN"));
            Assert.That(result, Does.Contain("AGSFNT1.WFN"));
            Assert.That(result, Does.Contain("AGSFNT2.WFN"));
            Assert.That(result, Does.Contain("Game.agf"));
            Assert.That(result, Does.Contain("GlobalScript.asc"));
            Assert.That(result, Does.Contain("GlobalScript.ash"));
            Assert.That(result, Does.Contain("KeyboardMovement.asc"));
            Assert.That(result, Does.Contain("KeyboardMovement.ash"));
            Assert.That(result, Does.Contain("KeyboardMovement.txt"));
            Assert.That(result, Does.Contain("room1.asc"));
            Assert.That(result, Does.Contain("room1.crm"));
            Assert.That(result, Does.Contain("Sprites/bluecup0.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults"));
            Assert.That(result, Does.Contain("Sprites/Defaults/Cursors"));
            Assert.That(result, Does.Contain("Sprites/Defaults/Cursors/cursor_interact.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults/ico_bluecup.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults/ico_key.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults/ico_x.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults/UI"));
            Assert.That(result, Does.Contain("Sprites/Defaults/UI/background_inventory.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults/Verbs"));
            Assert.That(result, Does.Contain("Sprites/Defaults/Verbs/btn_arrowdown_normal.png"));
            Assert.That(result, Does.Contain("Sprites/Defaults/Verbs/btn_arrowdown_over.png"));
            Assert.That(result, Does.Contain("template.files"));
            Assert.That(result, Does.Contain("template.ico"));
            Assert.That(result, Does.Contain("template.txt"));
            Assert.That(result, Does.Not.Contain("Game.agf.bak"));
            Assert.That(result, Does.Not.Contain("Game.agf.user"));

            Assert.That(result, Does.Not.Contain("_Debug"));
            Assert.That(result, Does.Not.Contain("_Debug/acsetup.cfg"));
            Assert.That(result, Does.Not.Contain("_Debug/SDL2.dll"));
            Assert.That(result, Does.Not.Contain("_Debug/Sierra-style.exe"));
            Assert.That(result, Does.Not.Contain("AudioCache"));
            Assert.That(result, Does.Not.Contain("Compiled"));
            Assert.That(result, Does.Not.Contain("Compiled/Data"));

            Assert.That(result.Length, Is.EqualTo(26));
        }
    }
}
