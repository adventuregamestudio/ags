using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor.Components
{
    internal abstract class BaseComponentWithScripts<TScript, TScriptFolder> : BaseComponentWithFolders<TScript, TScriptFolder>
        where TScript : IToXml
        where TScriptFolder : BaseFolderCollection<TScript, TScriptFolder>
    {
        public BaseComponentWithScripts(GUIController guiController, AGSEditor agsEditor, string topLevelCommandId)
            : base(guiController, agsEditor, topLevelCommandId)
        {
        }

        protected void ZoomToCorrectPositionInScript(ScriptEditor editor, ZoomToFileEventArgs evArgs)
        {
            bool result = true;
            if (evArgs.ZoomType == ZoomToFileZoomType.ZoomToCharacterPosition)
            {
                result = editor.GoToLineOfCharacterPosition(evArgs.ZoomPosition, evArgs.SelectLine);
            }
            else if (evArgs.ZoomType != ZoomToFileZoomType.DoNotMoveCursor)
            {
                if (evArgs.ZoomToText != null)
                {
                    switch (evArgs.MatchStyle)
                    {
                        case ZoomToFileMatchStyle.MatchExact:
                            evArgs.ZoomPosition = editor.GetLineNumberForText(evArgs.ZoomToText, true);
                            break;
                        case ZoomToFileMatchStyle.MatchRegex:
                            evArgs.ZoomPosition = editor.GetLineNumberForPattern(evArgs.ZoomToText, true);
                            break;
                        default:
                            evArgs.ZoomPosition = -1;
                            break;
                    }
                }

                result =
                    (evArgs.ZoomPosition >= 0 &&
                    editor.GoToLine(evArgs.ZoomPosition, evArgs.SelectLine, evArgs.ZoomToLineAfterOpeningBrace));

                if (evArgs.IsDebugExecutionPoint)
                {
                    editor.SetExecutionPointMarker(evArgs.ZoomPosition);
                    if (evArgs.ErrorMessage != null)
                    {
                        editor.SetErrorMessagePopup(evArgs.ErrorMessage);
                    }
                }
            }

            evArgs.Result = result ? ZoomToFileResult.Success : ZoomToFileResult.LocationNotFound;
        }

        protected abstract ContentDocument GetDocument(ScriptEditor editor);

        protected void ScriptEditor_IsModifiedChanged(object sender, EventArgs e)
        {
            ScriptEditor sendingPane = (ScriptEditor)sender;
            UpdateScriptWindowTitle(sendingPane);
        }

        protected void UpdateScriptWindowTitle(ScriptEditor editor)
        {
            string newTitle = editor.GetScriptTabName();
            ContentDocument document = GetDocument(editor);            
            if (document != null && document.Name != newTitle)
            {
                document.Name = newTitle;
                document.Control.DockingContainer.Text = newTitle;
                _guiController.DocumentTitlesChanged();
            }            
        }                
    }
}
