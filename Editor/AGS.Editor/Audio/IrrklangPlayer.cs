using AGS.Types;
#if IRRKLANG_AVAILABLE
using IrrKlang;
#endif
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
#if IRRKLANG_AVAILABLE
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

            _soundEngine = new ISoundEngine(SoundOutputDriver.AutoDetect);

            // we have to read it into memory and then play from memory,
            // because the built-in Irrklang play from file function keeps
            // the file open and locked
            byte[] audioData = File.ReadAllBytes(clip.CacheFileName);
            _source = _soundEngine.AddSoundSourceFromMemory(audioData, clip.CacheFileName);
            _audioClip = clip;
        }

        public AudioClip Clip
        {
            get { return _audioClip; }
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

        public MediaInfo GetMediaInfo()
        {
            return new MediaInfo(
                _source.AudioFormat.SampleRate,
                _source.AudioFormat.ChannelCount,
                // irrKlang's bytes per second is unreliable,
                // it seems to refer to the format in memory
                0);
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
            PlayFinished?.Invoke(this);
        }

        public void Dispose()
        {
            _soundEngine.Dispose();
        }
    }
#else
    /// <summary>
    /// Stub used when irrKlang.NET4.dll is not available.
    /// Audio preview is disabled; the Editor will still build and run.
    /// </summary>
    public class IrrklangPlayer : IAudioPreviewer
    {
        public event PlayFinishedHandler PlayFinished;

        public IrrklangPlayer(AudioClip clip)
        {
            throw new AGSEditorException("Audio preview is unavailable: irrKlang library not installed.");
        }

        public AudioClip Clip => null;
        public void Play() { }
        public bool IsPlaying() => false;
        public int GetPositionMs() => 0;
        public int GetLengthMs() => 0;
        public MediaInfo GetMediaInfo() => new MediaInfo(0, 0, 0);
        public void Pause() { }
        public void Resume() { }
        public void Stop() { }
        public void Poll() { }
        public void Dispose() { }
    }
#endif
}
