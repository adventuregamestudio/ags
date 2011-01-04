using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

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
    }
}
