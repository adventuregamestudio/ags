using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    /// <summary>
    /// ZoomToFileZoomType defines which location in script to move cursor to.
    /// </summary>
    public enum ZoomToFileZoomType
    {
        ZoomToLineNumber,
        ZoomToCharacterPosition,
        ZoomToText,
        DoNotMoveCursor
    }

    /// <summary>
    /// ZoomToFileMatchStyle defines how to treat the search text.
    /// </summary>
    public enum ZoomToFileMatchStyle
    {
        MatchExact,
        MatchRegex
    }

	public class ZoomToFileEventArgs
	{
        public ZoomToFileEventArgs(string fileName, ZoomToFileZoomType zoomType, ZoomToFileMatchStyle matchStyle,
                                   string zoomToText)
            : this(fileName, zoomType, matchStyle, 0, zoomToText, false, null, true)
        {
        }

        public ZoomToFileEventArgs(string fileName, ZoomToFileZoomType zoomType, ZoomToFileMatchStyle matchStyle,
                                   int zoomPosition, string zoomToText, 
                                   bool isDebugExecutionPoint, string errorMessage, bool activateEditor)
		{
			FileName = fileName;
			ZoomType = zoomType;
            MatchStyle = matchStyle;
			ZoomPosition = zoomPosition;
			ZoomToText = zoomToText;
			IsDebugExecutionPoint = isDebugExecutionPoint;
            ErrorMessage = errorMessage;
            ActivateEditor = activateEditor;
		}

		public string FileName;
		public ZoomToFileZoomType ZoomType;
        public ZoomToFileMatchStyle MatchStyle;
        public int ZoomPosition;
		public string ZoomToText;
		public bool IsDebugExecutionPoint;
		public bool SelectLine = true;
        public string ErrorMessage;
        public bool ZoomToLineAfterOpeningBrace = false;
        public bool ActivateEditor = true;
	}
}
