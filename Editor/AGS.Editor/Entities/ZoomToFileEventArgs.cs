using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public class ZoomToFileEventArgs
	{
		public ZoomToFileEventArgs(string fileName, ZoomToFileZoomType zoomType, int zoomPosition, string zoomToText, bool isDebugExecutionPoint, string errorMessage)
		{
			FileName = fileName;
			ZoomType = zoomType;
			ZoomPosition = zoomPosition;
			ZoomToText = zoomToText;
			IsDebugExecutionPoint = isDebugExecutionPoint;
            ErrorMessage = errorMessage;
		}

		public string FileName;
		public ZoomToFileZoomType ZoomType;
		public int ZoomPosition;
		public string ZoomToText;
		public bool IsDebugExecutionPoint;
		public bool SelectLine = true;
        public string ErrorMessage;
        public bool ZoomToLineAfterOpeningBrace = false;
	}
}
