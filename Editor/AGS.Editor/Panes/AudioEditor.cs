﻿using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class AudioEditor : EditorContentPanel
    {
        private delegate void NoParametersDelegate();

        private object _selectedItem = null;
        private IAudioPreviewer _previewer = null;
        private bool _paused = false;
        private Timer _timer;
        private object _timerLock = new object();

        public AudioEditor()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

        public object SelectedItem
        {
            get { return _selectedItem; }
            set { _selectedItem = value; SetupGUIForNewItem(value); }
        }

        private AudioClip SelectedClip
        {
            get { return (AudioClip)_selectedItem; }
        }

        private void SetupGUIForNewItem(object selectedItem)
        {
            StopAnyPlayingSound();

            grpAudioClip.Visible = false;
            grpAudioType.Visible = false;
            grpFolder.Visible = false;

            if (selectedItem is AudioClip)
            {
                lblSoundName.Text = "Audio Clip: " + ((AudioClip)selectedItem).ScriptName;
                lblCurrentPosition.Text = "";
                btnPlay.Enabled = true;
                grpAudioClip.Visible = true;
            }
            else if (selectedItem is AudioClipFolder)
            {
                lblFolderTitle.Text = "Folder: " + ((AudioClipFolder)selectedItem).Name;
                grpFolder.Visible = true;
            }
            else if (selectedItem is AudioClipType)
            {
                lblAudioTypeTitle.Text = "Audio Type: " + ((AudioClipType)selectedItem).Name;
                grpAudioType.Visible = true;
            }
        }

        private void StopAnyPlayingSound()
        {
            if (_previewer != null)
            {
                _previewer.Stop();
                _previewer = null;
            }
            lock (_timerLock)
            {
                if (_timer != null)
                {
                    _timer.Dispose();
                    _timer = null;
                }
            }
        }

        private void btnPlay_Click(object sender, EventArgs e)
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                if (this.SelectedClip.FileType == AudioClipFileType.MIDI)
                {
                    _previewer = new MidiPlayer();
                }
                else
                {
                    _previewer = new IrrklangPlayer();
                }
                _previewer.PlayFinished += new PlayFinishedHandler(_previewer_PlayFinished);
                _paused = false;
                _timer = new Timer();
                _timer.Interval = 900;
                _timer.Tick += new EventHandler(_timer_Tick);
                _timer.Start();

                if (_previewer.Play(this.SelectedClip))
                {
                    UpdateCurrentTime();
                    btnPlay.Enabled = false;
                    btnPause.Enabled = true;
                    btnStop.Enabled = true;
                }
                else
                {
                    Factory.GUIController.ShowMessage("Unable to play the sound. The file format may not be supported by the AGS Editor.", MessageBoxIcon.Warning);
                }
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
        }

        private void UpdateCurrentTime()
        {
            if (_previewer != null)
            {
                _previewer.Poll();
                int currentPos = _previewer.GetPositionMs();
                int currentLen = _previewer.GetLengthMs();
                lblCurrentPosition.Text = string.Format("{0:00}:{1:00} / {2:00}:{3:00}",
                                                (currentPos / 1000) / 60, (currentPos / 1000) % 60,
                                                (currentLen / 1000) / 60, (currentLen / 1000) % 60);
            }
            else
            {
                lblCurrentPosition.Text = "";
            }
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            this.Invoke(new NoParametersDelegate(UpdateCurrentTime));
        }

        protected override void OnPanelClosing(bool canCancel, ref bool cancelClose)
        {
            StopAnyPlayingSound();
        }

        private void ResetControlsForSoundFinished()
        {
            btnPlay.Enabled = true;
            btnPause.Enabled = false;
            btnStop.Enabled = false;
            lock (_timerLock)
            {
                if (_timer != null)
                {
                    _timer.Stop();
                    _timer.Dispose();
                    _timer = null;
                }
            }
            UpdateCurrentTime();
        }

        private void _previewer_PlayFinished(AudioClip clip)
        {
            btnPlay.Invoke(new NoParametersDelegate(ResetControlsForSoundFinished));
        }

        private void btnPause_Click(object sender, EventArgs e)
        {
            if ((_previewer != null) && (_paused))
            {
                _previewer.Resume();
                _paused = false;
            }
            else if (_previewer != null)
            {
                _previewer.Pause();
                _paused = true;
            }
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            if (_previewer != null)
            {
                _previewer.Stop();
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("audio-editor/background");
            ForeColor = t.GetColor("audio-editor/foreground");
            grpAudioType.BackColor = t.GetColor("audio-editor/audio-type/background");
            grpAudioType.ForeColor = t.GetColor("audio-editor/audio-type/foreground");
            grpAudioClip.BackColor = t.GetColor("audio-editor/audio-clip-box/background");
            grpAudioClip.ForeColor = t.GetColor("audio-editor/audio-clip-box/foreground");
            btnPlay.BackColor = t.GetColor("audio-editor/btn-play/background");
            btnPlay.ForeColor = t.GetColor("audio-editor/btn-play/foreground");
            btnPlay.FlatStyle = (FlatStyle)t.GetInt("audio-editor/btn-play/flat/style");
            btnPlay.FlatAppearance.BorderSize = t.GetInt("audio-editor/btn-play/flat/border/size");
            btnPlay.FlatAppearance.BorderColor = t.GetColor("audio-editor/btn-play/flat/border/color");
            btnPause.BackColor = t.GetColor("audio-editor/btn-pause/background");
            btnPause.ForeColor = t.GetColor("audio-editor/btn-pause/foreground");
            btnPause.FlatStyle = (FlatStyle)t.GetInt("audio-editor/btn-pause/flat/style");
            btnPause.FlatAppearance.BorderSize = t.GetInt("audio-editor/btn-pause/flat/border/size");
            btnPause.FlatAppearance.BorderColor = t.GetColor("audio-editor/btn-pause/flat/border/color");
            btnStop.BackColor = t.GetColor("audio-editor/btn-stop/background");
            btnStop.ForeColor = t.GetColor("audio-editor/btn-stop/foreground");
            btnStop.FlatStyle = (FlatStyle)t.GetInt("audio-editor/btn-stop/flat/style");
            btnStop.FlatAppearance.BorderSize = t.GetInt("audio-editor/btn-stop/flat/border/size");
            btnStop.FlatAppearance.BorderColor = t.GetColor("audio-editor/btn-stop/flat/border/color");
        }
    }
}
