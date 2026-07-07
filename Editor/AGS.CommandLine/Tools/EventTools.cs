using System;
using System.IO;
using Newtonsoft.Json.Linq;
using AGS.Types;
using AGS.CommandLine.Services;

namespace AGS.CommandLine.Tools
{
    public static class EventTools
    {
        // ── helpers ────────────────────────────────────────────────────────────

        private static JObject Err(string msg)
            => new JObject { ["success"] = false, ["error"] = msg };

        private static Game LoadAndPath(JObject args, out string projectPath)
        {
            projectPath = args["project_path"] != null ? args["project_path"].ToString() : null;
            if (string.IsNullOrEmpty(projectPath))
                throw new ArgumentException("project_path is required");
            return new GameLoader().Load(projectPath);
        }

        /// <summary>
        /// Maps our public event name strings to AGS InteractionSchema suffixes.
        /// </summary>
        private static string EventNameToSuffix(string entityType, string eventName)
        {
            switch (entityType)
            {
                case "hotspot":
                    switch (eventName)
                    {
                        case "on_interact":  return "Interact";
                        case "on_look_at":   return "Look";
                        case "on_use_inv":   return "UseInv";
                        case "on_walk_on":   return "WalkOn";
                        case "on_talk":      return "Talk";
                        case "on_any_click": return "AnyClick";
                        case "on_mouse_move":return "MouseMove";
                        default: return null;
                    }
                case "character":
                    switch (eventName)
                    {
                        case "on_interact":   return "Interact";
                        case "on_look_at":    return "Look";
                        case "on_talk_to":    return "Talk";
                        case "on_use_inv":    return "UseInv";
                        case "on_pickup":     return "PickUp";
                        case "on_any_click":  return "AnyClick";
                        case "on_usermode1":  return "UserMode1";
                        case "on_usermode2":  return "UserMode2";
                        default: return null;
                    }
                case "room":
                    switch (eventName)
                    {
                        case "on_enter":           return "Load";
                        case "on_firstload":       return "FirstLoad";
                        case "on_exit":            return "Leave";
                        case "on_before_fade_in":  return "AfterFadeIn";
                        case "on_after_fade_out":  return "Unload";
                        default: return null;
                    }
                case "inventory":
                    switch (eventName)
                    {
                        case "on_interact":    return "Interact";
                        case "on_look_at":     return "Look";
                        case "on_use_inv":     return "UseInv";
                        case "on_other_click": return "OtherClick";
                        default: return null;
                    }
                default: return null;
            }
        }

        private static string SuffixToEventName(string entityType, string suffix)
        {
            switch (entityType)
            {
                case "hotspot":
                    switch (suffix)
                    {
                        case "Interact":   return "on_interact";
                        case "Look":       return "on_look_at";
                        case "UseInv":     return "on_use_inv";
                        case "WalkOn":     return "on_walk_on";
                        case "Talk":       return "on_talk";
                        case "AnyClick":   return "on_any_click";
                        case "MouseMove":  return "on_mouse_move";
                        default: return suffix.ToLower();
                    }
                case "character":
                    switch (suffix)
                    {
                        case "Interact":   return "on_interact";
                        case "Look":       return "on_look_at";
                        case "Talk":       return "on_talk_to";
                        case "UseInv":     return "on_use_inv";
                        case "PickUp":     return "on_pickup";
                        case "AnyClick":   return "on_any_click";
                        case "UserMode1":  return "on_usermode1";
                        case "UserMode2":  return "on_usermode2";
                        default: return suffix.ToLower();
                    }
                case "room":
                    switch (suffix)
                    {
                        case "Load":       return "on_enter";
                        case "FirstLoad":  return "on_firstload";
                        case "Leave":      return "on_exit";
                        case "AfterFadeIn":return "on_before_fade_in";
                        case "Unload":     return "on_after_fade_out";
                        default: return suffix.ToLower();
                    }
                case "inventory":
                    switch (suffix)
                    {
                        case "Interact":    return "on_interact";
                        case "Look":        return "on_look_at";
                        case "UseInv":      return "on_use_inv";
                        case "OtherClick":  return "on_other_click";
                        default: return suffix.ToLower();
                    }
                default: return suffix.ToLower();
            }
        }

        // ── event_get_handlers ─────────────────────────────────────────────────

        public static JToken EventGetHandlers(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string entityType = args["entity_type"] != null ? args["entity_type"].ToString() : null;
                if (string.IsNullOrEmpty(entityType))
                    return Err("entity_type is required (hotspot, character, room, inventory)");
                if (args["entity_id"] == null) return Err("entity_id is required");
                int entityId = (int)args["entity_id"];
                
                // Optional: filter to single hotspot within a room (for hotspot type only)
                int? hotspotId = null;
                if (entityType == "hotspot" && args["hotspot_id"] != null)
                    hotspotId = (int)args["hotspot_id"];

                Interactions interactions = null;
                string entityName = null;

                switch (entityType)
                {
                    case "character":
                        Character ch = null;
                        foreach (Character c in game.Characters)
                            if (c.ID == entityId) { ch = c; break; }
                        if (ch == null) return Err(string.Format("Character {0} not found", entityId));
                        interactions = ch.Interactions;
                        entityName = ch.ScriptName;
                        break;

                    case "room":
                        IRoom rm = null;
                        foreach (IRoom r in game.Rooms)
                            if (r.Number == entityId) { rm = r; break; }
                        if (rm == null) return Err(string.Format("Room {0} not found", entityId));
                        // Load room to get its interactions
                        AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(rm as UnloadedRoom, game, projectPath);
                        if (fullRoom == null)
                            return new JObject
                            {
                                ["success"] = true,
                                ["entity_type"] = entityType,
                                ["entity_id"] = entityId,
                                ["handlers"] = new JArray(),
                                ["note"] = "Room .crm file not found or not yet compiled"
                            };
                        try
                        {
                            interactions = fullRoom.Interactions;
                            entityName = string.Format("Room {0}", entityId);
                            // Continue to build handlers response below
                        }
                        finally { if (fullRoom != null) NativeRoomLoader.UnloadRoom(fullRoom); }
                        break;

                    case "inventory":
                        InventoryItem inv = null;
                        foreach (InventoryItem i in game.InventoryItems)
                            if (i.ID == entityId) { inv = i; break; }
                        if (inv == null) return Err(string.Format("Inventory item {0} not found", entityId));
                        interactions = inv.Interactions;
                        entityName = inv.Name;
                        break;

                    case "hotspot":
                        // Hotspots require room loading
                        if (args["room_id"] == null)
                            return Err("room_id is required for hotspot queries");
                        int roomId = (int)args["room_id"];
                        
                        IRoom hotspotRoom = null;
                        foreach (IRoom r in game.Rooms)
                            if (r.Number == roomId) { hotspotRoom = r; break; }
                        if (hotspotRoom == null) return Err(string.Format("Room {0} not found", roomId));
                        
                        AGS.Types.Room fullHotspotRoom = NativeRoomLoader.LoadRoom(hotspotRoom as UnloadedRoom, game, projectPath);
                        if (fullHotspotRoom == null)
                            return new JObject
                            {
                                ["success"] = true,
                                ["entity_type"] = entityType,
                                ["entity_id"] = entityId,
                                ["room_id"] = roomId,
                                ["handlers"] = new JArray(),
                                ["note"] = "Room .crm file not found or not yet compiled"
                            };
                        
                        try
                        {
                            // If hotspot_id is specified, filter to that hotspot; otherwise return all for room
                            if (hotspotId.HasValue)
                            {
                                RoomHotspot hotspot = null;
                                foreach (RoomHotspot h in fullHotspotRoom.Hotspots)
                                    if (h.ID == hotspotId.Value) { hotspot = h; break; }
                                if (hotspot == null)
                                    return Err(string.Format("Hotspot {0} not found in room {1}", hotspotId.Value, roomId));
                                
                                interactions = hotspot.Interactions;
                                entityName = hotspot.Name;
                            }
                            else
                            {
                                // Return all hotspots in the room
                                var allHotspotHandlers = new JArray();
                                foreach (RoomHotspot h in fullHotspotRoom.Hotspots)
                                {
                                    if (h.Interactions == null) continue;
                                    string[] suffixes = h.Interactions.FunctionSuffixes;
                                    string[] names = h.Interactions.ScriptFunctionNames;
                                    for (int i = 0; i < suffixes.Length; i++)
                                    {
                                        if (!string.IsNullOrEmpty(names[i]))
                                        {
                                            allHotspotHandlers.Add(new JObject
                                            {
                                                ["hotspot_id"] = h.ID,
                                                ["hotspot_name"] = h.Name ?? string.Format("hHotspot{0}", h.ID),
                                                ["event"] = SuffixToEventName("hotspot", suffixes[i]),
                                                ["suffix"] = suffixes[i],
                                                ["script_method"] = names[i],
                                                ["script_module"] = h.Interactions.ScriptModule ?? string.Empty
                                            });
                                        }
                                    }
                                }
                                return new JObject
                                {
                                    ["success"] = true,
                                    ["entity_type"] = entityType,
                                    ["room_id"] = roomId,
                                    ["handlers"] = allHotspotHandlers
                                };
                            }
                        }
                        finally { if (fullHotspotRoom != null) NativeRoomLoader.UnloadRoom(fullHotspotRoom); }
                        break;

                    default:
                        return Err(string.Format("Unknown entity_type '{0}'", entityType));
                }

                var handlers = new JArray();
                if (interactions != null)
                {
                    string[] suffixes = interactions.FunctionSuffixes;
                    string[] names    = interactions.ScriptFunctionNames;
                    for (int i = 0; i < suffixes.Length; i++)
                    {
                        if (!string.IsNullOrEmpty(names[i]))
                        {
                            handlers.Add(new JObject
                            {
                                ["event"]         = SuffixToEventName(entityType, suffixes[i]),
                                ["suffix"]        = suffixes[i],
                                ["script_method"] = names[i],
                                ["script_module"] = interactions.ScriptModule ?? string.Empty
                            });
                        }
                    }
                }

                return new JObject
                {
                    ["success"]     = true,
                    ["entity_type"] = entityType,
                    ["entity_id"]   = entityId,
                    ["entity_name"] = entityName ?? string.Empty,
                    ["handlers"]    = handlers
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── event_bind_character ───────────────────────────────────────────────

        public static JToken EventBindCharacter(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["character_id"] == null) return Err("character_id is required");
                int id = (int)args["character_id"];
                string eventName    = args["event"] != null ? args["event"].ToString() : null;
                string scriptMethod = args["script_method"] != null ? args["script_method"].ToString() : null;
                if (string.IsNullOrEmpty(eventName))    return Err("event is required");
                if (string.IsNullOrEmpty(scriptMethod)) return Err("script_method is required");

                string suffix = EventNameToSuffix("character", eventName);
                if (suffix == null)
                    return Err(string.Format(
                        "Unknown event '{0}' for character. Valid: on_interact, on_look_at, on_use_inv, on_talk_to, on_pickup, on_any_click, on_usermode1, on_usermode2",
                        eventName));

                Character ch = null;
                foreach (Character c in game.Characters)
                    if (c.ID == id) { ch = c; break; }
                if (ch == null) return Err(string.Format("Character {0} not found", id));

                string module = args["script_module"] != null
                    ? args["script_module"].ToString()
                    : ch.Interactions.ScriptModule;

                ch.Interactions.SetScriptFunctionNameForInteractionSuffix(suffix, scriptMethod);
                if (!string.IsNullOrEmpty(module)) ch.Interactions.ScriptModule = module;

                GameSaver.Save(game, projectPath);
                return new JObject
                {
                    ["success"] = true,
                    ["message"] = string.Format("Bound character.{0} → {1}.{2}", eventName, module, scriptMethod)
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── event_bind_hotspot ────────────────────────────────────────────────

        public static JToken EventBindHotspot(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null) return Err("room_id is required");
                if (args["hotspot_id"] == null) return Err("hotspot_id is required");
                int roomId = (int)args["room_id"];
                int hotspotId = (int)args["hotspot_id"];
                string eventName    = args["event"] != null ? args["event"].ToString() : null;
                string scriptMethod = args["script_method"] != null ? args["script_method"].ToString() : null;
                if (string.IsNullOrEmpty(eventName))    return Err("event is required");
                if (string.IsNullOrEmpty(scriptMethod)) return Err("script_method is required");

                string suffix = EventNameToSuffix("hotspot", eventName);
                if (suffix == null)
                    return Err(string.Format(
                        "Unknown event '{0}' for hotspot. Valid: on_interact, on_look_at, on_use_inv, on_walk_on, on_talk, on_any_click, on_mouse_move",
                        eventName));

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

                    string module = args["script_module"] != null
                        ? args["script_module"].ToString()
                        : (hotspot.Interactions != null ? hotspot.Interactions.ScriptModule : null);

                    if (hotspot.Interactions == null)
                        return Err(string.Format("Hotspot {0} has no interaction schema (not loaded from .crm)", hotspotId));

                    hotspot.Interactions.SetScriptFunctionNameForInteractionSuffix(suffix, scriptMethod);
                    if (!string.IsNullOrEmpty(module)) hotspot.Interactions.ScriptModule = module;

                    NativeRoomLoader.SaveRoom(fullRoom, game, projectPath);
                    return new JObject
                    {
                        ["success"] = true,
                        ["message"] = string.Format("Bound hotspot.{0} (room {1}, hotspot {2}) → {3}.{4}", 
                            eventName, roomId, hotspotId, module, scriptMethod)
                    };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── event_bind_room ────────────────────────────────────────────────────

        public static JToken EventBindRoom(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["room_id"] == null) return Err("room_id is required");
                int roomId = (int)args["room_id"];
                string eventName    = args["event"] != null ? args["event"].ToString() : null;
                string scriptMethod = args["script_method"] != null ? args["script_method"].ToString() : null;
                if (string.IsNullOrEmpty(eventName))    return Err("event is required");
                if (string.IsNullOrEmpty(scriptMethod)) return Err("script_method is required");

                string suffix = EventNameToSuffix("room", eventName);
                if (suffix == null)
                    return Err(string.Format(
                        "Unknown event '{0}' for room. Valid: on_enter, on_exit, on_firstload, on_before_fade_in, on_after_fade_out",
                        eventName));

                IRoom unloaded = null;
                foreach (IRoom r in game.Rooms)
                    if (r.Number == roomId) { unloaded = r; break; }
                if (unloaded == null) return Err(string.Format("Room {0} not found", roomId));

                AGS.Types.Room fullRoom = NativeRoomLoader.LoadRoom(unloaded as UnloadedRoom, game, projectPath);
                if (fullRoom == null) return Err(string.Format("Room .crm not found: {0}", unloaded.FileName));
                try
                {
                    string module = args["script_module"] != null
                        ? args["script_module"].ToString()
                        : (fullRoom.Interactions != null ? fullRoom.Interactions.ScriptModule : null);

                    if (fullRoom.Interactions == null)
                        return Err(string.Format("Room {0} has no interaction schema (not loaded from .crm)", roomId));

                    fullRoom.Interactions.SetScriptFunctionNameForInteractionSuffix(suffix, scriptMethod);
                    if (!string.IsNullOrEmpty(module)) fullRoom.Interactions.ScriptModule = module;

                    NativeRoomLoader.SaveRoom(fullRoom, game, projectPath);
                    return new JObject
                    {
                        ["success"] = true,
                        ["message"] = string.Format("Bound room.{0} (room {1}) → {2}.{3}", 
                            eventName, roomId, module, scriptMethod)
                    };
                }
                finally { NativeRoomLoader.UnloadRoom(fullRoom); }
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── event_bind_inventory ───────────────────────────────────────────────

        public static JToken EventBindInventory(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                if (args["inventory_id"] == null) return Err("inventory_id is required");
                int id = (int)args["inventory_id"];
                string eventName    = args["event"] != null ? args["event"].ToString() : null;
                string scriptMethod = args["script_method"] != null ? args["script_method"].ToString() : null;
                if (string.IsNullOrEmpty(eventName))    return Err("event is required");
                if (string.IsNullOrEmpty(scriptMethod)) return Err("script_method is required");

                string suffix = EventNameToSuffix("inventory", eventName);
                if (suffix == null)
                    return Err(string.Format(
                        "Unknown event '{0}' for inventory. Valid: on_interact, on_look_at, on_use_inv, on_other_click",
                        eventName));

                InventoryItem inv = null;
                foreach (InventoryItem i in game.InventoryItems)
                    if (i.ID == id) { inv = i; break; }
                if (inv == null) return Err(string.Format("Inventory item {0} not found", id));

                string module = args["script_module"] != null
                    ? args["script_module"].ToString()
                    : inv.Interactions.ScriptModule;

                inv.Interactions.SetScriptFunctionNameForInteractionSuffix(suffix, scriptMethod);
                if (!string.IsNullOrEmpty(module)) inv.Interactions.ScriptModule = module;

                GameSaver.Save(game, projectPath);
                return new JObject
                {
                    ["success"] = true,
                    ["message"] = string.Format("Bound inventory.{0} → {1}.{2}", eventName, module, scriptMethod)
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── script_function_exists ─────────────────────────────────────────────

        public static JToken ScriptFunctionExists(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string moduleName    = args["script_module"] != null ? args["script_module"].ToString() : null;
                string functionName  = args["function_name"] != null ? args["function_name"].ToString() : null;
                if (string.IsNullOrEmpty(moduleName))   return Err("script_module is required");
                if (string.IsNullOrEmpty(functionName)) return Err("function_name is required");

                string filePath = ResolveScriptPath(game, projectPath, moduleName);
                if (filePath == null)
                    return Err(string.Format("Script module '{0}' not found. You can use project scripts (GlobalScript, etc.) or room scripts like 'room1', 'room2', 'room1.asc', 'room2.asc', etc.", moduleName));

                bool exists = ScriptEditor.FunctionExists(filePath, functionName);
                return new JObject { ["success"] = true, ["exists"] = exists, ["function_name"] = functionName, ["script_module"] = moduleName };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── script_function_add ────────────────────────────────────────────────

        public static JToken ScriptFunctionAdd(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string moduleName   = args["script_module"] != null ? args["script_module"].ToString() : null;
                string functionName = args["function_name"] != null ? args["function_name"].ToString() : null;
                if (string.IsNullOrEmpty(moduleName))   return Err("script_module is required");
                if (string.IsNullOrEmpty(functionName)) return Err("function_name is required");

                string returnType  = args["return_type"] != null ? args["return_type"].ToString() : "void";
                string parameters  = args["parameters"]  != null ? args["parameters"].ToString()  : string.Empty;
                string body        = args["body"]         != null ? args["body"].ToString()         : null;

                string filePath = ResolveScriptPath(game, projectPath, moduleName);
                if (filePath == null)
                    return Err(string.Format(
                        "Script module '{0}' not found. You can use project scripts (GlobalScript, etc.) or room scripts like 'room1', 'room2', 'room1.asc', 'room2.asc', etc.",
                        moduleName));

                if (ScriptEditor.FunctionExists(filePath, functionName))
                    return Err(string.Format("Function '{0}' already exists in {1}", functionName, moduleName));

                int lineNumber = ScriptEditor.AppendFunction(filePath, functionName, returnType, parameters, body);

                return new JObject
                {
                    ["success"] = true,
                    ["message"] = string.Format("Function '{0}' added to {1}", functionName, moduleName),
                    ["function_location"] = new JObject
                    {
                        ["script_module"] = moduleName,
                        ["file_path"]     = filePath,
                        ["line_number"]   = lineNumber
                    }
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── script_function_update ────────────────────────────────────────────

        public static JToken ScriptFunctionUpdate(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string moduleName   = args["script_module"] != null ? args["script_module"].ToString() : null;
                string functionName = args["function_name"] != null ? args["function_name"].ToString() : null;
                string newBody      = args["body"] != null ? args["body"].ToString() : null;

                if (string.IsNullOrEmpty(moduleName))   return Err("script_module is required");
                if (string.IsNullOrEmpty(functionName)) return Err("function_name is required");
                if (string.IsNullOrEmpty(newBody))      return Err("body is required (use empty string for empty body)");

                string filePath = ResolveScriptPath(game, projectPath, moduleName);
                if (filePath == null)
                    return Err(string.Format("Script module '{0}' not found. You can use project scripts (GlobalScript, etc.) or room scripts like 'room1', 'room2', 'room1.asc', 'room2.asc', etc.", moduleName));

                if (!ScriptEditor.FunctionExists(filePath, functionName))
                    return Err(string.Format("Function '{0}' not found in {1}", functionName, moduleName));

                string errorMessage;
                bool success = ScriptEditor.ReplaceFunction(filePath, functionName, newBody, out errorMessage);
                if (!success)
                    return Err(errorMessage ?? "Unknown error while replacing function body");

                return new JObject
                {
                    ["success"] = true,
                    ["message"] = string.Format("Function '{0}' in {1} updated successfully", functionName, moduleName),
                    ["function_location"] = new JObject
                    {
                        ["script_module"] = moduleName,
                        ["file_path"]     = filePath
                    }
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── declare_variable ──────────────────────────────────────────────────

        public static JToken DeclareVariable(JObject args)
        {
            try
            {
                string projectPath;
                Game game = LoadAndPath(args, out projectPath);

                string moduleName = args["script_module"] != null ? args["script_module"].ToString() : null;
                string varName    = args["var_name"]      != null ? args["var_name"].ToString()      : null;
                string varType    = args["var_type"]      != null ? args["var_type"].ToString()      : null;

                if (string.IsNullOrEmpty(moduleName)) return Err("script_module is required");
                if (string.IsNullOrEmpty(varName))    return Err("var_name is required");
                if (string.IsNullOrEmpty(varType))    return Err("var_type is required (e.g. int, bool, String)");

                string filePath = ResolveScriptPath(game, projectPath, moduleName);
                if (filePath == null)
                    return Err(string.Format(
                        "Script module '{0}' not found. You can use project scripts (GlobalScript, etc.) or room scripts like 'room1', 'room2', 'room1.asc', 'room2.asc', etc.",
                        moduleName));

                // Idempotent: skip if the variable is already declared
                if (Services.ScriptEditor.VariableExists(filePath, varName))
                    return new JObject
                    {
                        ["success"] = true,
                        ["exists"]  = true,
                        ["message"] = string.Format("Variable '{0}' already declared in {1}", varName, moduleName)
                    };

                Services.ScriptEditor.DeclareGlobalVariable(filePath, varType, varName);

                return new JObject
                {
                    ["success"]     = true,
                    ["exists"]      = false,
                    ["message"]     = string.Format("Variable '{0} {1}' declared in {2}", varType, varName, moduleName),
                    ["declaration"] = new JObject
                    {
                        ["script_module"] = moduleName,
                        ["file_path"]     = filePath,
                        ["var_type"]      = varType,
                        ["var_name"]      = varName
                    }
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── list_event_types ───────────────────────────────────────────────────

        public static JToken ListEventTypes(JObject args)
        {
            try
            {
                // Character events
                var characterEvents = new JArray(
                    "on_interact", "on_look_at", "on_use_inv", "on_talk_to",
                    "on_pickup", "on_any_click", "on_usermode1", "on_usermode2"
                );

                // Inventory events
                var inventoryEvents = new JArray(
                    "on_interact", "on_look_at", "on_use_inv", "on_other_click"
                );

                // Hotspot events (stored in .crm)
                var hotspotEvents = new JArray(
                    "on_interact", "on_look_at", "on_use_inv", "on_walk_on",
                    "on_talk", "on_any_click", "on_mouse_move"
                );

                // Room events (stored in .crm)
                var roomEvents = new JArray(
                    "on_enter", "on_exit", "on_firstload", "on_before_fade_in", "on_after_fade_out"
                );

                return new JObject
                {
                    ["success"] = true,
                    ["character_events"] = characterEvents,
                    ["inventory_events"] = inventoryEvents,
                    ["hotspot_events"] = hotspotEvents,
                    ["room_events"] = roomEvents,
                    ["note"] = "Use event_bind_character, event_bind_inventory, event_bind_hotspot, event_bind_room to bind handlers."
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── helpers ────────────────────────────────────────────────────────────

        /// <summary>
        /// Normalizes a script module name: accepts "room3", "Room3", "room3.asc" → lowercase + strip .asc.
        /// </summary>
        private static string NormalizeScriptName(string moduleName)
        {
            if (string.IsNullOrEmpty(moduleName)) return moduleName;
            string normalized = moduleName.ToLower().Trim();
            if (normalized.EndsWith(".asc", StringComparison.OrdinalIgnoreCase))
                normalized = normalized.Substring(0, normalized.Length - 4);
            return normalized;
        }

        /// <summary>
        /// Resolves the full path to a script module's .asc file.
        /// Script.FileName is relative (e.g. "GlobalScript.asc") — combine with projectPath.
        /// Supports both project scripts and room scripts (roomN.asc on disk).
        /// </summary>
        private static string ResolveScriptPath(Game game, string projectPath, string moduleName)
        {
            string normalized = NormalizeScriptName(moduleName);
            
            // First, try project scripts
            foreach (ScriptAndHeader sh in game.ScriptsAndHeaders)
            {
                if (string.Equals(sh.Name, normalized, StringComparison.OrdinalIgnoreCase) ||
                    string.Equals(NormalizeScriptName(sh.Name), normalized, StringComparison.OrdinalIgnoreCase))
                {
                    string relative = sh.Script != null ? sh.Script.FileName : null;
                    if (relative == null) return null;
                    // FileName may already be absolute (if loaded from disk) or relative
                    if (Path.IsPathRooted(relative)) return File.Exists(relative) ? relative : null;
                    string full = Path.Combine(projectPath, relative);
                    return File.Exists(full) ? full : null;
                }
            }

            // Fallback: check if roomN.asc exists on disk (for room scripts)
            if (normalized.StartsWith("room"))
            {
                string roomScriptPath = Path.Combine(projectPath, normalized + ".asc");
                if (File.Exists(roomScriptPath))
                    return roomScriptPath;
            }

            return null;
        }
    }
}
