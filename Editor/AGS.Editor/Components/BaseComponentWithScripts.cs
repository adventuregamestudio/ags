using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor.Components
{
    internal abstract class BaseComponentWithScripts : BaseComponent
    {
        public BaseComponentWithScripts(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
        }

        protected void ZoomToCorrectPositionInScript(ScriptEditor editor, ZoomToFileEventArgs evArgs)
        {
            if (editor == null)
            {
                return;
            }

            if (evArgs.ZoomType == ZoomToFileZoomType.ZoomToCharacterPosition)
            {
                editor.GoToLineOfCharacterPosition(evArgs.ZoomPosition, evArgs.SelectLine);
            }
            else if (evArgs.ZoomType != ZoomToFileZoomType.DoNotMoveCursor)
            {
                if (evArgs.ZoomToText != null)
                {
                    evArgs.ZoomPosition = editor.GetLineNumberForText(evArgs.ZoomToText);
                }
                editor.GoToLine(evArgs.ZoomPosition, evArgs.SelectLine, evArgs.ZoomToLineAfterOpeningBrace);

                if (evArgs.IsDebugExecutionPoint)
                {
                    editor.SetExecutionPointMarker(evArgs.ZoomPosition);
                    if (evArgs.ErrorMessage != null)
                    {
                        editor.SetErrorMessagePopup(evArgs.ErrorMessage);
                    }
                }
            }
        }
    }
}
