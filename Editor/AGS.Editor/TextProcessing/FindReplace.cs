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
        private static string _lastSearchText, _lastReplaceText;
        private bool _lastCaseSensitive;
        private static bool _creatingDialog;
        private static FindReplaceDialog _dialog;

        public delegate void LastSearchTextChangedHandler(string searchText);
        public event LastSearchTextChangedHandler LastSearchTextChanged;

        public FindReplace(IScript script, AGSEditor agsEditor,
            string lastSearchText, bool lastCaseSensitive)
        {
            this._script = script;
            this._agsEditor = agsEditor;
            if (!string.IsNullOrEmpty(lastSearchText))
            {
                _lastSearchText = lastSearchText;
            }
            this._lastCaseSensitive = lastCaseSensitive;
        }

        public static void CloseDialogIfNeeded()
        {
            if (_dialog != null && !_dialog.IsDisposed
                && !_creatingDialog)
            {
                _dialog.Close();
            }
        }
        
        public bool PerformFindReplace()
        {            
            bool showDialog = true;
            int currentScript = 0;
            bool foundOneReference = false;
            bool showAll = _dialog.ShowingAllDialog;

            _lastReplaceText = _dialog.TextToReplaceWith;
            _lastCaseSensitive = _dialog.CaseSensitive;

            if (_dialog.TextToFind.Length > 0)
            {
                if (IsUserRegrets(_dialog.IsReplace, showAll))
                {
                    return true;
                }
                _lastSearchText = _dialog.TextToFind;

                if (LastSearchTextChanged != null)
                {
                    LastSearchTextChanged(_lastSearchText);
                }

                bool jumpToStart;
                List<IScript> scriptsForFindReplace = GetScriptsForFindReplace(_dialog.LookIn, showAll, out jumpToStart);

                List<ScriptTokenReference> results = new List<ScriptTokenReference>();

                for (; currentScript < scriptsForFindReplace.Count; currentScript++)
                {
                    _script = scriptsForFindReplace[currentScript];
                    if (!FindReplaceInScript(showAll, ref currentScript, 
                        ref foundOneReference, jumpToStart, 
                        scriptsForFindReplace, results))
                    {
                        if (showAll || foundOneReference)
                        {
                            break;
                        }
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
            _creatingDialog = true;
            CreateDialog(showReplace, showAll);
            _dialog.Show();
            _dialog.Focus();
            _creatingDialog = false;
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
                    List<IScript> scripts = _agsEditor.GetAllScripts();
                    if (!findAll)
                    {
                        OrderScriptsBySelectedScript(scripts);
                    }
                    return scripts;
                default:
                    throw new NotSupportedException(string.Format("{0} is not supported yet", lookIn));
            }
        }

        private void OrderScriptsBySelectedScript(List<IScript> scripts)
        {
            List<IScript> tmpScripts = new List<IScript>(scripts);
            scripts.Clear();
            int currentScriptIndex = tmpScripts.FindIndex(c => c.FileName == _script.FileName);
            
            for (int i = currentScriptIndex; i < tmpScripts.Count; i++)
            {
                scripts.Add(tmpScripts[i]);
            }

            for (int i = 0; i < currentScriptIndex; i++)
            {
                scripts.Add(tmpScripts[i]);
            }
        }

        private void CreateDialog(bool showReplace, bool showAll)
        {
            if (_dialog == null || _dialog.IsDisposed)
            {
                _dialog = new FindReplaceDialog(_lastSearchText,
                    _lastReplaceText, _agsEditor.Preferences, this);
                _dialog.ShowingReplaceDialog = showReplace;
                _dialog.ShowingAllDialog = showAll;
                _dialog.CaseSensitive = _lastCaseSensitive;
            }            
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
                    //scriptEditorControl.ActivateTextEditor();
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

                ZoomToFileIfNeeded(script, ref scriptEditorControl, 
                    scriptTokenReference, showAll);
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

        private void ZoomToFileIfNeeded(IScript script, 
            ref IScriptEditorControl scriptEditorControl,
            ScriptTokenReference scriptTokenReference, bool showAll)
        {
            if (showAll && !_dialog.IsReplace) return;
            if (TextToFindIsCurrentlySelected(scriptEditorControl))
            {
                int startPos = scriptEditorControl.CursorPosition - _dialog.TextToFind.Length;
                Factory.GUIController.ZoomToFile(script.FileName, ZoomToFileZoomType.ZoomToCharacterPosition,
                    startPos, false, false, null, false);
                scriptEditorControl = Factory.GUIController.GetScriptEditorControl(script.FileName, false);
                scriptEditorControl.SetSelection(startPos, _dialog.TextToFind.Length);                
                scriptEditorControl.ActivateTextEditor();
                _dialog.Activate();
            }
        }

        private void ReplaceTextIfNeeded(IScript script, 
            IScriptEditorControl scriptEditorControl)
        {
            if (_dialog.IsReplace && TextToFindIsCurrentlySelected(scriptEditorControl))
                scriptEditorControl.ReplaceSelectedText(_dialog.TextToReplaceWith);            
        }
    }
}
