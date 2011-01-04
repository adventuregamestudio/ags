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
        private AudioClip _playingClip;
        private bool _sentStopEvent;

        [DllImport("winmm.dll")]
        private static extern uint mciSendString(string strCommand, StringBuilder strReturn, int iReturnLength, IntPtr oCallback);

        public bool RequiresPoll { get { return true; } }

        public bool Play(AudioClip clip)
        {
            if (!File.Exists(clip.CacheFileName))
            {
                return false;
            }
            StringBuilder sb = new StringBuilder(100);
            uint lRet = mciSendString(string.Format("open \"{0}\" alias track", clip.CacheFileName), sb, 0, IntPtr.Zero);
            mciSendString("set track time format ms", sb, 0, IntPtr.Zero);
            mciSendString("play track", sb, 0, IntPtr.Zero);
            _playingClip = clip;
            _sentStopEvent = false;
            return true;
        }

        private void SendStopEventToListeners()
        {
            if (!_sentStopEvent)
            {
                _sentStopEvent = true;
                if (PlayFinished != null)
                {
                    PlayFinished(_playingClip);
                }
            }
        }

        public bool IsPlaying()
        {
            StringBuilder sb = new StringBuilder(200);
            mciSendString("status track mode", sb, sb.Capacity, IntPtr.Zero);
            if (sb.ToString().Contains("playing"))
            {
                return true;
            }
            SendStopEventToListeners();
            return false;
        }

        public void Poll()
        {
            IsPlaying();
        }

        public int GetPositionMs()
        {
            IsPlaying();  // ensure the Finished event gets sent if appropriate
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
            mciSendString("pause track", null, 0, IntPtr.Zero);
        }

        public void Resume()
        {
            mciSendString("resume track", null, 0, IntPtr.Zero);
        }

        public void Stop()
        {
            mciSendString("close all", null, 0, IntPtr.Zero);
            SendStopEventToListeners();
        }

    }
}
