using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class SourceControlException : ApplicationException
    {
        private static Dictionary<int, string> _ErrorMessages = null;

        private int _sccErrorCode;
        private string _sccMethodName;

        static SourceControlException()
        {
            _ErrorMessages = new Dictionary<int, string>();
            _ErrorMessages.Add(2, "The operation was cancelled");
            _ErrorMessages.Add(5, "A new version of the file has been downloaded");
            _ErrorMessages.Add(-1, "Initialization failed");
            _ErrorMessages.Add(-4, "File is not checked out");
            _ErrorMessages.Add(-5, "File is already checked out to another user or on another machine");
            _ErrorMessages.Add(-6, "File is locked and cannot be changed");
            _ErrorMessages.Add(-7, "File is checked out by another user");
            _ErrorMessages.Add(-12, "File is checked out");
            _ErrorMessages.Add(-15, "Non-specific error");
            _ErrorMessages.Add(-24, "Invalid file path");
            _ErrorMessages.Add(-28, "File does not exist");
        }

        public SourceControlException(int sccErrorCode, string sccMethodName)
            : base("Source control error: " + sccMethodName + " returned " + sccErrorCode)
        {
            _sccErrorCode = sccErrorCode;
            _sccMethodName = sccMethodName;
        }

        public int SccErrorCode
        {
            get { return _sccErrorCode; }
        }

        public string SccMethodName
        {
            get { return _sccMethodName; }
        }

        public string SccErrorMessage
        {
            get
            {
                if (_ErrorMessages.ContainsKey(_sccErrorCode))
                {
                    return _ErrorMessages[_sccErrorCode];
                }
                return "Unknown error (code " + _sccErrorCode + ")";
            }
        }
    }
}
