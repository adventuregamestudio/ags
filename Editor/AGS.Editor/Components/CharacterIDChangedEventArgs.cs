using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Describes event of character having ID changed (usually in case of deleting other character).
    /// </summary>
    public class CharacterIDChangedEventArgs : EventArgs
    {
        public Character Character { get; private set; }
        public int OldID { get; private set; }

        public CharacterIDChangedEventArgs(Character character, int oldID)
        {
            Character = character;
            OldID = oldID;
        }
    }
}
