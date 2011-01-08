using System;
using System.Collections.Generic;
using System.Windows.Forms;
using AGS.Types;
using AGS.Types.AutoComplete;
using AGS.Types.Enums;
using AGS.Types.Interfaces;

namespace AGS.Editor.TextProcessing
{
    public class FindReplace
    {
        private IScript _script;
        private AGSEditor _agsEditor;
        private string _lastSearchText, _lastReplaceText;
        private bool _lastCaseSensitive;
        private FindReplaceDialog _dialog;

        public FindReplace(IScript script, AGSEditor agsEditor,
            string lastSearchText, string lastReplaceText, 
            bool lastCaseSensitive)
        {
            this._script = script;
            this._agsEditor = agsEditor;
            this._lastSearchText = lastSearchText;
            this._lastReplaceText = lastReplaceText;
            this._lastCaseSensitive = lastCaseSensitive;
        }
        
        public bool PerformFindReplace(FindReplaceDialog dialog)
        {
            _dialog = dialog;
            bool showDialog = true;
            int currentScript = 0;
            bool foundOneReference = false;
            bool showAll = dialog.ShowingAllDialog;

            _lastReplaceText = dialog.TextToReplaceWith;
            _lastCaseSensitive = dialog.CaseSensitive;

            if (dialog.TextToFind.Length > 0)
            {
                if (IsUserRegrets(dialog.IsReplace, showAll))
                {
                    return true;
                }
                _lastSearchText = dialog.TextToFind;

                bool jumpToStart;
                List<IScript> scriptsForFindReplace = GetScriptsForFindReplace(dialog.LookIn, showAll, out jumpToStart);

                List<ScriptTokenReference> results = new List<ScriptTokenReference>();

                for (; currentScript < scriptsForFindReplace.Count; currentScript++)
                {
                    if (!FindReplaceInScript(showAll, ref currentScript, 
                        ref foundOneReference, jumpToStart, 
                        scriptsForFindReplace, results))
                    {
                        break;
                    }
                }

                showDialog = ShowResults(showAll, showDialog, 
                    foundOneReference, results);
            }
            else
            {
                showDialog = false;
            }
            return showDialog;
        }

        public void ShowFindReplaceDialog(bool showReplace, bool showAll)
        {
            FindReplaceDialog dialog = CreateDialog(showReplace, showAll);
            dialog.Show();
        }

        private List<IScript> GetScriptsForFindReplace(LookInDocumentType lookIn, bool findAll, out bool jumpToStart)
        {
            switch (lookIn)
            {
                case LookInDocumentType.CurrentDocument:
                    jumpToStart = !findAll;
                    return new List<IScript> { _script };
                case LookInDocumentType.CurrentProject:
                    jumpToStart = false;
                    return _agsEditor.GetAllScripts();
                default:
                    throw new NotSupportedException(string.Format("{0} is not supported yet", lookIn));
            }
        }

        private FindReplaceDialog CreateDialog(bool showReplace, bool showAll)
        {
            FindReplaceDialog dialog = new FindReplaceDialog(_lastSearchText, 
                _lastReplaceText, _agsEditor.Preferences, this);
            dialog.ShowingReplaceDialog = showReplace;
            dialog.ShowingAllDialog = showAll;
            dialog.CaseSensitive = _lastCaseSensitive;
            return dialog; 
        }

        private bool IsUserRegrets(bool replace, bool showAll)
        {
            if (replace && showAll)
            {
                DialogResult result = Factory.GUIController.ShowQuestion("Are you sure you want to replace all?");
                if (result == DialogResult.No || result == DialogResult.Cancel)
                {
                    return true;
                }
            }
            return false;
        }

        private void ResetSelection(IScriptEditorControl scriptEditorControl)
        {
            scriptEditorControl.ResetSelection();
        }
        
        private bool ShowResults(bool showAll, bool showDialog, 
            bool foundOneReference, List<ScriptTokenReference> results)
        {
            if (!foundOneReference)
            {
                showDialog = !showAll;
                Factory.GUIController.ShowMessage("No occurrences of '" + _lastSearchText + "' were found.", MessageBoxIcon.Information);
            }
            else if (showAll)
            {
                showDialog = false;
                if (_dialog.IsReplace)
                {
                    Factory.GUIController.ShowMessage(string.Format
                        ("Replaced {0} occurences.", results.Count),
                        MessageBoxIcon.Information);
                }
                else
                {
                    Factory.GUIController.ShowFindSymbolResults(results, null);
                }
            }
            return showDialog;
        }

        private bool FindReplaceInScript(bool showAll, ref int currentScript, 
            ref bool foundOneReference, bool jumpToStart, 
            List<IScript> scriptsForFindReplace, 
            List<ScriptTokenReference> results)
        {
            IScript script = scriptsForFindReplace[currentScript];
            
            IScriptEditorControl scriptEditorControl = Factory.GUIController.GetScriptEditorControl(script.FileName, false);

            if (scriptEditorControl != null)
            {
                if (showAll)
                {
                    ResetSelection(scriptEditorControl);
                }
                else
                {
                    scriptEditorControl.ActivateTextEditor();
                }

                ScriptTokenReference scriptTokenReference =
                    FindReplaceNextOccurence(showAll,
                    ref foundOneReference, jumpToStart,
                    results, script, ref scriptEditorControl);

                if (scriptTokenReference != null && scriptTokenReference.Script == null)
                {
                    scriptTokenReference.Script = script;
                }

                if (foundOneReference && !showAll)
                {
                    currentScript = JumpToFirstScriptIfNeeded(currentScript,
                        scriptsForFindReplace, scriptTokenReference);
                    return false;
                }
                if (!showAll)
                {
                    ResetSelection(scriptEditorControl);
                }
            }
            return true;
        }

        private static int JumpToFirstScriptIfNeeded(int currentScript, 
            List<IScript> scriptsForFindReplace, 
            ScriptTokenReference scriptTokenReference)
        {
            if (scriptTokenReference == null &&
                currentScript == scriptsForFindReplace.Count - 1)
            {
                currentScript = 0;
            }
            return currentScript;
        }

        private ScriptTokenReference FindReplaceNextOccurence(bool showAll, 
            ref bool foundOneReference, bool jumpToStart, 
            List<ScriptTokenReference> results, IScript script, 
            ref IScriptEditorControl scriptEditorControl)
        {
            ScriptTokenReference scriptTokenReference;

            if ((_dialog.IsReplace) && (TextToFindIsCurrentlySelected(scriptEditorControl)))
            {
                scriptTokenReference = scriptEditorControl.GetTokenReferenceForCurrentState();
            }
            else
            {
                scriptTokenReference = scriptEditorControl.FindNextOccurrence(_dialog.TextToFind, _dialog.CaseSensitive, jumpToStart);
            }

            if ((scriptTokenReference != null) && showAll && _dialog.IsReplace)
            {
                // get a visible editor if we need to make changes to the text
                scriptEditorControl = Factory.GUIController.GetScriptEditorControl(script.FileName, true);
            }

            while (scriptTokenReference != null)
            {
                foundOneReference = true;

                ReplaceTextIfNeeded(script, scriptEditorControl);

                if (showAll)
                {
                    if (scriptTokenReference.Script == null)
                    {
                        scriptTokenReference.Script = script;
                    }
                    results.Add(scriptTokenReference);

                    scriptTokenReference = scriptEditorControl.FindNextOccurrence(_dialog.TextToFind, _dialog.CaseSensitive, jumpToStart);
                }
                else if (_dialog.IsReplace)
                {
                    scriptTokenReference = scriptEditorControl.FindNextOccurrence(_dialog.TextToFind, _dialog.CaseSensitive, jumpToStart);
                    break;
                }
                else
                {
                    break;
                }
            }
            return scriptTokenReference;
        }

        private bool TextToFindIsCurrentlySelected(IScriptEditorControl scriptEditorControl)
        {
            return (scriptEditorControl.SelectedText.ToLower() == _dialog.TextToFind.ToLower());
        }

        private void ReplaceTextIfNeeded(IScript script, IScriptEditorControl scriptEditorControl)
        {
            if (_dialog.IsReplace && TextToFindIsCurrentlySelected(scriptEditorControl))
            {
                Factory.GUIController.ZoomToFile(script.FileName);
                scriptEditorControl.ReplaceSelectedText(_dialog.TextToReplaceWith);
            }
        }
    }
}
