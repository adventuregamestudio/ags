using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IGame
    {
        string DirectoryPath { get; }
        IList<GUI> GUIs { get; }
        IList<InventoryItem> InventoryItems { get; }
        IList<Character> Characters { get; }
        IList<Dialog> Dialogs { get; }
        IList<MouseCursor> Cursors { get; }
        IList<Font> Fonts { get; }
        IList<Translation> Translations { get; }
        IList<IRoom> Rooms { get; }
        /// <summary>
        /// Accesses the root view folder, which contains all views
        /// at the top level as well as sub folders.
        /// RequiredAGSVersion: 3.0.1.33
        /// </summary>
        IViewFolder Views { get; }
        /// <summary>
        /// Accesses the root sprite folder, which contains all
        /// sprites at the top level, and subfolders.
        /// RequiredAGSVersion: 3.0.2.38
        /// </summary>
        ISpriteFolder Sprites { get; }
        Scripts Scripts { get; }
        Settings Settings { get; }
        /// <summary>
        /// Creates a new view in the specified folder, refreshes the project
        /// tree and returns the new view.
        /// </summary>
        View CreateNewView(IViewFolder createInFolder);
        /// <summary>
        /// Gets the list of ScriptandHeader
        /// RequiredAGSVersion: 3.3.0.1145
        /// </summary>
        ScriptsAndHeaders ScriptsAndHeaders { get; }
        /// <summary>
        /// Gets the player character
        /// RequiredAGSVersion: 3.3.0.1147
        /// </summary>
        Character PlayerCharacter { get; }
    }
}
