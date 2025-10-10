﻿using System;
using System.IO;
using System.Text;
using System.Xml;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor.Components
{
    /// <summary>
    /// Manages serialization of the design-time room data.
    /// -----------------------------------------------------------------------
    /// [ivan-mogilko] As of now design-time information is stored separately
    /// from the room objects, inside the IRoomEditorFilter classes. This is a
    /// temporary solution based on @tzachshabtay's original UI code. Partially
    /// the reason I kept this approach is that I was not confident in the best
    /// direction at this time.
    /// 
    /// In the future it would be a good idea to develop a concept of a
    /// design-time only properties "layer" over game objects. The question is
    /// how these properties should be implemented. They could be added
    /// directly in the entity classes, but in such case there has to be a way
    /// to distinct which of the properties are serialized as a game data and
    /// which as "user" data (property attributes?). On the other hand they
    /// could be implemented as a separate class paired with game entity and
    /// linked by some kind of a table, or attached otherwise (as component).
    /// </summary>
    public static class RoomDesignData
    {
        private const string XML_DATA_ROOT_NODE = "AGSRoomUserData";
        private const string XML_ROOM_NODE = "Room";
        private const string XML_ROOM_SELECTION = "SelectedItem";
        private const string XML_SELITEM_ATTRIB_PATH = "BreadcrumbPath";
        private const string XML_ROOM_LAYERS_NODE = "Layers";
        private const string XML_LAYER_ELEM_VISIBLE = "Visible";
        private const string XML_LAYER_ELEM_LOCKED = "Locked";
        private const string XML_LAYER_ITEMS_NODE = "Items";
        private const string XML_LAYER_ITEM_NODE = "Item";
        private const string XML_ITEM_ATTRIB_ID = "ID";
        private const int CURRENT_DATA_VERSION_INDEX = 2;

        /// <summary>
        /// Load a previously saved design-time user data for the current room.
        /// </summary>
        public static void LoadFromUserFile(Room room, RoomSettingsEditor editor)
        {
            string fileName = room.UserFileName;

            // User file missing is a valid case, and we simply skip deserialization
            if (!File.Exists(fileName))
                return;

            XmlNode docNode = null;
            try
            {
                XmlDocument doc = new XmlDocument();
                doc.Load(fileName);
                // Copy header style from Game.agf and Game.agf.user
                // TODO: unify all xml header read/write?
                if (doc.DocumentElement.Name != XML_DATA_ROOT_NODE)
                {
                    throw new AGSEditorException("Invalid room user data file. The file was corrupted, edited by hand or saved by incompatible version of AGS.");
                }
                string userDataSavedWithEditorVersion = null;
                XmlAttribute editorVersionNode = doc.DocumentElement.Attributes[AGSEditor.XML_ATTRIBUTE_EDITOR_VERSION];
                if (editorVersionNode != null)
                {
                    userDataSavedWithEditorVersion = editorVersionNode.InnerText;
                }
                int? versionIndex = null;
                XmlAttribute versionIndexNode = doc.DocumentElement.Attributes[AGSEditor.XML_ATTRIBUTE_VERSION_INDEX];
                if (versionIndexNode != null)
                    versionIndex = Convert.ToInt32(versionIndexNode.InnerText);
                if (!versionIndex.HasValue || (versionIndex < 1) || (versionIndex > CURRENT_DATA_VERSION_INDEX))
                {
                    throw new AGSEditorException("This game's user data file is from " +
                        ((userDataSavedWithEditorVersion == null) ? "a newer version" : ("version " + userDataSavedWithEditorVersion))
                        + " of AGS or an unsupported beta version. Please check the AGS website for a newer version of the editor.");
                }
                docNode = doc.DocumentElement;

                // Now parse actual data
                LoadDataFromXML(room, editor, docNode);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowError("Unable to read the room design-time preferences. This SHOULD NOT affect the actual game data, but the design-time state of all room items will be reset.", ex, MessageBoxIcon.Warning);
            }
        }

        /// <summary>
        /// Loads room user data from the given XML node.
        /// </summary>
        /// <param name="room"></param>
        /// <param name="editor"></param>
        /// <param name="node"></param>
        private static void LoadDataFromXML(Room room, RoomSettingsEditor editor, XmlNode node)
        {
            if (node == null)
                return;
            node = node.SelectSingleNode(XML_ROOM_NODE);
            if (node == null)
                return;
            XmlNode selectedObject = node.SelectSingleNode(XML_ROOM_SELECTION);
            if (selectedObject != null && selectedObject.Attributes[XML_SELITEM_ATTRIB_PATH] != null)
            {
                string selpath = selectedObject.Attributes[XML_SELITEM_ATTRIB_PATH].InnerText;
                if (!string.IsNullOrEmpty(selpath))
                {
                    editor.TrySelectNodeUsingDesignIDPath(selpath.Split('/'));
                }
            }
            node = node.SelectSingleNode(XML_ROOM_LAYERS_NODE);
            if (node == null)
                return;
            // Search for the layer node, if they exist then read layer preferences
            XmlNode collectionNode = node;
            foreach (IRoomEditorFilter layer in editor.Layers)
            {
                node = collectionNode.SelectSingleNode(layer.Name);
                if (node == null)
                    continue;

                // We do not have a distinct class which would only describe necessary fields,
                // so we do this part by hand for now, trying to be compliant with SerializeUtils.
                foreach (XmlNode child in node.ChildNodes)
                {
                    string elementName = child.Name;
                    string elementValue = child.InnerText;
                    if (elementName == XML_LAYER_ELEM_VISIBLE) layer.Visible = Convert.ToBoolean(elementValue);
                    else if (elementName == XML_LAYER_ELEM_LOCKED) layer.Locked = Convert.ToBoolean(elementValue);
                    else if (elementName == XML_LAYER_ITEMS_NODE)
                    {
                        foreach (XmlNode itemNode in child.ChildNodes)
                        {
                            string id = itemNode.Attributes[XML_ITEM_ATTRIB_ID].InnerText;
                            if (string.IsNullOrEmpty(id))
                                continue; // no ID, no way to apply the data
                            DesignTimeProperties props;
                            if (!layer.DesignItems.TryGetValue(id, out props))
                                continue; // no matching item
                            // Now read the design-time properties
                            SerializeUtils.DeserializePropertiesFromXML(props, itemNode);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Save design-time user data for the current room.
        /// </summary>
        public static void SaveToUserFile(Room room, RoomSettingsEditor editor, CompileMessages errors)
        {
            string fileName = room.UserFileName;

            StringWriter sw = new StringWriter();
            XmlTextWriter writer = new XmlTextWriter(sw);
            // Copy header style from Game.agf and Game.agf.user
            // TODO: unify all xml header read/write?
            writer.Formatting = Formatting.Indented;
            writer.WriteProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");
            writer.WriteComment("DO NOT EDIT THIS FILE. It is automatically generated by the AGS Editor, changing it manually could break your game");
            writer.WriteStartElement(XML_DATA_ROOT_NODE);
            writer.WriteAttributeString(AGSEditor.XML_ATTRIBUTE_VERSION_INDEX, CURRENT_DATA_VERSION_INDEX.ToString());
            writer.WriteAttributeString(AGSEditor.XML_ATTRIBUTE_EDITOR_VERSION, AGS.Types.Version.AGS_EDITOR_VERSION);

            SaveDataToXML(room, editor, writer);

            writer.WriteEndElement(); // Root node
            writer.Flush(); // Finish writing

            try
            {
                StreamWriter fileOutput = new StreamWriter(fileName, false, Types.Utilities.UTF8);
                fileOutput.Write(sw.ToString());
                fileOutput.Close();
                writer.Close();
            }
            catch (Exception ex)
            {
                // TODO: had to break long message into two separate warnings, because Output panel is not suited for line breaks.
                errors.Add(new CompileWarning("Unable to write the room design-time preferences. Ensure that you have write access to the game folder, and that the file is not already open"));
                errors.Add(new CompileWarning(ex.Message, ex));
            }
        }

        /// <summary>
        /// Saves room user data into the current XML node.
        /// </summary>
        /// <param name="room"></param>
        /// <param name="editor"></param>
        /// <param name="writer"></param>
        private static void SaveDataToXML(Room room, RoomSettingsEditor editor, XmlTextWriter writer)
        {
            // We do not have a distinct class which would only describe necessary fields,
            // so we do this part by hand for now, trying to be compliant with SerializeUtils.
            writer.WriteStartElement(XML_ROOM_NODE);

            var node = editor.CurrentNode as AGS.Editor.Panes.Room.RoomEditNode;
            string nodepath = "";
            while (node != null)
            {
                string id = string.IsNullOrEmpty(node.RoomItemID) ? node.UniqueID.ToString() : node.RoomItemID;
                nodepath = id + (string.IsNullOrEmpty(nodepath) ? "" : ("/" + nodepath));
                node = node.Parent as AGS.Editor.Panes.Room.RoomEditNode;
            }
            writer.WriteStartElement(XML_ROOM_SELECTION);
            writer.WriteAttributeString(XML_SELITEM_ATTRIB_PATH, nodepath);
            writer.WriteEndElement(); // Layer node

            writer.WriteStartElement(XML_ROOM_LAYERS_NODE);
            foreach (IRoomEditorFilter layer in editor.Layers)
            {
                writer.WriteStartElement(layer.Name);
                writer.WriteElementString(XML_LAYER_ELEM_VISIBLE, layer.Visible.ToString());
                writer.WriteElementString(XML_LAYER_ELEM_LOCKED, layer.Locked.ToString());
                writer.WriteStartElement(XML_LAYER_ITEMS_NODE);
                foreach (var item in layer.DesignItems)
                {
                    writer.WriteStartElement(XML_LAYER_ITEM_NODE);
                    writer.WriteAttributeString(XML_ITEM_ATTRIB_ID, item.Key);
                    // Let our generic serializer handle this
                    SerializeUtils.SerializePropertiesToXML(item.Value, writer);
                    writer.WriteEndElement(); // item node
                }
                writer.WriteEndElement(); // Items collection node
                writer.WriteEndElement(); // Layer node
            }
            writer.WriteEndElement(); // Layers collection node
            writer.WriteEndElement(); // Room node
        }
    }
}
