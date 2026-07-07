using System;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace AGS.CommandLine
{
    public class McpProtocol
    {
        private readonly Dictionary<string, Func<JObject, JToken>> _tools =
            new Dictionary<string, Func<JObject, JToken>>();

        public void RegisterTool(string name, Func<JObject, JToken> handler)
        {
            _tools[name] = handler;
        }

        public JObject HandleRequest(JObject request)
        {
            string method = request["method"] != null ? request["method"].ToString() : null;
            JToken id = request["id"];

            switch (method)
            {
                case "initialize":
                    return MakeResponse(id, new JObject
                    {
                        ["protocolVersion"] = "2024-11-05",
                        ["capabilities"] = new JObject { ["tools"] = new JObject() },
                        ["serverInfo"] = new JObject { ["name"] = "ags", ["version"] = "0.1.0" }
                    });
                case "notifications/initialized":
                    return null;
                case "tools/list":
                    var arr = new JArray();
                    foreach (var kv in _tools)
                        arr.Add(new JObject { ["name"] = kv.Key, ["description"] = GetDesc(kv.Key), ["inputSchema"] = GetSchema(kv.Key) });
                    return MakeResponse(id, new JObject { ["tools"] = arr });
                case "tools/call":
                    string toolName = (request["params"] != null && request["params"]["name"] != null) ? request["params"]["name"].ToString() : null;
                    JObject args = (request["params"] != null ? request["params"]["arguments"] as JObject : null) ?? new JObject();
                    if (string.IsNullOrEmpty(toolName)) return MakeError(id, -32602, "Missing tool name");
                    if (!_tools.ContainsKey(toolName)) return MakeError(id, -32601, string.Format("Tool not found: {0}", toolName));
                    try
                    {
                        JToken res = _tools[toolName](args);
                        return MakeResponse(id, new JObject { ["content"] = new JArray { new JObject { ["type"] = "text", ["text"] = res.ToString(Newtonsoft.Json.Formatting.None) } } });
                    }
                    catch (Exception ex)
                    {
                        return MakeResponse(id, new JObject { ["content"] = new JArray { new JObject { ["type"] = "text", ["text"] = string.Format("{{\"error\":\"{0}\"}}", ex.Message) } }, ["isError"] = true });
                    }
                default:
                    if (id == null) return null;
                    return MakeError(id, -32601, string.Format("Method not found: {0}", method));
            }
        }

        private JObject MakeResponse(JToken id, JToken result)
        {
            return new JObject { ["jsonrpc"] = "2.0", ["result"] = result, ["id"] = id };
        }

        private JObject MakeError(JToken id, int code, string message)
        {
            return new JObject { ["jsonrpc"] = "2.0", ["error"] = new JObject { ["code"] = code, ["message"] = message }, ["id"] = id };
        }

        private string GetDesc(string name)
        {
            switch (name)
            {
                case "project_info":    return "Get summary counts and settings for an AGS game project.";
                case "character_list":  return "List all characters in the game. Pass include_details=true for full fields.";
                case "character_get":   return "Get full details for a specific character by ID.";
                case "room_list":       return "List all rooms in the game. Pass include_details=true for extra fields.";
                case "room_get":        return "Get details for a specific room by its number.";
                case "room_hotspots":   return "List hotspots defined in a room (requires .crm; returns info on availability).";
                case "room_objects":    return "List objects defined in a room (requires .crm; returns info on availability).";
                case "dialog_list":     return "List all dialogs. Pass include_details=true for option counts.";
                case "dialog_get":      return "Get a dialog and all its options by ID.";
                case "inventory_list":  return "List all inventory items. Pass include_details=true for sprite/description.";
                case "inventory_get":   return "Get full details for a specific inventory item by ID.";
                case "gui_list":        return "List all GUIs. Pass include_details=true for position/size/control count.";
                case "gui_get":         return "Get a GUI and all its controls by ID.";
                case "view_list":       return "List all views. Pass include_details=true for loop/frame counts.";
                case "view_get":        return "Get a view with all loops and frames by ID.";
                case "audio_list":      return "List all audio clips. Pass include_details=true for type/format.";
                case "audio_get":       return "Get full details for a specific audio clip by ID.";
                case "script_list":     return "List all script modules. Pass include_details=true for line counts.";
                case "script_get":      return "Get details for a specific script module by index.";
                case "character_add":   return "Add a new character to the game.";
                case "character_update":return "Update properties of an existing character.";
                case "room_add":        return "Add a new room to the game.";
                case "room_update":     return "Update the description/name of an existing room.";
                case "dialog_add":      return "Add a new dialog to the game.";
                case "dialog_option_add": return "Add an option to an existing dialog.";
                case "inventory_item_add":    return "Add a new inventory item to the game.";
                case "inventory_item_update": return "Update properties of an existing inventory item.";
                case "inventory_item_rename": return "Rename an existing inventory item (with warnings about script references).";
                case "audio_add":       return "Add a new audio clip to the game by importing a source file. Copies the file to AudioCache, creates an AudioClip object, and saves the game. Parameters: file_path (required, must exist on disk; supported formats: .mp3 .ogg .wav .flac), name (optional script name; auto-generated from filename if omitted, e.g. 'aPalmtreePanic' from 'Palmtree Panic.mp3'), type (optional; 'music' maps to AudioClipType 2, 'sound' maps to AudioClipType 3 — default is 'sound').";
                case "list_event_types": return "List available event types for characters, inventory items, hotspots, and rooms.";
                case "event_get_handlers":     return "Get all event handlers bound to a game entity (character, inventory, room, or hotspot). For hotspots, pass room_id and optionally hotspot_id.";
                case "event_bind_character":   return "Bind an event handler to a character interaction event.";
                case "event_bind_room":        return "Bind a room lifecycle event handler (writes to .crm via AGS.Native).";
                case "event_bind_hotspot":     return "Bind an event handler to a room hotspot interaction event (writes .crm via AGS.Native).";
                case "event_bind_inventory":   return "Bind an event handler to an inventory item interaction event.";
                case "script_function_exists": return "Check if a function exists in a script module.";
                case "script_function_add":    return "Append a new function stub to a script module.";
                case "script_function_update": return "Replace the body of an existing function in a script module. Performs brace-aware parsing to handle braces in strings and comments.";
                case "declare_variable":       return "Declare a global variable at the top of a script module. Idempotent — returns exists:true without error if the variable is already declared. Parameters: script_module, var_name, var_type (e.g. int, bool, String).";
                case "game_build_check": return "Check if a project is ready to build (validates script/sprite/room/audio files exist).";
                case "game_build":       return "Report state of the existing compiled build output. This reports build status; does NOT trigger rebuild. To recompile scripts and assets, use the AGS Editor.";
                case "game_test_run":    return "Launch the compiled game executable for testing.";
                case "game_export":      return "Package the compiled game into a zip archive for distribution.";
                case "room_walkable_areas": return "List walkable areas in a room (reads .crm via AGS.Native).";
                case "room_regions":        return "List regions in a room (reads .crm via AGS.Native).";
                case "room_hotspot_add":    return "Add a named hotspot to an existing room (writes .crm via AGS.Native).";
                case "room_hotspot_update": return "Update name, description, or walk-to point of an existing room hotspot.";
                case "room_object_add":     return "Add a new object to an existing room (writes .crm via AGS.Native).";
                case "room_object_update":  return "Update properties of an existing room object.";
                default: return string.Empty;
            }
        }

        private static readonly string _pathDetailSchema =
            "{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"include_details\":{\"type\":\"boolean\"}},\"required\":[\"project_path\"]}";

        private static string IdSchema(string idName)
        {
            return string.Format("{{\"type\":\"object\",\"properties\":{{\"project_path\":{{\"type\":\"string\"}},\"{0}\":{{\"type\":\"integer\"}}}},\"required\":[\"project_path\",\"{0}\"]}}", idName);
        }

        private JObject GetSchema(string name)
        {
            switch (name)
            {
                case "project_info":
                case "room_list":
                case "dialog_list":
                case "inventory_list":
                case "gui_list":
                case "view_list":
                case "audio_list":
                case "script_list":
                case "character_list":
                    return JObject.Parse(_pathDetailSchema);
                case "character_get":
                    return JObject.Parse(IdSchema("character_id"));
                case "room_get":
                case "room_hotspots":
                case "room_objects":
                    return JObject.Parse(IdSchema("room_id"));
                case "dialog_get":
                    return JObject.Parse(IdSchema("dialog_id"));
                case "inventory_get":
                    return JObject.Parse(IdSchema("inventory_id"));
                case "gui_get":
                    return JObject.Parse(IdSchema("gui_id"));
                case "view_get":
                    return JObject.Parse(IdSchema("view_id"));
                case "audio_get":
                    return JObject.Parse(IdSchema("audio_id"));
                case "script_get":
                    return JObject.Parse(IdSchema("script_id"));
                case "character_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"real_name\":{\"type\":\"string\"},\"view\":{\"type\":\"integer\"},\"talking_color\":{\"type\":\"integer\"},\"starting_room\":{\"type\":\"integer\"},\"starting_x\":{\"type\":\"integer\"},\"starting_y\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"name\"]}");
                case "character_update":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"character_id\":{\"type\":\"integer\"},\"real_name\":{\"type\":\"string\"},\"view\":{\"type\":\"integer\"},\"talking_color\":{\"type\":\"integer\"},\"starting_room\":{\"type\":\"integer\"},\"starting_x\":{\"type\":\"integer\"},\"starting_y\":{\"type\":\"integer\"},\"idle_view\":{\"type\":\"integer\"},\"idle_time\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"character_id\"]}");
                case "room_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"name\"]}");
                case "room_update":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"room_id\"]}");
                case "dialog_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"show_always\":{\"type\":\"boolean\"}},\"required\":[\"project_path\",\"name\"]}");
                case "dialog_option_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"dialog_id\":{\"type\":\"integer\"},\"text\":{\"type\":\"string\"},\"show_always\":{\"type\":\"boolean\"},\"say_as_character\":{\"type\":\"boolean\"}},\"required\":[\"project_path\",\"dialog_id\",\"text\"]}");
                case "inventory_item_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"description\":{\"type\":\"string\"},\"sprite\":{\"type\":\"integer\"},\"cursor_sprite\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"name\"]}");
                case "inventory_item_update":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"inventory_id\":{\"type\":\"integer\"},\"description\":{\"type\":\"string\"},\"sprite\":{\"type\":\"integer\"},\"cursor_sprite\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"inventory_id\"]}");
                case "inventory_item_rename":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"inventory_id\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"inventory_id\",\"name\"]}");
                case "audio_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\",\"description\":\"Path to the AGS project directory\"},\"file_path\":{\"type\":\"string\",\"description\":\"Absolute or relative path to the audio source file (.mp3, .ogg, .wav, .flac)\"},\"name\":{\"type\":\"string\",\"description\":\"Script name for the clip (e.g. aPalmtreePanic). Auto-generated from filename if omitted.\"},\"type\":{\"type\":\"string\",\"enum\":[\"music\",\"sound\"],\"description\":\"Audio category: 'music' (AudioClipType 2) or 'sound' (AudioClipType 3). Defaults to 'sound'.\"}},\"required\":[\"project_path\",\"file_path\"]}");
                case "list_event_types":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{},\"required\":[]}");
                case "event_get_handlers":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"entity_type\":{\"type\":\"string\"},\"entity_id\":{\"type\":\"integer\"},\"room_id\":{\"type\":\"integer\"},\"hotspot_id\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"entity_type\",\"entity_id\"]}");
                case "event_bind_character":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"character_id\":{\"type\":\"integer\"},\"event\":{\"type\":\"string\"},\"script_method\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"character_id\",\"event\",\"script_method\"]}");
                case "event_bind_room":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"event\":{\"type\":\"string\"},\"script_method\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"room_id\",\"event\",\"script_method\"]}");
                case "event_bind_hotspot":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"hotspot_id\":{\"type\":\"integer\"},\"event\":{\"type\":\"string\"},\"script_method\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"room_id\",\"hotspot_id\",\"event\",\"script_method\"]}");
                case "event_bind_inventory":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"inventory_id\":{\"type\":\"integer\"},\"event\":{\"type\":\"string\"},\"script_method\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"inventory_id\",\"event\",\"script_method\"]}");
                case "script_function_exists":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"},\"function_name\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"script_module\",\"function_name\"]}");
                case "script_function_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"},\"function_name\":{\"type\":\"string\"},\"return_type\":{\"type\":\"string\"},\"parameters\":{\"type\":\"string\"},\"body\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"script_module\",\"function_name\"]}");
                case "script_function_update":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"},\"function_name\":{\"type\":\"string\"},\"body\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"script_module\",\"function_name\",\"body\"]}");
                case "declare_variable":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"script_module\":{\"type\":\"string\"},\"var_name\":{\"type\":\"string\"},\"var_type\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"script_module\",\"var_name\",\"var_type\"]}");
                case "game_build_check":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"check_scripts\":{\"type\":\"boolean\"},\"check_sprites\":{\"type\":\"boolean\"},\"check_rooms\":{\"type\":\"boolean\"},\"check_audio\":{\"type\":\"boolean\"}},\"required\":[\"project_path\"]}");
                case "game_build":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"output_dir\":{\"type\":\"string\"}},\"required\":[\"project_path\"]}");
                case "game_test_run":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"windowed_mode\":{\"type\":\"boolean\"}},\"required\":[\"project_path\"]}");
                case "game_export":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"export_dir\":{\"type\":\"string\"},\"version\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"export_dir\"]}");
                case "room_walkable_areas":
                case "room_regions":
                    return JObject.Parse(IdSchema("room_id"));
                case "room_hotspot_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"},\"description\":{\"type\":\"string\"},\"walk_to_x\":{\"type\":\"integer\"},\"walk_to_y\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"room_id\",\"name\"]}");
                case "room_hotspot_update":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"hotspot_id\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"},\"description\":{\"type\":\"string\"},\"walk_to_x\":{\"type\":\"integer\"},\"walk_to_y\":{\"type\":\"integer\"}},\"required\":[\"project_path\",\"room_id\",\"hotspot_id\"]}");
                case "room_object_add":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"},\"sprite\":{\"type\":\"integer\"},\"x\":{\"type\":\"integer\"},\"y\":{\"type\":\"integer\"},\"description\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"room_id\",\"name\"]}");
                case "room_object_update":
                    return JObject.Parse("{\"type\":\"object\",\"properties\":{\"project_path\":{\"type\":\"string\"},\"room_id\":{\"type\":\"integer\"},\"object_id\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"},\"sprite\":{\"type\":\"integer\"},\"x\":{\"type\":\"integer\"},\"y\":{\"type\":\"integer\"},\"visible\":{\"type\":\"boolean\"},\"clickable\":{\"type\":\"boolean\"},\"baseline\":{\"type\":\"integer\"},\"description\":{\"type\":\"string\"}},\"required\":[\"project_path\",\"room_id\",\"object_id\"]}");
                default:
                    return new JObject { ["type"] = "object", ["properties"] = new JObject() };
            }
        }
    }
}