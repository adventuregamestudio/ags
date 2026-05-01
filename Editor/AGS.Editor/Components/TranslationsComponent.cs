using AGS.Types;
using ScintillaNET;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;

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

        private const string COMPILED_TRANSLATION_FILE_SIGNATURE = "AGSTranslation\0";
        private const int TRANSLATION_BLOCK_TRANSLATION_DATA = 1;
        private const int TRANSLATION_BLOCK_GAME_ID = 2;
        private const int TRANSLATION_BLOCK_OPTIONS = 3;
        private const string TRANSLATION_BLOCK_STROPTIONS = "ext_sopts";
        private const string TRANSLATION_BLOCK_FONTOVERRIDES = "ext_fonts";
        private const string TRANSLATION_BLOCK_PARSERDICT = "ext_parserdict";
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

        private void WriteExtGameID(BinaryWriter bw, Translation translation, CompileMessages error)
        {
            bw.Write(_agsEditor.CurrentGame.Settings.UniqueID);
            WriteString(bw, _agsEditor.CurrentGame.Settings.GameName, _agsEditor.CurrentGame.TextEncoding);
        }

        private void WriteExtDictionary(BinaryWriter bw, Translation translation, List<Tuple<string, string>> translatedLines, CompileMessages error)
        {
            Encoding textEncoding = translation.Encoding;
            foreach (var item in translatedLines)
            {
                string line = item.Item1;
                string translatedLine = item.Item2;
                if (translatedLine.Length > 0)
                {
                    string src_line = TextUtils.PreprocessLineForOldStyleLinebreaks(line);
                    string trs_line = TextUtils.PreprocessLineForOldStyleLinebreaks(translatedLine);
                    WriteString(bw, Regex.Unescape(src_line), textEncoding);
                    WriteString(bw, Regex.Unescape(trs_line), textEncoding);
                }
            }
            WriteString(bw, string.Empty, textEncoding);
            WriteString(bw, string.Empty, textEncoding);
        }

        private void WriteExtParserDict(BinaryWriter bw, Translation translation, List<Tuple<string, ushort>> parserDict, CompileMessages errors)
        {
            // Follow same data format as the main parser dictionary in the game file
            DataFileWriter.WriteTextParserDictionary(parserDict, bw);
        }

        private void WriteExtTextOptions(BinaryWriter bw, Translation translation, CompileMessages error)
        {
            bw.Write(translation.NormalFont ?? -1);
            bw.Write(translation.SpeechFont ?? -1);
            bw.Write((translation.RightToLeftText == true) ? 2 : ((translation.RightToLeftText == false) ? 1 : -1));
            // Since 3.6.3.10: parser options flag set
            int flags = 0;
            if (translation.AutoTranslateParserSaid) flags |= 0x0001;
            bw.Write(flags);
        }

        private void WriteExtStrOptions(BinaryWriter bw, Translation translation, CompileMessages errors)
        {
            bw.Write((int)2); // size of key/value table
            DataFileWriter.FilePutString("encoding", bw);
            DataFileWriter.FilePutString(translation.EncodingHint, bw);
            DataFileWriter.FilePutString("language", bw);
            DataFileWriter.FilePutString(translation.TextLanguage, bw);
        }
        
        private void WriteExtFontOverrides(BinaryWriter bw, Translation translation, CompileMessages errors)
        {
            bw.Write(translation.FontOverrides.Count);
            foreach (var fontOverride in translation.FontOverrides)
            {
                bw.Write(fontOverride.Key); // font to override
                var font = fontOverride.Value; // font to override with
                bw.Write(font.ID);
                // ID >= 0 means a replacement is another built-in font
                // ID < 0 means a replacement is a runtime-generated font
                if (font.ID < 0)
                {
                    DataFileWriter.WriteFontInfo(bw, font);
                    // NOTE: we write 3.6.0 extension right away, because this TRA
                    // extension is introduced later. But if there will be more
                    // font extensions, then we must have a distinct ext in TRA as well!
                    DataFileWriter.WriteFontInfo_Ex360(bw, font);
                    // Explicit font's filename: this corresponds to 4.* font's FileName.
                    DataFileWriter.FilePutString(font.ProjectFilename, bw);
                }
            }
        }

        private void CompileTranslation(Translation translation, CompileMessages errors)
        {
            var load_errors = translation.TryLoadData();
            errors.AddRange(load_errors);
            if (load_errors.HasErrors)
                return;

            string tempFile = Path.GetTempFileName();
            Encoding textEncoding = translation.Encoding;

            var translatedLines = new List<Tuple<string, string>>();
            var parserDict = new List<Tuple<string, ushort>>();
            SplitTranslationItemsIntoSections(translation, ref translatedLines, ref parserDict);

            using (BinaryWriter bw = new BinaryWriter(new FileStream(tempFile, FileMode.Create, FileAccess.Write)))
            {
                bw.Write(Encoding.ASCII.GetBytes(COMPILED_TRANSLATION_FILE_SIGNATURE));

                DataFileWriter.WriteExtension(string.Empty, TRANSLATION_BLOCK_GAME_ID, DataFileWriter.DataExtFlags.ID32,
                    (w, e) => WriteExtGameID(w, translation, e), bw, errors);

                DataFileWriter.WriteExtension(string.Empty, TRANSLATION_BLOCK_TRANSLATION_DATA, DataFileWriter.DataExtFlags.ID32,
                    (w, e) => WriteExtDictionary(w, translation, translatedLines, e), bw, errors);

                if (parserDict.Count > 0)
                {
                    DataFileWriter.WriteExtension(TRANSLATION_BLOCK_PARSERDICT, 0, DataFileWriter.DataExtFlags.ID32,
                        (w, e) => WriteExtParserDict(w, translation, parserDict, e), bw, errors);
                }

                DataFileWriter.WriteExtension(string.Empty, TRANSLATION_BLOCK_OPTIONS, DataFileWriter.DataExtFlags.ID32,
                    (w, e) => WriteExtTextOptions(w, translation, e), bw, errors);

                DataFileWriter.WriteExtension(TRANSLATION_BLOCK_STROPTIONS, 0, DataFileWriter.DataExtFlags.ID32,
                    (w, e) => WriteExtStrOptions(w, translation, e), bw, errors);

                if (translation.FontOverrides.Count > 0)
                {
                    DataFileWriter.WriteExtension(TRANSLATION_BLOCK_FONTOVERRIDES, 0, DataFileWriter.DataExtFlags.ID32,
                        (w, e) => WriteExtFontOverrides(w, translation, e), bw, errors);
                }

                bw.Write(TRANSLATION_BLOCK_END_OF_FILE);
                bw.Write((int)0);
                bw.Close();
            }

            string destFile = Path.Combine(AGSEditor.OUTPUT_DIRECTORY,
                Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, translation.CompiledFileName));
            Utilities.ExecuteOrError(() =>
            {
                Utilities.TryDeleteFile(destFile);
                File.Move(tempFile, destFile);
            }, $"Failed to replace an old file {destFile}", errors);
        }

        private void SplitTranslationItemsIntoSections(Translation translation, ref List<Tuple<string, string>> translatedLines,
            ref List<Tuple<string, ushort>> wordsDictionary)
        {
            foreach (string line in translation.TranslatedLines.Keys)
            {
                string translatedLine = translation.TranslatedLines[line];
                if (translatedLine.Length > 0)
                {
                    TranslationEntryOptions options;
                    if (translation.TranslatedEntryOptions.TryGetValue(line, out options))
                    {
                        // Skip obsolete entries
                        if (options.IsObsolete)
                        {
                            continue;
                        }
                        // Words dictionary is written in a separate data section
                        if (options.IsParserDictionary)
                        {
                            wordsDictionary.Add(new Tuple<string, ushort>(translatedLine, (ushort)options.ParserWordID));
                            continue;
                        }
                    }

                    translatedLines.Add(new Tuple<string, string>(line, translatedLine));
                }
            }
        }

        private Dictionary<int, string> ConstructParserWordsList(TextParser parser)
        {
            var wordsPerID = new Dictionary<int, List<string>>();
            foreach (var word in parser.Words)
            {
                if (!wordsPerID.ContainsKey(word.WordGroup))
                    wordsPerID[word.WordGroup] = new List<string>();
                wordsPerID[word.WordGroup].Add(word.Word);
            }

            var wordLists = new Dictionary<int, string>();
            foreach (var wordList in wordsPerID)
            {
                StringBuilder sb = new StringBuilder();
                foreach (var word in wordList.Value)
                {
                    if (sb.Length > 0)
                        sb.Append(',');
                    sb.Append(word);
                }
                wordLists[wordList.Key] = sb.ToString();
            }
            return wordLists;
        }

        private object UpdateTranslationsProcess(IWorkProgress progress, object translationList)
        {
            List<Translation> translations = (List<Translation>)translationList;
            TranslationGenerator generator = new TranslationGenerator();
            CompileMessages messages = generator.CreateTranslationList(_agsEditor.CurrentGame);
            if (messages.HasErrors)
                return messages; // abort for avoiding corrupting translations
            // (Optionally) prepare Text Parser's dictionary
            Dictionary<int, string> parserWordLists = null;
            if (_agsEditor.CurrentGame.Settings.TranslateTextParser)
            {
                parserWordLists = ConstructParserWordsList(_agsEditor.CurrentGame.TextParser);
                // What we do here: because parser dictionary entry may match some
                // generic game text (e.g. if it's a item's name), the cheap and dumb solution
                // that we use is that we prefix the source line with a comma
                // (this works as it's a comma-separated list).
                var fixedWordLists = new Dictionary<int, string>();
                foreach (var wordKey in parserWordLists.Keys)
                {
                    fixedWordLists[wordKey] = $",{parserWordLists[wordKey]}";
                }
                parserWordLists = fixedWordLists;
            }

            HashSet<string> keyedTexts = new HashSet<string>(generator.LinesForTranslation);
            if (parserWordLists != null)
            {
                foreach (var list in parserWordLists)
                    keyedTexts.Add(list.Value);
            }

            // Merge game texts in
            foreach (Translation translation in translations)
            {
                // Remove or mark as obsolete those texts that are not found in game
                List<string> removeLines = new List<string>();
                foreach (var entry in translation.TranslatedLines)
                {
                    if (!keyedTexts.Contains(entry.Key))
                    {
                        if (string.IsNullOrEmpty(entry.Value))
                        {
                            removeLines.Add(entry.Key);
                            translation.Modified = true;
                        }
                        else
                        {
                            TranslationEntryOptions entryOptions = null;
                            if (translation.TranslatedEntryOptions.ContainsKey(entry.Key))
                                entryOptions = translation.TranslatedEntryOptions[entry.Key];
                            else
                                entryOptions = new TranslationEntryOptions();
                            if (!entryOptions.IsObsolete)
                            {
                                entryOptions.IsObsolete = true;
                                entryOptions.Metadata.Add("OBSOLETE");
                                translation.TranslatedEntryOptions[entry.Key] = entryOptions;
                                translation.Modified = true;
                            }
                        }
                    }
                }

                // Remove obsolete translation lines
                foreach (var remKey in removeLines)
                {
                    translation.TranslatedLines.Remove(remKey);
                }

                // Add current game texts
                foreach (string line in generator.LinesForTranslation)
                {
                    if (!translation.TranslatedLines.ContainsKey(line))
                    {
                        translation.TranslatedLines.Add(line, string.Empty);
                        translation.Modified = true;
                    }
                }
            }

            // (Optionally) add Text Parser's dictionary
            if (parserWordLists != null)
            {
                foreach (Translation translation in translations)
                {
                    foreach (var list in parserWordLists)
                    {
                        int wordsKey = list.Key;
                        string wordsValue = list.Value;
                        if (!translation.TranslatedLines.ContainsKey(wordsValue))
                        {
                            translation.TranslatedLines.Add(wordsValue, string.Empty);
                            var entryOptions = new TranslationEntryOptions();
                            entryOptions.Metadata.Add($"PARSERWORD={wordsKey}");
                            entryOptions.ParserWordID = wordsKey;
                            translation.TranslatedEntryOptions[wordsValue] = entryOptions;
                            translation.Modified = true;
                        }
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

        private void UpdateAllTranslationsWithNewDefaultText(Dictionary<string, string> textChanges, CompileMessages errors)
        {
            foreach (Translation otherTranslation in _agsEditor.CurrentGame.Translations)
            {
                var load_errors = otherTranslation.TryLoadData();
                errors.AddRange(load_errors);
                if (load_errors.HasErrors)
                    continue; // failure

                Dictionary<string, string> newTranslation = new Dictionary<string, string>();

                foreach (string sourceLine in otherTranslation.TranslatedLines.Keys)
                {
                    string otherTranslationOfThisLine = otherTranslation.TranslatedLines[sourceLine];
                    string newKeyName = null;
                    if ((textChanges.ContainsKey(sourceLine)) && (textChanges[sourceLine].Length > 0))
                    {
                        newKeyName = textChanges[sourceLine];

                        if (newTranslation.ContainsKey(newKeyName))
                        {
                            if (!string.IsNullOrEmpty(otherTranslationOfThisLine))
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
                        if (newKeyName == otherTranslationOfThisLine)
                        {
                            newTranslation.Add(newKeyName, string.Empty);
                        }
                        else
                        {
                            newTranslation.Add(newKeyName, otherTranslationOfThisLine);
                        }
                    }
                }

                otherTranslation.TranslatedLines = newTranslation;
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
            Dictionary<string,string> textChanges = new Dictionary<string,string>();
            foreach (string key in translation.TranslatedLines.Keys)
            {
                textChanges.Add(key, translation.TranslatedLines[key]);
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
                Translation newItem = new Translation(GetFreeNameForTranslation());
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
                if (_itemRightClicked != null)
                {
                    menu.Add(new MenuCommand(COMMAND_UPDATE_SOURCE, "Update", null));
                    menu.Add(new MenuCommand(COMMAND_COMPILE, "Compile", null));
                    menu.Add(MenuCommand.Separator);
                    menu.Add(new MenuCommand(COMMAND_RENAME_ITEM, "Rename", null));
                    menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete", null));
                    menu.Add(MenuCommand.Separator);
                    menu.Add(new MenuCommand(COMMAND_MAKE_DEFAULT, "Make default language", null));
                }
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

                if ((translation.TranslatedLines == null) || (translation.TranslatedLines.Count == 0))
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

    }
}
