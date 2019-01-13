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

        private ISoundEngine _soundEngine = null;
        private ISound _soundPlaying = null;
        private AudioClip _audioClip = null;
        private ISoundSource _source = null;

        public IrrklangPlayer(AudioClip clip)
        {
            if (!File.Exists(clip.CacheFileName))
            {
                throw new AGSEditorException("AudioClip file is missing from the audio cache");
            }

            if (Utilities.IsMonoRunning())
            {
                _soundEngine = new ISoundEngine(SoundOutputDriver.AutoDetect);
            }
            else
            {
                // explicitly ask for the software driver as there seem to
                // be issues with ISound returning bad data when re-using
                // the same source and certain audio drivers
                _soundEngine = new ISoundEngine(SoundOutputDriver.WinMM);
            }

            // we have to read it into memory and then play from memory,
            // because the built-in Irrklang play from file function keeps
            // the file open and locked
            byte[] audioData = File.ReadAllBytes(clip.CacheFileName);
            _source = _soundEngine.AddSoundSourceFromMemory(audioData, clip.CacheFileName);
            _audioClip = clip;
        }

        public void Play()
        {
            _soundPlaying = _soundEngine.Play2D(_source, false, false, false);

            if (!IsPlaying())
            {
                throw new AGSEditorException("Unable to play the audio file");
            }

            _soundPlaying.setSoundStopEventReceiver(this);
        }

        public bool IsPlaying()
        {
            return (_soundPlaying != null);
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
            if (_source == null)
            {
                return 0;
            }
            return (int)_source.PlayLength;
        }

        public void Pause()
        {
            if (_soundPlaying != null)
            {
                _soundPlaying.Paused = true;
            }
        }

        public void Resume()
        {
            if (_soundPlaying != null)
            {
                _soundPlaying.Paused = false;
            }
        }

        public void Stop()
        {
            if (_soundPlaying != null)
            {
                _soundPlaying.Stop();
            }
        }

        public void Poll()
        {
            // do nothing as the player will generate a stop event
            // (see below)
        }

        void ISoundStopEventReceiver.OnSoundStopped(ISound sound, StopEventCause reason, object userData)
        {
            _soundPlaying = null;
            PlayFinished?.Invoke(_audioClip);
        }

        public void Dispose()
        {
            _soundEngine.Dispose();
        }
    }
}
