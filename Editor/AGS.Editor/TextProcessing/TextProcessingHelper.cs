using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AGS.Editor
{
    public class TextProcessingHelper
    {
        public static void ProcessAllGameText(IGameTextProcessor processor, Game game, CompileMessages errors)
        {
            // Game info
            game.Settings.GameName = processor.ProcessText(game.Settings.GameName, "Game info", GameTextType.ItemDescription);
            game.Settings.Description = processor.ProcessText(game.Settings.Description, "Game info", GameTextType.ItemDescription);
            game.Settings.DeveloperName = processor.ProcessText(game.Settings.DeveloperName, "Game info", GameTextType.ItemDescription);
            game.Settings.DeveloperURL = processor.ProcessText(game.Settings.DeveloperURL, "Game info", GameTextType.ItemDescription);
            game.Settings.Genre = processor.ProcessText(game.Settings.Genre, "Game info", GameTextType.ItemDescription);

            foreach (Dialog dialog in game.RootDialogFolder.AllItemsFlat)
            {
                string sourceRef = string.IsNullOrEmpty(dialog.ScriptName) ? $"Dialog {dialog.ID}" : $"Dialog {dialog.ID}; {dialog.ScriptName}";
                foreach (DialogOption option in dialog.Options)
                {
                    option.Text = processor.ProcessText(GameTextLine.MakeSpeechLine(game.PlayerCharacter.ID, option.Text, sourceRef), GameTextType.DialogOption);
                    option.Text = processor.ProcessText(GameTextLine.MakeSpeechLine(game.PlayerCharacter.ID, option.Text, dialog.ScriptName), GameTextType.DialogOption);
                }

                dialog.Script = processor.ProcessText(dialog.Script, sourceRef, GameTextType.DialogScript);
                dialog.Script = processor.ProcessText(GameTextLine.MakeScript(dialog.Script, dialog.FileName, dialog.ScriptName), GameTextType.DialogScript);
            }

            foreach (ScriptAndHeader script in game.RootScriptFolder.AllItemsFlat)
            {
                string newScript = processor.ProcessText(GameTextLine.MakeScript(script.Script.Text, script.Script.FileName), GameTextType.Script);
                if (newScript != script.Script.Text)
                {
                    // Only cause it to flag Modified if we changed it
                    script.Script.Text = newScript;
                }
            }

            ProcessPropertySchema(processor, game.PropertySchema, errors);

            foreach (GUI gui in game.RootGUIFolder.AllItemsFlat)
            {
                foreach (GUIControl control in gui.Controls)
                {
                    GUILabel label = control as GUILabel;
                    if (label != null)
                    {
                        label.Text = processor.ProcessText(label.Text, "GUI", GameTextType.ItemDescription);
                    }
                    else
                    {
                        GUIButton button = control as GUIButton;
                        if (button != null)
                        {
                            button.Text = processor.ProcessText(button.Text, "GUI", GameTextType.ItemDescription);
                        }
                    }
                }
            }

            foreach (Character character in game.RootCharacterFolder.AllItemsFlat)
            {
                character.DisplayName = processor.ProcessText(character.DisplayName, "Characters", GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, character.Properties, "Characters", errors);
            }

            foreach (InventoryItem item in game.RootInventoryItemFolder.AllItemsFlat)
            {
                item.DisplayName = processor.ProcessText(item.DisplayName, "Inventory items", GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, item.Properties, "Inventory items", errors);
            }

            if (game.Settings.TranslateTextParser)
            {
                var parserWordLists = ConstructParserWordsList(game.TextParser);
                // What we do here: because parser dictionary entry may match some
                // generic game text (e.g. if it's a item's name), the cheap and dumb solution
                // that we use is that we prefix the source line with a comma
                // (this works as it's a comma-separated list).
                foreach (var wordKey in parserWordLists.Keys)
                {
                    string word = $",{parserWordLists[wordKey]}";
                    processor.ProcessText(GameTextLine.MakeParserWord(wordKey, word, "Text Parser"), GameTextType.TextParserWord);
                }
            }

            Factory.AGSEditor.RunProcessAllGameTextsEvent(processor, errors);
        }

        private static void ProcessPropertySchema(IGameTextProcessor processor, CustomPropertySchema schema,
            CompileMessages errors)
        {
            foreach (var def in schema.PropertyDefinitions.Where(
                n => (n.Type == CustomPropertyType.Text) && n.Translated))
            {
                def.DefaultValue = processor.ProcessText(def.DefaultValue, "Custom properties", GameTextType.ItemDescription);
            }
        }

        public static void ProcessProperties(IGameTextProcessor processor, CustomPropertySchema schema,
            CustomProperties props, string sourceRef, CompileMessages errors)
        {
            foreach (var def in schema.PropertyDefinitions.Where(
                n => (n.Type == CustomPropertyType.Text) && n.Translated))
            {
                CustomProperty prop;
                if (props.PropertyValues.TryGetValue(def.Name, out prop))
                {
                    prop.Value = processor.ProcessText(new GameTextLine(prop.Value, sourceRef), GameTextType.ItemDescription);
                }
            }
        }

        private static Dictionary<int, string> ConstructParserWordsList(TextParser parser)
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
    }
}
