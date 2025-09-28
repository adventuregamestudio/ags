using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class CharacterEditor : EditorContentPanel
    {
        private Character _character;
        private System.Drawing.Font _normalFont;
        private System.Drawing.Font _boldFont;

        public CharacterEditor()
        {
            InitializeComponent();
            _normalFont = lblIsPlayer.Font;
            _boldFont = new System.Drawing.Font(_normalFont.Name, _normalFont.Size, FontStyle.Bold);
            viewPreview1.AutoResize = true;
            viewPreview2.AutoResize = true;
            viewPreview1.ZoomLevel = sldZoomLevel.ZoomScale;
            viewPreview2.ZoomLevel = sldZoomLevel.ZoomScale;
        }

        public CharacterEditor(Character characterToEdit) : this()
        {
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            _character = characterToEdit;
            viewPreview1.IsCharacterView = true;
            viewPreview2.IsCharacterView = true;
            viewPreview1.DynamicUpdates = true;
            viewPreview2.DynamicUpdates = true;
            UpdateActivateCharacterText();
            UpdateViewPreview();
        }

        public Character ItemToEdit
        {
            get { return _character; }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Character Editor";
        }
        
        public void UpdateViewPreview()
        {
            viewPreview1.ViewToPreview = Factory.AGSEditor.CurrentGame.FindViewByID(_character.NormalView);
            viewPreview2.ViewToPreview = Factory.AGSEditor.CurrentGame.FindViewByID(_character.SpeechView);
        }

        private void UpdateActivateCharacterText()
        {
            if (Factory.AGSEditor.CurrentGame.PlayerCharacter == _character)
            {
                lblIsPlayer.Text = "This character is the player character. The game will start in this character's starting room.";
                btnMakePlayer.Enabled = false;
                lblIsPlayer.Font = _boldFont;
            }
            else
            {
                lblIsPlayer.Text = "This character is not the player character.";
                btnMakePlayer.Enabled = true;
                lblIsPlayer.Font = _normalFont;
            }
        }

        protected override void OnWindowActivated()
        {
            UpdateActivateCharacterText();
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            UpdateViewPreview();
        }

        private void btnMakePlayer_Click(object sender, EventArgs e)
        {
            if (Factory.GUIController.ShowQuestion("Are you sure you want to make this the player character? If so, the game will start off in this character's Starting Room and the player will be controlling this character.") == DialogResult.Yes)
            {
                Factory.AGSEditor.CurrentGame.PlayerCharacter = _character;
                UpdateActivateCharacterText();
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "character-editor");
            t.GroupBoxHelper(groupBox1, "character-editor/box");
            t.ButtonHelper(btnMakePlayer, "character-editor/btn-make");
        }

        private void CharacterEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
        
        private void sldZoomLevel_ValueChanged(object sender, EventArgs e)
        {
            viewPreview1.ZoomLevel = sldZoomLevel.ZoomScale;
            viewPreview2.ZoomLevel = sldZoomLevel.ZoomScale;
        }
    }
}
