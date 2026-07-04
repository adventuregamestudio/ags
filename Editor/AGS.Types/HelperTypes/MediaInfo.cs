using System;
using System.Linq;

namespace AGS.Types
{
    public struct MediaInfo
    {
        // Sample rate, in samples per second
        public int SampleRate;
        public int Channels;
        // Bit rate, in kilo-bytes per second
        public int BitRate;

        public MediaInfo(int srate, int chans, int bitrate)
        {
            SampleRate = srate;
            Channels = chans;
            BitRate = bitrate;
        }

        public override string ToString()
        {
            if (SampleRate <= 0)
                return string.Empty;
            string s = string.Format("{0:F1} kHz", (float)SampleRate / 1000.0f);
            if (Channels > 0)
                s = string.Concat(s, $", {Channels} chans");
            if (BitRate > 0)
                s = string.Concat(s, $", {BitRate} kb/s");
            return s;
        }
    }
}
