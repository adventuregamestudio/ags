namespace Scintilla.Enums
{
    using System;
    using System.Collections;
    using System.Windows.Forms;
    using System.Runtime.InteropServices;
    
    public enum WhiteSpace
    {
        Invisible = 0,
        VisibleAlways = 1,
        VisibleAfterIndent = 2,
    }
    public enum EndOfLine
    {
        Crlf = 0,
        CR = 1,
        LF = 2,
    }
    public enum MarkerSymbol
    {
        Circle = 0,
        RoundRectangle = 1,
        Arrow = 2,
        SmallRect = 3,
        ShortArrow = 4,
        Empty = 5,
        ArrowDown = 6,
        Minus = 7,
        Plus = 8,
        VLine = 9,
        LCorner = 10,
        TCorner = 11,
        BoxPlus = 12,
        BoxPlusConnected = 13,
        BoxMinus = 14,
        BoxMinusConnected = 15,
        LCornerCurve = 16,
        TCornerCurve = 17,
        CirclePlus = 18,
        CirclePlusConnected = 19,
        CircleMinus = 20,
        CircleMinusConnected = 21,
        Background = 22,
        Ellipsis = 23,
        Arrows = 24,
        PixMap = 25,
        FullRectangle = 26,
        Character = 10000,
    }
    public enum MarkerOutline
    {
        FolderEnd = 25,
        FolderOpenMid = 26,
        FolderMidTail = 27,
        FolderTail = 28,
        FolderSub = 29,
        Folder = 30,
        FolderOpen = 31,
    }
    public enum MarginType
    {
        Symbol = 0,
        Number = 1,
        Back = 2,
        Fore = 3,
    }
    public enum StylesCommon
    {
        Default = 32,
        LineNumber = 33,
        BraceLight = 34,
        BraceBad = 35,
        ControlChar = 36,
        IndentGuide = 37,
        CallTip = 38,
        LastPredefined = 39,
        Max = 127,
    }
    public enum CharacterSet
    {
        Ansi = 0,
        Default = 1,
        Baltic = 186,
        Chinesebig5 = 136,
        EastEurope = 238,
        Gb2312 = 134,
        Greek = 161,
        Hangul = 129,
        Mac = 77,
        Oem = 255,
        Russian = 204,
        Cyrillic = 1251,
        ShiftJis = 128,
        Symbol = 2,
        Turkish = 162,
        Johab = 130,
        Hebrew = 177,
        Arabic = 178,
        Vietnamese = 163,
        Thai = 222,
        CharSet885915 = 1000,
    }
    public enum CaseVisible
    {
        Mixed = 0,
        Upper = 1,
        Lower = 2,
    }
    public enum IndicatorStyle
    {
        Max = 7,
        Plain = 0,
        Squiggle = 1,
        LF = 2,
        Diagonal = 3,
        Strike = 4,
        Hidden = 5,
        Box = 6,
        RoundBox = 7,
    }
    public enum PrintOption
    {
        Normal = 0,
        InvertLight = 1,
        BlackOnWhite = 2,
        ColorOnWhite = 3,
        ColorOnWhiteDefaultBackground = 4,
    }
    public enum FindOption
    {
        WholeWord = 2,
        MatchCase = 4,
        WordStart = 0x00100000,
        RegularExpression = 0x00200000,
        Posix = 0x00400000,
    }
    public enum FoldLevel
    {
        Base = 0x400,
        WhiteFlag = 0x1000,
        HeaderFlag = 0x2000,
        BoxHeaderFlag = 0x4000,
        BoxFooterFlag = 0x8000,
        Contracted = 0x10000,
        Unindent = 0x20000,
        NumberMask = 0x0FFF,
    }
    public enum FoldFlag
    {
        LinebeforeExpanded = 0x0002,
        LinebeforeContracted = 0x0004,
        LineafterExpanded = 0x0008,
        LineafterContracted = 0x0010,
        LevelNumbers = 0x0040,
        Box = 0x0001,
    }
    public enum Wrap
    {
        None = 0,
        Word = 1,
        Char = 2,
    }
    public enum WrapVisualFlag
    {
        None = 0x0000,
        End = 0x0001,
        Start = 0x0002,
    }
    public enum WrapVisualLocation
    {
        Default = 0x0000,
        EndByText = 0x0001,
        StartByText = 0x0002,
    }
    public enum LineCache
    {
        None = 0,
        Caret = 1,
        Page = 2,
        Document = 3,
    }
    public enum EdgeVisualStyle
    {
        None = 0,
        Line = 1,
        Background = 2,
    }
    public enum CursorShape
    {
        Normal = -1,
        Wait = 4,
    }
    public enum CaretPolicy
    {
        Slop = 0x01,
        Strict = 0x04,
        Jumps = 0x10,
        Even = 0x08,
    }
    public enum SelectionMode
    {
        Stream = 0,
        Rectangle = 1,
        Lines = 2,
    }
    public enum ModificationFlags
    {
        InsertText = 0x1,
        DeleteText = 0x2,
        ChangeStyle = 0x4,
        ChangeFold = 0x8,
        User = 0x10,
        Undo = 0x20,
        Redo = 0x40,
        StepInUndoRedo = 0x100,
        ChangeMarker = 0x200,
        BeforeInsert = 0x400,
        BeforeDelete = 0x800,
    }
    public enum Keys
    {
        Down = 300,
        Up = 301,
        Left = 302,
        Right = 303,
        Home = 304,
        End = 305,
        Prior = 306,
        Next = 307,
        Delete = 308,
        Insert = 309,
        Escape = 7,
        Back = 8,
        Tab = 9,
        Return = 13,
        Add = 310,
        Subtract = 311,
        Divide = 312,
    }
    public enum KeyMod
    {
        Norm = 0,
        Shift = 1,
        Ctrl = 2,
        Alt = 4,
    }
    public enum Lexer
    {
        Container = 0,
        Null = 1,
        Python = 2,
        Cpp = 3,
        Html = 4,
        Xml = 5,
        Perl = 6,
        Sql = 7,
        VB = 8,
        Properties = 9,
        ErrorList = 10,
        MakeFile = 11,
        Batch = 12,
        XCode = 13,
        Latex = 14,
        Lua = 15,
        Diff = 16,
        Conf = 17,
        Pascal = 18,
        Ave = 19,
        Ada = 20,
        Lisp = 21,
        Ruby = 22,
        Eiffel = 23,
        EiffelKW = 24,
        Tcl = 25,
        NNCronTab = 26,
        Bullant = 27,
        VBScript = 28,
        Baan = 31,
        Matlab = 32,
        Scriptol = 33,
        Asm = 34,
        CppNoCase = 35,
        Fortran = 36,
        F77 = 37,
        Css = 38,
        Pov = 39,
        Lout = 40,
        EScript = 41,
        PS = 42,
        Nsis = 43,
        Mmixal = 44,
        Clw = 45,
        ClwNoCase = 46,
        Lot = 47,
        Yaml = 48,
        Tex = 49,
        MetaPost = 50,
        PowerBasic = 51,
        Forth = 52,
        Erlang = 53,
        Octave = 54,
        MSSql = 55,
        Verilog = 56,
        Kix = 57,
        Gui4cli = 58,
        SpecMan = 59,
        Au3 = 60,
        Apdl = 61,
        Bash = 62,
        Asn1 = 63,
        Vhdl = 64,
        Caml = 65,
        BlitzBasic = 66,
        PureBasic = 67,
        Haskell = 68,
        PhpScript = 69,
        Tads3 = 70,
        Rebol = 71,
        Smalltalk = 72,
        Flagship = 73,
        CSound = 74,
        FreeBasic = 75,
        InnoSetup = 76,
        Opal = 77,
        Spice = 78,
        Automatic = 1000,
    }
    public enum Events : uint
    {
        StyleNeeded = 2000,
        CharAdded = 2001,
        SavePointReached = 2002,
        SavePointLeft = 2003,
        ModifyAttemptRO = 2004,
        Key = 2005,
        DoubleClick = 2006,
        UpdateUI = 2007,
        Modified = 2008,
        MacroRecord = 2009,
        MarginClick = 2010,
        NeedShown = 2011,
        Painted = 2013,
        UserListSelection = 2014,
        UriDropped = 2015,
        DwellStart = 2016,
        DwellEnd = 2017,
        Zoom = 2018,
        HotspotClick = 2019,
        HotSpotDoubleClick = 2020,
        CallTipClick = 2021,
        AutoCSelection = 2022,
    }
}
