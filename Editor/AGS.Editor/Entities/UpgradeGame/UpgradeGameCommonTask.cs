using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// UpgradeGameCommonTask performs any routinely updates to the Game data
    /// that do not require user's attention. This task is run implicitly
    /// whenever AGS Editor loads an older project.
    /// </summary>
    public class UpgradeGameCommonTask : IUpgradeGameTask
    {
        /// <summary>
        /// A unique string identifier of this upgrade task.
        /// </summary>
        public string ID { get { return "UpgradeGameCommon"; } }
        /// <summary>
        /// An arbitrary title, used to identify this task when
        /// presenting to a user.
        /// </summary>
        public string Title { get { return "Update Game to the new version"; } }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description { get { return ""; } }
        /// <summary>
        /// A game project version that introduced this upgrade task.
        /// If a loaded game has a less project version, then this task
        /// must be applied, otherwise it should not.
        /// Returns null if should be applied regardless of the game version
        /// (but the execution process may still have version checks inside).
        /// </summary>
        public System.Version GameVersion { get { return null; /* any version */ } }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get { return null; /* any version */ } }
        /// <summary>
        /// Tells whether this upgrade task is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        public bool Implicit { get { return true; } }
        /// <summary>
        /// Tells whether this upgrade task may be disabled by user's choice.
        /// </summary>
        public bool Optional { get { return false; } }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// task had errors.
        /// </summary>
        public bool AllowToSkipIfHadErrors { get { return true; } }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this task had errors.
        /// </summary>
        public bool RequestConfirmationOnErrors { get { return false; } }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        /// <summary>
        /// Provides a WizardPage control used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public UpgradeGameWizardPage CreateWizardPage(Game game)
        {
            return null;
        }
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public void ApplyOptions(Dictionary<string, string> options)
        {
            // does not have any options
        }
        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
#pragma warning disable 0612, 0618
            int xmlVersionIndex = 0;
            if (game.SavedXmlVersionIndex.HasValue)
            {
                xmlVersionIndex = game.SavedXmlVersionIndex.Value;
            }

            if (xmlVersionIndex < 2)
            {
                // Upgrade old games to use the Anti-Glide Mode setting
                foreach (Character character in game.RootCharacterFolder.AllItemsFlat)
                {
                    character.MovementLinkedToAnimation = game.Settings.AntiGlideMode;
                }
            }

            if (xmlVersionIndex < 3)
            {
                // Upgrade old games to flatten the dialog scripts
                foreach (Dialog dialog in game.RootDialogFolder.AllItemsFlat)
                {
                    dialog.Script = RemoveAllLeadingSpacesFromLines(dialog.Script);
                }
            }

            if (xmlVersionIndex < 8)
            {
                // GUIListBox Translated property should be false, as they never translated in older games
                foreach (GUI gui in game.GUIs)
                {
                    foreach (GUIControl guic in gui.Controls)
                    {
                        if (guic is GUIListBox)
                            (guic as GUIListBox).Translated = false;
                    }
                }
            }

            if (xmlVersionIndex < 15)
            {
                game.DefaultSetup.SetDefaults();
            }

            if (xmlVersionIndex < 18)
            {
                foreach (Font font in game.Fonts)
                    font.SizeMultiplier = 1;
                // Apply font scaling to each individual font settings.
                // Bitmap fonts save multiplier explicitly, while vector fonts have their size doubled.
                if (game.Settings.HighResolution && !game.Settings.FontsForHiRes)
                {
                    foreach (Font font in game.Fonts)
                    {
                        if (font.PointSize == 0)
                        {
                            font.SizeMultiplier = 2;
                        }
                        else
                        {
                            font.PointSize *= 2;
                        }
                    }
                }
            }

            if (xmlVersionIndex < 18)
            {
                game.Settings.DefaultRoomMaskResolution = 1;
            }

            if (xmlVersionIndex < 19)
            {
                game.Settings.GameFileName = AGSEditor.Instance.BaseGameFileName ??
                    Path.GetFileName(game.DirectoryPath);

                var buildNames = new Dictionary<string, string>();
                foreach (IBuildTarget target in BuildTargetsInfo.GetRegisteredBuildTargets())
                {
                    buildNames[target.Name] = game.Settings.GameFileName;
                }
                game.WorkspaceState.SetLastBuildGameFiles(buildNames);
            }

            if (xmlVersionIndex < 20)
            {
                // Set the alpha channel requests for re-import based on the presence of an alpha channel
                foreach (Sprite sprite in game.RootSpriteFolder.GetAllSpritesFromAllSubFolders())
                {
                    sprite.ImportAlphaChannel = sprite.AlphaChannel;
                }
            }

            if (xmlVersionIndex < 21)
            {
                // Assign audio clip ids to match and solidify their current position in AudioClips array.
                int clipId = 0;
                foreach (AudioClip clip in game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
                {
                    clip.ID = clipId++;
                }
                game.RootAudioClipFolder.Sort(true);
            }

            if (xmlVersionIndex < 23)
            {
                // Set the import dimensions based on existing sprite dimensions
                foreach (Sprite sprite in game.RootSpriteFolder.GetAllSpritesFromAllSubFolders())
                {
                    sprite.ImportWidth = sprite.Width;
                    sprite.ImportHeight = sprite.Height;
                }
            }

            if (xmlVersionIndex < 24)
            {
                // get all known source images and their largest known size
                // (avoiding System.Drawing / GDI as a dependency to load the project)
                Dictionary<string, Tuple<int, int>> sourceMaxSize = new Dictionary<string, Tuple<int, int>>(StringComparer.OrdinalIgnoreCase);

                foreach (Sprite sprite in game.RootSpriteFolder.GetAllSpritesFromAllSubFolders())
                {
                    if (!string.IsNullOrWhiteSpace(sprite.SourceFile))
                    {
                        int currentX = sprite.OffsetX + sprite.ImportWidth;
                        int currentY = sprite.OffsetY + sprite.ImportHeight;

                        if (sourceMaxSize.ContainsKey(sprite.SourceFile))
                        {
                            int maxX = sourceMaxSize[sprite.SourceFile].Item1;
                            int maxY = sourceMaxSize[sprite.SourceFile].Item2;
                            if (maxX < currentX) maxX = currentX;
                            if (maxY < currentY) maxY = currentY;
                            sourceMaxSize[sprite.SourceFile] = Tuple.Create(maxX, maxY);
                        }
                        else
                        {
                            sourceMaxSize.Add(sprite.SourceFile, Tuple.Create(currentX, currentY));
                        }
                    }
                }

                // Set the tiled image flag for existing imports - the only misdetection would be
                // a single import from a source image that starts at 0,0, but wasn't for the
                // entire image
                foreach (Sprite sprite in game.RootSpriteFolder.GetAllSpritesFromAllSubFolders())
                {
                    if (sprite.OffsetX > 0 || sprite.OffsetY > 0)
                    {
                        sprite.ImportAsTile = true;
                    }
                    else if (sourceMaxSize.ContainsKey(sprite.SourceFile))
                    {
                        int maxX = sourceMaxSize[sprite.SourceFile].Item1;
                        int maxY = sourceMaxSize[sprite.SourceFile].Item2;
                        sprite.ImportAsTile = sprite.ImportWidth < maxX || sprite.ImportHeight < maxY;
                    }
                    else
                    {
                        sprite.ImportAsTile = false;
                    }
                }
            }

            if (xmlVersionIndex >= 21 && xmlVersionIndex <= 25)
            {
                // Remap erroneous volatile clip ID references back to Fixed Index
                Dictionary<int, int> audioIDToIndex = new Dictionary<int, int>();
                foreach (AudioClip clip in game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
                {
                    audioIDToIndex.Add(clip.ID, clip.Index);
                }

                foreach (Types.View view in game.RootViewFolder.AllItemsFlat)
                {
                    foreach (Types.ViewLoop loop in view.Loops)
                    {
                        foreach (Types.ViewFrame frame in loop.Frames)
                        {
                            if (frame.Sound == AudioClip.IDNoValue)
                            {
                                frame.Sound = AudioClip.FixedIndexNoValue;
                            }
                            else
                            {
                                frame.Sound = RemapAudioClipIDToFixedIndex(frame.Sound, audioIDToIndex);
                            }
                        }
                    }
                }
            }

            if (xmlVersionIndex < 3060000)
            {
                foreach (Character c in game.Characters)
                {
                    c.IdleAnimationDelay = c.AnimationDelay + 5;
                }

                game.Settings.TTFHeightDefinedBy = FontHeightDefinition.NominalHeight;
                game.Settings.TTFMetricsFixup = FontMetricsFixup.SetAscenderToHeight;
                foreach (Font font in game.Fonts)
                {
                    // For scaled-up bitmap fonts outline is xN
                    // NOTE: unfortunately as of now there's no direct way to determine if
                    // this is a bitmap font or TTF
                    if (!File.Exists(font.TTFFileName))
                    {
                        font.AutoOutlineThickness = font.SizeMultiplier;
                    }
                    font.TTFMetricsFixup = FontMetricsFixup.SetAscenderToHeight;
                }
                game.Settings.ClipGUIControls = false;
            }

            if (xmlVersionIndex < 3060020)
            {
                if (game.SavedXmlEncodingCodePage.HasValue &&
                    game.SavedXmlEncodingCodePage.Value == 65001)
                {
                    game.Settings.GameTextEncoding = "UTF-8";
                }
                else
                { // NOTE: use Encoding.GetEncoding(game.SavedXmlEncodingCodePage) if actual codepage is needed
                    game.Settings.GameTextEncoding = "ANSI";
                }

                game.Settings.UseOldKeyboardHandling = true;
            }

            if (xmlVersionIndex < 3060109)
            {
                game.Settings.ScaleCharacterSpriteOffsets = false;
            }

            if (xmlVersionIndex < 3060200 || (xmlVersionIndex > 3999900 && xmlVersionIndex < 4000010))
            {
                game.Settings.UseOldVoiceClipNaming = true;
            }

            if (xmlVersionIndex < 3060206 || (xmlVersionIndex > 3999900 && xmlVersionIndex < 4000014))
            {
                game.Settings.GameFPS = 40; // 40 was historical default FPS
            }

            // Update all the ColourNumber property values in game
            if (xmlVersionIndex < 4000009)
            {
                RemapLegacyColourProperties(game);
            }
            else if (xmlVersionIndex < 4000014)
            {
                RemapOpaqueColourProperties(game);
            }

            // Update ScriptCompiler selection
            if (xmlVersionIndex < 3999900)
            {
                game.Settings.ScriptCompiler = AGSEditor.DEFAULT_LEGACY_SCRIPT_COMPILER;
            }
            else if (xmlVersionIndex < 4000010)
            {
                game.Settings.ScriptCompiler = game.Settings.ExtendedCompiler ?
                    AGSEditor.DEFAULT_SCRIPT_COMPILER : AGSEditor.DEFAULT_LEGACY_SCRIPT_COMPILER;
            }

            // Update viewframes
            if (xmlVersionIndex < 4000012)
            {
                foreach (AGS.Types.View view in game.ViewFlatList)
                {
                    foreach (ViewLoop loop in view.Loops)
                    {
                        foreach (ViewFrame frame in loop.Frames)
                        {
                            frame.Flip = frame.Flipped ? SpriteFlipStyle.Horizontal : SpriteFlipStyle.None;
                        }
                    }
                }
            }

            if (string.IsNullOrEmpty(game.Settings.ScriptCompiler))
            {
                var compiler = Factory.NativeProxy.GetEmbeddedScriptCompilers().FirstOrDefault();
                game.Settings.ScriptCompiler = compiler != null ? compiler.GetName() : string.Empty;
            }

            System.Version editorVersion = new System.Version(AGS.Types.Version.AGS_EDITOR_VERSION);
            System.Version projectVersion = game.SavedXmlEditorVersion;
            if (projectVersion == null || projectVersion < editorVersion)
                game.SetScriptAPIForOldProject();
#pragma warning restore 0612, 0618
        }

        private string RemoveAllLeadingSpacesFromLines(string script)
        {
            StringReader sr = new StringReader(script);
            StringWriter sw = new StringWriter();
            string thisLine;
            while ((thisLine = sr.ReadLine()) != null)
            {
                sw.WriteLine(thisLine.Trim());
            }
            string returnValue = sw.ToString();
            sr.Close();
            sw.Close();
            return returnValue;
        }

        private static int RemapAudioClipIDToFixedIndex(int id, Dictionary<int, int> audioIDToIndex)
        {
            int fixedIndex;
            if (audioIDToIndex.TryGetValue(id, out fixedIndex))
                return fixedIndex;
            else
                return AudioClip.FixedIndexNoValue;
        }

        /// <summary>
        /// Remaps historical 16-bit R6G5B6 values to proper 32-bit ARGB.
        /// </summary>
        private static void RemapLegacyColourProperties(Game game)
        {
            Tasks.RemapColourProperty remapColor = (color, isBg) => {
                return ColorMapper.RemapFromLegacyColourNumber(color, game.Palette, game.Settings.ColorDepth, isBg);
            };
            Tasks.RemapColourProperties(game, remapColor);
        }

        /// <summary>
        /// Remaps 32-bit RGB color number to proper 32-bit ARGB.
        /// This method has a nuance: the background colors of value 0 are treated as "transparent",
        /// while foreground colors of value 0 are treated as "black".
        /// </summary>
        private static void RemapOpaqueColourProperties(Game game)
        {
            Tasks.RemapColourProperty remapColor = (color, isBg) => {
                if (isBg && (color == 0))
                    return 0;
                return ColorMapper.MakeOpaque(color, game.Settings.ColorDepth);
            };
            Tasks.RemapColourProperties(game, remapColor);
        }
    }
}
