namespace Scintilla
{
    using System;
    using System.Collections;
    using System.Windows.Forms;
    using System.ComponentModel;
    
    // Function void AddStyledText(int,cells) skipped.
    // Function int GetStyledText(,textrange) skipped.
    // Function position FindText(int,findtext) skipped.
    // Function position FormatRange(bool,formatrange) skipped.
    // Function int GetTextRange(,textrange) skipped.
    // Getter int CharAt(position,) skipped.
    // Getter int StyleAt(position,) skipped.
    // Getter int MarginTypeN(int,) skipped.
    // Getter int MarginWidthN(int,) skipped.
    // Getter int MarginMaskN(int,) skipped.
    // Getter bool MarginSensitiveN(int,) skipped.
    // Getter int IndicGetStyle(int,) skipped.
    // Getter colour IndicGetFore(int,) skipped.
    // Getter int LineState(int,) skipped.
    // Getter int LineIndentation(int,) skipped.
    // Getter position LineIndentPosition(int,) skipped.
    // Getter int Column(position,) skipped.
    // Getter int LineEndPosition(int,) skipped.
    // Getter int FoldLevel(int,) skipped.
    // Getter int LastChild(int,int) skipped.
    // Getter int FoldParent(int,) skipped.
    // Getter bool LineVisible(int,) skipped.
    // Getter bool FoldExpanded(int,) skipped.
    // Getter int PropertyInt(string,) skipped.
    // Setter void MarginTypeN(int,int) skipped.
    // Setter void MarginWidthN(int,int) skipped.
    // Setter void MarginMaskN(int,int) skipped.
    // Setter void MarginSensitiveN(int,bool) skipped.
    // Setter void StyleClearAll(,) skipped.
    // Setter void StyleSetFore(int,colour) skipped.
    // Setter void StyleSetBack(int,colour) skipped.
    // Setter void StyleSetBold(int,bool) skipped.
    // Setter void StyleSetItalic(int,bool) skipped.
    // Setter void StyleSetSize(int,int) skipped.
    // Setter void StyleSetFont(int,string) skipped.
    // Setter void StyleSetEOLFilled(int,bool) skipped.
    // Setter void StyleSetUnderline(int,bool) skipped.
    // Setter void StyleSetCase(int,int) skipped.
    // Setter void StyleSetCharacterSet(int,int) skipped.
    // Setter void StyleSetHotSpot(int,bool) skipped.
    // Setter void StyleSetVisible(int,bool) skipped.
    // Setter void WordChars(,string) skipped.
    // Setter void IndicSetStyle(int,int) skipped.
    // Setter void IndicSetFore(int,colour) skipped.
    // Setter void LineState(int,int) skipped.
    // Setter void StyleSetChangeable(int,bool) skipped.
    // Setter void AutoCSetFillUps(,string) skipped.
    // Setter void LineIndentation(int,int) skipped.
    // Setter void CallTipSetBack(colour,) skipped.
    // Setter void CallTipSetFore(colour,) skipped.
    // Setter void CallTipSetForeHlt(colour,) skipped.
    // Setter void CallTipUseStyle(int,) skipped.
    // Setter void FoldLevel(int,int) skipped.
    // Setter void FoldExpanded(int,bool) skipped.
    // Setter void HotspotActiveFore(bool,colour) skipped.
    // Setter void HotspotActiveBack(bool,colour) skipped.
    // Setter void HotspotActiveUnderline(bool,) skipped.
    // Setter void HotspotSingleLine(bool,) skipped.
    // Setter void WhitespaceChars(,string) skipped.
    // Setter void Property(string,string) skipped.
    // Setter void KeyWords(int,string) skipped.
    // Setter void LexerLanguage(,string) skipped.
    // Event void StyleNeeded(int position) skipped.
    // Event void CharAdded(int ch) skipped.
    // Event void SavePointReached(void) skipped.
    // Event void SavePointLeft(void) skipped.
    // Event void ModifyAttemptRO(void) skipped.
    // Event void Key(int ch, int modifiers) skipped.
    // Event void DoubleClick(void) skipped.
    // Event void UpdateUI(void) skipped.
    // Event void Modified(int position, int modificationType, string text, int length, int linesAdded, int line, int foldLevelNow, int foldLevelPrev) skipped.
    // Event void MacroRecord(int message, int wParam, int lParam) skipped.
    // Event void MarginClick(int modifiers, int position, int margin) skipped.
    // Event void NeedShown(int position, int length) skipped.
    // Event void Painted(void) skipped.
    // Event void UserListSelection(int listType, string text) skipped.
    // Event void URIDropped(string text) skipped.
    // Event void DwellStart(int position) skipped.
    // Event void DwellEnd(int position) skipped.
    // Event void Zoom(void) skipped.
    // Event void HotSpotClick(int modifiers, int position) skipped.
    // Event void HotSpotDoubleClick(int modifiers, int position) skipped.
    // Event void CallTipClick(int position) skipped.
    // Event void AutoCSelection(string text) skipped.
    public partial class ScintillaControl
    {
        /// <summary>
        /// Are there any redoable actions in the undo history? 
        /// </summary>
        [Browsable(false)]
        public virtual bool CanRedo
        {
            get
            {
                return (this.SendMessageDirect(2016) != 0);
            }
        }
        /// <summary>
        /// Is there an auto-completion list visible? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsAutoCActive
        {
            get
            {
                return (this.SendMessageDirect(2102) != 0);
            }
        }
        /// <summary>
        /// Will a paste succeed? 
        /// </summary>
        [Browsable(false)]
        public virtual bool CanPaste
        {
            get
            {
                return (this.SendMessageDirect(2173) != 0);
            }
        }
        /// <summary>
        /// Are there any undoable actions in the undo history? 
        /// </summary>
        [Browsable(false)]
        public virtual bool CanUndo
        {
            get
            {
                return (this.SendMessageDirect(2174) != 0);
            }
        }
        /// <summary>
        /// Is there an active call tip? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsCallTipActive
        {
            get
            {
                return (this.SendMessageDirect(2202) != 0);
            }
        }
        /// <summary>
        /// Returns the number of characters in the document. 
        /// </summary>
        [Browsable(false)]
        public virtual int Length
        {
            get
            {
                return this.SendMessageDirect(2006);
            }
        }
        /// <summary>
        /// Returns the position of the caret. 
        /// </summary>
        [Browsable(false)]
        public virtual int CurrentPos
        {
            get
            {
                return this.SendMessageDirect(2008);
            }
            set
            {
                this.SendMessageDirect(2141, value);
            }
        }
        /// <summary>
        /// Returns the position of the opposite end of the selection to the caret. 
        /// </summary>
        [Browsable(false)]
        public virtual int Anchor_
        {
            get
            {
                return this.SendMessageDirect(2009);
            }
            set
            {
                this.SendMessageDirect(2026, value);
            }
        }
        /// <summary>
        /// Is undo history being collected? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsUndoCollection
        {
            get
            {
                return (this.SendMessageDirect(2019) != 0);
            }
            set
            {
                this.SendMessageDirect(2012, value);
            }
        }
        /// <summary>
        /// Are white space characters currently visible? Returns one of SCWS_* constants. 
        /// </summary>
        [Browsable(false)]
        public virtual int ViewWS
        {
            get
            {
                return this.SendMessageDirect(2020);
            }
            set
            {
                this.SendMessageDirect(2021, value);
            }
        }
        /// <summary>
        /// Retrieve the position of the last correctly styled character. 
        /// </summary>
        [Browsable(false)]
        public virtual int EndStyled
        {
            get
            {
                return this.SendMessageDirect(2028);
            }
        }
        /// <summary>
        /// Retrieve the current end of line mode - one of CRLF, CR, or LF. 
        /// </summary>
        [Browsable(false)]
        public virtual int EOLMode
        {
            get
            {
                return this.SendMessageDirect(2030);
            }
            set
            {
                this.SendMessageDirect(2031, value);
            }
        }
        /// <summary>
        /// Is drawing done first into a buffer or direct to the screen? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsBufferedDraw
        {
            get
            {
                return (this.SendMessageDirect(2034) != 0);
            }
            set
            {
                this.SendMessageDirect(2035, value);
            }
        }
        /// <summary>
        /// Retrieve the visible size of a tab. 
        /// </summary>
        [Browsable(false)]
        public virtual int TabWidth
        {
            get
            {
                return this.SendMessageDirect(2121);
            }
            set
            {
                this.SendMessageDirect(2036, value);
            }
        }
        /// <summary>
        /// Get the alpha of the selection. 
        /// </summary>
        [Browsable(false)]
        public virtual int SelAlpha
        {
            get
            {
                return this.SendMessageDirect(2477);
            }
            set
            {
                this.SendMessageDirect(2478, value);
            }
        }
        /// <summary>
        /// Get the time in milliseconds that the caret is on and off. 
        /// </summary>
        [Browsable(false)]
        public virtual int CaretPeriod
        {
            get
            {
                return this.SendMessageDirect(2075);
            }
            set
            {
                this.SendMessageDirect(2076, value);
            }
        }
        /// <summary>
        /// Retrieve number of bits in style bytes used to hold the lexical state. 
        /// </summary>
        [Browsable(false)]
        public virtual int StyleBits
        {
            get
            {
                return this.SendMessageDirect(2091);
            }
            set
            {
                this.SendMessageDirect(2090, value);
            }
        }
        /// <summary>
        /// Retrieve the last line number that has line state. 
        /// </summary>
        [Browsable(false)]
        public virtual int MaxLineState
        {
            get
            {
                return this.SendMessageDirect(2094);
            }
        }
        /// <summary>
        /// Is the background of the line containing the caret in a different colour? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsCaretLineVisible
        {
            get
            {
                return (this.SendMessageDirect(2095) != 0);
            }
            set
            {
                this.SendMessageDirect(2096, value);
            }
        }
        /// <summary>
        /// Get the colour of the background of the line containing the caret. 
        /// </summary>
        [Browsable(false)]
        public virtual int CaretLineBack
        {
            get
            {
                return this.SendMessageDirect(2097);
            }
            set
            {
                this.SendMessageDirect(2098, value);
            }
        }
        /// <summary>
        /// Retrieve the auto-completion list separator character. 
        /// </summary>
        [Browsable(false)]
        public virtual int AutoCSeparator
        {
            get
            {
                return this.SendMessageDirect(2107);
            }
            set
            {
                this.SendMessageDirect(2106, value);
            }
        }
        /// <summary>
        /// Retrieve whether auto-completion cancelled by backspacing before start. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsAutoCCancelAtStart
        {
            get
            {
                return (this.SendMessageDirect(2111) != 0);
            }
            set
            {
                this.SendMessageDirect(2110, value);
            }
        }
        /// <summary>
        /// Retrieve whether a single item auto-completion list automatically choose the item. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsAutoCChooseSingle
        {
            get
            {
                return (this.SendMessageDirect(2114) != 0);
            }
            set
            {
                this.SendMessageDirect(2113, value);
            }
        }
        /// <summary>
        /// Retrieve state of ignore case flag. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsAutoCIgnoreCase
        {
            get
            {
                return (this.SendMessageDirect(2116) != 0);
            }
            set
            {
                this.SendMessageDirect(2115, value);
            }
        }
        /// <summary>
        /// Retrieve whether or not autocompletion is hidden automatically when nothing matches. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsAutoCAutoHide
        {
            get
            {
                return (this.SendMessageDirect(2119) != 0);
            }
            set
            {
                this.SendMessageDirect(2118, value);
            }
        }
        /// <summary>
        /// Retrieve whether or not autocompletion deletes any word characters after the inserted text upon completion. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsAutoCDropRestOfWord
        {
            get
            {
                return (this.SendMessageDirect(2271) != 0);
            }
            set
            {
                this.SendMessageDirect(2270, value);
            }
        }
        /// <summary>
        /// Retrieve the auto-completion list type-separator character. 
        /// </summary>
        [Browsable(false)]
        public virtual int AutoCTypeSeparator
        {
            get
            {
                return this.SendMessageDirect(2285);
            }
            set
            {
                this.SendMessageDirect(2286, value);
            }
        }
        /// <summary>
        /// Get the maximum width, in characters, of auto-completion and user lists. 
        /// </summary>
        [Browsable(false)]
        public virtual int AutoCMaxWidth
        {
            get
            {
                return this.SendMessageDirect(2209);
            }
            set
            {
                this.SendMessageDirect(2208, value);
            }
        }
        /// <summary>
        /// Set the maximum height, in rows, of auto-completion and user lists. 
        /// </summary>
        [Browsable(false)]
        public virtual int AutoCMaxHeight
        {
            get
            {
                return this.SendMessageDirect(2211);
            }
            set
            {
                this.SendMessageDirect(2210, value);
            }
        }
        /// <summary>
        /// Retrieve indentation size. 
        /// </summary>
        [Browsable(false)]
        public virtual int Indent
        {
            get
            {
                return this.SendMessageDirect(2123);
            }
            set
            {
                this.SendMessageDirect(2122, value);
            }
        }
        /// <summary>
        /// Retrieve whether tabs will be used in indentation. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsUseTabs
        {
            get
            {
                return (this.SendMessageDirect(2125) != 0);
            }
            set
            {
                this.SendMessageDirect(2124, value);
            }
        }
        /// <summary>
        /// Is the horizontal scroll bar visible? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsHScrollBar
        {
            get
            {
                return (this.SendMessageDirect(2131) != 0);
            }
            set
            {
                this.SendMessageDirect(2130, value);
            }
        }
        /// <summary>
        /// Are the indentation guides visible? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsIndentationGuides
        {
            get
            {
                return (this.SendMessageDirect(2133) != 0);
            }
            set
            {
                this.SendMessageDirect(2132, value);
            }
        }
        /// <summary>
        /// Get the highlighted indentation guide column. 
        /// </summary>
        [Browsable(false)]
        public virtual int HighlightGuide
        {
            get
            {
                return this.SendMessageDirect(2135);
            }
            set
            {
                this.SendMessageDirect(2134, value);
            }
        }
        /// <summary>
        /// Get the code page used to interpret the bytes of the document as characters. 
        /// </summary>
        [Browsable(false)]
        public virtual int CodePage
        {
            get
            {
                return this.SendMessageDirect(2137);
            }
            set
            {
                this.SendMessageDirect(2037, value);
            }
        }
        /// <summary>
        /// Get the foreground colour of the caret. 
        /// </summary>
        [Browsable(false)]
        public virtual int CaretFore
        {
            get
            {
                return this.SendMessageDirect(2138);
            }
            set
            {
                this.SendMessageDirect(2069, value);
            }
        }
        /// <summary>
        /// In palette mode? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsUsePalette
        {
            get
            {
                return (this.SendMessageDirect(2139) != 0);
            }
            set
            {
                this.SendMessageDirect(2039, value);
            }
        }
        /// <summary>
        /// In read-only mode? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsReadOnly
        {
            get
            {
                return (this.SendMessageDirect(2140) != 0);
            }
            set
            {
                this.SendMessageDirect(2171, value);
            }
        }
        /// <summary>
        /// Returns the position at the start of the selection. 
        /// </summary>
        [Browsable(false)]
        public virtual int SelectionStart
        {
            get
            {
                return this.SendMessageDirect(2143);
            }
            set
            {
                this.SendMessageDirect(2142, value);
            }
        }
        /// <summary>
        /// Returns the position at the end of the selection. 
        /// </summary>
        [Browsable(false)]
        public virtual int SelectionEnd
        {
            get
            {
                return this.SendMessageDirect(2145);
            }
            set
            {
                this.SendMessageDirect(2144, value);
            }
        }
        /// <summary>
        /// Returns the print magnification. 
        /// </summary>
        [Browsable(false)]
        public virtual int PrintMagnification
        {
            get
            {
                return this.SendMessageDirect(2147);
            }
            set
            {
                this.SendMessageDirect(2146, value);
            }
        }
        /// <summary>
        /// Returns the print colour mode. 
        /// </summary>
        [Browsable(false)]
        public virtual int PrintColorMode
        {
            get
            {
                return this.SendMessageDirect(2149);
            }
            set
            {
                this.SendMessageDirect(2148, value);
            }
        }
        /// <summary>
        /// Retrieve the display line at the top of the display. 
        /// </summary>
        [Browsable(false)]
        public virtual int FirstVisibleLine
        {
            get
            {
                return this.SendMessageDirect(2152);
            }
        }
        /// <summary>
        /// Returns the number of lines in the document. There is always at least one. 
        /// </summary>
        [Browsable(false)]
        public virtual int LineCount
        {
            get
            {
                return this.SendMessageDirect(2154);
            }
        }
        /// <summary>
        /// Returns the size in pixels of the left margin. 
        /// </summary>
        [Browsable(false)]
        public virtual int MarginLeft
        {
            get
            {
                return this.SendMessageDirect(2156);
            }
            set
            {
                this.SendMessageDirect(2155, value);
            }
        }
        /// <summary>
        /// Returns the size in pixels of the right margin. 
        /// </summary>
        [Browsable(false)]
        public virtual int MarginRight
        {
            get
            {
                return this.SendMessageDirect(2158);
            }
            set
            {
                this.SendMessageDirect(2157, value);
            }
        }
        /// <summary>
        /// Is the document different from when it was last saved? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsModify
        {
            get
            {
                return (this.SendMessageDirect(2159) != 0);
            }
        }
        /// <summary>
        /// Retrieve the number of characters in the document. 
        /// </summary>
        [Browsable(false)]
        public virtual int TextLength
        {
            get
            {
                return this.SendMessageDirect(2183);
            }
        }
        /// <summary>
        /// Retrieve a pointer to a function that processes messages for this Scintilla. 
        /// </summary>
        [Browsable(false)]
        public virtual int DirectFunction
        {
            get
            {
                return this.SendMessageDirect(2184);
            }
        }
        /// <summary>
        /// Retrieve a pointer value to use as the first argument when calling the function returned by GetDirectFunction. 
        /// </summary>
        [Browsable(false)]
        public virtual int DirectPointer
        {
            get
            {
                return this.SendMessageDirect(2185);
            }
        }
        /// <summary>
        /// Returns true if overtype mode is active otherwise false is returned. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsOvertype
        {
            get
            {
                return (this.SendMessageDirect(2187) != 0);
            }
            set
            {
                this.SendMessageDirect(2186, value);
            }
        }
        /// <summary>
        /// Returns the width of the insert mode caret. 
        /// </summary>
        [Browsable(false)]
        public virtual int CaretWidth
        {
            get
            {
                return this.SendMessageDirect(2189);
            }
            set
            {
                this.SendMessageDirect(2188, value);
            }
        }
        /// <summary>
        /// Get the position that starts the target. 
        /// </summary>
        [Browsable(false)]
        public virtual int TargetStart
        {
            get
            {
                return this.SendMessageDirect(2191);
            }
            set
            {
                this.SendMessageDirect(2190, value);
            }
        }
        /// <summary>
        /// Get the position that ends the target. 
        /// </summary>
        [Browsable(false)]
        public virtual int TargetEnd
        {
            get
            {
                return this.SendMessageDirect(2193);
            }
            set
            {
                this.SendMessageDirect(2192, value);
            }
        }
        /// <summary>
        /// Get the search flags used by SearchInTarget. 
        /// </summary>
        [Browsable(false)]
        public virtual int SearchFlags
        {
            get
            {
                return this.SendMessageDirect(2199);
            }
            set
            {
                this.SendMessageDirect(2198, value);
            }
        }
        /// <summary>
        /// Does a tab pressed when caret is within indentation indent? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsTabIndents
        {
            get
            {
                return (this.SendMessageDirect(2261) != 0);
            }
            set
            {
                this.SendMessageDirect(2260, value);
            }
        }
        /// <summary>
        /// Does a backspace pressed when caret is within indentation unindent? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsBackSpaceUnIndents
        {
            get
            {
                return (this.SendMessageDirect(2263) != 0);
            }
            set
            {
                this.SendMessageDirect(2262, value);
            }
        }
        /// <summary>
        /// Retrieve the time the mouse must sit still to generate a mouse dwell event. 
        /// </summary>
        [Browsable(false)]
        public virtual int MouseDwellTime
        {
            get
            {
                return this.SendMessageDirect(2265);
            }
            set
            {
                this.SendMessageDirect(2264, value);
            }
        }
        /// <summary>
        /// Retrieve whether text is word wrapped. 
        /// </summary>
        [Browsable(false)]
        public virtual int WrapMode
        {
            get
            {
                return this.SendMessageDirect(2269);
            }
            set
            {
                this.SendMessageDirect(2268, value);
            }
        }
        /// <summary>
        /// Retrive the display mode of visual flags for wrapped lines. 
        /// </summary>
        [Browsable(false)]
        public virtual int WrapVisualFlags
        {
            get
            {
                return this.SendMessageDirect(2461);
            }
            set
            {
                this.SendMessageDirect(2460, value);
            }
        }
        /// <summary>
        /// Retrive the location of visual flags for wrapped lines. 
        /// </summary>
        [Browsable(false)]
        public virtual int WrapVisualFlagsLocation
        {
            get
            {
                return this.SendMessageDirect(2463);
            }
            set
            {
                this.SendMessageDirect(2462, value);
            }
        }
        /// <summary>
        /// Retrive the start indent for wrapped lines. 
        /// </summary>
        [Browsable(false)]
        public virtual int WrapStartIndent
        {
            get
            {
                return this.SendMessageDirect(2465);
            }
            set
            {
                this.SendMessageDirect(2464, value);
            }
        }
        /// <summary>
        /// Retrieve the degree of caching of layout information. 
        /// </summary>
        [Browsable(false)]
        public virtual int LayoutCache
        {
            get
            {
                return this.SendMessageDirect(2273);
            }
            set
            {
                this.SendMessageDirect(2272, value);
            }
        }
        /// <summary>
        /// Retrieve the document width assumed for scrolling. 
        /// </summary>
        [Browsable(false)]
        public virtual int ScrollWidth
        {
            get
            {
                return this.SendMessageDirect(2275);
            }
            set
            {
                this.SendMessageDirect(2274, value);
            }
        }
        /// <summary>
        /// Retrieve whether the maximum scroll position has the last line at the bottom of the view. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsEndAtLastLine
        {
            get
            {
                return (this.SendMessageDirect(2278) != 0);
            }
            set
            {
                this.SendMessageDirect(2277, value);
            }
        }
        /// <summary>
        /// Is the vertical scroll bar visible? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsVScrollBar
        {
            get
            {
                return (this.SendMessageDirect(2281) != 0);
            }
            set
            {
                this.SendMessageDirect(2280, value);
            }
        }
        /// <summary>
        /// Is drawing done in two phases with backgrounds drawn before faoregrounds? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsTwoPhaseDraw
        {
            get
            {
                return (this.SendMessageDirect(2283) != 0);
            }
            set
            {
                this.SendMessageDirect(2284, value);
            }
        }
        /// <summary>
        /// Are the end of line characters visible? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsViewEOL
        {
            get
            {
                return (this.SendMessageDirect(2355) != 0);
            }
            set
            {
                this.SendMessageDirect(2356, value);
            }
        }
        /// <summary>
        /// Retrieve a pointer to the document object. 
        /// </summary>
        [Browsable(false)]
        public virtual int DocPointer
        {
            get
            {
                return this.SendMessageDirect(2357);
            }
            set
            {
                this.SendMessageDirect(2358, value);
            }
        }
        /// <summary>
        /// Retrieve the column number which text should be kept within. 
        /// </summary>
        [Browsable(false)]
        public virtual int EdgeColumn
        {
            get
            {
                return this.SendMessageDirect(2360);
            }
            set
            {
                this.SendMessageDirect(2361, value);
            }
        }
        /// <summary>
        /// Retrieve the edge highlight mode. 
        /// </summary>
        [Browsable(false)]
        public virtual int EdgeMode
        {
            get
            {
                return this.SendMessageDirect(2362);
            }
            set
            {
                this.SendMessageDirect(2363, value);
            }
        }
        /// <summary>
        /// Retrieve the colour used in edge indication. 
        /// </summary>
        [Browsable(false)]
        public virtual int EdgeColor
        {
            get
            {
                return this.SendMessageDirect(2364);
            }
            set
            {
                this.SendMessageDirect(2365, value);
            }
        }
        /// <summary>
        /// Retrieves the number of lines completely visible. 
        /// </summary>
        [Browsable(false)]
        public virtual int LinesOnScreen
        {
            get
            {
                return this.SendMessageDirect(2370);
            }
        }
        /// <summary>
        /// Is the selection rectangular? The alternative is the more common stream selection. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsSelectionIsRectangle
        {
            get
            {
                return (this.SendMessageDirect(2372) != 0);
            }
        }
        /// <summary>
        /// Set the zoom level. This number of points is added to the size of all fonts. It may be positive to magnify or negative to reduce. Retrieve the zoom level. 
        /// </summary>
        [Browsable(false)]
        public virtual int Zoom
        {
            get
            {
                return this.SendMessageDirect(2374);
            }
            set
            {
                this.SendMessageDirect(2373, value);
            }
        }
        /// <summary>
        /// Get which document modification events are sent to the container. 
        /// </summary>
        [Browsable(false)]
        public virtual int ModEventMask
        {
            get
            {
                return this.SendMessageDirect(2378);
            }
            set
            {
                this.SendMessageDirect(2359, value);
            }
        }
        /// <summary>
        /// Change internal focus flag. Get internal focus flag. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsFocus
        {
            get
            {
                return (this.SendMessageDirect(2381) != 0);
            }
            set
            {
                this.SendMessageDirect(2380, value);
            }
        }
        /// <summary>
        /// Change error status - 0 = OK. Get error status. 
        /// </summary>
        [Browsable(false)]
        public virtual int Status
        {
            get
            {
                return this.SendMessageDirect(2383);
            }
            set
            {
                this.SendMessageDirect(2382, value);
            }
        }
        /// <summary>
        /// Set whether the mouse is captured when its button is pressed. Get whether mouse gets captured. 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsMouseDownCaptures
        {
            get
            {
                return (this.SendMessageDirect(2385) != 0);
            }
            set
            {
                this.SendMessageDirect(2384, value);
            }
        }
        /// <summary>
        /// Sets the cursor to one of the SC_CURSOR* values. Get cursor type. 
        /// </summary>
        [Browsable(false)]
        public virtual int Cursor_
        {
            get
            {
                return this.SendMessageDirect(2387);
            }
            set
            {
                this.SendMessageDirect(2386, value);
            }
        }
        /// <summary>
        /// Change the way control characters are displayed: If symbol is < 32, keep the drawn way, else, use the given character. Get the way control characters are displayed. 
        /// </summary>
        [Browsable(false)]
        public virtual int ControlCharSymbol
        {
            get
            {
                return this.SendMessageDirect(2389);
            }
            set
            {
                this.SendMessageDirect(2388, value);
            }
        }
        /// <summary>
        /// Get and Set the xOffset (ie, horizonal scroll position). 
        /// </summary>
        [Browsable(false)]
        public virtual int XOffset
        {
            get
            {
                return this.SendMessageDirect(2398);
            }
            set
            {
                this.SendMessageDirect(2397, value);
            }
        }
        /// <summary>
        /// Is printing line wrapped? 
        /// </summary>
        [Browsable(false)]
        public virtual int PrintWrapMode
        {
            get
            {
                return this.SendMessageDirect(2407);
            }
            set
            {
                this.SendMessageDirect(2406, value);
            }
        }
        /// <summary>
        /// Get the mode of the current selection. 
        /// </summary>
        [Browsable(false)]
        public virtual int SelectionMode
        {
            get
            {
                return this.SendMessageDirect(2423);
            }
            set
            {
                this.SendMessageDirect(2422, value);
            }
        }
        /// <summary>
        /// Can the caret preferred x position only be changed by explicit movement commands? 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsCaretSticky
        {
            get
            {
                return (this.SendMessageDirect(2457) != 0);
            }
            set
            {
                this.SendMessageDirect(2458, value);
            }
        }
        /// <summary>
        /// Get convert-on-paste setting 
        /// </summary>
        [Browsable(false)]
        public virtual bool IsPasteConvertEndings
        {
            get
            {
                return (this.SendMessageDirect(2468) != 0);
            }
            set
            {
                this.SendMessageDirect(2467, value);
            }
        }
        /// <summary>
        /// Get the background alpha of the caret line. 
        /// </summary>
        [Browsable(false)]
        public virtual int CaretLineBackAlpha
        {
            get
            {
                return this.SendMessageDirect(2471);
            }
            set
            {
                this.SendMessageDirect(2470, value);
            }
        }
        /// <summary>
        /// Retrieve the lexing language of the document. 
        /// </summary>
        [Browsable(false)]
        public virtual int Lexer
        {
            get
            {
                return this.SendMessageDirect(4002);
            }
            set
            {
                this.SendMessageDirect(4001, value);
            }
        }
        /// <summary>
        /// Retrieve the number of bits the current lexer needs for styling. 
        /// </summary>
        [Browsable(false)]
        public virtual int StyleBitsNeeded
        {
            get
            {
                return this.SendMessageDirect(4011);
            }
        }
        /// <summary>
        /// Add text to the document at current position. 
        /// </summary>
        public virtual void AddText(string text)
        {
            this.SendMessageDirect(2001, text.Length, text);
        }
        /// <summary>
        /// Insert string at a position. 
        /// </summary>
        public virtual void InsertText(int pos, string text)
        {
            this.SendMessageDirect(2003, pos, text);
        }
        /// <summary>
        /// Delete all text in the document. 
        /// </summary>
        public virtual void ClearAll()
        {
            this.SendMessageDirect(2004);
        }
        /// <summary>
        /// Set all style bytes to 0, remove all folding information. 
        /// </summary>
        public virtual void ClearDocumentStyle()
        {
            this.SendMessageDirect(2005);
        }
        /// <summary>
        /// Redoes the next action on the undo history. 
        /// </summary>
        public virtual void Redo()
        {
            this.SendMessageDirect(2011);
        }
        /// <summary>
        /// Select all the text in the document. 
        /// </summary>
        public virtual void SelectAll()
        {
            this.SendMessageDirect(2013);
        }
        /// <summary>
        /// Remember the current position in the undo history as the position at which the document was saved. 
        /// </summary>
        public virtual void SetSavePoint()
        {
            this.SendMessageDirect(2014);
        }
        /// <summary>
        /// Retrieve the line number at which a particular marker is located. 
        /// </summary>
        public virtual int MarkerLineFromHandle(int handle)
        {
            return this.SendMessageDirect(2017, handle);
        }
        /// <summary>
        /// Delete a marker. 
        /// </summary>
        public virtual void MarkerDeleteHandle(int handle)
        {
            this.SendMessageDirect(2018, handle);
        }
        /// <summary>
        /// Find the position from a point within the window. 
        /// </summary>
        public virtual int PositionFromPoint(int x, int y)
        {
            return this.SendMessageDirect(2022, x, y);
        }
        /// <summary>
        /// Find the position from a point within the window but return INVALID_POSITION if not close to text. 
        /// </summary>
        public virtual int PositionFromPointClose(int x, int y)
        {
            return this.SendMessageDirect(2023, x, y);
        }
        /// <summary>
        /// Set caret to start of a line and ensure it is visible. 
        /// </summary>
        public virtual void GotoLine(int line)
        {
            this.SendMessageDirect(2024, line);
        }
        /// <summary>
        /// Set caret to a position and ensure it is visible. 
        /// </summary>
        public virtual void GotoPos(int pos)
        {
            this.SendMessageDirect(2025, pos);
        }
        /// <summary>
        /// Retrieve the text of the line containing the caret. Returns the index of the caret on the line. 
        /// </summary>
        public virtual string GetCurLine()
        {
            string result;
            this.SendMessageDirect(2027, out result);
            return result;
        }
        /// <summary>
        /// Set the current styling position to pos and the styling mask to mask. The styling mask can be used to protect some bits in each styling byte from modification. 
        /// </summary>
        public virtual void StartStyling(int pos, int mask)
        {
            this.SendMessageDirect(2032, pos, mask);
        }
        /// <summary>
        /// Change style from current styling position for length characters to a style and move the current styling position to after this newly styled segment. 
        /// </summary>
        public virtual void SetStyling(int length, int style)
        {
            this.SendMessageDirect(2033, length, style);
        }
        /// <summary>
        /// Set the symbol used for a particular marker number. 
        /// </summary>
        public virtual void MarkerDefine(int markerNumber, int markerSymbol)
        {
            this.SendMessageDirect(2040, markerNumber, markerSymbol);
        }
        /// <summary>
        /// Set the foreground colour used for a particular marker number. 
        /// </summary>
        public virtual void MarkerSetFore(int markerNumber, System.Drawing.Color fore)
        {
            this.SendMessageDirect(2041, markerNumber, Utilities.ColorToRgb(fore));
        }
        /// <summary>
        /// Set the background colour used for a particular marker number. 
        /// </summary>
        public virtual void MarkerSetBack(int markerNumber, System.Drawing.Color back)
        {
            this.SendMessageDirect(2042, markerNumber, Utilities.ColorToRgb(back));
        }
        /// <summary>
        /// Add a marker to a line, returning an ID which can be used to find or delete the marker. 
        /// </summary>
        public virtual int MarkerAdd(int line, int markerNumber)
        {
            return this.SendMessageDirect(2043, line, markerNumber);
        }
        /// <summary>
        /// Delete a marker from a line. 
        /// </summary>
        public virtual void MarkerDelete(int line, int markerNumber)
        {
            this.SendMessageDirect(2044, line, markerNumber);
        }
        /// <summary>
        /// Delete all markers with a particular number from all lines. 
        /// </summary>
        public virtual void MarkerDeleteAll(int markerNumber)
        {
            this.SendMessageDirect(2045, markerNumber);
        }
        /// <summary>
        /// Get a bit mask of all the markers set on a line. 
        /// </summary>
        public virtual int MarkerGet(int line)
        {
            return this.SendMessageDirect(2046, line);
        }
        /// <summary>
        /// Find the next line after lineStart that includes a marker in mask. 
        /// </summary>
        public virtual int MarkerNext(int lineStart, int markerMask)
        {
            return this.SendMessageDirect(2047, lineStart, markerMask);
        }
        /// <summary>
        /// Find the previous line before lineStart that includes a marker in mask. 
        /// </summary>
        public virtual int MarkerPrevious(int lineStart, int markerMask)
        {
            return this.SendMessageDirect(2048, lineStart, markerMask);
        }
        /// <summary>
        /// Define a marker from a pixmap. 
        /// </summary>
        public virtual void MarkerDefinePixmap(int markerNumber, string pixmap)
        {
            this.SendMessageDirect(2049, markerNumber, pixmap);
        }
        /// <summary>
        /// Add a set of markers to a line. 
        /// </summary>
        public virtual void MarkerAddSet(int line, int set)
        {
            this.SendMessageDirect(2466, line, set);
        }
        /// <summary>
        /// Set the alpha used for a marker that is drawn in the text area, not the margin. 
        /// </summary>
        public virtual void MarkerSetAlpha(int markerNumber, int alpha)
        {
            this.SendMessageDirect(2476, markerNumber, alpha);
        }
        /// <summary>
        /// Reset the default style to its state at startup 
        /// </summary>
        public virtual void StyleResetDefault()
        {
            this.SendMessageDirect(2058);
        }
        /// <summary>
        /// When key+modifier combination km is pressed perform msg. 
        /// </summary>
        public virtual void AssignCmdKey(int km, int msg)
        {
            this.SendMessageDirect(2070, km, msg);
        }
        /// <summary>
        /// When key+modifier combination km is pressed do nothing. 
        /// </summary>
        public virtual void ClearCmdKey(int km)
        {
            this.SendMessageDirect(2071, km);
        }
        /// <summary>
        /// Drop all key mappings. 
        /// </summary>
        public virtual void ClearAllCmdKeys()
        {
            this.SendMessageDirect(2072);
        }
        /// <summary>
        /// Set the styles for a segment of the document. 
        /// </summary>
        public virtual void SetStylingEx(string styles)
        {
            this.SendMessageDirect(2073, styles.Length, styles);
        }
        /// <summary>
        /// Start a sequence of actions that is undone and redone as a unit. May be nested. 
        /// </summary>
        public virtual void BeginUndoAction()
        {
            this.SendMessageDirect(2078);
        }
        /// <summary>
        /// End a sequence of actions that is undone and redone as a unit. 
        /// </summary>
        public virtual void EndUndoAction()
        {
            this.SendMessageDirect(2079);
        }
        /// <summary>
        /// Set the foreground colour of all whitespace and whether to use this setting. 
        /// </summary>
        public virtual void SetWhitespaceFore(bool useSetting, int fore)
        {
            this.SendMessageDirect(2084, useSetting, fore);
        }
        /// <summary>
        /// Set the background colour of all whitespace and whether to use this setting. 
        /// </summary>
        public virtual void SetWhitespaceBack(bool useSetting, int back)
        {
            this.SendMessageDirect(2085, useSetting, back);
        }
        /// <summary>
        /// Display a auto-completion list. The lenEntered parameter indicates how many characters before the caret should be used to provide context. 
        /// </summary>
        public virtual void AutoCShow(int lenEntered, string itemList)
        {
            this.SendMessageDirect(2100, lenEntered, itemList);
        }
        /// <summary>
        /// Remove the auto-completion list from the screen. 
        /// </summary>
        public virtual void AutoCCancel()
        {
            this.SendMessageDirect(2101);
        }
        /// <summary>
        /// Retrieve the position of the caret when the auto-completion list was displayed. 
        /// </summary>
        public virtual int AutoCPosStart()
        {
            return this.SendMessageDirect(2103);
        }
        /// <summary>
        /// User has selected an item so remove the list and insert the selection. 
        /// </summary>
        public virtual void AutoCComplete()
        {
            this.SendMessageDirect(2104);
        }
        /// <summary>
        /// Define a set of character that when typed cancel the auto-completion list. 
        /// </summary>
        public virtual void AutoCStops(string characterSet)
        {
            this.SendMessageDirect(2105, VOID.NULL, characterSet);
        }
        /// <summary>
        /// Select the item in the auto-completion list that starts with a string. 
        /// </summary>
        public virtual void AutoCSelect(string text)
        {
            this.SendMessageDirect(2108, VOID.NULL, text);
        }
        /// <summary>
        /// Display a list of strings and send notification when user chooses one. 
        /// </summary>
        public virtual void UserListShow(int listType, string itemList)
        {
            this.SendMessageDirect(2117, listType, itemList);
        }
        /// <summary>
        /// Register an XPM image for use in autocompletion lists. 
        /// </summary>
        public virtual void RegisterImage(int type, string xpmData)
        {
            this.SendMessageDirect(2405, type, xpmData);
        }
        /// <summary>
        /// Clear all the registered XPM images. 
        /// </summary>
        public virtual void ClearRegisteredImages()
        {
            this.SendMessageDirect(2408);
        }
        /// <summary>
        /// Select a range of text. 
        /// </summary>
        public virtual void SetSel(int start, int end)
        {
            this.SendMessageDirect(2160, start, end);
        }
        /// <summary>
        /// Retrieve the selected text. Return the length of the text. 
        /// </summary>
        public virtual string GetSelText()
        {
            string result;
            this.SendMessageDirect(2161, out result);
            return result;
        }
        /// <summary>
        /// Draw the selection in normal style or with selection highlighted. 
        /// </summary>
        public virtual void HideSelection(bool normal)
        {
            this.SendMessageDirect(2163, normal);
        }
        /// <summary>
        /// Retrieve the x value of the point in the window where a position is displayed. 
        /// </summary>
        public virtual int PointXFromPosition(int pos)
        {
            return this.SendMessageDirect(2164, VOID.NULL, pos);
        }
        /// <summary>
        /// Retrieve the y value of the point in the window where a position is displayed. 
        /// </summary>
        public virtual int PointYFromPosition(int pos)
        {
            return this.SendMessageDirect(2165, VOID.NULL, pos);
        }
        /// <summary>
        /// Retrieve the line containing a position. 
        /// </summary>
        public virtual int LineFromPosition(int pos)
        {
            return this.SendMessageDirect(2166, pos);
        }
        /// <summary>
        /// Retrieve the position at the start of a line. 
        /// </summary>
        public virtual int PositionFromLine(int line)
        {
            return this.SendMessageDirect(2167, line);
        }
        /// <summary>
        /// Scroll horizontally and vertically. 
        /// </summary>
        public virtual void LineScroll(int columns, int lines)
        {
            this.SendMessageDirect(2168, columns, lines);
        }
        /// <summary>
        /// Ensure the caret is visible. 
        /// </summary>
        public virtual void ScrollCaret()
        {
            this.SendMessageDirect(2169);
        }
        /// <summary>
        /// Replace the selected text with the argument text. 
        /// </summary>
        public virtual void ReplaceSel(string text)
        {
            this.SendMessageDirect(2170, VOID.NULL, text);
        }
        /// <summary>
        /// Null operation. 
        /// </summary>
        public virtual void Null()
        {
            this.SendMessageDirect(2172);
        }
        /// <summary>
        /// Delete the undo history. 
        /// </summary>
        public virtual void EmptyUndoBuffer()
        {
            this.SendMessageDirect(2175);
        }
        /// <summary>
        /// Undo one action in the undo history. 
        /// </summary>
        public virtual void Undo()
        {
            this.SendMessageDirect(2176);
        }
        /// <summary>
        /// Cut the selection to the clipboard. 
        /// </summary>
        public virtual void Cut()
        {
            this.SendMessageDirect(2177);
        }
        /// <summary>
        /// Copy the selection to the clipboard. 
        /// </summary>
        public virtual void Copy()
        {
            this.SendMessageDirect(2178);
        }
        /// <summary>
        /// Paste the contents of the clipboard into the document replacing the selection. 
        /// </summary>
        public virtual void Paste()
        {
            this.SendMessageDirect(2179);
        }
        /// <summary>
        /// Clear the selection. 
        /// </summary>
        public virtual void Clear()
        {
            this.SendMessageDirect(2180);
        }
        /// <summary>
        /// Replace the contents of the document with the argument text. 
        /// </summary>
        public virtual void SetText(string text)
        {
            this.SendMessageDirect(2181, VOID.NULL, text);
        }
        /// <summary>
        /// Retrieve all the text in the document. Returns number of characters retrieved. 
        /// </summary>
        public virtual string GetText()
        {
            string result;
            int length = SendMessageDirect(2182, 0, 0);
            this.SendMessageDirect(2182, length, out result);
            return result;
        }
        /// <summary>
        /// Replace the target text with the argument text. Text is counted so it can contain NULs. Returns the length of the replacement text. 
        /// </summary>
        public virtual int ReplaceTarget(string text)
        {
            return this.SendMessageDirect(2194, text.Length, text);
        }
        /// <summary>
        /// Replace the target text with the argument text after \d processing. Text is counted so it can contain NULs. Looks for \d where d is between 1 and 9 and replaces these with the strings matched in the last search operation which were surrounded by \( and \). Returns the length of the replacement text including any change caused by processing the \d patterns. 
        /// </summary>
        public virtual int ReplaceTargetRE(string text)
        {
            return this.SendMessageDirect(2195, text.Length, text);
        }
        /// <summary>
        /// Search for a counted string in the target and set the target to the found range. Text is counted so it can contain NULs. Returns length of range or -1 for failure in which case target is not moved. 
        /// </summary>
        public virtual int SearchInTarget(string text)
        {
            return this.SendMessageDirect(2197, text.Length, text);
        }
        /// <summary>
        /// Show a call tip containing a definition near position pos. 
        /// </summary>
        public virtual void CallTipShow(int pos, string definition)
        {
            this.SendMessageDirect(2200, pos, definition);
        }
        /// <summary>
        /// Remove the call tip from the screen. 
        /// </summary>
        public virtual void CallTipCancel()
        {
            this.SendMessageDirect(2201);
        }
        /// <summary>
        /// Retrieve the position where the caret was before displaying the call tip. 
        /// </summary>
        public virtual int CallTipPosStart()
        {
            return this.SendMessageDirect(2203);
        }
        /// <summary>
        /// Highlight a segment of the definition. 
        /// </summary>
        public virtual void CallTipSetHlt(int start, int end)
        {
            this.SendMessageDirect(2204, start, end);
        }
        /// <summary>
        /// Find the display line of a document line taking hidden lines into account. 
        /// </summary>
        public virtual int VisibleFromDocLine(int line)
        {
            return this.SendMessageDirect(2220, line);
        }
        /// <summary>
        /// Find the document line of a display line taking hidden lines into account. 
        /// </summary>
        public virtual int DocLineFromVisible(int lineDisplay)
        {
            return this.SendMessageDirect(2221, lineDisplay);
        }
        /// <summary>
        /// The number of display lines needed to wrap a document line 
        /// </summary>
        public virtual int WrapCount(int line)
        {
            return this.SendMessageDirect(2235, line);
        }
        /// <summary>
        /// Make a range of lines visible. 
        /// </summary>
        public virtual void ShowLines(int lineStart, int lineEnd)
        {
            this.SendMessageDirect(2226, lineStart, lineEnd);
        }
        /// <summary>
        /// Make a range of lines invisible. 
        /// </summary>
        public virtual void HideLines(int lineStart, int lineEnd)
        {
            this.SendMessageDirect(2227, lineStart, lineEnd);
        }
        /// <summary>
        /// Switch a header line between expanded and contracted. 
        /// </summary>
        public virtual void ToggleFold(int line)
        {
            this.SendMessageDirect(2231, line);
        }
        /// <summary>
        /// Ensure a particular line is visible by expanding any header line hiding it. 
        /// </summary>
        public virtual void EnsureVisible(int line)
        {
            this.SendMessageDirect(2232, line);
        }
        /// <summary>
        /// Set some style options for folding. 
        /// </summary>
        public virtual void SetFoldFlags(int flags)
        {
            this.SendMessageDirect(2233, flags);
        }
        /// <summary>
        /// Ensure a particular line is visible by expanding any header line hiding it. Use the currently set visibility policy to determine which range to display. 
        /// </summary>
        public virtual void EnsureVisibleEnforcePolicy(int line)
        {
            this.SendMessageDirect(2234, line);
        }
        /// <summary>
        /// Get position of start of word. 
        /// </summary>
        public virtual int WordStartPosition(int pos, bool onlyWordCharacters)
        {
            return this.SendMessageDirect(2266, pos, onlyWordCharacters);
        }
        /// <summary>
        /// Get position of end of word. 
        /// </summary>
        public virtual int WordEndPosition(int pos, bool onlyWordCharacters)
        {
            return this.SendMessageDirect(2267, pos, onlyWordCharacters);
        }
        /// <summary>
        /// Measure the pixel width of some text in a particular style. NUL terminated text argument. Does not handle tab or control characters. 
        /// </summary>
        public virtual int TextWidth(int style, string text)
        {
            return this.SendMessageDirect(2276, style, text);
        }
        /// <summary>
        /// Retrieve the height of a particular line of text in pixels. 
        /// </summary>
        public virtual int TextHeight(int line)
        {
            return this.SendMessageDirect(2279, line);
        }
        /// <summary>
        /// Append a string to the end of the document without changing the selection. 
        /// </summary>
        public virtual void AppendText(string text)
        {
            this.SendMessageDirect(2282, text.Length, text);
        }
        /// <summary>
        /// Make the target range start and end be the same as the selection range start and end. 
        /// </summary>
        public virtual void TargetFromSelection()
        {
            this.SendMessageDirect(2287);
        }
        /// <summary>
        /// Join the lines in the target. 
        /// </summary>
        public virtual void LinesJoin()
        {
            this.SendMessageDirect(2288);
        }
        /// <summary>
        /// Split the lines in the target into lines that are less wide than pixelWidth where possible. 
        /// </summary>
        public virtual void LinesSplit(int pixelWidth)
        {
            this.SendMessageDirect(2289, pixelWidth);
        }
        /// <summary>
        /// Set the colours used as a chequerboard pattern in the fold margin 
        /// </summary>
        public virtual void SetFoldMarginColor(bool useSetting, int back)
        {
            this.SendMessageDirect(2290, useSetting, back);
        }
        /// <summary>
        /// Set the colours used as a chequerboard pattern in the fold margin 
        /// </summary>
        public virtual void SetFoldMarginHiColor(bool useSetting, int fore)
        {
            this.SendMessageDirect(2291, useSetting, fore);
        }
        /// <summary>
        /// Move caret down one line. 
        /// </summary>
        public virtual void LineDown()
        {
            this.SendMessageDirect(2300);
        }
        /// <summary>
        /// Move caret down one line extending selection to new caret position. 
        /// </summary>
        public virtual void LineDownExtend()
        {
            this.SendMessageDirect(2301);
        }
        /// <summary>
        /// Move caret up one line. 
        /// </summary>
        public virtual void LineUp()
        {
            this.SendMessageDirect(2302);
        }
        /// <summary>
        /// Move caret up one line extending selection to new caret position. 
        /// </summary>
        public virtual void LineUpExtend()
        {
            this.SendMessageDirect(2303);
        }
        /// <summary>
        /// Move caret left one character. 
        /// </summary>
        public virtual void CharLeft()
        {
            this.SendMessageDirect(2304);
        }
        /// <summary>
        /// Move caret left one character extending selection to new caret position. 
        /// </summary>
        public virtual void CharLeftExtend()
        {
            this.SendMessageDirect(2305);
        }
        /// <summary>
        /// Move caret right one character. 
        /// </summary>
        public virtual void CharRight()
        {
            this.SendMessageDirect(2306);
        }
        /// <summary>
        /// Move caret right one character extending selection to new caret position. 
        /// </summary>
        public virtual void CharRightExtend()
        {
            this.SendMessageDirect(2307);
        }
        /// <summary>
        /// Move caret left one word. 
        /// </summary>
        public virtual void WordLeft()
        {
            this.SendMessageDirect(2308);
        }
        /// <summary>
        /// Move caret left one word extending selection to new caret position. 
        /// </summary>
        public virtual void WordLeftExtend()
        {
            this.SendMessageDirect(2309);
        }
        /// <summary>
        /// Move caret right one word. 
        /// </summary>
        public virtual void WordRight()
        {
            this.SendMessageDirect(2310);
        }
        /// <summary>
        /// Move caret right one word extending selection to new caret position. 
        /// </summary>
        public virtual void WordRightExtend()
        {
            this.SendMessageDirect(2311);
        }
        /// <summary>
        /// Move caret to first position on line. 
        /// </summary>
        public virtual void Home()
        {
            this.SendMessageDirect(2312);
        }
        /// <summary>
        /// Move caret to first position on line extending selection to new caret position. 
        /// </summary>
        public virtual void HomeExtend()
        {
            this.SendMessageDirect(2313);
        }
        /// <summary>
        /// Move caret to last position on line. 
        /// </summary>
        public virtual void LineEnd()
        {
            this.SendMessageDirect(2314);
        }
        /// <summary>
        /// Move caret to last position on line extending selection to new caret position. 
        /// </summary>
        public virtual void LineEndExtend()
        {
            this.SendMessageDirect(2315);
        }
        /// <summary>
        /// Move caret to first position in document. 
        /// </summary>
        public virtual void DocumentStart()
        {
            this.SendMessageDirect(2316);
        }
        /// <summary>
        /// Move caret to first position in document extending selection to new caret position. 
        /// </summary>
        public virtual void DocumentStartExtend()
        {
            this.SendMessageDirect(2317);
        }
        /// <summary>
        /// Move caret to last position in document. 
        /// </summary>
        public virtual void DocumentEnd()
        {
            this.SendMessageDirect(2318);
        }
        /// <summary>
        /// Move caret to last position in document extending selection to new caret position. 
        /// </summary>
        public virtual void DocumentEndExtend()
        {
            this.SendMessageDirect(2319);
        }
        /// <summary>
        /// Move caret one page up. 
        /// </summary>
        public virtual void PageUp()
        {
            this.SendMessageDirect(2320);
        }
        /// <summary>
        /// Move caret one page up extending selection to new caret position. 
        /// </summary>
        public virtual void PageUpExtend()
        {
            this.SendMessageDirect(2321);
        }
        /// <summary>
        /// Move caret one page down. 
        /// </summary>
        public virtual void PageDown()
        {
            this.SendMessageDirect(2322);
        }
        /// <summary>
        /// Move caret one page down extending selection to new caret position. 
        /// </summary>
        public virtual void PageDownExtend()
        {
            this.SendMessageDirect(2323);
        }
        /// <summary>
        /// Switch from insert to overtype mode or the reverse. 
        /// </summary>
        public virtual void EditToggleOvertype()
        {
            this.SendMessageDirect(2324);
        }
        /// <summary>
        /// Cancel any modes such as call tip or auto-completion list display. 
        /// </summary>
        public virtual void Cancel()
        {
            this.SendMessageDirect(2325);
        }
        /// <summary>
        /// Delete the selection or if no selection, the character before the caret. 
        /// </summary>
        public virtual void DeleteBack()
        {
            this.SendMessageDirect(2326);
        }
        /// <summary>
        /// If selection is empty or all on one line replace the selection with a tab character. If more than one line selected, indent the lines. 
        /// </summary>
        public virtual void Tab()
        {
            this.SendMessageDirect(2327);
        }
        /// <summary>
        /// Dedent the selected lines. 
        /// </summary>
        public virtual void BackTab()
        {
            this.SendMessageDirect(2328);
        }
        /// <summary>
        /// Insert a new line, may use a CRLF, CR or LF depending on EOL mode. 
        /// </summary>
        public virtual void NewLine()
        {
            this.SendMessageDirect(2329);
        }
        /// <summary>
        /// Insert a Form Feed character. 
        /// </summary>
        public virtual void FormFeed()
        {
            this.SendMessageDirect(2330);
        }
        /// <summary>
        /// Move caret to before first visible character on line. If already there move to first character on line. 
        /// </summary>
        public virtual void VCHome()
        {
            this.SendMessageDirect(2331);
        }
        /// <summary>
        /// Like VCHome but extending selection to new caret position. 
        /// </summary>
        public virtual void VCHomeExtend()
        {
            this.SendMessageDirect(2332);
        }
        /// <summary>
        /// Magnify the displayed text by increasing the sizes by 1 point. 
        /// </summary>
        public virtual void ZoomIn()
        {
            this.SendMessageDirect(2333);
        }
        /// <summary>
        /// Make the displayed text smaller by decreasing the sizes by 1 point. 
        /// </summary>
        public virtual void ZoomOut()
        {
            this.SendMessageDirect(2334);
        }
        /// <summary>
        /// Delete the word to the left of the caret. 
        /// </summary>
        public virtual void DelWordLeft()
        {
            this.SendMessageDirect(2335);
        }
        /// <summary>
        /// Delete the word to the right of the caret. 
        /// </summary>
        public virtual void DelWordRight()
        {
            this.SendMessageDirect(2336);
        }
        /// <summary>
        /// Cut the line containing the caret. 
        /// </summary>
        public virtual void LineCut()
        {
            this.SendMessageDirect(2337);
        }
        /// <summary>
        /// Delete the line containing the caret. 
        /// </summary>
        public virtual void LineDelete()
        {
            this.SendMessageDirect(2338);
        }
        /// <summary>
        /// Switch the current line with the previous. 
        /// </summary>
        public virtual void LineTranspose()
        {
            this.SendMessageDirect(2339);
        }
        /// <summary>
        /// Duplicate the current line. 
        /// </summary>
        public virtual void LineDuplicate()
        {
            this.SendMessageDirect(2404);
        }
        /// <summary>
        /// Transform the selection to lower case. 
        /// </summary>
        public virtual void LowerCase()
        {
            this.SendMessageDirect(2340);
        }
        /// <summary>
        /// Transform the selection to upper case. 
        /// </summary>
        public virtual void UpperCase()
        {
            this.SendMessageDirect(2341);
        }
        /// <summary>
        /// Scroll the document down, keeping the caret visible. 
        /// </summary>
        public virtual void LineScrollDown()
        {
            this.SendMessageDirect(2342);
        }
        /// <summary>
        /// Scroll the document up, keeping the caret visible. 
        /// </summary>
        public virtual void LineScrollUp()
        {
            this.SendMessageDirect(2343);
        }
        /// <summary>
        /// Delete the selection or if no selection, the character before the caret. Will not delete the character before at the start of a line. 
        /// </summary>
        public virtual void DeleteBackNotLine()
        {
            this.SendMessageDirect(2344);
        }
        /// <summary>
        /// Move caret to first position on display line. 
        /// </summary>
        public virtual void HomeDisplay()
        {
            this.SendMessageDirect(2345);
        }
        /// <summary>
        /// Move caret to first position on display line extending selection to new caret position. 
        /// </summary>
        public virtual void HomeDisplayExtend()
        {
            this.SendMessageDirect(2346);
        }
        /// <summary>
        /// Move caret to last position on display line. 
        /// </summary>
        public virtual void LineEndDisplay()
        {
            this.SendMessageDirect(2347);
        }
        /// <summary>
        /// Move caret to last position on display line extending selection to new caret position. 
        /// </summary>
        public virtual void LineEndDisplayExtend()
        {
            this.SendMessageDirect(2348);
        }
        /// <summary>
        /// 
        /// </summary>
        public virtual void HomeWrap()
        {
            this.SendMessageDirect(2349);
        }
        /// <summary>
        /// 
        /// </summary>
        public virtual void HomeWrapExtend()
        {
            this.SendMessageDirect(2450);
        }
        /// <summary>
        /// 
        /// </summary>
        public virtual void LineEndWrap()
        {
            this.SendMessageDirect(2451);
        }
        /// <summary>
        /// 
        /// </summary>
        public virtual void LineEndWrapExtend()
        {
            this.SendMessageDirect(2452);
        }
        /// <summary>
        /// 
        /// </summary>
        public virtual void VCHomeWrap()
        {
            this.SendMessageDirect(2453);
        }
        /// <summary>
        /// 
        /// </summary>
        public virtual void VCHomeWrapExtend()
        {
            this.SendMessageDirect(2454);
        }
        /// <summary>
        /// Copy the line containing the caret. 
        /// </summary>
        public virtual void LineCopy()
        {
            this.SendMessageDirect(2455);
        }
        /// <summary>
        /// Move the caret inside current view if it's not there already. 
        /// </summary>
        public virtual void MoveCaretInsideView()
        {
            this.SendMessageDirect(2401);
        }
        /// <summary>
        /// How many characters are on a line, not including end of line characters? 
        /// </summary>
        public virtual int LineLength(int line)
        {
            return this.SendMessageDirect(2350, line);
        }
        /// <summary>
        /// Highlight the characters at two positions. 
        /// </summary>
        public virtual void BraceHighlight(int pos1, int pos2)
        {
            this.SendMessageDirect(2351, pos1, pos2);
        }
        /// <summary>
        /// Highlight the character at a position indicating there is no matching brace. 
        /// </summary>
        public virtual void BraceBadLight(int pos)
        {
            this.SendMessageDirect(2352, pos);
        }
        /// <summary>
        /// Find the position of a matching brace or INVALID_POSITION if no match. 
        /// </summary>
        public virtual int BraceMatch(int pos)
        {
            return this.SendMessageDirect(2353, pos, 0);
        }
        /// <summary>
        /// Sets the current caret position to be the search anchor. 
        /// </summary>
        public virtual void SearchAnchor()
        {
            this.SendMessageDirect(2366);
        }
        /// <summary>
        /// Find some text starting at the search anchor. Does not ensure the selection is visible. 
        /// </summary>
        public virtual int SearchNext(int flags, string text)
        {
            return this.SendMessageDirect(2367, flags, text);
        }
        /// <summary>
        /// Find some text starting at the search anchor and moving backwards. Does not ensure the selection is visible. 
        /// </summary>
        public virtual int SearchPrev(int flags, string text)
        {
            return this.SendMessageDirect(2368, flags, text);
        }
        /// <summary>
        /// Set whether a pop up menu is displayed automatically when the user presses the wrong mouse button. 
        /// </summary>
        public virtual void UsePopUp(bool allowPopUp)
        {
            this.SendMessageDirect(2371, allowPopUp);
        }
        /// <summary>
        /// Create a new document object. Starts with reference count of 1 and not selected into editor. 
        /// </summary>
        public virtual int CreateDocument()
        {
            return this.SendMessageDirect(2375);
        }
        /// <summary>
        /// Create a new document object. Starts with reference count of 1 and not selected into editor. Extend life of document. 
        /// </summary>
        public virtual void AddRefDocument(int doc)
        {
            this.SendMessageDirect(2376, VOID.NULL, doc);
        }
        /// <summary>
        /// Create a new document object. Starts with reference count of 1 and not selected into editor. Extend life of document. Release a reference to the document, deleting document if it fades to black. 
        /// </summary>
        public virtual void ReleaseDocument(int doc)
        {
            this.SendMessageDirect(2377, VOID.NULL, doc);
        }
        /// <summary>
        /// Move to the previous change in capitalisation. 
        /// </summary>
        public virtual void WordPartLeft()
        {
            this.SendMessageDirect(2390);
        }
        /// <summary>
        /// Move to the previous change in capitalisation. Move to the previous change in capitalisation extending selection to new caret position. 
        /// </summary>
        public virtual void WordPartLeftExtend()
        {
            this.SendMessageDirect(2391);
        }
        /// <summary>
        /// Move to the previous change in capitalisation. Move to the previous change in capitalisation extending selection to new caret position. Move to the change next in capitalisation. 
        /// </summary>
        public virtual void WordPartRight()
        {
            this.SendMessageDirect(2392);
        }
        /// <summary>
        /// Move to the previous change in capitalisation. Move to the previous change in capitalisation extending selection to new caret position. Move to the change next in capitalisation. Move to the next change in capitalisation extending selection to new caret position. 
        /// </summary>
        public virtual void WordPartRightExtend()
        {
            this.SendMessageDirect(2393);
        }
        /// <summary>
        /// Constants for use with SetVisiblePolicy, similar to SetCaretPolicy. Set the way the display area is determined when a particular line is to be moved to by Find, FindNext, GotoLine, etc. 
        /// </summary>
        public virtual void SetVisiblePolicy(int visiblePolicy, int visibleSlop)
        {
            this.SendMessageDirect(2394, visiblePolicy, visibleSlop);
        }
        /// <summary>
        /// Delete back from the current position to the start of the line. 
        /// </summary>
        public virtual void DelLineLeft()
        {
            this.SendMessageDirect(2395);
        }
        /// <summary>
        /// Delete forwards from the current position to the end of the line. 
        /// </summary>
        public virtual void DelLineRight()
        {
            this.SendMessageDirect(2396);
        }
        /// <summary>
        /// Set the last x chosen value to be the caret x position. 
        /// </summary>
        public virtual void ChooseCaretX()
        {
            this.SendMessageDirect(2399);
        }
        /// <summary>
        /// Set the focus to this Scintilla widget. 
        /// </summary>
        public virtual void GrabFocus()
        {
            this.SendMessageDirect(2400);
        }
        /// <summary>
        /// Set the way the caret is kept visible when going sideway. The exclusion zone is given in pixels. 
        /// </summary>
        public virtual void SetXCaretPolicy(int caretPolicy, int caretSlop)
        {
            this.SendMessageDirect(2402, caretPolicy, caretSlop);
        }
        /// <summary>
        /// Set the way the line the caret is on is kept visible. The exclusion zone is given in lines. 
        /// </summary>
        public virtual void SetYCaretPolicy(int caretPolicy, int caretSlop)
        {
            this.SendMessageDirect(2403, caretPolicy, caretSlop);
        }
        /// <summary>
        /// Move caret between paragraphs (delimited by empty lines). 
        /// </summary>
        public virtual void ParaDown()
        {
            this.SendMessageDirect(2413);
        }
        /// <summary>
        /// Move caret between paragraphs (delimited by empty lines). 
        /// </summary>
        public virtual void ParaDownExtend()
        {
            this.SendMessageDirect(2414);
        }
        /// <summary>
        /// Move caret between paragraphs (delimited by empty lines). 
        /// </summary>
        public virtual void ParaUp()
        {
            this.SendMessageDirect(2415);
        }
        /// <summary>
        /// Move caret between paragraphs (delimited by empty lines). 
        /// </summary>
        public virtual void ParaUpExtend()
        {
            this.SendMessageDirect(2416);
        }
        /// <summary>
        /// Given a valid document position, return the previous position taking code page into account. Returns 0 if passed 0. 
        /// </summary>
        public virtual int PositionBefore(int pos)
        {
            return this.SendMessageDirect(2417, pos);
        }
        /// <summary>
        /// Given a valid document position, return the next position taking code page into account. Maximum value returned is the last position in the document. 
        /// </summary>
        public virtual int PositionAfter(int pos)
        {
            return this.SendMessageDirect(2418, pos);
        }
        /// <summary>
        /// Copy a range of text to the clipboard. Positions are clipped into the document. 
        /// </summary>
        public virtual void CopyRange(int start, int end)
        {
            this.SendMessageDirect(2419, start, end);
        }
        /// <summary>
        /// Copy argument text to the clipboard. 
        /// </summary>
        public virtual void CopyText(string text)
        {
            this.SendMessageDirect(2420, text.Length, text);
        }
        /// <summary>
        /// Retrieve the position of the start of the selection at the given line (INVALID_POSITION if no selection on this line). 
        /// </summary>
        public virtual int GetLineSelStartPosition(int line)
        {
            return this.SendMessageDirect(2424, line);
        }
        /// <summary>
        /// Retrieve the position of the end of the selection at the given line (INVALID_POSITION if no selection on this line). 
        /// </summary>
        public virtual int GetLineSelEndPosition(int line)
        {
            return this.SendMessageDirect(2425, line);
        }
        /// <summary>
        /// Move caret down one line, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void LineDownRectExtend()
        {
            this.SendMessageDirect(2426);
        }
        /// <summary>
        /// Move caret up one line, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void LineUpRectExtend()
        {
            this.SendMessageDirect(2427);
        }
        /// <summary>
        /// Move caret left one character, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void CharLeftRectExtend()
        {
            this.SendMessageDirect(2428);
        }
        /// <summary>
        /// Move caret right one character, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void CharRightRectExtend()
        {
            this.SendMessageDirect(2429);
        }
        /// <summary>
        /// Move caret to first position on line, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void HomeRectExtend()
        {
            this.SendMessageDirect(2430);
        }
        /// <summary>
        /// Move caret to before first visible character on line. If already there move to first character on line. In either case, extend rectangular selection to new caret position. 
        /// </summary>
        public virtual void VCHomeRectExtend()
        {
            this.SendMessageDirect(2431);
        }
        /// <summary>
        /// Move caret to last position on line, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void LineEndRectExtend()
        {
            this.SendMessageDirect(2432);
        }
        /// <summary>
        /// Move caret one page up, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void PageUpRectExtend()
        {
            this.SendMessageDirect(2433);
        }
        /// <summary>
        /// Move caret one page down, extending rectangular selection to new caret position. 
        /// </summary>
        public virtual void PageDownRectExtend()
        {
            this.SendMessageDirect(2434);
        }
        /// <summary>
        /// Move caret to top of page, or one page up if already at top of page. 
        /// </summary>
        public virtual void StutteredPageUp()
        {
            this.SendMessageDirect(2435);
        }
        /// <summary>
        /// Move caret to top of page, or one page up if already at top of page, extending selection to new caret position. 
        /// </summary>
        public virtual void StutteredPageUpExtend()
        {
            this.SendMessageDirect(2436);
        }
        /// <summary>
        /// Move caret to bottom of page, or one page down if already at bottom of page. 
        /// </summary>
        public virtual void StutteredPageDown()
        {
            this.SendMessageDirect(2437);
        }
        /// <summary>
        /// Move caret to bottom of page, or one page down if already at bottom of page, extending selection to new caret position. 
        /// </summary>
        public virtual void StutteredPageDownExtend()
        {
            this.SendMessageDirect(2438);
        }
        /// <summary>
        /// Move caret left one word, position cursor at end of word. 
        /// </summary>
        public virtual void WordLeftEnd()
        {
            this.SendMessageDirect(2439);
        }
        /// <summary>
        /// Move caret left one word, position cursor at end of word, extending selection to new caret position. 
        /// </summary>
        public virtual void WordLeftEndExtend()
        {
            this.SendMessageDirect(2440);
        }
        /// <summary>
        /// Move caret right one word, position cursor at end of word. 
        /// </summary>
        public virtual void WordRightEnd()
        {
            this.SendMessageDirect(2441);
        }
        /// <summary>
        /// Move caret right one word, position cursor at end of word, extending selection to new caret position. 
        /// </summary>
        public virtual void WordRightEndExtend()
        {
            this.SendMessageDirect(2442);
        }
        /// <summary>
        /// Reset the set of characters for whitespace and word characters to the defaults. 
        /// </summary>
        public virtual void SetCharsDefault()
        {
            this.SendMessageDirect(2444);
        }
        /// <summary>
        /// Get currently selected item position in the auto-completion list 
        /// </summary>
        public virtual int AutoCGetCurrent()
        {
            return this.SendMessageDirect(2445);
        }
        /// <summary>
        /// Enlarge the document to a particular size of text bytes. 
        /// </summary>
        public virtual void Allocate(int bytes)
        {
            this.SendMessageDirect(2446, bytes);
        }
        /// <summary>
        /// Returns the target converted to UTF8. Return the length in bytes. 
        /// </summary>
        public virtual string TargetAsUTF8()
        {
            string result;
            this.SendMessageDirect(2447, out result);
            return result;
        }
        /// <summary>
        /// Set the length of the utf8 argument for calling EncodedFromUTF8. Set to -1 and the string will be measured to the first nul. 
        /// </summary>
        public virtual void SetLengthForEncode(int bytes)
        {
            this.SendMessageDirect(2448, bytes);
        }
        /// <summary>
        /// Translates a UTF8 string into the document encoding. Return the length of the result in bytes. On error return 0. 
        /// </summary>
        public virtual string EncodedFromUTF8(string utf8)
        {
            string result;
            this.SendMessageDirect(2449, utf8, out result);
            return result;
        }
        /// <summary>
        /// Find the position of a column on a line taking into account tabs and multi-byte characters. If beyond end of line, return line end position. 
        /// </summary>
        public virtual int FindColumn(int line, int column)
        {
            return this.SendMessageDirect(2456, line, column);
        }
        /// <summary>
        /// Switch between sticky and non-sticky: meant to be bound to a key. 
        /// </summary>
        public virtual void ToggleCaretSticky()
        {
            this.SendMessageDirect(2459);
        }
        /// <summary>
        /// Duplicate the selection. If selection empty duplicate the line containing the caret. 
        /// </summary>
        public virtual void SelectionDuplicate()
        {
            this.SendMessageDirect(2469);
        }
        /// <summary>
        /// Start notifying the container of all key presses and commands. 
        /// </summary>
        public virtual void StartRecord()
        {
            this.SendMessageDirect(3001);
        }
        /// <summary>
        /// Stop notifying the container of all key presses and commands. 
        /// </summary>
        public virtual void StopRecord()
        {
            this.SendMessageDirect(3002);
        }
        /// <summary>
        /// Colourise a segment of the document using the current lexing language. 
        /// </summary>
        public virtual void Colorize(int start, int end)
        {
            this.SendMessageDirect(4003, start, end);
        }
        /// <summary>
        /// Load a lexer library (dll / so). 
        /// </summary>
        public virtual void LoadLexerLibrary(string path)
        {
            this.SendMessageDirect(4007, VOID.NULL, path);
        }
        /// <summary>
        /// Retrieve a "property" value previously set with SetProperty. 
        /// </summary>
        public virtual string GetProperty(string key)
        {
            string result;
            this.SendMessageDirect(4008, key, out result);
            return result;
        }
        /// <summary>
        /// Retrieve a "property" value previously set with SetProperty, with "$()" variable replacement on returned buffer. 
        /// </summary>
        public virtual string GetPropertyExpanded(string key)
        {
            string result;
            this.SendMessageDirect(4009, key, out result);
            return result;
        }
    }
}
