using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;

namespace AGS.Types
{
	public class BaseTemplate
	{
		protected string _fileName;
		protected string _friendlyName;
        protected string _description;
        protected Icon _icon;

        public BaseTemplate(string fileName, string description, Icon icon)
		{
			_fileName = fileName;
            _description = description;
            _icon = icon;
			if (_fileName != null)
			{
				_friendlyName = Path.GetFileNameWithoutExtension(_fileName);
			}
		}

		public string FileName
		{
			get { return _fileName; }
		}

		public string FriendlyName
		{
			get { return _friendlyName; }
		}

        public string Description
        {
            get { return _description; }
        }

		public Icon Icon
		{
			get { return _icon; }
		}
	}
}
