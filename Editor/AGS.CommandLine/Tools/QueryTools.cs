using System;
using System.IO;
using System.Linq;
using Newtonsoft.Json.Linq;
using AGS.Types;
using AGS.CommandLine.Services;

namespace AGS.CommandLine.Tools
{
    public static class QueryTools
    {
        // ── helpers ────────────────────────────────────────────────────────────

        private static JObject Err(string msg)
        {
            return new JObject { ["success"] = false, ["error"] = msg };
        }

        /// <summary>
        /// Strips text after '>' character (fixes encoding bug in descriptions).
        /// </summary>
        private static string CleanDescription(string description)
        {
            if (string.IsNullOrEmpty(description)) return description;
            int idx = description.IndexOf('>');
            if (idx >= 0) return description.Substring(0, idx);
            return description;
        }

        private static Game LoadGame(JObject args)
        {
            string path = args["project_path"] != null ? args["project_path"].ToString() : null;
            if (string.IsNullOrEmpty(path))
                throw new ArgumentException("project_path is required");
            return new GameLoader().Load(path);
        }

        // ── character_list ─────────────────────────────────────────────────────

        public static JToken CharacterList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (Character c in game.Characters)
                {
                    if (details)
                        arr.Add(CharacterDetail(c, game));
                    else
                        arr.Add(new JObject { ["id"] = c.ID, ["name"] = c.ScriptName });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["characters"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── character_get ──────────────────────────────────────────────────────

        public static JToken CharacterGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["character_id"] == null) return Err("character_id is required");
                int id = (int)args["character_id"];
                Character c = null;
                foreach (Character ch in game.Characters)
                    if (ch.ID == id) { c = ch; break; }
                if (c == null) return Err(string.Format("Character {0} not found", id));
                return new JObject { ["success"] = true, ["character"] = CharacterDetail(c, game) };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        private static JObject CharacterDetail(Character c, Game game)
        {
            bool isPlayer = game.PlayerCharacter != null && game.PlayerCharacter.ID == c.ID;
            return new JObject
            {
                ["id"] = c.ID,
                ["name"] = c.ScriptName,
                ["real_name"] = c.RealName,
                ["view"] = c.NormalView,
                ["speech_view"] = c.SpeechView,
                ["idle_view"] = c.IdleView,
                ["idle_delay"] = c.IdleDelay,
                ["speech_color"] = c.SpeechColor,
                ["starting_room"] = c.StartingRoom,
                ["starting_x"] = c.StartX,
                ["starting_y"] = c.StartY,
                ["is_player"] = isPlayer
            };
        }

        // ── room_list ──────────────────────────────────────────────────────────

        public static JToken RoomList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (IRoom r in game.Rooms)
                {
                    if (details)
                        arr.Add(RoomBrief(r));
                    else
                        arr.Add(new JObject { ["id"] = r.Number, ["name"] = r.Description });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["rooms"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_get ───────────────────────────────────────────────────────────

        public static JToken RoomGet(JObject args)
        {
            try
            {
                string projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
                if (string.IsNullOrEmpty(projectPath)) return Err("project_path is required");
                Game game = LoadGame(args);
                if (args["room_id"] == null) return Err("room_id is required");
                int id = (int)args["room_id"];
                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == id) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", id));

                // Try to load full room data via native bridge
                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                try
                {
                    if (fullRoom != null)
                    {
                        // Count user-defined hotspots (skip slot 0 and auto-generated defaults)
                        int hotspotCount = 0;
                        foreach (RoomHotspot h in fullRoom.Hotspots)
                        {
                            if (h.ID == 0) continue;
                            string defName = string.Format("hHotspot{0}", h.ID);
                            if (!string.IsNullOrEmpty(h.Name) && h.Name != defName) hotspotCount++;
                        }
                        bool hasScript = File.Exists(Path.Combine(projectPath, unloaded.ScriptFileName));
                        return new JObject { ["success"] = true, ["room"] = new JObject
                        {
                            ["id"]             = unloaded.Number,
                            ["name"]           = unloaded.Description ?? string.Empty,
                            ["width"]          = fullRoom.Width,
                            ["height"]         = fullRoom.Height,
                            ["color_depth"]    = fullRoom.ColorDepth,
                            ["hotspot_count"]  = hotspotCount,
                            ["object_count"]   = fullRoom.Objects.Count,
                            ["background_count"] = fullRoom.BackgroundCount,
                            ["script_file"]    = unloaded.ScriptFileName,
                            ["has_script"]     = hasScript,
                            ["crm_file"]       = unloaded.FileName
                        }};
                    }
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }

                // Fallback: no .crm yet
                return new JObject { ["success"] = true, ["room"] = RoomBrief(unloaded) };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        private static JObject RoomBrief(IRoom r)
        {
            return new JObject
            {
                ["id"]          = r.Number,
                ["name"]        = r.Description ?? string.Empty,
                ["script_file"] = r.ScriptFileName,
                ["crm_file"]    = r.FileName
            };
        }

        // ── room_hotspots ──────────────────────────────────────────────────────

        public static JToken RoomHotspots(JObject args)
        {
            try
            {
                string projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
                if (string.IsNullOrEmpty(projectPath)) return Err("project_path is required");
                Game game = LoadGame(args);
                if (args["room_id"] == null) return Err("room_id is required");
                int id = (int)args["room_id"];
                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == id) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", id));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null)
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = 0,
                        ["hotspots"] = new JArray(),
                        ["note"] = string.Format("{0} not found — build the room in the AGS Editor first", unloaded.FileName) };
                try
                {
                    var arr = new JArray();
                    foreach (RoomHotspot h in fullRoom.Hotspots)
                    {
                        // Skip slot 0 (always "No Hotspot") and auto-generated default names
                        if (h.ID == 0) continue;
                        string defaultName = string.Format("hHotspot{0}", h.ID);
                        if (string.IsNullOrEmpty(h.Name) || h.Name == defaultName) continue;
                        arr.Add(new JObject
                        {
                            ["id"]          = h.ID,
                            ["name"]        = h.Name,
                            ["description"] = CleanDescription(h.Description ?? string.Empty),
                            ["walk_to_x"]   = h.WalkToPoint.X,
                            ["walk_to_y"]   = h.WalkToPoint.Y
                        });
                    }
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = arr.Count, ["hotspots"] = arr };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_objects ───────────────────────────────────────────────────────

        public static JToken RoomObjects(JObject args)
        {
            try
            {
                string projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
                if (string.IsNullOrEmpty(projectPath)) return Err("project_path is required");
                Game game = LoadGame(args);
                if (args["room_id"] == null) return Err("room_id is required");
                int id = (int)args["room_id"];
                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == id) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", id));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null)
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = 0,
                        ["objects"] = new JArray(),
                        ["note"] = string.Format("{0} not found — build the room in the AGS Editor first", unloaded.FileName) };
                try
                {
                    var arr = new JArray();
                    foreach (RoomObject obj in fullRoom.Objects)
                    {
                        arr.Add(new JObject
                        {
                            ["id"]                   = obj.ID,
                            ["name"]                 = obj.Name ?? string.Empty,
                            ["description"]          = CleanDescription(obj.Description ?? string.Empty),
                            ["sprite"]               = obj.Image,
                            ["x"]                    = obj.StartX,
                            ["y"]                    = obj.StartY,
                            ["baseline"]             = obj.Baseline,
                            ["baseline_overridden"]  = obj.BaselineOverridden,
                            ["visible"]              = obj.Visible,
                            ["clickable"]            = obj.Clickable
                        });
                    }
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = arr.Count, ["objects"] = arr };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_walkable_areas ────────────────────────────────────────────────

        public static JToken RoomWalkableAreas(JObject args)
        {
            try
            {
                string projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
                if (string.IsNullOrEmpty(projectPath)) return Err("project_path is required");
                Game game = LoadGame(args);
                if (args["room_id"] == null) return Err("room_id is required");
                int id = (int)args["room_id"];
                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == id) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", id));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null)
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = 0, ["walkable_areas"] = new JArray() };
                try
                {
                    var arr = new JArray();
                    foreach (RoomWalkableArea area in fullRoom.WalkableAreas)
                    {
                        arr.Add(new JObject
                        {
                            ["id"] = area.ID,
                            ["scaling_level"] = area.ScalingLevel,
                            ["min_scaling"] = area.MinScalingLevel,
                            ["max_scaling"] = area.MaxScalingLevel,
                            ["area_specific_view"] = area.AreaSpecificView,
                            ["use_continuous_scaling"] = area.UseContinuousScaling
                        });
                    }
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = arr.Count, ["walkable_areas"] = arr };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_regions ───────────────────────────────────────────────────────

        public static JToken RoomRegions(JObject args)
        {
            try
            {
                string projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
                if (string.IsNullOrEmpty(projectPath)) return Err("project_path is required");
                Game game = LoadGame(args);
                if (args["room_id"] == null) return Err("room_id is required");
                int id = (int)args["room_id"];
                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == id) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", id));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null)
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = 0, ["regions"] = new JArray() };
                try
                {
                    var arr = new JArray();
                    foreach (RoomRegion region in fullRoom.Regions)
                    {
                        arr.Add(new JObject
                        {
                            ["id"] = region.ID,
                            ["light_level"] = region.LightLevel,
                            ["use_colour_tint"] = region.UseColourTint,
                            ["red_tint"] = region.RedTint,
                            ["green_tint"] = region.GreenTint,
                            ["blue_tint"] = region.BlueTint,
                            ["tint_saturation"] = region.TintSaturation,
                            ["tint_luminance"] = region.TintLuminance
                        });
                    }
                    return new JObject { ["success"] = true, ["room_id"] = id, ["count"] = arr.Count, ["regions"] = arr };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── dialog_list ────────────────────────────────────────────────────────

        public static JToken DialogList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (Dialog d in game.Dialogs)
                {
                    if (details)
                        arr.Add(new JObject { ["id"] = d.ID, ["name"] = d.Name, ["option_count"] = d.Options.Count, ["show_text_parser"] = d.ShowTextParser });
                    else
                        arr.Add(new JObject { ["id"] = d.ID, ["name"] = d.Name });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["dialogs"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── dialog_get ─────────────────────────────────────────────────────────

        public static JToken DialogGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["dialog_id"] == null) return Err("dialog_id is required");
                int id = (int)args["dialog_id"];
                Dialog d = null;
                foreach (Dialog dlg in game.Dialogs)
                    if (dlg.ID == id) { d = dlg; break; }
                if (d == null) return Err(string.Format("Dialog {0} not found", id));
                var options = new JArray();
                foreach (DialogOption o in d.Options)
                    options.Add(new JObject { ["id"] = o.ID, ["text"] = o.Text, ["show"] = o.Show, ["say"] = o.Say });
                var obj = new JObject { ["id"] = d.ID, ["name"] = d.Name, ["option_count"] = d.Options.Count,
                    ["show_text_parser"] = d.ShowTextParser, ["options"] = options };
                return new JObject { ["success"] = true, ["dialog"] = obj };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── inventory_list ─────────────────────────────────────────────────────

        public static JToken InventoryList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (InventoryItem item in game.InventoryItems)
                {
                    if (details)
                        arr.Add(new JObject { ["id"] = item.ID, ["name"] = item.Name, ["description"] = CleanDescription(item.Description),
                            ["sprite"] = item.Image, ["cursor_sprite"] = item.CursorImage });
                    else
                        arr.Add(new JObject { ["id"] = item.ID, ["name"] = item.Name });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["inventory_items"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── inventory_get ──────────────────────────────────────────────────────

        public static JToken InventoryGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["inventory_id"] == null) return Err("inventory_id is required");
                int id = (int)args["inventory_id"];
                InventoryItem item = null;
                foreach (InventoryItem i in game.InventoryItems)
                    if (i.ID == id) { item = i; break; }
                if (item == null) return Err(string.Format("Inventory item {0} not found", id));
                var obj = new JObject { ["id"] = item.ID, ["name"] = item.Name, ["description"] = CleanDescription(item.Description),
                    ["sprite"] = item.Image, ["cursor_sprite"] = item.CursorImage,
                    ["player_starts_with"] = item.PlayerStartsWithItem,
                    ["hotspot_x"] = item.HotspotX, ["hotspot_y"] = item.HotspotY };
                return new JObject { ["success"] = true, ["inventory_item"] = obj };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── gui_list ───────────────────────────────────────────────────────────

        public static JToken GuiList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (GUI g in game.GUIs)
                {
                    if (details)
                    {
                        var gui = g as NormalGUI;
                        arr.Add(new JObject { ["id"] = g.ID, ["name"] = g.Name,
                            ["x"] = gui != null ? gui.Left : 0, ["y"] = gui != null ? gui.Top : 0,
                            ["width"] = gui != null ? gui.Width : 0, ["height"] = gui != null ? gui.Height : 0,
                            ["control_count"] = g.Controls.Count, ["type"] = g.GetType().Name });
                    }
                    else
                        arr.Add(new JObject { ["id"] = g.ID, ["name"] = g.Name });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["guis"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── gui_get ────────────────────────────────────────────────────────────

        public static JToken GuiGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["gui_id"] == null) return Err("gui_id is required");
                int id = (int)args["gui_id"];
                GUI g = null;
                foreach (GUI gui in game.GUIs)
                    if (gui.ID == id) { g = gui; break; }
                if (g == null) return Err(string.Format("GUI {0} not found", id));
                var normalGui = g as NormalGUI;
                var controls = new JArray();
                foreach (GUIControl ctrl in g.Controls)
                    controls.Add(new JObject { ["id"] = ctrl.ID, ["name"] = ctrl.Name,
                        ["type"] = ctrl.ControlType, ["x"] = ctrl.Left, ["y"] = ctrl.Top,
                        ["width"] = ctrl.Width, ["height"] = ctrl.Height });
                var obj = new JObject { ["id"] = g.ID, ["name"] = g.Name,
                    ["type"] = g.GetType().Name,
                    ["x"] = normalGui != null ? normalGui.Left : 0,
                    ["y"] = normalGui != null ? normalGui.Top : 0,
                    ["width"] = normalGui != null ? normalGui.Width : 0,
                    ["height"] = normalGui != null ? normalGui.Height : 0,
                    ["control_count"] = g.Controls.Count, ["controls"] = controls };
                return new JObject { ["success"] = true, ["gui"] = obj };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── view_list ──────────────────────────────────────────────────────────

        public static JToken ViewList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (View v in game.ViewFlatList)
                {
                    int frameCount = 0;
                    foreach (ViewLoop l in v.Loops) frameCount += l.Frames.Count;
                    if (details)
                        arr.Add(new JObject { ["id"] = v.ID, ["name"] = v.Name,
                            ["loop_count"] = v.Loops.Count, ["frame_count"] = frameCount });
                    else
                        arr.Add(new JObject { ["id"] = v.ID, ["name"] = v.Name });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["views"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── view_get ───────────────────────────────────────────────────────────

        public static JToken ViewGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["view_id"] == null) return Err("view_id is required");
                int id = (int)args["view_id"];
                View v = null;
                foreach (View vv in game.ViewFlatList)
                    if (vv.ID == id) { v = vv; break; }
                if (v == null) return Err(string.Format("View {0} not found", id));
                var loops = new JArray();
                foreach (ViewLoop l in v.Loops)
                {
                    var frames = new JArray();
                    foreach (ViewFrame f in l.Frames)
                        frames.Add(new JObject { ["id"] = f.ID, ["sprite"] = f.Image, ["flipped"] = f.Flipped, ["delay"] = f.Delay });
                    loops.Add(new JObject { ["id"] = l.ID, ["direction"] = l.DirectionDescription,
                        ["frame_count"] = l.Frames.Count, ["run_next"] = l.RunNextLoop, ["frames"] = frames });
                }
                return new JObject { ["success"] = true, ["view"] = new JObject { ["id"] = v.ID, ["name"] = v.Name,
                    ["loop_count"] = v.Loops.Count, ["loops"] = loops } };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── audio_list ─────────────────────────────────────────────────────────

        public static JToken AudioList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                foreach (AudioClip a in game.AudioClips)
                {
                    if (details)
                        arr.Add(new JObject { ["id"] = a.ID, ["name"] = a.ScriptName,
                            ["file_type"] = a.FileType.ToString(), ["type_id"] = a.Type,
                            ["source_file"] = a.SourceFileName ?? string.Empty });
                    else
                        arr.Add(new JObject { ["id"] = a.ID, ["name"] = a.ScriptName });
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["audio_clips"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── audio_get ──────────────────────────────────────────────────────────

        public static JToken AudioGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["audio_id"] == null) return Err("audio_id is required");
                int id = (int)args["audio_id"];
                AudioClip a = null;
                foreach (AudioClip clip in game.AudioClips)
                    if (clip.ID == id) { a = clip; break; }
                if (a == null) return Err(string.Format("Audio clip {0} not found", id));
                var obj = new JObject { ["id"] = a.ID, ["name"] = a.ScriptName,
                    ["file_type"] = a.FileType.ToString(), ["type_id"] = a.Type,
                    ["source_file"] = a.SourceFileName ?? string.Empty,
                    ["bundling_type"] = a.BundlingType.ToString(),
                    ["default_volume"] = a.DefaultVolume,
                    ["default_repeat"] = a.DefaultRepeat.ToString() };
                return new JObject { ["success"] = true, ["audio_clip"] = obj };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── script_list ────────────────────────────────────────────────────────

        public static JToken ScriptList(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                bool details = args["include_details"] != null && (bool)args["include_details"];
                var arr = new JArray();
                int idx = 0;
                foreach (ScriptAndHeader sh in game.ScriptsAndHeaders)
                {
                    int lineCount = 0;
                    if (details && sh.Script != null && sh.Script.Text != null)
                        lineCount = sh.Script.Text.Split('\n').Length;
                    if (details)
                        arr.Add(new JObject { ["id"] = idx, ["name"] = sh.Name,
                            ["has_header"] = sh.Header != null,
                            ["line_count"] = lineCount });
                    else
                        arr.Add(new JObject { ["id"] = idx, ["name"] = sh.Name });
                    idx++;
                }
                return new JObject { ["success"] = true, ["count"] = arr.Count, ["scripts"] = arr };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── script_get ─────────────────────────────────────────────────────────

        public static JToken ScriptGet(JObject args)
        {
            try
            {
                Game game = LoadGame(args);
                if (args["script_id"] == null) return Err("script_id is required");
                int id = (int)args["script_id"];
                int idx = 0;
                ScriptAndHeader found = null;
                foreach (ScriptAndHeader sh in game.ScriptsAndHeaders)
                {
                    if (idx == id) { found = sh; break; }
                    idx++;
                }
                if (found == null) return Err(string.Format("Script {0} not found", id));
                int lineCount = found.Script != null && found.Script.Text != null
                    ? found.Script.Text.Split('\n').Length : 0;
                int headerLineCount = found.Header != null && found.Header.Text != null
                    ? found.Header.Text.Split('\n').Length : 0;
                var obj = new JObject { ["id"] = id, ["name"] = found.Name,
                    ["has_header"] = found.Header != null,
                    ["implementation_path"] = found.Script != null ? found.Script.FileName : string.Empty,
                    ["header_path"] = found.Header != null ? found.Header.FileName : string.Empty,
                    ["line_count"] = lineCount, ["line_count_header"] = headerLineCount };
                return new JObject { ["success"] = true, ["script"] = obj };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }
    }
}
