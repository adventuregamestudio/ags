using AGS.Types;
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

    public enum ZoomToFileResult
    {
        // Operation was not performed yet
        None,
        // Operation successful
        Success,
        // Script was not found or could not be opened
        ScriptNotFound,
        // Requested location was not found within the script
        LocationNotFound
    }

    /// <summary>
    /// ZoomToFileEventArgs object describes the location that has to be opened
    /// in a script editor, and result of this operation. This object may be passed
    /// through multiple event handlers, and they have to check its Handled property
    /// to know if any of the previous ones have already claimed this request.
    /// </summary>
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

        /// <summary>
        /// Tells if operation was claimed by one of the handlers, either
        /// successfully or unsuccessfully. In principle this means that
        /// at least the requested script was found.
        /// </summary>
        public bool Handled
        {
            get
            {
                return Result != ZoomToFileResult.None && Result != ZoomToFileResult.ScriptNotFound;
            }
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
        public ZoomToFileResult Result = ZoomToFileResult.None;
    }
}
