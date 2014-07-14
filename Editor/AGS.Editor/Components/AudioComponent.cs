using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using AGS.Types;

namespace AGS.Editor.Components
{
    class AudioComponent : BaseComponentWithFolders<AudioClip, AudioClipFolder>, IProjectTreeSingleClickHandler
    {
        private const string COMMAND_ADD_AUDIO = "AddAudioClipCmd";
        private const string COMMAND_PROPERTIES = "PropertiesAudioClip";
        private const string COMMAND_RENAME = "RenameAudioClip";
        private const string COMMAND_DELETE = "DeleteAudioClip";
        private const string SPEECH_NODE_ID = "DummySpeechNode";
        private const string AUDIO_TYPES_FOLDER_NODE_ID = "AudioTypesFolderNode";
        private const string NODE_ID_PREFIX_CLIP_TYPE = "AudioClipType";
        private const string COMMAND_NEW_CLIP_TYPE = "NewAudioClipType";
        private const string COMMAND_RENAME_CLIP_TYPE = "RenameAudioClipType";
        private const string COMMAND_DELETE_CLIP_TYPE = "DeleteAudioClipType";
        private const string COMMAND_PROPERTIES_CLIP_TYPE = "PropertiesAudioClipType";

        private const string AUDIO_CLIP_TYPE_ICON = "AGSAudioClipTypeIcon";

        private const string AUDIO_FILES_FILTER = "All supported audio (*.ogg; *.mp3; *.wav; *.voc; *.mid; *.mod; *.xm; *.s3m; *.it)|*.ogg;*.mp3;*.wav;*.voc;*.mid;*.mod;*.xm;*.s3m;*.it|OGG digital sound file (*.ogg)|*.ogg|MP3 file (*.mp3)|*.mp3|WAV uncompressed audio (*.wav)|*.wav|Creative Labs VOC (*.voc)|*.voc|MIDI music (*.mid)|*.mid|Digital tracker formats (*.mod; *.xm; *.s3m; *.it)|*.mod;*.xm;*.s3m;*.it";
        private const int DEFAULT_AUDIO_TYPE_MUSIC = 2;
        private const int DEFAULT_AUDIO_TYPE_SOUND = 3;

        private Dictionary<string, AudioClipFileType> _fileTypeMappings = new Dictionary<string, AudioClipFileType>();
        private Dictionary<AudioClipFileType, string> _iconMappings = new Dictionary<AudioClipFileType, string>();
        private AudioEditor _editor;
        private ContentDocument _document;

        public AudioComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, "AGSAudioClips")
        {
            _fileTypeMappings.Add(".mp3", AudioClipFileType.MP3);
            _fileTypeMappings.Add(".ogg", AudioClipFileType.OGG);
            _fileTypeMappings.Add(".voc", AudioClipFileType.VOC);
            _fileTypeMappings.Add(".wav", AudioClipFileType.WAV);
            _fileTypeMappings.Add(".mid", AudioClipFileType.MIDI);
            _fileTypeMappings.Add(".mod", AudioClipFileType.MOD);
            _fileTypeMappings.Add(".xm", AudioClipFileType.MOD);
            _fileTypeMappings.Add(".s3m", AudioClipFileType.MOD);
            _fileTypeMappings.Add(".it", AudioClipFileType.MOD);

            _iconMappings.Add(AudioClipFileType.MP3, "AGSAudioClipIconMp3");
            _iconMappings.Add(AudioClipFileType.OGG, "AGSAudioClipIconOgg");
            _iconMappings.Add(AudioClipFileType.VOC, "AGSAudioClipIconVoc");
            _iconMappings.Add(AudioClipFileType.WAV, "AGSAudioClipIconWav");
            _iconMappings.Add(AudioClipFileType.MIDI, "AGSAudioClipIconMidi");
            _iconMappings.Add(AudioClipFileType.MOD, "AGSAudioClipIconMod");

            _agsEditor.GetSourceControlFileList += new GetSourceControlFileListHandler(_agsEditor_GetSourceControlFileList);
            _agsEditor.PreCompileGame += new AGSEditor.PreCompileGameHandler(_agsEditor_PreCompileGame);

            RecreateDocument();
            _guiController.RegisterIcon("AGSAudioClipsIcon", Resources.ResourceManager.GetIcon("audio.ico"));
            _guiController.RegisterIcon("AGSAudioClipIconMidi", Resources.ResourceManager.GetIcon("audio-midi.ico"));
            _guiController.RegisterIcon("AGSAudioClipIconMod", Resources.ResourceManager.GetIcon("audio-mod.ico"));
            _guiController.RegisterIcon("AGSAudioClipIconMp3", Resources.ResourceManager.GetIcon("audio-mp3.ico"));
            _guiController.RegisterIcon("AGSAudioClipIconOgg", Resources.ResourceManager.GetIcon("audio-ogg.ico"));
            _guiController.RegisterIcon("AGSAudioClipIconWav", Resources.ResourceManager.GetIcon("audio-wav.ico"));
            _guiController.RegisterIcon("AGSAudioClipIconVoc", Resources.ResourceManager.GetIcon("audio-voc.ico"));
            _guiController.RegisterIcon("AGSAudioSpeechIcon", Resources.ResourceManager.GetIcon("audio_speech.ico"));
            _guiController.RegisterIcon(AUDIO_CLIP_TYPE_ICON, Resources.ResourceManager.GetIcon("tree_audio_generic.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Audio", "AGSAudioClipsIcon");
            _guiController.ProjectTree.OnAfterLabelEdit += new ProjectTree.AfterLabelEditHandler(ProjectTree_OnAfterLabelEdit);
            RePopulateTreeView();
        }

        private void RecreateDocument()
        {
            _editor = new AudioEditor();
            _document = new ContentDocument(_editor, "Audio", this, AUDIO_CLIP_TYPE_ICON);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Audio; }
        }

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_ADD_AUDIO)
            {
                string[] selectedFiles = _guiController.ShowOpenFileDialogMultipleFiles("Select audio to add", AUDIO_FILES_FILTER);
                if (selectedFiles.Length > 0)
                {
                    ImportAudioFiles(selectedFiles);
                }
            }
            else if (controlID == COMMAND_RENAME)
            {
                _guiController.ProjectTree.BeginLabelEdit(this, _rightClickedID);
            }
            else if (controlID == COMMAND_DELETE)
            {
                AudioClip clipToDelete = _items[_rightClickedID];
                if (_guiController.ShowQuestion("Are you sure you want to delete audio '" + clipToDelete.ScriptName + "'?") == System.Windows.Forms.DialogResult.Yes)
                {
                    DeleteSingleItem(clipToDelete);
                }
            }
            else if (controlID == SPEECH_NODE_ID)
            {
                _guiController.ShowMessage("You create voice speech files by placing them in the Speech sub-folder in explorer, and they must have special names corresponding to the line of text that they represent.\n\nPlease look up 'Voice Speech' in the help file for details.", MessageBoxIconType.Information);
            }
            else if (controlID == COMMAND_PROPERTIES)
            {
                ShowPaneForItem(_rightClickedID);
            }
            else if (controlID == COMMAND_NEW_CLIP_TYPE)
            {
                CreateNewAudioClipType();
            }
            else if (controlID == COMMAND_RENAME_CLIP_TYPE)
            {
                _guiController.ProjectTree.BeginLabelEdit(this, _rightClickedID);
            }
            else if (controlID == COMMAND_DELETE_CLIP_TYPE)
            {
                AudioClipType typeToDelete = FindAudioClipTypeByNodeID(_rightClickedID, true);
                DeleteAudioClipType(typeToDelete);
            }
            else if (controlID.StartsWith(NODE_ID_PREFIX_CLIP_TYPE))
            {
                ShowPaneForItem(controlID);
            }
            else if (controlID == COMMAND_PROPERTIES_CLIP_TYPE)
            {
                ShowPaneForItem(_rightClickedID);
            }
            else if (controlID != AUDIO_TYPES_FOLDER_NODE_ID)
            {
                ShowPaneForItem(controlID);
            }
        }

        void IProjectTreeSingleClickHandler.SingleClick(string controlID)
        {
            if (_guiController.ActivePane == _document)
            {
                ShowPaneForItem(controlID);
            }
        }

        private void DeleteAudioClipType(AudioClipType typeToDelete)
        {
            if (_agsEditor.CurrentGame.AudioClipTypes.Count <= 1)
            {
                _guiController.ShowMessage("You cannot delete this audio type, as the game must contain at least one.", MessageBoxIconType.Warning);
                return;
            }

            if (typeToDelete.BackwardsCompatibilityType)
            {
                if (_guiController.ShowQuestion("This audio type is required for backwards compatibility with old-style audio scripting. If you delete it, commands like PlayMusic and PlayAmbientSound will no longer work correctly. Are you sure you want to continue?", System.Windows.Forms.MessageBoxIcon.Warning) == System.Windows.Forms.DialogResult.No)
                {
                    return;
                }
            }

            int typeUsedByClipCount = 0;
            foreach (AudioClip clip in _agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.Type == typeToDelete.TypeID)
                {
                    typeUsedByClipCount++;
                }
            }

            if (typeUsedByClipCount > 0)
            {
                _guiController.ShowMessage("This audio type is in use by " + typeUsedByClipCount + " audio clips and cannot be deleted", MessageBoxIconType.Warning);
                return;
            }

            if (_guiController.ShowQuestion("Are you sure you want to delete audio type '" + typeToDelete.Name + "'?") == System.Windows.Forms.DialogResult.Yes)
            {
                _agsEditor.CurrentGame.AudioClipTypes.Remove(typeToDelete);
                AdjustAudioTypeIDsAfterDeletingOne(typeToDelete);
                RePopulateTreeView(AUDIO_TYPES_FOLDER_NODE_ID);
                AudioClipTypeTypeConverter.RefreshAudioClipTypeList();
            }
        }

        private void AdjustAudioTypeIDsAfterDeletingOne(AudioClipType typeJustDeleted)
        {
            foreach (AudioClipType type in _agsEditor.CurrentGame.AudioClipTypes)
            {
                if (type.TypeID > typeJustDeleted.TypeID)
                {
                    type.TypeID--;
                }
            }

            foreach (AudioClip clip in _agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.Type > typeJustDeleted.TypeID)
                {
                    clip.Type--;
                }
            }
        }

        private AudioClipType FindAudioClipTypeByNodeID(string nodeID, bool mustFind)
        {
            if (nodeID.StartsWith(NODE_ID_PREFIX_CLIP_TYPE))
            {
                int clipTypeId = Convert.ToInt32(nodeID.Substring(NODE_ID_PREFIX_CLIP_TYPE.Length));
                foreach (AudioClipType audioType in _agsEditor.CurrentGame.AudioClipTypes)
                {
                    if (audioType.TypeID == clipTypeId)
                    {
                        return audioType;
                    }
                }
            }

            if (mustFind)
            {
                throw new InvalidOperationException("No audio clip type found for node ID " + nodeID);
            }

            return null;
        }

        private void CreateNewAudioClipType()
        {
            _guiController.ProjectTree.StartFromNode(this, AUDIO_TYPES_FOLDER_NODE_ID);
            AudioClipType newClipType = new AudioClipType(_agsEditor.CurrentGame.AudioClipTypes.Count + 1, "New audio type", 0, 0, false, CrossfadeSpeed.No);
            _agsEditor.CurrentGame.AudioClipTypes.Add(newClipType);
            string newNodeID = AddTreeNodeForAudioClipType(newClipType);
            _guiController.ProjectTree.BeginLabelEdit(this, newNodeID);
            AudioClipTypeTypeConverter.RefreshAudioClipTypeList();
        }

        private void ShowPaneForItem(string controlID)
        {
            object itemToEdit = null;
            if (_items.ContainsKey(controlID))
            {
                itemToEdit = _items[controlID];
            }
            else if (_folders.ContainsKey(controlID))
            {
                itemToEdit = _folders[controlID];
            }
            else
            {
                itemToEdit = FindAudioClipTypeByNodeID(controlID, false);
            }

            if (itemToEdit != null)
            {
                if (_document.Control.IsDisposed)
                {
                    RecreateDocument();
                }
                _editor.SelectedItem = itemToEdit;
                _document.SelectedPropertyGridObject = itemToEdit;
                _document.TreeNodeID = _rightClickedID;
                _guiController.AddOrShowPane(_document);
            }
        }

        private void ImportAudioFiles(string[] selectedFiles)
        {
            string lastAddedId = null;
            foreach (string fileName in selectedFiles)
            {
                AudioClip newClip = CreateAudioClipForFile(fileName);
                if (newClip != null)
                {
                    AudioClipFolder parentFolder = _folders[_rightClickedID];
                    newClip.BundlingType = parentFolder.DefaultBundlingType;
                    newClip.Type = parentFolder.DefaultType;
                    parentFolder.Items.Add(newClip);
                    lastAddedId = newClip.ScriptName;
                }
                else
                {
                    _guiController.ShowMessage("The file '" + fileName + "' could not be imported. The file type was not recognised.", MessageBoxIconType.Warning);
                }
            }

            if (lastAddedId != null)
            {
                RePopulateTreeView(ITEM_COMMAND_PREFIX + lastAddedId);
                AudioClipTypeConverter.SetAudioClipList(_agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders());
            }
        }

        private string GetNodeIDForAudioClip(AudioClip clip)
        {
            return ITEM_COMMAND_PREFIX + clip.ScriptName;
        }

        private AudioClip CreateAudioClipForFile(string sourceFileName)
        {
            string fileExtension = Path.GetExtension(sourceFileName).ToLower();
            if (_fileTypeMappings.ContainsKey(fileExtension))
            {
                string newScriptName = EnsureScriptNameIsUnique(Path.GetFileNameWithoutExtension(sourceFileName));
                AudioClip newClip = new AudioClip(newScriptName, _agsEditor.CurrentGame.GetNextAudioIndex());
                newClip.SourceFileName = sourceFileName;
                newClip.FileType = _fileTypeMappings[fileExtension];
                newClip.FileLastModifiedDate = File.GetLastWriteTimeUtc(sourceFileName);
                Utilities.CopyFileAndSetDestinationWritable(sourceFileName, newClip.CacheFileName);
                Utilities.DeleteFileIfExists(AGSEditor.AUDIO_VOX_FILE_NAME);
                _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                return newClip;
            }
            return null;
        }

        private string EnsureScriptNameIsUnique(string nameToTry)
        {
            nameToTry = AGS.Types.Utilities.RemoveInvalidCharactersFromScriptName(nameToTry);
            nameToTry = "a" + Char.ToUpper(nameToTry[0]) + nameToTry.Substring(1);
            string tryThisTime = nameToTry;
            int suffix = 0;
            while (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(tryThisTime, null))
            {
                suffix++;
                tryThisTime = nameToTry + suffix;
            }
            return tryThisTime;
        }

        public override void RefreshDataFromGame()
        {
            if (_agsEditor.CurrentGame.AudioClipTypes.Count == 0)
            {
                CreateDefaultAudioClipTypes();
            }

            IList<AudioClip> allAudio = null;

            if ((!_agsEditor.CurrentGame.SavedXmlVersionIndex.HasValue) ||
                (_agsEditor.CurrentGame.SavedXmlVersionIndex < 5))
            {
                ImportSoundAndMusicFromOldVersion();
                allAudio = _agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders();

                UpdateScoreSound(allAudio);
                UpdateViewFrameSounds(allAudio, _agsEditor.CurrentGame.RootViewFolder);
            }

            if (allAudio == null)
            {
                allAudio = _agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders();
            }
            AudioClipTypeTypeConverter.SetAudioClipTypeList(_agsEditor.CurrentGame.AudioClipTypes);
            AudioClipTypeConverter.SetAudioClipList(allAudio);

            RePopulateTreeView();
        }

        private void UpdateScoreSound(IList<AudioClip> allAudio)
        {
            if (_agsEditor.CurrentGame.Settings.PlaySoundOnScore > 0)
            {
                AudioClip clip = _agsEditor.CurrentGame.FindAudioClipForOldSoundNumber(allAudio, _agsEditor.CurrentGame.Settings.PlaySoundOnScore);
                if (clip != null)
                {
                    _agsEditor.CurrentGame.Settings.PlaySoundOnScore = clip.Index;
                }
                else
                {
                    _agsEditor.CurrentGame.Settings.PlaySoundOnScore = 0;
                }
            }
        }

        private void UpdateViewFrameSounds(IList<AudioClip> allAudio, ViewFolder views)
        {
            foreach (View view in views.Views)
            {
                foreach (ViewLoop loop in view.Loops)
                {
                    foreach (ViewFrame frame in loop.Frames)
                    {
                        if (frame.Sound > 0)
                        {
                            AudioClip clip = _agsEditor.CurrentGame.FindAudioClipForOldSoundNumber(allAudio, frame.Sound);
                            if (clip != null)
                            {
                                frame.Sound = clip.Index;
                            }
                            else
                            {
                                frame.Sound = 0;
                            }
                        }
                    }
                }
            }

            foreach (ViewFolder folder in views.SubFolders)
            {
                UpdateViewFrameSounds(allAudio, folder);
            }
        }

        private void CreateDefaultAudioClipTypes()
        {
            _agsEditor.CurrentGame.AudioClipTypes.Add(new AudioClipType(1, "Ambient Sound", 1, 0, true, CrossfadeSpeed.No));
            _agsEditor.CurrentGame.AudioClipTypes.Add(new AudioClipType(DEFAULT_AUDIO_TYPE_MUSIC, "Music", 1, 30, true, _agsEditor.CurrentGame.Settings.CrossfadeMusic));
            _agsEditor.CurrentGame.AudioClipTypes.Add(new AudioClipType(DEFAULT_AUDIO_TYPE_SOUND, "Sound", 0, 0, false, CrossfadeSpeed.No));
        }

        private void ImportSoundAndMusicFromOldVersion()
        {
            ImportAllFilesFromDirectoryIntoNewFolder("Music", "Music", "music*.*", AudioFileBundlingType.InSeparateVOX, true, DEFAULT_AUDIO_TYPE_MUSIC);
            ImportAllFilesFromDirectoryIntoNewFolder("Sounds", "Sound", "sound*.*", AudioFileBundlingType.InGameEXE, false, DEFAULT_AUDIO_TYPE_SOUND);
        }

        private void ImportAllFilesFromDirectoryIntoNewFolder(string newFolderName, string sourceDir, string fileMask, AudioFileBundlingType bundlingType, bool repeat, int type)
        {
            AudioClipFolder soundFolder = new AudioClipFolder(newFolderName);
            soundFolder.DefaultBundlingType = bundlingType;
            soundFolder.DefaultRepeat = repeat ? InheritableBool.True : InheritableBool.False;
            soundFolder.DefaultType = (int)type;
            _agsEditor.CurrentGame.RootAudioClipFolder.SubFolders.Add(soundFolder);

            if (Directory.Exists(sourceDir))
            {
                foreach (string fileName in Utilities.GetDirectoryFileList(sourceDir, fileMask))
                {
                    string fileNameLowerCase = fileName.ToLower();
                    AudioClip newClip = CreateAudioClipForFile(fileNameLowerCase);
                    if (newClip != null)
                    {
                        newClip.BundlingType = bundlingType;
                        newClip.Type = type;
                        soundFolder.Items.Add(newClip);
                    }
                }
            }
        }

        private void _agsEditor_GetSourceControlFileList(IList<string> fileNames)
        {
            if (_agsEditor.CurrentGame.Settings.BinaryFilesInSourceControl)
            {
                string audioFolderPath = Path.Combine(_agsEditor.GameDirectory, AudioClip.AUDIO_CACHE_DIRECTORY);
                foreach (AudioClip clip in _agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
                {
                    fileNames.Add(Path.Combine(audioFolderPath, clip.CacheFileName));
                }
            }
        }

        private void SetActualAudioClipProperties(AudioClip clip, int inheritedVolume, bool inheritedRepeat, AudioClipPriority inheritedPriority)
        {
            if (clip.DefaultPriority == AudioClipPriority.Inherit)
            {
                clip.ActualPriority = inheritedPriority;
            }
            else
            {
                clip.ActualPriority = clip.DefaultPriority;
            }

            if (clip.DefaultRepeat == InheritableBool.Inherit)
            {
                clip.ActualRepeat = inheritedRepeat;
            }
            else
            {
                clip.ActualRepeat = (clip.DefaultRepeat == InheritableBool.True);
            }

            if (clip.DefaultVolume < 0)
            {
                clip.ActualVolume = inheritedVolume;
            }
            else
            {
                clip.ActualVolume = clip.DefaultVolume;
            }
        }

        private void AddAudioClipToListIfFileNeedsToBeCopiedFromSource(AudioClip clip, PreCompileGameEventArgs evArgs, List<AudioClip> filesToCopy, List<string> fileNamesToUpdate)
        {
            string compiledFileName = clip.CacheFileName;
            if (File.Exists(clip.SourceFileName))
            {
                bool needToCopy = false;
                if ((!File.Exists(compiledFileName)) || (evArgs.ForceRebuild))
                {
                    needToCopy = true;
                }
                else if (File.GetLastWriteTimeUtc(compiledFileName) != File.GetLastWriteTimeUtc(clip.SourceFileName))
                {
                    needToCopy = true;
                }

                if (needToCopy)
                {
                    filesToCopy.Add(clip);
                    fileNamesToUpdate.Add(compiledFileName);
                }
            }
            else if (!File.Exists(compiledFileName))
            {
                evArgs.AllowCompilation = false;
                evArgs.Errors.Add(new CompileError(clip.ScriptName + " file missing: " + clip.SourceFileName));
            }
        }

        private void PerformPreCompilationStepForFolder(AudioClipFolder folder, PreCompileGameEventArgs evArgs, List<AudioClip> filesToCopy, List<string> fileNamesToUpdate, int inheritedVolume, bool inheritedRepeat, AudioClipPriority inheritedPriority)
        {
            if (folder.DefaultPriority != AudioClipPriority.Inherit)
            {
                inheritedPriority = folder.DefaultPriority;
            }
            if (folder.DefaultRepeat != InheritableBool.Inherit)
            {
                inheritedRepeat = (folder.DefaultRepeat == InheritableBool.True) ? true : false;
            }
            if (folder.DefaultVolume >= 0)
            {
                inheritedVolume = folder.DefaultVolume;
            }

            foreach (AudioClip clip in folder.Items)
            {
                AddAudioClipToListIfFileNeedsToBeCopiedFromSource(clip, evArgs, filesToCopy, fileNamesToUpdate);
                SetActualAudioClipProperties(clip, inheritedVolume, inheritedRepeat, inheritedPriority);
            }

            foreach (AudioClipFolder subFolder in folder.SubFolders)
            {
                PerformPreCompilationStepForFolder(subFolder, evArgs, filesToCopy, fileNamesToUpdate, inheritedVolume, inheritedRepeat, inheritedPriority);
            }
        }

        private void _agsEditor_PreCompileGame(PreCompileGameEventArgs evArgs)
        {
            List<AudioClip> filesToCopy = new List<AudioClip>();
            List<string> fileNamesToUpdate = new List<string>();
            PerformPreCompilationStepForFolder(_agsEditor.CurrentGame.RootAudioClipFolder, evArgs, filesToCopy, fileNamesToUpdate, 100, false, AudioClipPriority.Normal);

            if (_agsEditor.AttemptToGetWriteAccess(fileNamesToUpdate))
            {
                for (int i = 0; i < filesToCopy.Count; i++)
                {
                    Utilities.CopyFileAndSetDestinationWritable(filesToCopy[i].SourceFileName, fileNamesToUpdate[i]);
                }
            }

            _agsEditor.CurrentGame.UpdateCachedAudioClipList();
        }

        private void ProjectTree_OnAfterLabelEdit(string commandID, ProjectTreeItem treeItem)
        {
            if (commandID.StartsWith(NODE_ID_PREFIX_CLIP_TYPE))
            {
                // this must be first because the AudioClipType prefix
                // is also the AudioClip prefix if we don't check this!
                AudioClipTypeTypeConverter.RefreshAudioClipTypeList();
            }
            else if ((commandID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!commandID.StartsWith(NODE_ID_PREFIX_FOLDER)))
            {
                AudioClip itemBeingEdited = (AudioClip)treeItem.LabelTextDataSource;

                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName, itemBeingEdited))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIconType.Warning);
                    itemBeingEdited.ScriptName = treeItem.LabelTextBeforeLabelEdit;
                    treeItem.TreeNode.Text = itemBeingEdited.ScriptName;
                }

                AudioClipTypeConverter.RefreshAudioClipList();
            }
        }

        private string GetIconKeyForAudioClip(AudioClip clip)
        {
            return _iconMappings[clip.FileType];
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "ScriptName")
            {
                AudioClip itemBeingEdited = (AudioClip)_editor.SelectedItem;
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName, itemBeingEdited))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIconType.Warning);
                    itemBeingEdited.ScriptName = (string)oldValue;
                }
                else
                {
                    RePopulateTreeView(GetNodeIDForAudioClip(itemBeingEdited));
                    AudioClipTypeConverter.RefreshAudioClipList();
                }
            }
            else if (propertyName == "Name")
            {
                RePopulateTreeView();
                if (_editor.SelectedItem is AudioClipFolder)
                {
                    _guiController.ProjectTree.SelectNode(this, GetNodeIDForFolder((AudioClipFolder)_editor.SelectedItem));
                }
                else
                {
                    _guiController.ProjectTree.SelectNode(this, AUDIO_TYPES_FOLDER_NODE_ID);
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if (controlID == SPEECH_NODE_ID)
            {
                menu.Add(new MenuCommand(SPEECH_NODE_ID, "How do I import speech?", null));
            }
            else if (controlID.StartsWith(NODE_ID_PREFIX_CLIP_TYPE))
            {
                menu.Add(new MenuCommand(COMMAND_RENAME_CLIP_TYPE, "Rename", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_CLIP_TYPE, "Delete", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_PROPERTIES_CLIP_TYPE, "Properties", null));
            }
            else if (controlID == AUDIO_TYPES_FOLDER_NODE_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_CLIP_TYPE, "New audio type", null));
            }
            else if (!IsFolderNode(controlID))
            {
                menu.Add(new MenuCommand(COMMAND_RENAME, "Rename", null));
                menu.Add(new MenuCommand(COMMAND_DELETE, "Delete", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_PROPERTIES, "Properties", null));
            }
            return menu;
        }

        protected override AudioClipFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootAudioClipFolder;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(AudioClip item)
        {
            string nodeID = GetNodeIDForAudioClip(item);
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf(this, nodeID, item.ScriptName, GetIconKeyForAudioClip(item));
            treeItem.AllowLabelEdit = true;
            treeItem.LabelTextProperty = item.GetType().GetProperty("ScriptName");
            treeItem.LabelTextDescriptionProperty = item.GetType().GetProperty("ScriptName");
            treeItem.LabelTextDataSource = item;
            return treeItem;
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_ADD_AUDIO, "Add audio file(s)...", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(MenuCommand.Separator);
            menu.Add(new MenuCommand(COMMAND_PROPERTIES, "Properties", null));
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all the audio clips that it contains?";
        }

        protected override bool CanFolderBeDeleted(AudioClipFolder folder)
        {
            return true;
        }

        protected override void AddExtraManualNodesToTree()
        {
            _guiController.ProjectTree.AddTreeLeaf(this, SPEECH_NODE_ID, "Speech", "AGSAudioSpeechIcon");
            _guiController.ProjectTree.AddTreeBranch(this, AUDIO_TYPES_FOLDER_NODE_ID, "Types", "GenericFolderIcon");
            _guiController.ProjectTree.StartFromNode(this, AUDIO_TYPES_FOLDER_NODE_ID);
            foreach (AudioClipType clipType in _agsEditor.CurrentGame.AudioClipTypes)
            {
                AddTreeNodeForAudioClipType(clipType);
            }
        }

        private string AddTreeNodeForAudioClipType(AudioClipType clipType)
        {
            string newNodeID = NODE_ID_PREFIX_CLIP_TYPE + clipType.TypeID;
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf(this, newNodeID, clipType.Name, AUDIO_CLIP_TYPE_ICON);
            treeItem.AllowLabelEdit = true;
            treeItem.LabelTextProperty = clipType.GetType().GetProperty("Name");
            treeItem.LabelTextDescriptionProperty = clipType.GetType().GetProperty("Name");
            treeItem.LabelTextDataSource = clipType;
            return newNodeID;
        }

        protected override void DeleteResourcesUsedByFolder(AudioClipFolder folder)
        {
            DeleteResourcesForAudioClips(folder.GetAllAudioClipsFromAllSubFolders());
        }

        protected override void DeleteResourcesUsedByItem(AudioClip item)
        {
            DeleteResourcesForAudioClip(item);
            AudioClipTypeConverter.SetAudioClipList(_agsEditor.CurrentGame.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders());
        }

        private void DeleteResourcesForAudioClip(AudioClip clipToDelete)
        {
            List<AudioClip> tempList = new List<AudioClip>();
            tempList.Add(clipToDelete);
            DeleteResourcesForAudioClips(tempList);
        }

        private void DeleteResourcesForAudioClips(IList<AudioClip> clipsToDelete)
        {
            List<string> filesToDelete = new List<string>();
            foreach (AudioClip clip in clipsToDelete)
            {
                if (_editor.SelectedItem == clip)
                {
                    _editor.SelectedItem = null;
                }
                filesToDelete.Add(clip.CacheFileName);
            }
            _agsEditor.DeleteFileOnDiskAndSourceControl(filesToDelete.ToArray());
            _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
            Utilities.DeleteFileIfExists(AGSEditor.AUDIO_VOX_FILE_NAME);
        }
    }
}
