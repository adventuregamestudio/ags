using System;
using System.ComponentModel;
using System.Xml;

namespace AGS.Types
{
    public class AudioClipType
    {
        [ReadOnly(true)]
        [Description("The internal ID number of this audio type")]
        [Category("Design")]
        public int TypeID { get; set; }

        [Description("The name of this audio type")]
        [Category("Design")]
        [RefreshProperties(RefreshProperties.All)]
        public string Name { get; set; }

        [Description("The maximum number of clips of this type that can play at the same time (0=unlimited)")]
        [Category("Design")]
        public int MaxChannels { get; set; }

        [Description("Number of percentage points to reduce the volume of clips of this type by while speech is playing")]
        [Category("Design")]
        public int VolumeReductionWhileSpeechPlaying { get; set; }

        [DisplayName("Crossfade tracks")]
        [Description("When all channels are full, the new track will crossfade over the one it is replacing")]
        [DefaultValue(CrossfadeSpeed.No)]
        [Category("Design")]
        public CrossfadeSpeed CrossfadeClips { get; set; }

        [Browsable(false)]
        public bool BackwardsCompatibilityType { get; set; }

        [AGSNoSerialize]
        [Description("The name by which the script will know this audio type")]
        [Category("Design")]
        public string ScriptID
        {
            get
            {
                if (Name.Length < 1)
                {
                    return string.Empty;
                }
                string scriptName = "eAudioType" + Name;
                for (int i = 0; i < scriptName.Length; i++)
                {
                    if (!Char.IsLetterOrDigit(scriptName[i]))
                    {
                        scriptName = scriptName.Replace(scriptName[i].ToString(), string.Empty);
                    }
                }
                return scriptName;
            }
        }

        public AudioClipType(int typeID, string name, int maxChannels, int volumeReductionWhileSpeechPlaying, bool backwardsCompatType, CrossfadeSpeed crossfading)
        {
            this.TypeID = typeID;
            this.Name = name;
            this.MaxChannels = maxChannels;
            this.VolumeReductionWhileSpeechPlaying = volumeReductionWhileSpeechPlaying;
            this.BackwardsCompatibilityType = backwardsCompatType;
            this.CrossfadeClips = crossfading;
        }

        public AudioClipType(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }
    }
}
