using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace AGS.Editor
{
    internal class SpeechLipSyncLine
    {
        public string FileName;
        public List<SpeechLipSyncPhenome> Phenomes = new List<SpeechLipSyncPhenome>();
    }

    internal class SpeechLipSyncPhenome : IComparable<SpeechLipSyncPhenome>
    {
        public SpeechLipSyncPhenome(int endTimeOffset, short frame)
        {
            this.EndTimeOffset = endTimeOffset;
            this.Frame = frame;
        }

        public int EndTimeOffset;
        public short Frame;

        public int CompareTo(SpeechLipSyncPhenome other)
        {
            return EndTimeOffset - other.EndTimeOffset;
        }
    }
}
