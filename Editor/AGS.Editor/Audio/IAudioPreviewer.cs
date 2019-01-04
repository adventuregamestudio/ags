using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    public delegate void PlayFinishedHandler(AudioClip clip);

    internal interface IAudioPreviewer: IDisposable
    {
        event PlayFinishedHandler PlayFinished;

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
