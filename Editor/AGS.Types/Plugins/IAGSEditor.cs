using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
	public delegate void GetScriptHeaderListHandler(GetScriptHeaderListEventArgs evArgs);
    public delegate void GetScriptModuleListHandler(GetScriptModuleListEventArgs evArgs);

	public interface IAGSEditor
	{
		/// <summary>
		/// Adds a new component to the editor. Once added, it cannot be removed
		/// </summary>
		void AddComponent(IEditorComponent component);
		/// <summary>
		/// Returns the GUI controller, which provides access to various
		/// aspects of the editor GUI.
		/// </summary>
		IGUIController GUIController { get; }
		/// <summary>
		/// Returns the game which is loaded into the editor
		/// </summary>
		IGame CurrentGame { get; }
		/// <summary>
		/// Editor version number
		/// </summary>
		string Version { get; }
		/// <summary>
		/// Allows you to add extra built-in script headers to the compiler.
		/// You can't add extra functions because your plugin will not be
		/// loaded at run-time, but you can add enums and #defines.
		/// </summary>
		event GetScriptHeaderListHandler GetScriptHeaderList;
		/// <summary>
		/// Re-constructs the autocomplete data for the specified script.
		/// </summary>
		void RebuildAutocompleteCache(Script script);
		/// <summary>
		/// Retrieves a list of all the script headers that will be passed
		/// to the compiler, in the correct order.
		/// </summary>
		IList<Script> GetAllScriptHeaders();
		/// <summary>
		/// Gets the image for the specified sprite number.
		/// RequiredAGSVersion: 3.0.1.31.
		/// </summary>
		Bitmap GetSpriteImage(int spriteNumber);
        /// <summary>
        /// Replaces the specified sprite with a new image.
        /// RequiredAGSVersion: 3.0.2.38
        /// </summary>
        void ChangeSpriteImage(int spriteNumber, Bitmap newImage, SpriteImportTransparency transparencyType, bool useAlphaChannel);
        /// <summary>
        /// Creates a new sprite in the specified folder, using the
        /// supplied image, and returns it.
        /// RequiredAGSVersion: 3.0.2.38
        /// </summary>
        Sprite CreateNewSprite(ISpriteFolder inFolder, Bitmap newImage, SpriteImportTransparency transparencyType, bool useAlphaChannel);
        /// <summary>
        /// Deletes the specified sprite from the game.
        /// RequiredAGSVersion: 3.0.2.40
        /// </summary>
        /// <exception cref="AGS.Types.SpriteInUseException" />
        void DeleteSprite(int spriteNumber);
        /// <summary>
        /// Gets a text report of places where the sprite is in use. This 
        /// cannot automatically detect uses of the sprite in scripts.
        /// Returns null if no uses of the sprite could be detected.
        /// RequiredAGSVersion: 3.0.2.40
        /// </summary>
        string GetSpriteUsageReport(int spriteNumber);
		/// <summary>
		/// Gets the Source Control Integration object, which allows you
		/// to perform source control-related operations.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		ISourceControlIntegration SourceControl { get; }
		/// <summary>
		/// Gets the RoomController, which provides methods to manipulate
		/// the currently loaded room file.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		IRoomController RoomController { get; }
        /// <summary>
        /// Allows you to set the SourceControlProvider via a plugin, for
        /// example, in a plugin component's constructor. This allows you to
        /// replace the default provider AGS uses without having to change it
        /// in the OS. For other source control functions use the
        /// SourceControl property instead.
        /// RequiredAGSVersion: 3.2.2.112
        /// </summary>
        ISourceControlProvider SourceControlProvider { set; }
	}
}
