using AGS.Types;
using System;

namespace AGS.Editor
{
    public class CharacterDescriptionChangedEventArgs
    {
        /// <summary>
        /// A character whose Description (aka Real Name) got changed.
        /// </summary>
        public Character Character { get; private set; }
        /// <summary>
        /// Previous character's Description, for the reference.
        /// </summary>
        public string OldDescription { get; private set; }

        public CharacterDescriptionChangedEventArgs(Character character, string oldDesc)
        {
            Character = character;
            OldDescription = oldDesc;
        }
    }
}
