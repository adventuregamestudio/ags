using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    public delegate void PlayFinishedHandler(AudioClip clip);

    internal interface IAudioPreviewer
    {
        event PlayFinishedHandler PlayFinished;

        bool RequiresPoll { get; }
        bool Play(AudioClip clip);
        bool IsPlaying();
        void Poll();
        int GetLengthMs();
        int GetPositionMs();
        void Pause();
        void Resume();
        void Stop();
    }
}
