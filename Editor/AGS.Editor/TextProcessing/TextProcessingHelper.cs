using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class TextProcessingHelper
    {
        public static void ProcessAllGameText(IGameTextProcessor processor, Game game, CompileMessages errors)
        {
            foreach (Dialog dialog in game.Dialogs)
            {
                foreach (DialogOption option in dialog.Options)
                {
                    option.Text = processor.ProcessText(option.Text, GameTextType.DialogOption, game.PlayerCharacter.ID);
                }

                dialog.Script = processor.ProcessText(dialog.Script, GameTextType.DialogScript);
            }

            foreach (Script script in game.Scripts)
            {
                if (!script.IsHeader)
                {
                    string newScript = processor.ProcessText(script.Text, GameTextType.Script);
                    if (newScript != script.Text)
                    {
                        // Only cause it to flag Modified if we changed it
                        script.Text = newScript;
                    }
                }
            }

            foreach (GUI gui in game.GUIs)
            {
                foreach (GUIControl control in gui.Controls)
                {
                    if (control is GUILabel)
                    {
                        ((GUILabel)control).Text = processor.ProcessText(((GUILabel)control).Text, GameTextType.ItemDescription);
                    }
                    else if (control is GUIButton)
                    {
						((GUIButton)control).Text = processor.ProcessText(((GUIButton)control).Text, GameTextType.ItemDescription);
                    }
                }
            }

            foreach (Character character in game.Characters)
            {
                character.RealName = processor.ProcessText(character.RealName, GameTextType.ItemDescription);
            }

            foreach (InventoryItem item in game.InventoryItems)
            {
                item.Description = processor.ProcessText(item.Description, GameTextType.ItemDescription);
            }

			for (int i = 0; i < game.GlobalMessages.Length; i++)
			{
				game.GlobalMessages[i] = processor.ProcessText(game.GlobalMessages[i], GameTextType.Message);
			}

            Factory.AGSEditor.RunProcessAllGameTextsEvent(processor, errors);
        }
    }
}
