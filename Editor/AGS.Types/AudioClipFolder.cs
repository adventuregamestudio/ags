using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class AudioClipFolders : FolderListHybrid<AudioClip, AudioClipFolder>
    {
        public AudioClipFolders() : base(new AudioClipFolder()) { }

        public AudioClipFolders(string name) : base(new AudioClipFolder(name)) { }

        public AudioClipFolders(XmlNode node) :
            base(new AudioClipFolder(node)) { }
    }

    public class AudioClipFolder : BaseFolderCollection<AudioClip, AudioClipFolder>
    {
        private int _volume = 100;
        private AudioClipPriority _priority = AudioClipPriority.Inherit;
        private InheritableBool _repeat = InheritableBool.Inherit;
        private int _defaultTypeID = 1;
        private AudioFileBundlingType _bundlingType = AudioFileBundlingType.InGameEXE;

        public AudioClipFolder(string name)
            : base(name)
        {
        }

        public AudioClipFolder() : this("Default") { }

        public AudioClipFolder(XmlNode node) : base(node)
        {
            SerializeUtils.DeserializeFromXML(this, node.SelectSingleNode(this.GetType().Name));
        }

        [Description("The volume that will be assigned to all clips in this folder, where the clip volume is -1")]
        public int DefaultVolume
        {
            get { return _volume; }
            set { _volume = value; }
        }

        [Description("The priority that will be assigned to all clips in this folder, where the priority is Inherited")]
        public AudioClipPriority DefaultPriority
        {
            get { return _priority; }
            set { _priority = value; }
        }

        [Description("The repeat setting that will be assigned to all clips in this folder, where the Repeat is Inherited")]
        public InheritableBool DefaultRepeat
        {
            get { return _repeat; }
            set { _repeat = value; }
        }

        [Description("New audio clips imported into this folder will have this bundling type by default")]
        public AudioFileBundlingType DefaultBundlingType
        {
            get { return _bundlingType; }
            set { _bundlingType = value; }
        }

        [Description("New audio clips imported into this folder will have this type by default")]
        [TypeConverter(typeof(AudioClipTypeTypeConverter))]
        public int DefaultType
        {
            get { return _defaultTypeID; }
            set { _defaultTypeID = value; }
        }

        public IList<AudioClip> GetAllAudioClipsFromAllSubFolders()
        {
            List<AudioClip> allClips = new List<AudioClip>();

            this.AddAllClipsFromThisAndSubFoldersToList(allClips);

            return allClips;
        }

        public override AudioClipFolder CreateChildFolder(string name)
        {
            AudioClipFolder childFolder = new AudioClipFolder(name);
            childFolder.DefaultType = DefaultType;
            childFolder.DefaultBundlingType = DefaultBundlingType;
            return childFolder;
        }

        private void AddAllClipsFromThisAndSubFoldersToList(List<AudioClip> clipList)
        {
            clipList.AddRange(this.Items);

            foreach (AudioClipFolder subFolder in this.SubFolders)
            {
                subFolder.AddAllClipsFromThisAndSubFoldersToList(clipList);
            }
        }

        protected override void ToXmlExtend(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        protected override AudioClipFolder CreateFolder(XmlNode node)
        {
            return new AudioClipFolder(node);
        }

        protected override AudioClip CreateItem(XmlNode node)
        {
            return new AudioClip(node);
        }
    }
}
