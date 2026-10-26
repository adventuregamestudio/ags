using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using AGS.Editor.Resources;
using AGS.Types;

namespace AGS.Editor.Components
{
    public abstract class BaseComponent : IEditorComponent
    {
        public abstract string ComponentID { get; }

        protected GUIController _guiController;
        protected AGSEditor _agsEditor;

        protected BaseComponent(GUIController guiController, AGSEditor agsEditor)
        {
            _guiController = guiController;
            _agsEditor = agsEditor;

            _guiController.RegisterIcon("FixedFolderIcon", ResourceManager.GetIcon("folder_locked.ico"));
        }

        public virtual void CommandClick(string controlID)
        {
        }

        public virtual IList<MenuCommand> GetContextMenu(string controlID)
        {
            return null;
        }

        public virtual void PropertyChanged(string propertyName, object oldValue)
        {
        }

        public virtual void BeforeSaveGame()
        {
        }

        public virtual void RefreshDataFromGame()
        {
        }

        public virtual void GameSettingsChanged()
        {
        }

        public virtual void ToXml(System.Xml.XmlTextWriter writer)
        {
        }

        public virtual void FromXml(System.Xml.XmlNode node)
        {
        }

        public virtual void EditorShutdown()
        {
        }

        public virtual bool ShowItemPaneByName(string name)
        {
            return false;
        }

        public virtual IList<string> GetManagedScriptElements()
        {
            return new string[] {};
        }

        // TODO: we need a intermediate class of BaseComponent of ObjectType,
        // which could be a parent to both BaseComponentWithFolders and any
        // other specific Component that does not have folders (e.g. CursorsComponent).
        // When we have one: move the SafeSwapItemIDs (and any similar helpers) there.

        /// <summary>
        /// A helper method for Components, that swaps two game objects in the flat list,
        /// exchanging their IDs. If any of the two objects does not exist, but its index
        /// is valid (within List), then swaps the only existing object with empty slot.
        /// If at least one of the indexes is invalid (outside of List's contents), then fails.
        /// If neither objects exist, then fails.
        /// Returns whether the swap (or ID reassignment) was successful.
        /// </summary>
        protected bool SafeSwapItemIDs<TItem>(IList<TItem> items, int index1, int index2)
            where TItem : IHasID
        {
            var item1 = items.FirstOrDefault((TItem item) => { return item != null && item.ID == index1; });
            var item2 = items.FirstOrDefault((TItem item) => { return item != null && item.ID == index2; });
            if (item1 == null && item2 == null)
                return false; // nothing to swap

            // Swap positions first, reassign IDs only if successful;
            // note that this may swap a single existing object with null.
            if (!items.Swap(index1, index2))
                return false;
            if (item1 != null)
                item1.ID = index2;
            if (item2 != null)
                item2.ID = index1;
            return true;
        }
    }
}
