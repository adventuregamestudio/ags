using System;
using System.IO;
using System.Linq;
using AGS.CommandLine.Services;
using Newtonsoft.Json.Linq;

namespace AGS.CommandLine.Tools
{
    /// <summary>
    /// MCP tools for AGS project inspection.
    /// </summary>
    public static class ProjectTools
    {
        /// <summary>
        /// Returns summary information about an AGS game project.
        /// Arguments: { "project_path": "C:\\path\\to\\project" }
        /// </summary>
        public static JToken ProjectInfo(JObject arguments)
        {
            string projectPath = arguments["project_path"] != null
                ? arguments["project_path"].ToString() : null;

            if (string.IsNullOrEmpty(projectPath))
                return new JObject
                {
                    ["success"] = false,
                    ["error"] = "project_path is required"
                };

            // Resolve relative paths against current directory
            if (!Path.IsPathRooted(projectPath))
                projectPath = Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, projectPath));

            try
            {
                var loader = new GameLoader();
                var game = loader.Load(projectPath);

                return new JObject
                {
                    ["success"] = true,
                    ["game_name"] = game.Settings.GameName,
                    ["resolution"] = string.Format("{0}x{1}",
                        game.Settings.CustomResolution.Width,
                        game.Settings.CustomResolution.Height),
                    ["color_depth"] = game.Settings.ColorDepth.ToString(),
                    ["character_count"] = game.Characters.Count,
                    ["room_count"] = game.Rooms.Count,
                    ["dialog_count"] = game.Dialogs.Count,
                    ["view_count"] = game.ViewFlatList.Count,
                    ["gui_count"] = game.GUIs.Count,
                    ["script_count"] = game.RootScriptFolder.AllItemsFlat.Count(),
                    ["inventory_count"] = game.InventoryItems.Count,
                    ["audio_clip_count"] = game.AudioClips.Count
                };
            }
            catch (Exception ex)
            {
                return new JObject
                {
                    ["success"] = false,
                    ["error"] = ex.Message
                };
            }
        }
    }
}
