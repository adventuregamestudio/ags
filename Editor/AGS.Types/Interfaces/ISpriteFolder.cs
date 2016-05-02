using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Provides access to sprites and their folders.
    /// </summary>
    public interface ISpriteFolder
    {
        /// <summary>
        /// The name of this sprite folder.
        /// </summary>
        string Name { get; set; }

        /// <summary>
        /// Sub-folders contained within this folder.
        /// </summary>
        IList<ISpriteFolder> SubFolders { get; }

        /// <summary>
        /// Sprites in this folder.
        /// </summary>
        IList<Sprite> Sprites { get; }

        /// <summary>
        /// Finds the Sprite object for the specified sprite number.
        /// Returns null if the sprite is not found.
        /// </summary>
        /// <param name="spriteNumber">Sprite number to look for</param>
        /// <param name="recursive">Whether to also search sub-folders</param>
        Sprite FindSpriteByID(int spriteNumber, bool recursive);

        /// <summary>
        /// Finds the SpriteFolder object for the folder that contains the sprite.
        /// Returns null if the sprite is not found.
        /// </summary>
        /// <param name="spriteNumber">Sprite number to look for</param>
        SpriteFolder FindFolderThatContainsSprite(int spriteNumber);

        /// <summary>
        /// Assembles a list of all the sprites in the current folder and sub-folders
        /// </summary>
        /// <returns>
        /// Returns an IList with all sprites from this folder and sub-folders
        /// </returns>
        IList<Sprite> GetAllSpritesFromAllSubFolders();
    }
}
