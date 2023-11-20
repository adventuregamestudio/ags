using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
using TypeUtils = AGS.Types.Utilities;

namespace AGS.Editor.Components
{
    class TranslationsComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Translations";
        private const string COMMAND_NEW_ITEM = "NewTranslation";
        private const string COMMAND_DELETE_ITEM = "DeleteTranslation";
        private const string COMMAND_RENAME_ITEM = "RenameTranslation";
        private const string COMMAND_UPDATE_SOURCE = "UpdateTranslation";
        private const string COMMAND_UPDATE_ALL = "UpdateAllTranslations";
        private const string COMMAND_COMPILE = "CompileTranslation";
        private const string COMMAND_MAKE_DEFAULT = "MakeDefaultTranslation";

        private const string TRANSLATION_SOURCE_FILE_LEGACY_EXTENSION = ".trs";

        private const string COMPILED_TRANSLATION_FILE_SIGNATURE = "AGSTranslation\0";
        private const int TRANSLATION_BLOCK_TRANSLATION_DATA = 1;
        private const int TRANSLATION_BLOCK_GAME_ID = 2;
        private const int TRANSLATION_BLOCK_OPTIONS = 3;
        private const string TRANSLATION_BLOCK_STROPTIONS = "ext_sopts";
        private const int TRANSLATION_BLOCK_END_OF_FILE = -1;

//        private Dictionary<AGS.Types.Font, ContentDocument> _documents;
        private Translation _itemRightClicked = null;
        private string _commandIDRightClicked = null;
        private Timer _timer = new Timer();
        private string _oldNameBeforeRename;

        public TranslationsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _guiController.ProjectTree.OnAfterLabelEdit += new ProjectTree.AfterLabelEditHandler(ProjectTree_OnAfterLabelEdit);
            _agsEditor.PreCompileGame += new AGSEditor.PreCompileGameHandler(AGSEditor_PreCompileGame);
            _timer.Interval = 20;
            _timer.Tick += new EventHandler(_timer_Tick);

            //_documents = new Dictionary<AGS.Types.Font, ContentDocument>();
            _guiController.RegisterIcon("TranslationsIcon", Resources.ResourceManager.GetIcon("translations.ico"));
            _guiController.RegisterIcon("TranslationIcon", Resources.ResourceManager.GetIcon("translations.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Translations", "TranslationsIcon");
            Factory.Events.GamePostLoad += ConvertTranslationsToPOFormat;
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Translations; }
        }

        private string GetFreeNameForTranslation()
        {
            int attempt = 0;
            string newFileName;
            do
            {
                attempt++;
                newFileName = "New Translation" + ((attempt == 1) ? "" : attempt.ToString());
            }
            while (_agsEditor.CurrentGame.Translations.FirstOrDefault(t => t.Name == newFileName) != null);
            return newFileName;
        }

        private void StartRename(Translation itemToRename, string nodeID)
        {
            _guiController.ProjectTree.BeginLabelEdit(this, nodeID);
        }

        private void AGSEditor_PreCompileGame(PreCompileGameEventArgs evArgs)
        {
            foreach (Translation translation in _agsEditor.CurrentGame.Translations)
            {
                if (File.Exists(translation.FileName))
                {
                    string compiledPath = Path.Combine(AGSEditor.OUTPUT_DIRECTORY,
                        Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, translation.CompiledFileName));

                    if ((evArgs.ForceRebuild) ||
						(Utilities.DoesFileNeedRecompile(translation.FileName, compiledPath)))
                    {
                        try
                        {
                            CompileTranslation(translation, evArgs.Errors);
                        }
                        catch (Exception e)
                        {
                            evArgs.Errors.Add(new CompileError(string.Format("Translation '{0}' compiled with errors: \n{1}",
                                translation.FileName, e.Message)));
                        }
                    }
                }
            }
        }

        // "Avis Durgan" in ASCII
        private byte[] EncryptKey = Encoding.ASCII.GetBytes("Avis Durgan");

        private byte[] EncryptStringToBytes(string text, Encoding encoding)
        {
            var bytes = encoding.GetBytes(text);
            for (int i = 0, key = 0; i < bytes.Length; ++i)
            {
                bytes[i] += EncryptKey[key++];
                if (key > 10)
                    key = 0;
            }
            return bytes;
        }

        private void WriteString(BinaryWriter bw, string text, Encoding encoding)
        {
            var bytes = EncryptStringToBytes(text, encoding);
            bw.Write(bytes.Length);
            bw.Write(bytes);
        }

        private void CompileTranslation(Translation translation, CompileMessages errors)
        {
            var load_errors = translation.TryLoadData();
            errors.AddRange(load_errors);
            if (load_errors.HasErrors)
                return;

            string tempFile = Path.GetTempFileName();
            Encoding textEncoding = translation.Encoding;

            using (BinaryWriter bw = new BinaryWriter(new FileStream(tempFile, FileMode.Create, FileAccess.Write)))
            {
                bw.Write(Encoding.ASCII.GetBytes(COMPILED_TRANSLATION_FILE_SIGNATURE));
                bw.Write(TRANSLATION_BLOCK_GAME_ID);
                byte[] gameName = EncryptStringToBytes(_agsEditor.CurrentGame.Settings.GameName, _agsEditor.CurrentGame.TextEncoding);
                bw.Write(gameName.Length + 4 + 4);
                bw.Write(_agsEditor.CurrentGame.Settings.UniqueID);
                bw.Write(gameName.Length);
                bw.Write(gameName);
                bw.Write(TRANSLATION_BLOCK_TRANSLATION_DATA);
                long offsetOfBlockSize = bw.BaseStream.Position;
                bw.Write((int)0); // placeholder for block size, will be filled later
                foreach (string line in translation.TranslatedEntries.Keys)
                {
                    if (translation.TranslatedEntries[line].Value.Length > 0)
                    {
                        WriteString(bw, Regex.Unescape(line), textEncoding);
                        WriteString(bw, Regex.Unescape(translation.TranslatedEntries[line].Value), textEncoding);
                    }
                }
                WriteString(bw, string.Empty, textEncoding);
                WriteString(bw, string.Empty, textEncoding);
                long mainBlockSize = (bw.BaseStream.Position - offsetOfBlockSize) - 4;
                bw.Write(TRANSLATION_BLOCK_OPTIONS);
                bw.Write((int)12);
                bw.Write(translation.NormalFont ?? -1);
                bw.Write(translation.SpeechFont ?? -1);
                bw.Write((translation.RightToLeftText == true) ? 2 : ((translation.RightToLeftText == false) ? 1 : -1));

                bw.Write((int)0); // required for compatibility
                DataFileWriter.WriteString(TRANSLATION_BLOCK_STROPTIONS, 16, bw);
                var data_len_pos = bw.BaseStream.Position;
                bw.Write((long)0); // data length placeholder
                bw.Write((int)1); // size of key/value table
                DataFileWriter.FilePutString("encoding", bw);
                DataFileWriter.FilePutString(translation.EncodingHint, bw);
                var end_pos = bw.BaseStream.Position;
                var data_len = end_pos - data_len_pos - 8;
                bw.Seek((int)data_len_pos, SeekOrigin.Begin);
                bw.Write(data_len);
                bw.Seek((int)end_pos, SeekOrigin.Begin);

                bw.Write(TRANSLATION_BLOCK_END_OF_FILE);
                bw.Write((int)0);
                bw.Seek((int)offsetOfBlockSize, SeekOrigin.Begin);
                bw.Write((int)mainBlockSize);
                bw.Close();
            }

            string destFile = Path.Combine(AGSEditor.OUTPUT_DIRECTORY,
                Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, translation.CompiledFileName));
            Utilities.TryDeleteFile(destFile);
            File.Move(tempFile, destFile);
        }

        private object UpdateTranslationsProcess(IWorkProgress progress, object translationList)
        {
            List<Translation> translations = (List<Translation>)translationList;
            TranslationGenerator generator = new TranslationGenerator();
            CompileMessages messages = generator.CreateTranslationList(_agsEditor.CurrentGame);

            if (messages.HasErrors)
                return messages; // abort for avoiding corrupting translations

            foreach (string line in generator.LinesForTranslation)
            {
                foreach (Translation translation in translations)
                {
                    if (!translation.TranslatedEntries.ContainsKey(line))
                    {
                        TranslationEntry entry = new TranslationEntry();
                        entry.Key = line;
                        entry.Value = string.Empty;
                        translation.TranslatedEntries.Add(line, entry);
                        translation.Modified = true;
                    }
                }
            }
            foreach (Translation translation in translations)
            {
                if (translation.Modified)
                {
                    if (_agsEditor.AttemptToGetWriteAccess(translation.FileName))
                    {
                        translation.SaveData();
                    }
                }
            }
            return messages;
        }

        private void DoTranslationUpdate(IList<Translation> translations)
        {
            _agsEditor.SaveGameFiles();

            List<Translation> translationsToUpdate = new List<Translation>();
            // reload current translations, in case the trs files were modified externally
            CompileMessages messages = new CompileMessages();
            foreach (var translation in translations)
            {
                CompileMessages errors = null;
                if (File.Exists(translation.FileName))
                {
                    errors = translation.TryLoadData();
                    messages.AddRange(errors);
                }
                if (errors == null || !errors.HasErrors)
                    translationsToUpdate.Add(translation);
            }

            if (translationsToUpdate.Count > 0)
            {
                messages.AddRange((CompileMessages)BusyDialog.Show("Please wait while the translation(s) are updated...",
                    new BusyDialog.ProcessingHandler(UpdateTranslationsProcess), translationsToUpdate));
            }
            _guiController.ShowOutputPanel(messages);
            if (!messages.HasErrors)
                _guiController.ShowMessage("Translation(s) updated.", MessageBoxIcon.Information);
            else
                _guiController.ShowMessage("Translation(s) update failed. Check the output window for details.", MessageBoxIcon.Error);
        }

        private void UpdateAllTranslationsWithNewDefaultText(Dictionary<string, TranslationEntry> textChanges, CompileMessages errors)
        {
            foreach (Translation otherTranslation in _agsEditor.CurrentGame.Translations)
            {
                var load_errors = otherTranslation.TryLoadData();
                errors.AddRange(load_errors);
                if (load_errors.HasErrors)
                    continue; // failure

                Dictionary<string, TranslationEntry> newTranslation = new Dictionary<string, TranslationEntry>();

                foreach (string sourceLine in otherTranslation.TranslatedEntries.Keys)
                {
                    TranslationEntry otherTranslationOfThisLine = otherTranslation.TranslatedEntries[sourceLine];
                    string newKeyName = null;
                    if ((textChanges.ContainsKey(sourceLine)) && (textChanges[sourceLine].Value.Length > 0))
                    {
                        newKeyName = textChanges[sourceLine].Value;

                        if (newTranslation.ContainsKey(newKeyName))
                        {
                            if (!string.IsNullOrEmpty(otherTranslationOfThisLine.Value))
                            {
                                errors.Add(new CompileWarning("Text '" + newKeyName + "' already has a translation; '" + sourceLine + "' translation will be lost"));
                            }
                            newKeyName = null;
                        }
                    }
                    else if (!newTranslation.ContainsKey(sourceLine))
                    {
                        newKeyName = sourceLine;
                    }

                    if (newKeyName != null)
                    {
                        if (newKeyName == otherTranslationOfThisLine.Value)
                        {
                            TranslationEntry entry = new TranslationEntry();
                            entry.Key = newKeyName;
                            entry.Value = string.Empty;
                            newTranslation.Add(newKeyName, entry);
                        }
                        else
                        {
                            otherTranslationOfThisLine.Key = newKeyName;
                            newTranslation.Add(newKeyName, otherTranslationOfThisLine);
                        }
                    }
                }

                otherTranslation.TranslatedEntries = newTranslation;
                otherTranslation.Modified = true;
                otherTranslation.SaveData();
            }
        }

        private object ReplaceGameTextWithTranslationProcess(IWorkProgress progress, object translationAsObj)
        {
            Translation translation = (Translation)translationAsObj;
            CompileMessages errors = TextImporter.ReplaceAllGameText(_agsEditor.CurrentGame, translation);
            // Make a copy of the dictionary, otherwise it can get overwritten
            // while updating its translation
            Dictionary<string, TranslationEntry> textChanges = new Dictionary<string, TranslationEntry>();
            foreach (string key in translation.TranslatedEntries.Keys)
            {
                textChanges.Add(key, translation.TranslatedEntries[key]);
            }

            UpdateAllTranslationsWithNewDefaultText(textChanges, errors);
            return errors;
        }

        private bool CheckAllTranslationsAreWritable()
        {
            List<string> fileNames = new List<string>();
            foreach (Translation trans in _agsEditor.CurrentGame.Translations)
            {
                fileNames.Add(trans.FileName);
            }
            return _agsEditor.AttemptToGetWriteAccess(fileNames);
        }

        private void ReplaceGameTextWithTranslation(Translation translation)
        {
            _agsEditor.SaveGameFiles();

            if (!CheckAllTranslationsAreWritable())
            {
                return;
            }

            CompileMessages errors = translation.TryLoadData();
            if (errors.HasErrors)
            {
            _guiController.ShowOutputPanel(errors);
                _guiController.ShowMessage("There were errors when loading the translation. Please consult the output window for details.", MessageBoxIcon.Error);
                return;
            }

            errors.AddRange((CompileMessages)BusyDialog.Show("Please wait while the game text is replaced...",
                new BusyDialog.ProcessingHandler(ReplaceGameTextWithTranslationProcess), translation));
            _guiController.ShowOutputPanel(errors);
            Factory.Events.OnRefreshAllComponentsFromGame();
            _agsEditor.SaveGameFiles(); 
            _guiController.ShowMessage("Game text replaced with the translation text.", MessageBoxIcon.Information);
        }

        private void UpdateTranslations(IList<Translation> translations)
        {
            if (_guiController.ShowQuestion("Updating the translation can take some time depending on the size of your game, and will save your game beforehand. Do you want to continue?") == DialogResult.Yes)
            {
                DoTranslationUpdate(translations);
            }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                IList<Translation> items = _agsEditor.CurrentGame.Translations;
                Translation newItem = new Translation(GetFreeNameForTranslation(), _agsEditor.CurrentGame.Settings.GameTextLanguage);
                newItem.Name = newItem.Name;
                items.Add(newItem);

                // Create a dummy placeholder file
                StreamWriter sw = new StreamWriter(newItem.FileName);
                sw.Close();
                _guiController.ShowOutputPanel(newItem.TryLoadData());

                string newNodeID = "Trl" + (items.Count - 1);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                AddTreeLeafForTranslation(newNodeID, newItem);
                _guiController.ProjectTree.SelectNode(this, newNodeID);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;

                StartRename(newItem, newNodeID);
            }
            else if (controlID == COMMAND_RENAME_ITEM)
            {
                StartRename(_itemRightClicked, _commandIDRightClicked);
            }
            else if (controlID == COMMAND_MAKE_DEFAULT)
            {
                if (_guiController.ShowQuestion("This command will replace all the text in your game with the translations in this file. This means that your current default language will be lost in the process.\n\nAdditionally, all your translations will be updated to contain this translation as the source text.\n\nAre you really sure you want to continue?", MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    ReplaceGameTextWithTranslation(_itemRightClicked);
                }
            }
            else if (controlID == COMMAND_UPDATE_SOURCE)
            {
                List<Translation> translations = new List<Translation>();
                translations.Add(_itemRightClicked);
                UpdateTranslations(translations);
            }
            else if (controlID == COMMAND_UPDATE_ALL)
            {
                UpdateTranslations(_agsEditor.CurrentGame.Translations);
            }
            else if (controlID == COMMAND_COMPILE)
            {
                CompileMessages errors = new CompileMessages();
                try
                {
                    CompileTranslation(_itemRightClicked, errors);
                }
                catch (Exception e)
                {
                    errors.Add(new CompileError(e.Message));
                }
                if (errors.Count > 0)
                {
                    _guiController.ShowMessage(string.Format("Translation compiled with errors: \n\n{0}",
                        errors[0].Message), MessageBoxIcon.Warning);
                }
                else
                {
                    _guiController.ShowMessage("Translation compiled successfully.", MessageBoxIcon.Information);
                }
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (_guiController.ShowQuestion("Are you sure you want to delete this translation?") == DialogResult.Yes)
                {
                    string removing = _itemRightClicked.FileName;
                    /*                    if (_documents.ContainsKey(_itemRightClicked))
                                        {
                                            _guiController.RemovePaneIfExists(_documents[_itemRightClicked]);
                                            _documents.Remove(_itemRightClicked);
                                        }*/
                    _agsEditor.CurrentGame.Translations.Remove(_itemRightClicked);
					_agsEditor.CurrentGame.FilesAddedOrRemoved = true;

                    if (File.Exists(_itemRightClicked.FileName))
                    {
						_agsEditor.DeleteFileOnDisk(_itemRightClicked.FileName);
                    }
                    RePopulateTreeView();
                    TranslationListTypeConverter.SetTranslationsList(_agsEditor.CurrentGame.Translations);
                }
            }
            else
            {
                if (controlID != TOP_LEVEL_COMMAND_ID)
                {
                    Translation translation = _agsEditor.CurrentGame.Translations[Convert.ToInt32(controlID.Substring(3))];
                    _guiController.ShowMessage("Currently you cannot edit translations within the editor. Load the file " + translation.FileName + " into your favourite text editor to do your translation work.", MessageBoxIcon.Information);
                    /*                    if (!_documents.ContainsKey(chosenFont))
                                        {
                                            Dictionary<string, object> list = new Dictionary<string, object>();
                                            list.Add(chosenFont.Name + " (Font " + chosenFont.ID + ")", chosenFont);

                                            _documents.Add(chosenFont, new ContentDocument(new FontEditor(chosenFont), chosenFont.WindowTitle, this, list));
                                            _documents[chosenFont].SelectedPropertyGridObject = chosenFont;
                                        }
                                        _guiController.AddOrShowPane(_documents[chosenFont]);*/
                }
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                RePopulateTreeView();
                /*
                foreach (ContentDocument doc in _documents.Values)
                {
                    doc.Name = ((FontEditor)doc.Control).ItemToEdit.WindowTitle;
                }*/
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_UPDATE_ALL, "Update all", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New translation", null));

                if (_agsEditor.CurrentGame.Translations.Count < 1)
                {
                    // can't update if none there!
                    menu[0].Enabled = false;
                }
            }
            else
            {
                int translationIndex = Convert.ToInt32(controlID.Substring(3));
                _itemRightClicked = _agsEditor.CurrentGame.Translations[translationIndex];
                _commandIDRightClicked = controlID;
                menu.Add(new MenuCommand(COMMAND_UPDATE_SOURCE, "Update", null));
                menu.Add(new MenuCommand(COMMAND_COMPILE, "Compile", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_RENAME_ITEM, "Rename", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_MAKE_DEFAULT, "Make default language", null));
/*                if (fontID < BUILT_IN_FONTS)
                {
                    // can't delete built-in fonts
                    menu[menu.Count - 1].Enabled = false;
                }*/
            }
            return menu;
        }

        public override void RefreshDataFromGame()
        {/*
            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();
            */
            RePopulateTreeView();
            TranslationListTypeConverter.SetTranslationsList(_agsEditor.CurrentGame.Translations);
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            _timer.Stop();

            Translation translation = _itemRightClicked;

            if (File.Exists(translation.FileName))
            {
                _guiController.ShowMessage("A translation with the name " + translation.FileName + " already exists.", MessageBoxIcon.Warning);
                translation.Name = _oldNameBeforeRename;
                RePopulateTreeView();
            }
            else
            {
                string sourcePath = Path.GetFullPath(_oldNameBeforeRename + Translation.TRANSLATION_SOURCE_FILE_EXTENSION);
                string destPath = Path.GetFullPath(translation.FileName);

                File.Move(sourcePath, destPath);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;

                if ((translation.TranslatedEntries == null) || (translation.TranslatedEntries.Count == 0))
                {
                    if (_guiController.ShowQuestion("Would you like to update the new translation file with all the latest game messages? This could take some time, and your game will be saved first. You can do this later yourself by choosing 'Update' on the context menu?") == DialogResult.Yes)
                    {
                        List<Translation> translations = new List<Translation>();
                        translations.Add(translation);
                        DoTranslationUpdate(translations);
                    }
                }
            }
            TranslationListTypeConverter.SetTranslationsList(_agsEditor.CurrentGame.Translations);
        }

        private void ProjectTree_OnAfterLabelEdit(string commandID, ProjectTreeItem treeItem)
        {
            if (commandID.StartsWith("Trl"))
            {
                Translation translation = (Translation)treeItem.LabelTextDataSource;
                _oldNameBeforeRename = treeItem.LabelTextBeforeLabelEdit;
                if (translation.Name != _oldNameBeforeRename)
                {
                    _itemRightClicked = translation;
                    _timer.Start();
                }
            }
        }

        private void AddTreeLeafForTranslation(string key, Translation item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf(this, key, item.Name, "TranslationIcon");
            treeItem.AllowLabelEdit = true;
            treeItem.LabelTextProperty = item.GetType().GetProperty("Name");
            treeItem.LabelTextDescriptionProperty = item.GetType().GetProperty("FileName");
            treeItem.LabelTextDataSource = item;
        }

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            int index = 0;
            foreach (Translation item in _agsEditor.CurrentGame.Translations)
            {
                AddTreeLeafForTranslation("Trl" + index, item);
                index++;
            }
            /*
            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                FontEditor editor = (FontEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "Trl" + editor.ItemToEdit.ID);
            }
            else*/ if (_agsEditor.CurrentGame.Translations.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Trl0");
            }
        }

        /// <summary>
        /// Finishes migration from old translation sources to the new PO format.
        /// CHECKME: the format conversion itself is currently done within the Translation
        /// class itself. I'm not entirely convinced this is the optimal way, as AGS.Types
        /// may be linked by plugins and third-party tools, so maybe think that over later.
        /// </summary>
        private void ConvertTranslationsToPOFormat(Game game)
        {
            if (_agsEditor.CurrentGame.SavedXmlVersionIndex >= AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_PO_TRANSLATIONS)
                return; // Should be using PO source files at this point

            foreach (var translation in _agsEditor.CurrentGame.Translations)
            {
                string legacyFileName = translation.Name + TRANSLATION_SOURCE_FILE_LEGACY_EXTENSION;
                if (File.Exists(legacyFileName))
                {
                    LoadTranslationFromLegacySource(translation, legacyFileName);
                    translation.SaveData();
                    Utilities.TryDeleteFile(legacyFileName);
                }
            }
        }

        private bool LoadTranslationFromLegacySource(Translation translation, string fileName)
        {
            string fileEncodingHint;
            if (LoadTranslationFromLegacySource(translation, fileName, out fileEncodingHint))
                return true;
            if (string.Compare(fileEncodingHint, translation.EncodingHint) != 0)
            {
                // Try again with a proper encoding
                translation.EncodingHint = fileEncodingHint;
                return LoadTranslationFromLegacySource(translation, fileName, out fileEncodingHint);
            }
            return false;
        }

        private bool LoadTranslationFromLegacySource(Translation translation, string fileName, out string fileEncodingHint)
        {
            fileEncodingHint = translation.EncodingHint;
            using (StreamReader sr = new StreamReader(fileName, translation.Encoding))
            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    if (line.StartsWith("//"))
                    {
                        LegacyReadSpecialTags(translation, line, ref fileEncodingHint);
                        if (string.Compare(translation.EncodingHint, fileEncodingHint) != 0)
                        {
                            // Source file requires different encoding
                            return false;
                        }
                        continue;
                    }
                    string originalText = line;
                    string translatedText = sr.ReadLine();
                    if (translatedText == null)
                    {
                        break;
                    }
                    // Silently ignore any duplicates, as we can't report warnings here
                    if (!translation.TranslatedEntries.ContainsKey(originalText))
                    {
                        TranslationEntry entry = new TranslationEntry();
                        entry.Key = originalText;
                        entry.Value = translatedText;
                        translation.TranslatedEntries.Add(originalText, entry);
                    }
                }
            }
            return true;
        }

        private void LegacyReadSpecialTags(Translation translation, string line, ref string encodingHint)
        {
            const string NORMAL_FONT_TAG = "//#NormalFont=";
            const string SPEECH_FONT_TAG = "//#SpeechFont=";
            const string TEXT_DIRECTION_TAG = "//#TextDirection=";
            const string ENCODING_TAG = "//#Encoding=";
            const string TAG_DEFAULT = "DEFAULT";
            const string TAG_DIRECTION_LEFT = "LEFT";
            const string TAG_DIRECTION_RIGHT = "RIGHT";

            if (line.StartsWith(NORMAL_FONT_TAG))
            {
                translation.NormalFont = TypeUtils.ParseNullableInt(line.Substring(NORMAL_FONT_TAG.Length), TAG_DEFAULT);
            }
            else if (line.StartsWith(SPEECH_FONT_TAG))
            {
                translation.SpeechFont = TypeUtils.ParseNullableInt(line.Substring(SPEECH_FONT_TAG.Length), TAG_DEFAULT);
            }
            else if (line.StartsWith(TEXT_DIRECTION_TAG))
            {
                string directionText = line.Substring(TEXT_DIRECTION_TAG.Length);
                if (directionText == TAG_DIRECTION_LEFT)
                {
                    translation.RightToLeftText = false;
                }
                else if (directionText == TAG_DIRECTION_RIGHT)
                {
                    translation.RightToLeftText = true;
                }
                else
                {
                    translation.RightToLeftText = null;
                }
            }
            else if (line.StartsWith(ENCODING_TAG))
            {
                encodingHint = line.Substring(ENCODING_TAG.Length);
            }
        }
    }
}
