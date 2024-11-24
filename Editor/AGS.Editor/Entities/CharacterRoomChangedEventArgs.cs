using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Describes event of character having StartingRoom changed.
    /// </summary>
    public class CharacterRoomChangedEventArgs : EventArgs
    {
        public Character Character { get; private set; }
        public int PreviousRoom { get; private set; }

        public CharacterRoomChangedEventArgs(Character character, int oldRoom)
        {
            Character = character;
            PreviousRoom = oldRoom;
        }
    }
}
