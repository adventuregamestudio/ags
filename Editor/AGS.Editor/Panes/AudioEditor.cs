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
        private object _selectedItem = null;
        private IAudioPreviewer _previewer = null;
        private bool _paused = false;
        private Timer _timer;
        private object _timerLock = new object();

        public AudioEditor()
        {
            InitializeComponent();
        }

        protected override string OnGetHelpKeyword()
        {
            return "Music and sound";
        }

        public object SelectedItem
        {
            get { return _selectedItem; }
            set { _selectedItem = value; SetupForNewItem(value); }
        }

        public int LengthMs
        {
            get
            {
                if (_selectedItem != null && _previewer != null)
                    return _previewer.GetLengthMs();
                return 0;
            }
        }

        private AudioClip SelectedClip
        {
            get { return (AudioClip)_selectedItem; }
        }

        private void SetupForNewItem(object selectedItem)
        {
            lock (_timerLock)
            {
                if (_timer != null)
                {
                    _timer.Dispose();
                    _timer = null;
                }
            }

            if (_previewer != null)
            {
                _previewer.Stop();
                _previewer.Dispose();
            }

            grpAudioClip.Visible = false;
            grpAudioType.Visible = false;
            grpFolder.Visible = false;

            if (selectedItem is AudioClip)
            {
                lblSoundName.Text = "Audio Clip: " + ((AudioClip)selectedItem).ScriptName;
                lblCurrentPosition.Text = "00:00";
                lblClipLength.Text = "/ 00:00";

                try
                {
                    if (this.SelectedClip.FileType == AudioClipFileType.MIDI)
                    {
                        _previewer = new MidiPlayer(this.SelectedClip);
                    }
                    else
                    {
                        _previewer = new IrrklangPlayer(this.SelectedClip);
                    }
                }
                catch (AGSEditorException ex)
                {
                    string message = ex.Message;
                    Factory.GUIController.ShowMessage(message, MessageBoxIcon.Warning);
                    btnPlay.Enabled = false;
                }

                if (_previewer != null)
                {
                    _previewer.PlayFinished += _previewer_PlayFinished;
                    btnPlay.Enabled = true;
                    grpAudioClip.Visible = true;

                    int length = _previewer.GetLengthMs();
                    lblClipLength.Text = string.Format("/ {0:00}:{1:00}", (length / 1000) / 60, (length / 1000) % 60);
                }
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

        private void btnPlay_Click(object sender, EventArgs e)
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                _paused = false;
                _timer = new Timer();
                _timer.Interval = 900;
                _timer.Tick += _timer_Tick;
                _timer.Start();
                _previewer.Play();
                btnPlay.Enabled = false;
                btnPause.Enabled = true;
                btnStop.Enabled = true;
            }
            catch (AGSEditorException ex)
            {
                string message = ex.Message;
                Factory.GUIController.ShowMessage(message, MessageBoxIcon.Warning);
                btnPlay.Enabled = false;
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
        }

        private void PollPreviewer()
        {
            if (_previewer != null)
            {
                _previewer.Poll();
                int position = _previewer.GetPositionMs();
                lblCurrentPosition.Text = string.Format("{0:00}:{1:00}", (position / 1000) / 60, (position / 1000) % 60);
            }
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            this.Invoke(new Action(PollPreviewer));
        }

        protected override void OnPanelClosing(bool canCancel, ref bool cancelClose)
        {
            if (_previewer != null)
            {
                _previewer.Stop();
                _previewer.Dispose();
            }
        }

        private void ResetControlsForSoundFinished()
        {
            btnPlay.Enabled = true;
            btnPause.Enabled = false;
            btnStop.Enabled = false;
            lblCurrentPosition.Text = "00:00";

            lock (_timerLock)
            {
                if (_timer != null)
                {
                    _timer.Stop();
                    _timer.Dispose();
                    _timer = null;
                }
            }
        }

        private void _previewer_PlayFinished(AudioClip clip)
        {
            btnPlay.Invoke(new Action(ResetControlsForSoundFinished));
        }

        private void btnPause_Click(object sender, EventArgs e)
        {
            if ((_previewer != null) && (_paused))
            {
                _paused = false;
                _previewer.Resume();
                _timer.Start();
            }
            else if (_previewer != null)
            {
                _timer.Stop();
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
            t.ControlHelper(this, "audio-editor");
            t.GroupBoxHelper(grpAudioType, "audio-editor/audio-type");
            t.GroupBoxHelper(grpAudioClip, "audio-editor/audio-clip-box");
            t.ButtonHelper(btnPlay, "audio-editor/btn-play");
            t.ButtonHelper(btnPause, "audio-editor/btn-pause");
            t.ButtonHelper(btnStop, "audio-editor/btn-stop");
        }

        private void AudioEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
