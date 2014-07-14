using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class UnloadedRoomFolders : FolderListHybrid<IRoom, UnloadedRoomFolder>
    {
        public UnloadedRoomFolders() : base(new UnloadedRoomFolder()) { }

        public UnloadedRoomFolders(string name) : base(new UnloadedRoomFolder(name)) { }

        public UnloadedRoomFolders(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(new UnloadedRoomFolder(node, parentNodeForBackwardsCompatability)) { }
    }

    public class UnloadedRoomFolder : BaseFolderCollection<IRoom, UnloadedRoomFolder>
    {
        public const string MAIN_UNLOADED_ROOM_FOLDER_NAME = "Main";

        public UnloadedRoomFolder(string name) : base(name) { }

        public UnloadedRoomFolder() : this("Default") { }

        public UnloadedRoomFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(node, parentNodeForBackwardsCompatability) { }

        private UnloadedRoomFolder(XmlNode node) : base(node) { }

        public override UnloadedRoomFolder CreateChildFolder(string name)
        {
            return new UnloadedRoomFolder(name);
        }

        protected override string OverrideXmlItemListNodeName()
        {
            return "UnloadedRooms";
        }

        public IRoom FindUnloadedRoomByID(int unloadedRoomNumber, bool recursive)
        {
            return FindItem(IsItem, unloadedRoomNumber, recursive);
        }

        protected override void FromXmlBackwardsCompatability(System.Xml.XmlNode parentNodeForBackwardsCompatability)
        {
            Init(MAIN_UNLOADED_ROOM_FOLDER_NAME);
            foreach (XmlNode unloadedRoomNode in SerializeUtils.GetChildNodes(parentNodeForBackwardsCompatability, "Rooms"))
            {
                _items.Add(new UnloadedRoom(unloadedRoomNode));
            }
        }

        protected override UnloadedRoomFolder CreateFolder(XmlNode node)
        {
            return new UnloadedRoomFolder(node);
        }

        protected override IRoom CreateItem(XmlNode node)
        {
            return new UnloadedRoom(node);
        }

        private bool IsItem(IRoom unloadedRoom, int unloadedRoomNumber)
        {
            return unloadedRoom.Number == unloadedRoomNumber;
        }

    }
}
