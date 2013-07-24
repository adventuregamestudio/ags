using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class CharacterFolders : FolderListHybrid<Character, CharacterFolder>
    {
        public CharacterFolders() : base(new CharacterFolder()) { }

        public CharacterFolders(string name) : base(new CharacterFolder(name)) { }

        public CharacterFolders(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(new CharacterFolder(node, parentNodeForBackwardsCompatability)) { }
    }

    public class CharacterFolder : BaseFolderCollection<Character, CharacterFolder>
    {
        public const string MAIN_CHARACTER_FOLDER_NAME = "Main";

        public CharacterFolder(string name) : base(name) { }

        public CharacterFolder() : this("Default") { }

        public CharacterFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability) : 
            base(node, parentNodeForBackwardsCompatability) { }

        private CharacterFolder(XmlNode node) : base(node) { }

        public override CharacterFolder CreateChildFolder(string name)
        {
            return new CharacterFolder(name);
        }

        public Character FindCharacterByID(int charID, bool recursive)
        {
            return FindItem(IsItem, charID, recursive);
        }

        protected override void FromXmlBackwardsCompatability(XmlNode parentNodeForBackwardsCompatability)
        {
            Init(MAIN_CHARACTER_FOLDER_NAME);
            foreach (XmlNode invNode in SerializeUtils.GetChildNodes(parentNodeForBackwardsCompatability, "Characters"))
            {
                _items.Add(CreateItem(invNode));
            }           
        }

        protected override CharacterFolder CreateFolder(XmlNode node)
        {
            return new CharacterFolder(node);
        }

        protected override Character CreateItem(XmlNode node)
        {
            return new Character(node);
        }

        private bool IsItem(Character character, int charID)
        {
            return character.ID == charID;
        }
    }
}
