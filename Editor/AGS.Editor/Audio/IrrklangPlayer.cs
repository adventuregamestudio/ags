using AGS.Types;
using IrrKlang;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class IrrklangPlayer : IAudioPreviewer, ISoundStopEventReceiver
    {
        public event PlayFinishedHandler PlayFinished;

        private ISoundEngine _soundEngine = new ISoundEngine();
        private ISound _soundPlaying = null;
        private AudioClip _audioClip = null;

        public bool RequiresPoll { get { return false; } }

        public bool Play(AudioClip clip)
        {
            if (!File.Exists(clip.CacheFileName))
            {
                return false;
            }
            // we have to read it into memory and then play from memory,
            // because the built-in Irrklang play from file function keeps
            // the file open and locked
            byte[] audioData = File.ReadAllBytes(clip.CacheFileName);
            ISoundSource source = _soundEngine.AddSoundSourceFromMemory(audioData, clip.CacheFileName);
            _soundPlaying = _soundEngine.Play2D(source, false, false, false);
            if (_soundPlaying == null)
            {
                return false;
            }
            _audioClip = clip;
            _soundPlaying.setSoundStopEventReceiver(this);
            return true;
        }

        public bool IsPlaying()
        {
            return (_soundPlaying != null);
        }

        public void Poll()
        {
            // do nothing
        }

        public int GetPositionMs()
        {
            if (_soundPlaying == null)
            {
                return 0;
            }
            return (int)_soundPlaying.PlayPosition;
        }

        public int GetLengthMs()
        {
            if (_soundPlaying == null)
            {
                return 0;
            }
            return (int)_soundPlaying.PlayLength;
        }

        public void Pause()
        {
            _soundPlaying.Paused = true;
        }

        public void Resume()
        {
            _soundPlaying.Paused = false;
        }

        public void Stop()
        {
            if (_soundPlaying != null)
            {
                _soundPlaying.Stop();
                _soundPlaying = null;
                _soundEngine.RemoveAllSoundSources();
                _soundEngine.StopAllSounds();
            }
        }

        void ISoundStopEventReceiver.OnSoundStopped(ISound sound, StopEventCause reason, object userData)
        {
            _soundPlaying = null;
            if (PlayFinished != null)
            {
                PlayFinished(_audioClip);
                _audioClip = null;
            }
        }
    }
}
