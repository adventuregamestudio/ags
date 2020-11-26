using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Describes event of character having ID changed (usually in case of deleting other character).
    /// </summary>
    public class CharacterIDChangedEventArgs : EventArgs
    {
        /// <summary>
        /// A character whose ID got changed. Character.ID should already be a new ID.
        /// </summary>
        public Character Character { get; private set; }
        /// <summary>
        /// Previous character's ID, for the reference.
        /// </summary>
        public int OldID { get; private set; }

        public CharacterIDChangedEventArgs(Character character, int oldID)
        {
            Character = character;
            OldID = oldID;
        }
    }
}
