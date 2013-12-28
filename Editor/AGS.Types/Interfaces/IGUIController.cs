using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
	public interface IGUIController
	{
		/// <summary>
		/// Registers an icon with the AGS Editor. Supply a unique ID which
		/// you use later to refer to this icon.
		/// </summary>
		void RegisterIcon(string key, Icon icon);
		/// <summary>
		/// Makes the specified pane into the active pane in the editor,
		/// or adds it if it doesn't already exist.
		/// </summary>
		void AddOrShowPane(ContentDocument pane);
		/// <summary>
		/// Removes the pane from the main tabbed window area. If it doesn't
		/// exist there, nothing happens.
		/// </summary>
        void RemovePaneIfExists(ContentDocument pane);
		/// <summary>
		/// Shows a message box with the specified message and icon
		/// </summary>
		void ShowMessage(string message, MessageBoxIconType icon);
		/// <summary>
		/// Adds a new main menu to the editor. The ID will be used
		/// to identify it in code, and Title is the user friendly name.
		/// </summary>
		void AddMenu(IEditorComponent component, string id, string title, string insertAfterMenu);
		/// <summary>
		/// Adds a set of new commands to the editor main menus
		/// </summary>
		void AddMenuItems(IEditorComponent component, MenuCommands commands);
		/// <summary>
		/// Creates a MenuCommand for the specified component
		/// </summary>
		MenuCommand CreateMenuCommand(IEditorComponent component, string id, string title);
		/// <summary>
		/// Removes a set of previously added commands from the editor main menus
		/// </summary>
		void RemoveMenuItems(MenuCommands commands);
		/// <summary>
		/// Gets the Project Tree controller which provides access to
		/// adding and removing nodes from the main tree.
		/// </summary>
		IProjectTree ProjectTree { get; }
		/// <summary>
		/// The menu ID of the File menu
		/// </summary>
		string FileMenuID { get; }
		/// <summary>
		/// Creates a script editor control at the specified position and size.
		/// This allows you to embed a script editor onto your pane.
		/// </summary>
		IScriptEditorControl CreateScriptEditor(Point position, Size size);
		/// <summary>
		/// Opens the script editor for the specified script file and positions
		/// the cursor on the specified line number.
		/// RequiredAGSVersion: 3.0.1.24.
		/// </summary>
		void OpenEditorForScript(string fileName, int lineNumber);
		/// <summary>
		/// Launches the AGS Help File and attempts to display the specified keyword.
		/// RequiredAGSVersion: 3.0.1.26.
		/// </summary>
		void ShowHelpFile(string indexKeyword);
		/// <summary>
		/// Renders the specified sprite to the Graphics context, resized to fit within
		/// the specified width/height and with aspect ratio maintained.
		/// RequiredAGSVersion: 3.0.1.34
		/// </summary>
		void DrawSprite(Graphics g, int spriteNumber, int x, int y, int width, int height, bool centreHorizontally);
        /// <summary>
        /// Displays the supplied text on the editor status bar.
        /// RequiredAGSVersion: 3.0.2.38
        /// </summary>
        void SetStatusBarText(string text);
        /// <summary>
        /// Displays a pop-up dialog asking the user to select a sprite.
        /// Returns the selected sprite, or null if they cancel.
        /// RequiredAGSVersion: 3.0.2.38
        /// </summary>
        Sprite ShowSpriteSelector(int initiallySelectedSpriteNumber);
        /// <summary>
        /// Retrieves the pane that is currently selected. This could be null if
        /// no tabs are open.
        /// RequiredAGSVersion: 3.2.0.94
        /// </summary>
        ContentDocument ActivePane { get; }
        /// <summary>
        /// Repopulate the tree view of a selected component
        /// RequiredAGSVersion: 3.3.0.1145
        /// </summary>
        /// <param name="component">Reference to a component</param>
        /// <param name="selectedNode">Optional node name to select after refresh</param>
        void RePopulateTreeView(IEditorComponent component, string selectedNode);
        /// <summary>
        /// Repopulate the tree view of a selected component
        /// RequiredAGSVersion: 3.3.0.1145
        /// </summary>
        /// <param name="component">Reference to a component</param>
        void RePopulateTreeView(IEditorComponent component);
    }
}
