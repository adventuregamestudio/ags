using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Types
{
	public class ContentDocument : IDisposable
	{
		public event EventHandler PanelClosed;

		private EditorContentPanel _control;
		private string _name;
		private IEditorComponent _owner;
		private Dictionary<string, object> _propertyGridObjectList;
		private List<MenuCommand> _toolbarCommands;
		private MenuCommands _mainMenu;
		private object _selectedPropertyGridObject;
		private object[] _selectedPropertyGridObjects;
        private string _selectedPropertyGridTab = string.Empty;
        private string _selectedPropertyGridItem = null;
		private int _tabXOffset;
		private int _tabWidth;
		private bool _visible;

		public ContentDocument(EditorContentPanel control, string name, IEditorComponent owner)
		{
			_control = control;
			_name = name;
			_owner = owner;
			_toolbarCommands = null;
		}

		public ContentDocument(EditorContentPanel control, string name, IEditorComponent owner, Dictionary<string, object> propertyGridObjectList)
			: this(control, name, owner)
		{
			_propertyGridObjectList = propertyGridObjectList;
		}

		public EditorContentPanel Control
		{
			get { return _control; }
		}

		public IEditorComponent Owner
		{
			get { return _owner; }
		}

		public string Name
		{
			get { return _name; }
			set { _name = value; }
		}

		public int TabXOffset
		{
			get { return _tabXOffset; }
			set { _tabXOffset = value; }
		}

		public int TabWidth
		{
			get { return _tabWidth; }
			set { _tabWidth = value; }
		}

		public bool Visible
		{
			get { return _visible; }
			set
			{
				_visible = value;
				if ((PanelClosed != null) && (_visible == false))
				{
					PanelClosed(this, null);
				}
			}
		}

		public Dictionary<string, object> PropertyGridObjectList
		{
			get { return _propertyGridObjectList; }
			set { _propertyGridObjectList = value; }
		}

		public object SelectedPropertyGridObject
		{
			get { return _selectedPropertyGridObject; }
			set { _selectedPropertyGridObject = value; }
		}

		public object[] SelectedPropertyGridObjects
		{
			get { return _selectedPropertyGridObjects; }
			set { _selectedPropertyGridObjects = value; }
		}

        public string SelectedPropertyGridTab
        {
            get { return _selectedPropertyGridTab; }
            set { _selectedPropertyGridTab = value; }
        }

        public string SelectedPropertyGridItem
        {
            get { return _selectedPropertyGridItem; }
            set { _selectedPropertyGridItem = value; }
        }

		public List<MenuCommand> ToolbarCommands
		{
			get { return _toolbarCommands; }
			set { _toolbarCommands = value; }
		}

		public MenuCommands MainMenu
		{
			get { return _mainMenu; }
			set { _mainMenu = value; }
		}

		public void Dispose()
		{
			_control.Dispose();
		}
	}
}
