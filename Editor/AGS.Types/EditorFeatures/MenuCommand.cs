using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Types
{
	public class MenuCommand
	{
		public const string MENU_TEXT_SEPARATOR = "-";

		public static MenuCommand Separator
		{
			get { return new MenuCommand(null, MenuCommand.MENU_TEXT_SEPARATOR); }
		}

		public MenuCommand(string itemID, string itemName)
		{
			_id = itemID;
			_name = itemName;
			_iconKey = null;
			_enabled = true;
		}

		public MenuCommand(string itemID, string itemName, string iconKey)
			: this(itemID, itemName)
		{
			_iconKey = iconKey;
		}

		public MenuCommand(string itemID, string itemName, Keys shortcutKey)
			: this(itemID, itemName)
		{
			_shortcutKey = shortcutKey;
		}

		public MenuCommand(string itemID, string itemName, Keys shortcutKey, string iconKey)
			: this(itemID, itemName)
		{
			_shortcutKey = shortcutKey;
			_iconKey = iconKey;
		}

		private string _id;
		private string _idPrefix;
		private string _name;
		private string _iconKey = string.Empty;
		private bool _enabled;
		private bool _checked;
		private Keys _shortcutKey = Keys.None;

		public bool IsSeparator
		{
			get { return _name == MENU_TEXT_SEPARATOR; }
		}

		public Keys ShortcutKey
		{
			get { return _shortcutKey; }
			set { _shortcutKey = value; }
		}

		public string IconKey
		{
			get { return _iconKey; }
			set { _iconKey = value; }
		}

		public string Name
		{
			get { return _name; }
			set { _name = value; }
		}

		public string ID
		{
			get { return (_id == null) ? _id : (_idPrefix + _id); }
			set { _id = value; }
		}

		public string IDPrefix
		{
			get { return _idPrefix; }
			set { _idPrefix = value; }
		}

		public bool Enabled
		{
			get { return _enabled; }
			set { _enabled = value; }
		}

		public bool Checked
		{
			get { return _checked; }
			set { _checked = value; }
		}
	}
}
