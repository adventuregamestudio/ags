using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class GetAboutDialogTextEventArgs
    {
        private string _text;

        public GetAboutDialogTextEventArgs(string text)
        {
            _text = text;
        }

        /// <summary>
        /// The text to be displayed.
        /// </summary>
        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }

    }
}
