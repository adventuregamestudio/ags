using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
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

            foreach (RecentGame game in Factory.AGSEditor.Settings.RecentGames)
            {
                lstRecentGames.Items.Add(game.Name).SubItems.Add(game.Path);
            }

            if (lstRecentGames.Items.Count == 0)
            {
                radRecent.Enabled = false;
                lstRecentGames.Enabled = false;
                radNewGame.Checked = true;
            }
            else
            {
                lstRecentGames.SelectedIndices.Add(0);
            }
        }

        public string GetSelectedRecentGamePath()
        {
            string gameName = lstRecentGames.SelectedItems[0].Text;
            RecentGame game = Factory.AGSEditor.Settings.RecentGames.Find(
                delegate(RecentGame rg) { return rg.Name == gameName; });

            return game.Path; 
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