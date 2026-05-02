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
                string sourceRef = string.IsNullOrEmpty(dialog.Name) ? $"Dialog {dialog.ID}" : $"Dialog {dialog.ID}; {dialog.Name}";
                foreach (DialogOption option in dialog.Options)
                {
                    option.Text = processor.ProcessText(new GameTextLine(game.PlayerCharacter.ID, option.Text, sourceRef), GameTextType.DialogOption);
                }

                dialog.Script = processor.ProcessText(dialog.Script, sourceRef, GameTextType.DialogScript);
            }

            foreach (ScriptAndHeader script in game.RootScriptFolder.AllItemsFlat)
            {                                
                string newScript = processor.ProcessText(script.Script.Text, script.Script.FileName, GameTextType.Script);
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
                character.RealName = processor.ProcessText(character.RealName, "Characters", GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, character.Properties, "Characters", errors);
            }

            foreach (InventoryItem item in game.RootInventoryItemFolder.AllItemsFlat)
            {
                item.Description = processor.ProcessText(item.Description, "Inventory items", GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, item.Properties, "Inventory items", errors);
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
                    prop.Value = processor.ProcessText(prop.Value, sourceRef, GameTextType.ItemDescription);
                }
            }
        }
    }
}
