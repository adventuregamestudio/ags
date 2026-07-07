using System;
using System.IO;
using System.Text.RegularExpressions;
using Newtonsoft.Json.Linq;
using AGS.Types;
using AGS.CommandLine.Services;

namespace AGS.CommandLine.Tools
{
    public static class WriteTools
    {
        // ── shared helpers ─────────────────────────────────────────────────────

        private static JObject Err(string msg)
            => new JObject { ["success"] = false, ["error"] = msg };

        private static readonly Regex _identRegex =
            new Regex(@"^[A-Za-z_][A-Za-z0-9_]*$", RegexOptions.Compiled);

        /// <summary>Returns null if valid, or an error string if invalid.</summary>
        public static string ValidateIdentifier(string name)
        {
            if (string.IsNullOrEmpty(name)) return "name is required";
            if (!_identRegex.IsMatch(name))
                return string.Format(
                    "'{0}' is not a valid AGS identifier (alphanumeric + underscore, must start with letter or underscore)",
                    name);
            return null;
        }

        private static Game LoadAndPath(JObject args, out string projectPath)
        {
            projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
            if (string.IsNullOrEmpty(projectPath))
                throw new ArgumentException("project_path is required");
            return new GameLoader().Load(projectPath);
        }

        private static JObject SaveOk(Game game, string projectPath, JObject payload)
        {
            GameSaver.Save(game, projectPath);
            payload["success"] = true;
            return payload;
        }

        // ── character_add ──────────────────────────────────────────────────────

        public static JToken CharacterAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string name = args["name"] != null ? args["name"].ToString() : null;
                string idErr = ValidateIdentifier(name);
                if (idErr != null) return Err(idErr);

                // Uniqueness check
                foreach (Character c in game.Characters)
                    if (string.Equals(c.ScriptName, name, StringComparison.OrdinalIgnoreCase))
                        return Err(string.Format("Character '{0}' already exists", name));

                Character newChar = new Character();
                newChar.ID = game.RootCharacterFolder.GetAllItemsCount();
                newChar.ScriptName = name;
                newChar.RealName = args["real_name"] != null ? args["real_name"].ToString() : name;
                if (args["view"] != null) newChar.NormalView = (int)args["view"];
                if (args["speech_view"] != null) newChar.SpeechView = (int)args["speech_view"];
                if (args["talking_color"] != null) newChar.SpeechColor = (int)args["talking_color"];
                if (args["starting_room"] != null) newChar.StartingRoom = (int)args["starting_room"];
                if (args["starting_x"] != null) newChar.StartX = (int)args["starting_x"];
                if (args["starting_y"] != null) newChar.StartY = (int)args["starting_y"];

                game.RootCharacterFolder.Items.Add(newChar);

                return SaveOk(game, projectPath, new JObject
                {
                    ["character_id"] = newChar.ID,
                    ["message"] = string.Format("Character '{0}' created successfully", name)
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── character_update ───────────────────────────────────────────────────

        public static JToken CharacterUpdate(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["character_id"] == null) return Err("character_id is required");
                int id = (int)args["character_id"];
                Character c = null;
                foreach (Character ch in game.Characters)
                    if (ch.ID == id) { c = ch; break; }
                if (c == null) return Err(string.Format("Character {0} not found", id));

                if (args["real_name"] != null) c.RealName = args["real_name"].ToString();
                if (args["view"] != null) c.NormalView = (int)args["view"];
                if (args["speech_view"] != null) c.SpeechView = (int)args["speech_view"];
                if (args["idle_view"] != null) c.IdleView = (int)args["idle_view"];
                if (args["idle_time"] != null) c.IdleDelay = (int)args["idle_time"];
                if (args["talking_color"] != null) c.SpeechColor = (int)args["talking_color"];
                if (args["starting_room"] != null) c.StartingRoom = (int)args["starting_room"];
                if (args["starting_x"] != null) c.StartX = (int)args["starting_x"];
                if (args["starting_y"] != null) c.StartY = (int)args["starting_y"];

                return SaveOk(game, projectPath, new JObject
                    { ["message"] = "Character updated successfully" });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_add ───────────────────────────────────────────────────────────

        public static JToken RoomAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string name = args["name"] != null ? args["name"].ToString() : null;
                if (string.IsNullOrEmpty(name)) return Err("name is required");

                // Uniqueness check
                foreach (IRoom r in game.Rooms)
                    if (string.Equals(r.Description, name, StringComparison.OrdinalIgnoreCase))
                        return Err(string.Format("Room '{0}' already exists", name));

                // Find next available room number (not already used)
                int roomNumber = 1;
                while (true)
                {
                    bool used = false;
                    foreach (IRoom r in game.Rooms)
                        if (r.Number == roomNumber) { used = true; break; }
                    if (!used) break;
                    roomNumber++;
                }

                UnloadedRoom newRoom = new UnloadedRoom(roomNumber);
                newRoom.Description = name;

                // Create a blank room script file on disk — the Editor requires
                // the .asc to exist even before the room is edited
                string scriptPath = Path.Combine(projectPath, newRoom.ScriptFileName);
                if (!File.Exists(scriptPath))
                    File.WriteAllText(scriptPath,
                        string.Format("// Room script file\r\n", roomNumber),
                        new System.Text.UTF8Encoding(false));

                // Create a blank .crm binary file via AGS.Native so the Editor can open it
                NativeRoomLoader.CreateDefaultRoom(newRoom, game, projectPath);

                game.RootRoomFolder.Items.Add(newRoom);

                return SaveOk(game, projectPath, new JObject
                {
                    ["room_id"] = roomNumber,
                    ["message"] = string.Format("Room '{0}' created as room{1}", name, roomNumber)
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_update ────────────────────────────────────────────────────────

        public static JToken RoomUpdate(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null) return Err("room_id is required");
                int id = (int)args["room_id"];
                IRoom room = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == id) { room = r; break; }
                if (room == null) return Err(string.Format("Room {0} not found", id));

                if (args["name"] != null)
                {
                    string newName = args["name"].ToString();
                    // Uniqueness check (skip current)
                    foreach (IRoom r in game.Rooms)
                        if (r.Number != id && string.Equals(r.Description, newName, StringComparison.OrdinalIgnoreCase))
                            return Err(string.Format("Room name '{0}' already in use", newName));
                    room.Description = newName;
                }

                return SaveOk(game, projectPath, new JObject
                    { ["message"] = "Room updated successfully" });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── dialog_add ─────────────────────────────────────────────────────────

        public static JToken DialogAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string name = args["name"] != null ? args["name"].ToString() : null;
                string idErr = ValidateIdentifier(name);
                if (idErr != null) return Err(idErr);

                foreach (Dialog d in game.Dialogs)
                    if (string.Equals(d.Name, name, StringComparison.OrdinalIgnoreCase))
                        return Err(string.Format("Dialog '{0}' already exists", name));

                Dialog newDialog = new Dialog();
                newDialog.ID = game.RootDialogFolder.GetAllItemsCount();
                newDialog.Name = name;
                if (args["show_always"] != null) newDialog.ShowTextParser = (bool)args["show_always"];

                game.RootDialogFolder.Items.Add(newDialog);

                return SaveOk(game, projectPath, new JObject
                {
                    ["dialog_id"] = newDialog.ID,
                    ["message"] = string.Format("Dialog '{0}' created successfully", name)
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── dialog_option_add ──────────────────────────────────────────────────

        public static JToken DialogOptionAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["dialog_id"] == null) return Err("dialog_id is required");
                int id = (int)args["dialog_id"];
                Dialog d = null;
                foreach (Dialog dlg in game.Dialogs)
                    if (dlg.ID == id) { d = dlg; break; }
                if (d == null) return Err(string.Format("Dialog {0} not found", id));

                string text = args["text"] != null ? args["text"].ToString() : null;
                if (string.IsNullOrEmpty(text)) return Err("text is required");
                if (text.Length > 256) return Err("text must be <= 256 characters");

                DialogOption opt = new DialogOption();
                opt.ID = d.Options.Count + 1;
                opt.Text = text;
                opt.Show = args["show_always"] == null || (bool)args["show_always"];
                opt.Say  = args["say_as_character"] == null || (bool)args["say_as_character"];
                d.Options.Add(opt);

                return SaveOk(game, projectPath, new JObject
                {
                    ["option_index"] = opt.ID,
                    ["message"] = "Dialog option added successfully"
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── inventory_item_add ─────────────────────────────────────────────────

        public static JToken InventoryItemAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string name = args["name"] != null ? args["name"].ToString() : null;
                string idErr = ValidateIdentifier(name);
                if (idErr != null) return Err(idErr);

                foreach (InventoryItem i in game.InventoryItems)
                    if (string.Equals(i.Name, name, StringComparison.OrdinalIgnoreCase))
                        return Err(string.Format("Inventory item '{0}' already exists", name));

                InventoryItem newItem = new InventoryItem();
                newItem.ID = game.RootInventoryItemFolder.GetAllItemsCount() + 1;
                newItem.Name = name;
                if (args["description"] != null) newItem.Description = args["description"].ToString();
                if (args["sprite"] != null) newItem.Image = (int)args["sprite"];
                if (args["cursor_sprite"] != null) newItem.CursorImage = (int)args["cursor_sprite"];

                game.RootInventoryItemFolder.Items.Add(newItem);

                return SaveOk(game, projectPath, new JObject
                {
                    ["inventory_id"] = newItem.ID,
                    ["message"] = string.Format("Inventory item '{0}' created successfully", name)
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── inventory_item_update ──────────────────────────────────────────────

        public static JToken InventoryItemUpdate(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["inventory_id"] == null) return Err("inventory_id is required");
                int id = (int)args["inventory_id"];
                InventoryItem item = null;
                foreach (InventoryItem i in game.InventoryItems)
                    if (i.ID == id) { item = i; break; }
                if (item == null) return Err(string.Format("Inventory item {0} not found", id));

                if (args["description"] != null) item.Description = args["description"].ToString();
                if (args["sprite"] != null) item.Image = (int)args["sprite"];
                if (args["cursor_sprite"] != null) item.CursorImage = (int)args["cursor_sprite"];

                return SaveOk(game, projectPath, new JObject
                    { ["message"] = "Inventory item updated successfully" });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── inventory_item_rename ──────────────────────────────────────────────

        public static JToken InventoryItemRename(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["inventory_id"] == null) return Err("inventory_id is required");
                string newName = args["name"] != null ? args["name"].ToString() : null;
                if (string.IsNullOrEmpty(newName)) return Err("name is required");

                string idErr = ValidateIdentifier(newName);
                if (idErr != null) return Err(idErr);

                int id = (int)args["inventory_id"];
                InventoryItem item = null;
                foreach (InventoryItem i in game.InventoryItems)
                    if (i.ID == id) { item = i; break; }
                if (item == null) return Err(string.Format("Inventory item {0} not found", id));

                // Check for uniqueness (skip current)
                foreach (InventoryItem i in game.InventoryItems)
                    if (i.ID != id && string.Equals(i.Name, newName, StringComparison.OrdinalIgnoreCase))
                        return Err(string.Format("Inventory item '{0}' already exists", newName));

                string oldName = item.Name;
                item.Name = newName;

                // Scan event handlers that reference the old name
                var warnings = new JArray();
                var scriptRefs = new JArray();

                if (item.Interactions != null)
                {
                    string[] suffixes = item.Interactions.FunctionSuffixes;
                    string[] names    = item.Interactions.ScriptFunctionNames;
                    string   module   = item.Interactions.ScriptModule ?? string.Empty;

                    for (int idx = 0; idx < names.Length; idx++)
                    {
                        string fnName = names[idx];
                        if (string.IsNullOrEmpty(fnName)) continue;

                        // Check if the handler function name contains the old item name
                        // AGS convention: iItemName_Event (e.g., iSword_Interact)
                        if (fnName.IndexOf(oldName, StringComparison.OrdinalIgnoreCase) >= 0)
                        {
                            string suffix = (idx < suffixes.Length) ? suffixes[idx] : string.Format("event{0}", idx);
                            scriptRefs.Add(new JObject
                            {
                                ["script_method"] = fnName,
                                ["script_module"] = module,
                                ["event_suffix"]  = suffix,
                                ["note"]          = string.Format(
                                    "Handler '{0}' references old name '{1}' — rename it to use '{2}' instead.",
                                    fnName, oldName, fnName.Replace(oldName, newName))
                            });
                        }
                    }
                }

                if (scriptRefs.Count > 0)
                {
                    warnings.Add(new JObject
                    {
                        ["type"]     = "script_references",
                        ["message"]  = string.Format(
                            "{0} event handler(s) reference the old name '{1}' and will need manual updates.",
                            scriptRefs.Count, oldName),
                        ["handlers"] = scriptRefs
                    });
                }
                else
                {
                    warnings.Add(new JObject
                    {
                        ["type"]    = "info",
                        ["message"] = string.Format(
                            "No bound event handlers reference the old name '{0}'. " +
                            "Any inline script references (e.g., 'inventory[\"{0}\"]') still need manual review.",
                            oldName)
                    });
                }

                return SaveOk(game, projectPath, new JObject
                {
                    ["message"] = string.Format("Inventory item renamed from '{0}' to '{1}'", oldName, newName),
                    ["old_name"] = oldName,
                    ["new_name"] = newName,
                    ["warnings"] = warnings
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── audio_add ──────────────────────────────────────────────────────────

        /// <summary>
        /// Sanitize a filename stem into a valid AGS script identifier.
        /// Replaces spaces and invalid characters with underscores (matching
        /// the logic in AGS.Types.Utilities.RemoveInvalidCharactersFromScriptName),
        /// then prefixes with 'a' per AGS audio-clip naming convention.
        /// </summary>
        private static string ScriptNameFromFileName(string stem)
        {
            var sb = new System.Text.StringBuilder();
            foreach (char c in stem)
            {
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '_')
                    sb.Append(c);
                else
                    sb.Append('_');
            }
            string clean = sb.ToString().TrimStart('_');
            if (clean.Length == 0) clean = "clip";
            // If first char is a digit, underscore-prefix before the 'a' prefix
            if (char.IsDigit(clean[0])) clean = "_" + clean;
            return "a" + char.ToUpperInvariant(clean[0]) + clean.Substring(1);
        }

        public static JToken AudioAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string filePath = args["file_path"] != null ? args["file_path"].ToString() : null;
                string name     = args["name"] != null ? args["name"].ToString() : null;
                string typeStr  = args["type"] != null ? args["type"].ToString() : null;

                // ── 4.2 Validate required parameters ──────────────────────────
                if (string.IsNullOrEmpty(filePath)) return Err("file_path is required");
                if (!File.Exists(filePath))
                    return Err(string.Format("Audio file not found: {0}", filePath));

                // ── 4.4 Validate file format ───────────────────────────────────
                string ext = Path.GetExtension(filePath).ToLower();
                AudioClipFileType fileType;
                // 4.7: Map extension → AudioClipFileType enum
                switch (ext)
                {
                    case ".mp3":  fileType = AudioClipFileType.MP3; break;
                    case ".ogg":  fileType = AudioClipFileType.OGG; break;
                    case ".wav":  fileType = AudioClipFileType.WAV; break;
                    case ".flac": fileType = AudioClipFileType.WAV; break; // AGS has no FLAC enum; WAV covers PCM container
                    default:
                        return Err(string.Format(
                            "Unsupported audio format '{0}'. Supported: .mp3, .ogg, .wav, .flac", ext));
                }

                // ── 4.3 Map type → AudioClipTypes integer ──────────────────────
                // Accepts "music" (→2) or "sound" (→3) as documented, or an
                // integer string for advanced use.  Defaults to Sound (3).
                int typeId = 3; // default: Sound
                if (!string.IsNullOrEmpty(typeStr))
                {
                    string typeLower = typeStr.Trim().ToLowerInvariant();
                    if (typeLower == "music")
                        typeId = 2;
                    else if (typeLower == "sound")
                        typeId = 3;
                    else if (!int.TryParse(typeStr, out typeId))
                        return Err("type must be \"music\", \"sound\", or a valid integer type ID");
                    if (typeId < 1 || typeId > 10)
                        return Err(string.Format("type ID {0} is out of valid range (1-10)", typeId));
                }

                // ── 4.6 Auto-generate script name if not provided ──────────────
                if (string.IsNullOrEmpty(name))
                    name = ScriptNameFromFileName(Path.GetFileNameWithoutExtension(filePath));

                // ── 4.2 name validation ────────────────────────────────────────
                string idErr = ValidateIdentifier(name);
                if (idErr != null) return Err(idErr);

                // ── 4.5 Validate uniqueness ────────────────────────────────────
                foreach (AudioClip clip in game.AudioClips)
                    if (string.Equals(clip.ScriptName, name, StringComparison.OrdinalIgnoreCase))
                        return Err(string.Format("Audio clip '{0}' already exists", name));

                // ── 4.1 Copy file to AudioCache ────────────────────────────────
                string audioCacheDir = Path.Combine(projectPath, "AudioCache");
                Directory.CreateDirectory(audioCacheDir);

                // Use the game's stable FixedIndex counter (same as the Editor)
                int fixedIndex = game.GetNextAudioIndex();
                // Assign a sequential display ID
                int clipId = game.RootAudioClipFolder.GetAllItemsCount();

                string cacheFileName = string.Format("au{0}{1}", fixedIndex, ext);
                string cachePath = Path.Combine(audioCacheDir, cacheFileName);
                File.Copy(filePath, cachePath, overwrite: true);

                // ── 4.1 Create AudioClip object and add to game ────────────────
                AudioClip newClip = new AudioClip(name, fixedIndex)
                {
                    ID              = clipId,
                    Type            = typeId,
                    FileType        = fileType,
                    BundlingType    = AudioFileBundlingType.InGameEXE,
                    SourceFileName  = filePath,
                    CacheFileName   = cachePath,
                    DefaultVolume   = -1,                        // Inherit from folder
                    DefaultPriority = AudioClipPriority.Inherit,
                    DefaultRepeat   = InheritableBool.Inherit
                };
                newClip.FileLastModifiedDate = File.GetLastWriteTime(filePath);

                game.RootAudioClipFolder.Items.Add(newClip);

                return SaveOk(game, projectPath, new JObject
                {
                    ["audio_id"]    = clipId,
                    ["fixed_index"] = fixedIndex,
                    ["script_name"] = name,
                    ["type_id"]     = typeId,
                    ["file_type"]   = fileType.ToString(),
                    ["cache_file"]  = cacheFileName,
                    ["message"]     = string.Format("Audio clip '{0}' added successfully", name)
                });
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_hotspot_add ───────────────────────────────────────────────────

        public static JToken RoomHotspotAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null) return Err("room_id is required");
                int roomId = (int)args["room_id"];
                string name = args["name"] != null ? args["name"].ToString() : null;
                string idErr = ValidateIdentifier(name);
                if (idErr != null) return Err(idErr);

                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == roomId) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", roomId));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null) return Err(string.Format("Room .crm not found: {0}", unloaded.FileName));
                try
                {
                    // Find first free hotspot slot (slot 0 reserved; auto-generated names = free)
                    RoomHotspot slot = null;
                    int slotId = -1;
                    foreach (RoomHotspot h in fullRoom.Hotspots)
                    {
                        if (h.ID == 0) continue;
                        string defaultName = string.Format("hHotspot{0}", h.ID);
                        if (string.IsNullOrEmpty(h.Name) || h.Name == defaultName)
                        { slot = h; slotId = h.ID; break; }
                    }
                    if (slot == null) return Err("No free hotspot slots available in this room (max 49)");

                    // Check name uniqueness
                    foreach (RoomHotspot h in fullRoom.Hotspots)
                        if (string.Equals(h.Name, name, StringComparison.OrdinalIgnoreCase))
                            return Err(string.Format("Hotspot '{0}' already exists in room {1}", name, roomId));

                    slot.Name = name;
                    if (args["description"] != null) slot.Description = args["description"].ToString();
                    if (args["walk_to_x"] != null || args["walk_to_y"] != null)
                    {
                        int wx = args["walk_to_x"] != null ? (int)args["walk_to_x"] : slot.WalkToPoint.X;
                        int wy = args["walk_to_y"] != null ? (int)args["walk_to_y"] : slot.WalkToPoint.Y;
                        slot.WalkToPoint = new System.Drawing.Point(wx, wy);
                    }

                    NativeRoomLoader.SaveRoom(fullRoom, game, projectPath);
                    return new JObject { ["success"] = true, ["hotspot_id"] = slotId,
                        ["message"] = string.Format("Hotspot '{0}' added to room {1}", name, roomId) };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_hotspot_update ────────────────────────────────────────────────

        public static JToken RoomHotspotUpdate(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null)    return Err("room_id is required");
                if (args["hotspot_id"] == null) return Err("hotspot_id is required");
                int roomId    = (int)args["room_id"];
                int hotspotId = (int)args["hotspot_id"];

                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == roomId) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", roomId));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null) return Err(string.Format("Room .crm not found: {0}", unloaded.FileName));
                try
                {
                    RoomHotspot hotspot = null;
                    foreach (RoomHotspot h in fullRoom.Hotspots)
                        if (h.ID == hotspotId) { hotspot = h; break; }
                    if (hotspot == null) return Err(string.Format("Hotspot {0} not found in room {1}", hotspotId, roomId));

                    if (args["name"] != null)        hotspot.Name        = args["name"].ToString();
                    if (args["description"] != null) hotspot.Description = args["description"].ToString();
                    if (args["walk_to_x"] != null || args["walk_to_y"] != null)
                    {
                        int wx = args["walk_to_x"] != null ? (int)args["walk_to_x"] : hotspot.WalkToPoint.X;
                        int wy = args["walk_to_y"] != null ? (int)args["walk_to_y"] : hotspot.WalkToPoint.Y;
                        hotspot.WalkToPoint = new System.Drawing.Point(wx, wy);
                    }

                    NativeRoomLoader.SaveRoom(fullRoom, game, projectPath);
                    return new JObject { ["success"] = true, ["message"] = "Hotspot updated successfully" };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_object_add ────────────────────────────────────────────────────

        public static JToken RoomObjectAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null) return Err("room_id is required");
                int roomId = (int)args["room_id"];
                string name = args["name"] != null ? args["name"].ToString() : null;
                string idErr = ValidateIdentifier(name);
                if (idErr != null) return Err(idErr);

                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == roomId) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", roomId));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null) return Err(string.Format("Room .crm not found: {0}", unloaded.FileName));
                try
                {
                    // Check name uniqueness
                    foreach (RoomObject obj in fullRoom.Objects)
                        if (string.Equals(obj.Name, name, StringComparison.OrdinalIgnoreCase))
                            return Err(string.Format("Object '{0}' already exists in room {1}", name, roomId));

                    RoomObject newObj = new RoomObject(fullRoom);
                    newObj.ID    = fullRoom.Objects.Count;
                    newObj.Name  = name;
                    if (args["sprite"] != null)      newObj.Image  = (int)args["sprite"];
                    if (args["x"] != null)           newObj.StartX = (int)args["x"];
                    if (args["y"] != null)           newObj.StartY = (int)args["y"];
                    if (args["description"] != null) newObj.Description = args["description"].ToString();
                    if (args["visible"] != null)     newObj.Visible  = (bool)args["visible"];
                    if (args["clickable"] != null)   newObj.Clickable = (bool)args["clickable"];
                    fullRoom.Objects.Add(newObj);

                    NativeRoomLoader.SaveRoom(fullRoom, game, projectPath);
                    return new JObject { ["success"] = true, ["object_id"] = newObj.ID,
                        ["message"] = string.Format("Object '{0}' added to room {1}", name, roomId) };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── room_object_update ─────────────────────────────────────────────────

        public static JToken RoomObjectUpdate(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null)  return Err("room_id is required");
                if (args["object_id"] == null) return Err("object_id is required");
                int roomId   = (int)args["room_id"];
                int objectId = (int)args["object_id"];

                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == roomId) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", roomId));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null) return Err(string.Format("Room .crm not found: {0}", unloaded.FileName));
                try
                {
                    RoomObject obj = null;
                    foreach (RoomObject o in fullRoom.Objects)
                        if (o.ID == objectId) { obj = o; break; }
                    if (obj == null) return Err(string.Format("Object {0} not found in room {1}", objectId, roomId));

                    if (args["name"] != null)        obj.Name        = args["name"].ToString();
                    if (args["description"] != null) obj.Description = args["description"].ToString();
                    if (args["sprite"] != null)      obj.Image       = (int)args["sprite"];
                    if (args["x"] != null)           obj.StartX      = (int)args["x"];
                    if (args["y"] != null)           obj.StartY      = (int)args["y"];
                    if (args["visible"] != null)     obj.Visible     = (bool)args["visible"];
                    if (args["clickable"] != null)   obj.Clickable   = (bool)args["clickable"];
                    if (args["baseline"] != null)
                    {
                        obj.Baseline = (int)args["baseline"];
                        obj.BaselineOverridden = true;
                    }

                    NativeRoomLoader.SaveRoom(fullRoom, game, projectPath);
                    return new JObject { ["success"] = true, ["message"] = "Room object updated successfully" };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }
    }
}
