using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace AGS.Editor
{
    internal class SpeechLipSyncLine
    {
        public string FileName;
        public List<SpeechLipSyncPhoneme> Phonemes = new List<SpeechLipSyncPhoneme>();
    }

    internal class SpeechLipSyncPhoneme : IComparable<SpeechLipSyncPhoneme>
    {
        public SpeechLipSyncPhoneme(int endTimeOffset, short frame)
        {
            this.EndTimeOffset = endTimeOffset;
            this.Frame = frame;
        }

        public int EndTimeOffset;
        public short Frame;

        public int CompareTo(SpeechLipSyncPhoneme other)
        {
            return EndTimeOffset - other.EndTimeOffset;
        }
    }
}
