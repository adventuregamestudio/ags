﻿using System;
using System.ComponentModel;
using System.IO;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("BundlingType")]
    public class AudioClip : IToXml, IComparable<AudioClip>
    {
        // FIXME: this must not be in AudioClip or AGS.Types, it's up for the application to determine a cache folder!!
        //        move to AudioComponent?
        public const string AUDIO_CACHE_DIRECTORY = "AudioCache";

        private int _id;
        private string _sourceFileName;
        private string _cacheFileName;
        private string _scriptName;
        // FixedID is a clip's UID which never change, even if the list got reordered
        private int _fixedID;
        private int _typeID;
        private AudioFileBundlingType _bundlingType = AudioFileBundlingType.InGameEXE;
        private AudioClipFileType _fileType;
        private DateTime _fileLastModifiedDate = DateTime.MinValue;
        private TimeSpan _fileLength = TimeSpan.MinValue;
        private int _volume = -1;
        private AudioClipPriority _priority = AudioClipPriority.Inherit;
        private InheritableBool _repeat = InheritableBool.Inherit;
        private int _actualVolume;
        private AudioClipPriority _actualPriority;
        private bool _actualRepeat;
        private CustomProperties _properties = new CustomProperties();

        // The value of a "no sound reference"
        public const int FixedIndexNoValue = 0;
        // The value of a "first index"
        public const int FixedIndexBase = 1;
        // The value of a "no sound" for AudioClip.ID
        public const int IDNoValue = -1;

        public AudioClip(string scriptName, int fixed_index)
        {
            _scriptName = scriptName;
            _fixedID = fixed_index;
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public string CacheFileName
        {
            get { return _cacheFileName; }
            set { _cacheFileName = value; }
        }

        [AGSNoSerialize]
        [ReadOnly(true)]
        [Description("The file name that AGS stores this sound as in the AudioCache")]
        [DisplayName("Cache File Name")]
        public string CacheFileNameWithoutPath
        {
            get { return Path.GetFileName(CacheFileName); }
        }

        [DisplayName("ID")]
        [Description("The ID number of the clip")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The file from which this audio clip was imported")]
        [Editor(typeof(AudioClipSourceFileUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(ReadOnlyConverter))]
        public string SourceFileName
        {
            get { return _sourceFileName; }
            set { _sourceFileName = value; }
        }

        [Description("The name by which this audio clip can be referenced in the script")]
        public string ScriptName
        {
            get { return _scriptName; }
            set { _scriptName = Utilities.ValidateScriptName(value); }
        }

        // This is a "Fixed Index" that is used as a stable reference the clip,
        // regardless of any clip list rearrangements.
        [Browsable(false)]
        public int Index
        {
            get { return _fixedID; }
            set { _fixedID = value; }
        }

        [Description("Which type of audio does this clip contain")]
        [TypeConverter(typeof(AudioClipTypeTypeConverter))]
        public int Type
        {
            get { return _typeID; }
            set { _typeID = value; }
        }

        [Description("How this file is compiled when you build the game EXE")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public AudioFileBundlingType BundlingType
        {
            get { return _bundlingType; }
            set { _bundlingType = value; }
        }

        [ReadOnly(true)]
        [Description("The file type of this audio clip")]
        public AudioClipFileType FileType
        {
            get { return _fileType; }
            set { _fileType = value; }
        }

        [AGSNoSerialize]
        [Description("The date/time the audio file was last modified")]
        [DisplayName("Last Modified")]
        [ReadOnly(true)]
        public DateTime FileLastModifiedDate
        {
            get { return _fileLastModifiedDate; }
            set { _fileLastModifiedDate = value; }
        }

        [AGSNoSerialize]
        [Description("The audio file's length (hh:mm:ss.ms)")]
        [ReadOnly(true)]
        public TimeSpan Length
        {
            get { return _fileLength; }
            set { _fileLength = value; }
        }

        [Description("The volume (0..100) that this clip will play at, if the script does not specify it. -1 inherits from parent folder.")]
        public int DefaultVolume
        {
            get { return _volume; }
            set
            {
                if ((value < -1) || (value > 100))
                {
                    throw new ArgumentOutOfRangeException("Volume must be 0-100, or -1 to inherit");
                }
                _volume = value;
            }
        }

        [Description("The priority that this clip will play at, if the script does not specify it")]
        public AudioClipPriority DefaultPriority
        {
            get { return _priority; }
            set { _priority = value; }
        }

        [Description("Whether this clip will loop by default, if the script does not specify")]
        public InheritableBool DefaultRepeat
        {
            get { return _repeat; }
            set { _repeat = value; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public int ActualVolume
        {
            get { return _actualVolume; }
            set { _actualVolume = value; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public AudioClipPriority ActualPriority
        {
            get { return _actualPriority; }
            set { _actualPriority = value; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public bool ActualRepeat
        {
            get { return _actualRepeat; }
            set { _actualRepeat = value; }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this audio clip")]
        [Category("Properties")]
        [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set { _properties = value; }
        }

        public AudioClip(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            writer.WriteElementString("FileLastModifiedDate", _fileLastModifiedDate.ToString("u"));
            writer.WriteEndElement();
        }

        #region IComparable<AudioClip> Members

        public int CompareTo(AudioClip other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
