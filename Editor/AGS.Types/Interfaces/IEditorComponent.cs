using System.Collections.Generic;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// NOTE: This interface is used by plugins, do not modify it or existing
    /// plugins will break!
    /// </summary>
	public interface IEditorComponent
	{
		string ComponentID { get; }
		void CommandClick(string controlID);
		IList<MenuCommand> GetContextMenu(string controlID);
		void PropertyChanged(string propertyName, object oldValue);
		void BeforeSaveGame();
		void RefreshDataFromGame();
		void GameSettingsChanged();
		void ToXml(XmlTextWriter writer);
		void FromXml(XmlNode node);
		void EditorShutdown();
	}
}
