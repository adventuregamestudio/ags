using System;
using System.IO;
using System.Windows.Forms;
using AGS.Editor.Preferences;

namespace AGS.Editor
{
    public enum WelcomeScreenSelection
    {
        StartNewGame,
        LoadExistingGame,
        ContinueRecentGame
    }

    public partial class WelcomeScreen : Form
    {
        public WelcomeScreen()
        {
            InitializeComponent();
            labelVersion.Text = $"Build {AGS.Types.Version.AGS_EDITOR_VERSION} {AGS.Types.Version.AGS_EDITOR_TARGETNAME}, {AGS.Types.Version.AGS_EDITOR_DATE}";

            foreach (RecentGame game in Factory.AGSEditor.Settings.RecentGames)
            {
                if (Directory.Exists(game.Path))
                {
                    lstRecentGames.Items.Add(game.Name).SubItems.Add(game.Path);
                }
            }

            if (lstRecentGames.Items.Count == 0)
            {
                radRecent.Enabled = false;
                lstRecentGames.Enabled = false;
                radNewGame.Checked = true;
            }
            else
            {
                radRecent.Checked = true;
                lstRecentGames.SelectedIndices.Add(0);
            }
        }

        public string GetSelectedRecentGamePath()
        {
            return lstRecentGames.SelectedItems[0].SubItems[1].Text;
        }

        public WelcomeScreenSelection SelectedOption
        {
            get { return (radNewGame.Checked) ? WelcomeScreenSelection.StartNewGame : (radLoadGame.Checked ? WelcomeScreenSelection.LoadExistingGame : WelcomeScreenSelection.ContinueRecentGame); }
        }

        private void lstRecentGames_ItemActivate(object sender, EventArgs e)
        {
            btnContinue_Click(sender, e);
        }

        private void btnContinue_Click(object sender, EventArgs e)
        {
            if ((radRecent.Checked) && (lstRecentGames.SelectedIndices.Count == 0))
            {
                MessageBox.Show("You must select a game to edit.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void radRecent_CheckedChanged(object sender, EventArgs e)
        {
            lstRecentGames.Enabled = radRecent.Checked;
        }
    }
}