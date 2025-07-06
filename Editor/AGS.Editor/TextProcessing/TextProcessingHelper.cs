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
            game.Settings.GameName = processor.ProcessText(game.Settings.GameName, GameTextType.ItemDescription);
            game.Settings.Description = processor.ProcessText(game.Settings.Description, GameTextType.ItemDescription);
            game.Settings.DeveloperName = processor.ProcessText(game.Settings.DeveloperName, GameTextType.ItemDescription);
            game.Settings.DeveloperURL = processor.ProcessText(game.Settings.DeveloperURL, GameTextType.ItemDescription);
            game.Settings.Genre = processor.ProcessText(game.Settings.Genre, GameTextType.ItemDescription);

            foreach (Dialog dialog in game.RootDialogFolder.AllItemsFlat)
            {
                foreach (DialogOption option in dialog.Options)
                {
                    option.Text = processor.ProcessText(option.Text, GameTextType.DialogOption, game.PlayerCharacter.ID);
                }

                dialog.Script = processor.ProcessText(dialog.Script, GameTextType.DialogScript);
            }

            foreach (ScriptAndHeader script in game.RootScriptFolder.AllItemsFlat)
            {                                
                string newScript = processor.ProcessText(script.Script.Text, GameTextType.Script);
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
                        label.Text = processor.ProcessText(label.Text, GameTextType.ItemDescription);
                    }
                    else
                    {
                        GUIButton button = control as GUIButton;
                        if (button != null)
                        {
                            button.Text = processor.ProcessText(button.Text, GameTextType.ItemDescription);
                        }
                    }
                }
            }

            foreach (Character character in game.RootCharacterFolder.AllItemsFlat)
            {
                character.RealName = processor.ProcessText(character.RealName, GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, character.Properties, errors);
            }

            foreach (InventoryItem item in game.RootInventoryItemFolder.AllItemsFlat)
            {
                item.Description = processor.ProcessText(item.Description, GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, item.Properties, errors);
            }

			for (int i = 0; i < game.GlobalMessages.Length; i++)
			{
				game.GlobalMessages[i] = processor.ProcessText(game.GlobalMessages[i], GameTextType.Message);
			}

            Factory.AGSEditor.RunProcessAllGameTextsEvent(processor, errors);
        }

        private static void ProcessPropertySchema(IGameTextProcessor processor, CustomPropertySchema schema,
            CompileMessages errors)
        {
            foreach (var def in schema.PropertyDefinitions.Where(n => n.Type == CustomPropertyType.Text))
            {
                def.DefaultValue = processor.ProcessText(def.DefaultValue, GameTextType.ItemDescription);
            }
        }

        public static void ProcessProperties(IGameTextProcessor processor, CustomPropertySchema schema,
            CustomProperties props, CompileMessages errors)
        {
            foreach (var def in schema.PropertyDefinitions.Where(n => n.Type == CustomPropertyType.Text))
            {
                CustomProperty prop;
                if (props.PropertyValues.TryGetValue(def.Name, out prop))
                {
                    prop.Value = processor.ProcessText(prop.Value, GameTextType.ItemDescription);
                }
            }
        }
    }
}
