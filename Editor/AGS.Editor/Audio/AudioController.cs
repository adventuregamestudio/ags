using System;
using AGS.Types;

namespace AGS.Editor
{
    public class AudioController
    {
        private static AudioController _instance;

        public IAudioPreviewer CreatePlayback(AudioClip clip)
        {
            if (clip.FileType == AudioClipFileType.MIDI)
            {
                return new MidiPlayer(clip);
            }
            else
            {
                return new IrrklangPlayer(clip);
            }
        }

        public static AudioController Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new AudioController();
                }
                return _instance;
            }
        }
    }
}
