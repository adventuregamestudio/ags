using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Types
{
	public interface IScriptEditorControl : IDisposable
	{
		void ActivateTextEditor();
		void SetKeyWords(string spaceDelimitedListOfKeywords);
		void ShowLineNumbers();
		bool AutoCompleteEnabled { get; set; }
		bool AutoSpaceAfterComma { get; set; }
		bool CallTipsEnabled { get; set; }
		Control Control { get; }
		string Text { get; set; }
        /// <summary>
        /// The start offset of the current selection.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        int SelectionStart { get; }
        /// <summary>
        /// The end offset of the current selection.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        int SelectionEnd { get; }
        /// <summary>
        /// The currently selected text.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        string SelectedText { get; }
        /// <summary>
        /// The current offset of the cursor.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        int CursorPosition { get; }
        /// <summary>
        /// Gets the line number that the specified position is on.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        int GetLineNumberForPosition(int position);
        /// <summary>
        /// Gets the text on the specified line.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        string GetTextForLine(int lineNumber);
        /// <summary>
        /// Returns the full variable/function name for the text under the
        /// cursor. If it cannot be resolved, the word under the cursor is
        /// returned.
        /// RequiredAGSVersion: 3.1.0.48
        /// </summary>
        string GetTypeNameAtCursor();
	}
}
