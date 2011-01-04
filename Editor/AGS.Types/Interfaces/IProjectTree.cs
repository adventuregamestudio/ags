using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public delegate void BeforeShowContextMenuHandler(BeforeShowContextMenuEventArgs evArgs);

	public interface IProjectTree
	{
		/// <summary>
		/// Adds a new root to the project tree.
		/// </summary>
		/// <param name="component">The owning component</param>
		/// <param name="id">Unique ID for this tree node</param>
		/// <param name="name">User-friendly name to display</param>
		/// <param name="iconKey">The icon to display, registered earlier with RegisterIcon</param>
		void AddTreeRoot(IEditorComponent component, string id, string name, string iconKey);
		/// <summary>
		/// Sets the project tree's internal marker to the specified node.
		/// Any AddTreeLeaf commands will add them as children of this node.
		/// </summary>
		void StartFromNode(IEditorComponent component, string id);
		/// <summary>
		/// Adds a new child node to the project tree.
		/// </summary>
		/// <param name="component">The owning component</param>
		/// <param name="id">Unique ID for this tree node</param>
		/// <param name="name">User-friendly name to display</param>
		/// <param name="iconKey">The icon to display, registered earlier with RegisterIcon</param>
		/// <param name="greyedOut">Whether this item should be greyed out (normally false)</param>
		/// <returns></returns>
		IProjectTreeItem AddTreeLeaf(IEditorComponent component, string id, string name, string iconKey, bool greyedOut);
		/// <summary>
		/// Sets the project tree to display this node as selected.
		/// </summary>
		void SelectNode(IEditorComponent plugin, string id);
		/// <summary>
		/// Removes all child nodes of the specified node. This allows you
		/// to refresh your list of child nodes by adding them all back
		/// again.
		/// </summary>
		void RemoveAllChildNodes(IEditorComponent component, string parentID);
		/// <summary>
		/// Fired just before a context menu is displayed in the project tree.
		/// </summary>
		event BeforeShowContextMenuHandler BeforeShowContextMenu;
	}
}
