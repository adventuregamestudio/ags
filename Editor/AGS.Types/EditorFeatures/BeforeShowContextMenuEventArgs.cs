using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public class BeforeShowContextMenuEventArgs
	{
		private IEditorComponent _component;
		private MenuCommands _menuCommands;
		private string _nodeId;

		public BeforeShowContextMenuEventArgs(string nodeId, IEditorComponent component, MenuCommands menuCommands)
		{
			_component = component;
			_menuCommands = menuCommands;
			_nodeId = nodeId;
		}

		/// <summary>
		/// The component that created the activated node.
		/// </summary>
		public IEditorComponent Component
		{
			get { return _component; }
		}

		/// <summary>
		/// The context menu commands that have been created by the
		/// owning component.
		/// </summary>
		public MenuCommands MenuCommands
		{
			get { return _menuCommands; }
		}

		/// <summary>
		/// The ID of the tree node that was right-clicked.
		/// </summary>
		public string SelectedNodeID
		{
			get { return _nodeId; }
		}
	}
}
