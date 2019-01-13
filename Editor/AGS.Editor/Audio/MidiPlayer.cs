using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    internal class MidiPlayer : IAudioPreviewer
    {
        public event PlayFinishedHandler PlayFinished;
        private AudioClip _audioClip;
        private bool _paused;

        [DllImport("winmm.dll")]
        private static extern uint mciSendString(string strCommand, StringBuilder strReturn, int iReturnLength, IntPtr oCallback);

        public MidiPlayer(AudioClip clip)
        {
            if (!File.Exists(clip.CacheFileName))
            {
                throw new AGSEditorException("AudioClip file is missing from the audio cache");
            }

            _audioClip = clip;
            _paused = false;
            uint rc = mciSendString(string.Format("open \"{0}\" alias track", clip.CacheFileName), null, 0, IntPtr.Zero);

            if (rc != 0)
            {
                throw new AGSEditorException($"Unable to open MIDI file: error code {rc}");
            }

            mciSendString("set track time format ms", null, 0, IntPtr.Zero);
        }

        public void Play()
        {
            if (0 != mciSendString("play track", null, 0, IntPtr.Zero))
            {
                throw new AGSEditorException("Unable to play the MIDI file");
            }
        }

        public void Poll()
        {
            if (!IsPlaying() && !_paused)
            {
                Stop();
            }
        }

        private void SendStopEventToListeners()
        {
            PlayFinished?.Invoke(_audioClip);
        }

        public bool IsPlaying()
        {
            StringBuilder sb = new StringBuilder(200);
            mciSendString("status track mode", sb, sb.Capacity, IntPtr.Zero);
            return sb.ToString().Contains("playing");
        }

        public int GetPositionMs()
        {
            StringBuilder sb = new StringBuilder(200);
            mciSendString("status track position", sb, sb.Capacity, IntPtr.Zero);
            int position;
            if (int.TryParse(sb.ToString(), out position))
            {
                return position;
            }
            return 0;
        }

        public int GetLengthMs()
        {
            StringBuilder sb = new StringBuilder(200);
            mciSendString("status track length", sb, sb.Capacity, IntPtr.Zero);
            int length;
            if (int.TryParse(sb.ToString(), out length))
            {
                return length;
            }
            return 0;
        }

        public void Pause()
        {
            mciSendString("stop track", null, 0, IntPtr.Zero);
            _paused = true;
        }

        public void Resume()
        {
            // needs to play 'from' to prevent all instruments getting reset to defaults (piano)
            StringBuilder sb = new StringBuilder(200);
            mciSendString("status track position", sb, sb.Capacity, IntPtr.Zero);
            mciSendString(string.Format("play track from {0}", sb.ToString()), null, 0, IntPtr.Zero);
            _paused = false;
        }

        public void Stop()
        {
            mciSendString("stop track", null, 0, IntPtr.Zero);
            mciSendString("seek track to start", null, 0, IntPtr.Zero);
            SendStopEventToListeners();
        }

        public void Dispose()
        {
            mciSendString("close all", null, 0, IntPtr.Zero);
        }
    }
}
