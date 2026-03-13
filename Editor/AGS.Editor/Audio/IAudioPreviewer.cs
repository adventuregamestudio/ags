using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    public delegate void PlayFinishedHandler(IAudioPreviewer playback);

    public interface IAudioPreviewer: IDisposable
    {
        event PlayFinishedHandler PlayFinished;

        AudioClip Clip { get; }

        void Play();
        bool IsPlaying();
        void Poll();
        int GetLengthMs();
        int GetPositionMs();
        void Pause();
        void Resume();
        void Stop();
    }
}
