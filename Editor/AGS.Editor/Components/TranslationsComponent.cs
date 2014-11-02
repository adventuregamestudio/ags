using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

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
            while (File.Exists(newFileName + Translation.TRANSLATION_SOURCE_FILE_EXTENSION));
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
                    string compiledPath = Path.Combine(AGSEditor.Instance.CompiledDirectory, translation.CompiledFileName);

                    if ((evArgs.ForceRebuild) ||
						(Utilities.DoesFileNeedRecompile(translation.FileName, compiledPath)))
                    {
                        CompileTranslation(translation, evArgs.Errors);
                    }
                }
            }
        }

        private string ConvertEscapedCharacters(string scriptStyleString)
        {
            // The translation source has \" and \\, but the engine expects " and \
            return scriptStyleString.Replace("\\\"", "\"").Replace("\\\\", "\\");
        }

        private void CompileTranslation(Translation translation, CompileMessages errors)
        {
            translation.LoadData();

            if (translation.TranslatedLines.Count < 1)
            {
                errors.Add(new CompileError("Translation " + translation.FileName + " appears to be empty. You must update the source before compiling.", translation.FileName, 1));
                return;
            }

            string compiledFile = Path.Combine(AGSEditor.Instance.CompiledDirectory, translation.CompiledFileName);
            bool foundTranslatedLine = false;

            using (BinaryWriter bw = new BinaryWriter(new FileStream(compiledFile, FileMode.Create, FileAccess.Write)))
            {
                bw.Write(Encoding.ASCII.GetBytes(COMPILED_TRANSLATION_FILE_SIGNATURE));
                bw.Write(TRANSLATION_BLOCK_GAME_ID);
                byte[] gameName = Factory.NativeProxy.TransformStringToBytes(_agsEditor.CurrentGame.Settings.GameName);
                bw.Write(gameName.Length + 4);
                bw.Write(_agsEditor.CurrentGame.Settings.UniqueID);
                bw.Write(gameName);
                bw.Write(TRANSLATION_BLOCK_TRANSLATION_DATA);
                long offsetOfBlockSize = bw.BaseStream.Position;
                bw.Write((int)0);
                foreach (string line in translation.TranslatedLines.Keys)
                {
                    if (translation.TranslatedLines[line].Length > 0)
                    {
                        foundTranslatedLine = true;
                        bw.Write(Factory.NativeProxy.TransformStringToBytes(ConvertEscapedCharacters(line)));
                        bw.Write(Factory.NativeProxy.TransformStringToBytes(ConvertEscapedCharacters(translation.TranslatedLines[line])));
                    }
                }
                bw.Write(Factory.NativeProxy.TransformStringToBytes(string.Empty));
                bw.Write(Factory.NativeProxy.TransformStringToBytes(string.Empty));
                long mainBlockSize = (bw.BaseStream.Position - offsetOfBlockSize) - 4;
                bw.Write(TRANSLATION_BLOCK_OPTIONS);
                bw.Write((int)12);
                bw.Write(translation.NormalFont ?? -1);
                bw.Write(translation.SpeechFont ?? -1);
                bw.Write((translation.RightToLeftText == true) ? 2 : ((translation.RightToLeftText == false) ? 1 : -1));
                bw.Write(TRANSLATION_BLOCK_END_OF_FILE);
                bw.Write((int)0);
                bw.Seek((int)offsetOfBlockSize, SeekOrigin.Begin);
                bw.Write((int)mainBlockSize);
                bw.Close();
            }

            if (!foundTranslatedLine)
            {
                errors.Add(new CompileError("Translation " + translation.FileName + " did not appear to have any translated lines. Make sure you translate some text before compiling the translation.", translation.FileName, 1));
                File.Delete(compiledFile);
            }
        }

        private object UpdateTranslationsProcess(object translationList)
        {
            List<Translation> translations = (List<Translation>)translationList;
            TranslationGenerator generator = new TranslationGenerator();
            CompileMessages errors = generator.CreateTranslationList(_agsEditor.CurrentGame);
            foreach (string line in generator.LinesForTranslation)
            {
                foreach (Translation translation in translations)
                {
                    if (!translation.TranslatedLines.ContainsKey(line))
                    {
                        translation.TranslatedLines.Add(line, string.Empty);
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
            return errors;
        }

        private void DoTranslationUpdate(IList<Translation> translations)
        {
            _agsEditor.SaveGameFiles();
            CompileMessages errors = (CompileMessages)BusyDialog.Show("Please wait while the translation(s) are updated...", new BusyDialog.ProcessingHandler(UpdateTranslationsProcess), translations);
            _guiController.ShowOutputPanel(errors);
            _guiController.ShowMessage("Translation(s) updated.", MessageBoxIcon.Information);
        }

        private void UpdateAllTranslationsWithNewDefaultText(Dictionary<string, string> textChanges, CompileMessages errors)
        {
            foreach (Translation otherTranslation in _agsEditor.CurrentGame.Translations)
            {
                otherTranslation.LoadData();
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

        private object ReplaceGameTextWithTranslationProcess(object translationAsObj)
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
            
            translation.LoadData();
            CompileMessages errors = (CompileMessages)BusyDialog.Show("Please wait while the game text is replaced...", new BusyDialog.ProcessingHandler(ReplaceGameTextWithTranslationProcess), translation);
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
                newItem.LoadData();
                
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
                CompileTranslation(_itemRightClicked, errors);
                if (errors.Count > 0)
                {
                    _guiController.ShowMessage(errors[0].Message, MessageBoxIcon.Warning);
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
						_agsEditor.DeleteFileOnDiskAndSourceControl(_itemRightClicked.FileName);
                    }
                    RePopulateTreeView();
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

                if (_agsEditor.SourceControlProvider.IsFileUnderSourceControl(sourcePath))
                {
                    _agsEditor.SourceControlProvider.RenameFile(sourcePath, destPath);
                }

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
