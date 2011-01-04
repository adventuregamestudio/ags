using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class PaletteComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Palette";

        private PaletteEditor _palEditor;
        private ContentDocument _document;

        public PaletteComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _palEditor = new PaletteEditor();
            RecreateDocument();
            _guiController.RegisterIcon("PaletteIcon", Resources.ResourceManager.GetIcon("iconpal.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Colours", "PaletteIcon");
        }

        private void RecreateDocument()
        {
            _document = new ContentDocument(_palEditor, "Colours", this, ConstructPropertyObjectList());
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Palette; }
        }

        public override void CommandClick(string controlID)
        {
            _guiController.AddOrShowPane(_document);
            _palEditor.OnShow();
			_guiController.ShowCuppit("The Colours window allows you to find out the AGS Colour Number for a colour, and also to set up your palette in 256-colour games.", "Colours introduction");
		}

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == PaletteEntry.PROPERTY_COLOR_TYPE)
            {
                // ensure the sprite remapping code knows which colours are available
                Factory.NativeProxy.GameSettingsChanged(_agsEditor.CurrentGame);
            }
            else if (propertyName == PaletteEntry.PROPERTY_COLOR_RGB)
            {
                // ensure that any currently open room backgrounds are repainted
                // using the new colours
                Factory.NativeProxy.PaletteColoursChanged(_agsEditor.CurrentGame);
            }
        }

        public override void RefreshDataFromGame()
        {
            _guiController.RemovePaneIfExists(_document);
            RecreateDocument();
            _palEditor.GameChanged();
        }

        public override void GameSettingsChanged()
        {
            _palEditor.GameChanged();
        }

        private Dictionary<string, object> ConstructPropertyObjectList()
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            for (int i = 0; i < _agsEditor.CurrentGame.Palette.Length; i++)
            {
                list.Add("Palette entry " + i.ToString() + " (Palette entry)", _agsEditor.CurrentGame.Palette[i]);
            }
            return list;
        }

    }
}
