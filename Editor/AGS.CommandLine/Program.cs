using System;
using AGS.CommandLine.Tools;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace AGS.CommandLine
{
    class Program
    {
        static void Main(string[] args)
        {
            // MCP stdio: must be UTF-8, LF-only line endings (not Windows CRLF)
            Console.InputEncoding  = System.Text.Encoding.UTF8;
            Console.OutputEncoding = System.Text.Encoding.UTF8;

            // Open stdout once as a raw binary stream so we control line endings.
            // Console.WriteLine on Windows emits \r\n which breaks MCP framing.
            var rawOut = new System.IO.BufferedStream(Console.OpenStandardOutput());

            var protocol = new McpProtocol();

            // Phase 0
            protocol.RegisterTool("project_info", ProjectTools.ProjectInfo);

            // Phase 1 — list tools
            protocol.RegisterTool("character_list",  QueryTools.CharacterList);
            protocol.RegisterTool("room_list",        QueryTools.RoomList);
            protocol.RegisterTool("dialog_list",      QueryTools.DialogList);
            protocol.RegisterTool("inventory_list",   QueryTools.InventoryList);
            protocol.RegisterTool("gui_list",         QueryTools.GuiList);
            protocol.RegisterTool("view_list",        QueryTools.ViewList);
            protocol.RegisterTool("audio_list",       QueryTools.AudioList);
            protocol.RegisterTool("script_list",      QueryTools.ScriptList);

            // Phase 1 — get / detail tools
            protocol.RegisterTool("character_get",    QueryTools.CharacterGet);
            protocol.RegisterTool("room_get",         QueryTools.RoomGet);
            protocol.RegisterTool("room_hotspots",    QueryTools.RoomHotspots);
            protocol.RegisterTool("room_objects",     QueryTools.RoomObjects);
            protocol.RegisterTool("dialog_get",       QueryTools.DialogGet);
            protocol.RegisterTool("inventory_get",    QueryTools.InventoryGet);
            protocol.RegisterTool("gui_get",          QueryTools.GuiGet);
            protocol.RegisterTool("view_get",         QueryTools.ViewGet);
            protocol.RegisterTool("audio_get",        QueryTools.AudioGet);
            protocol.RegisterTool("script_get",       QueryTools.ScriptGet);

            // Phase 2 — write tools
            protocol.RegisterTool("character_add",        WriteTools.CharacterAdd);
            protocol.RegisterTool("character_update",     WriteTools.CharacterUpdate);
            protocol.RegisterTool("room_add",             WriteTools.RoomAdd);
            protocol.RegisterTool("room_update",          WriteTools.RoomUpdate);
            protocol.RegisterTool("dialog_add",           WriteTools.DialogAdd);
            protocol.RegisterTool("dialog_option_add",    WriteTools.DialogOptionAdd);
            protocol.RegisterTool("inventory_item_add",   WriteTools.InventoryItemAdd);
            protocol.RegisterTool("inventory_item_update",WriteTools.InventoryItemUpdate);
            protocol.RegisterTool("inventory_item_rename",WriteTools.InventoryItemRename);
            protocol.RegisterTool("audio_add",            WriteTools.AudioAdd);

            // Phase 3 — event binding + script tools
            protocol.RegisterTool("event_get_handlers",    EventTools.EventGetHandlers);
            protocol.RegisterTool("event_bind_character",  EventTools.EventBindCharacter);
            protocol.RegisterTool("event_bind_room",       EventTools.EventBindRoom);
            protocol.RegisterTool("event_bind_hotspot",    EventTools.EventBindHotspot);
            protocol.RegisterTool("event_bind_inventory",  EventTools.EventBindInventory);
            protocol.RegisterTool("list_event_types",      EventTools.ListEventTypes);
            protocol.RegisterTool("script_function_exists",EventTools.ScriptFunctionExists);
            protocol.RegisterTool("script_function_add",   EventTools.ScriptFunctionAdd);
            protocol.RegisterTool("script_function_update",EventTools.ScriptFunctionUpdate);
            protocol.RegisterTool("declare_variable",      EventTools.DeclareVariable);

            // Phase 4 — build & export tools
            protocol.RegisterTool("game_build_check", BuildTools.GameBuildCheck);
            protocol.RegisterTool("game_build",       BuildTools.GameBuild);
            protocol.RegisterTool("game_test_run",    BuildTools.GameTestRun);
            protocol.RegisterTool("game_export",      BuildTools.GameExport);

            // Phase 5 — room binary data via AGS.Native
            protocol.RegisterTool("room_walkable_areas",  QueryTools.RoomWalkableAreas);
            protocol.RegisterTool("room_regions",         QueryTools.RoomRegions);
            protocol.RegisterTool("room_hotspot_add",     WriteTools.RoomHotspotAdd);
            protocol.RegisterTool("room_hotspot_update",  WriteTools.RoomHotspotUpdate);
            protocol.RegisterTool("room_object_add",      WriteTools.RoomObjectAdd);
            protocol.RegisterTool("room_object_update",   WriteTools.RoomObjectUpdate);

            // MCP stdio loop: one JSON object per line in, one per line out
            string line;
            while ((line = Console.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line))
                    continue;

                try
                {
                    JObject request = JObject.Parse(line);
                    JObject response = protocol.HandleRequest(request);
                    if (response != null)
                    {
                        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(
                            response.ToString(Formatting.None) + "\n");
                        rawOut.Write(bytes, 0, bytes.Length);
                        rawOut.Flush();
                    }
                }
                catch (JsonReaderException)
                {
                    // Ignore parse errors silently — don't write to stderr
                }
                catch (Exception)
                {
                    // Ignore runtime errors silently — response was not sent
                }
            }
        }
    }
}
