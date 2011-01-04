using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace AGS.Types
{
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
