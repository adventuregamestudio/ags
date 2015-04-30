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

        public CharacterEditor(Character characterToEdit)
        {
            InitializeComponent();
            this.LoadColorTheme();
            _character = characterToEdit;
            _normalFont = lblIsPlayer.Font;
            _boldFont = new System.Drawing.Font(_normalFont.Name, _normalFont.Size, FontStyle.Bold);
            viewPreview1.IsCharacterView = true;
            viewPreview2.IsCharacterView = true;
            UpdateActivateCharacterText();
            UpdateViewPreview();
        }

        public Character ItemToEdit
        {
            get { return _character; }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Characters";
        }
        
        private void UpdateViewPreview()
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

        private void LoadColorTheme()
        {
            ColorTheme colorTheme = Factory.GUIController.UserColorTheme;
            colorTheme.Color_EditorContentPanel(this);
            colorTheme.Color_GroupBox(this.groupBox1);
            colorTheme.Color_Button(this.btnMakePlayer);
        }
    }
}
