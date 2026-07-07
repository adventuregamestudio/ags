---
inclusion: auto
---
# AGS MCP — Index & Quick Reference

The AGS MCP server (`AGS.CommandLine.exe`) lets you build Adventure Game Studio games
programmatically. **Every tool requires `project_path`** (absolute path to the folder
containing `Game.agf`). All responses return `{"success": true/false, ...}`.

## Detailed Steering Files (pull with `#` when needed)

| File | When to include |
|------|----------------|
| `MCP_TOOLS_READ.md` | Reading entities — characters, rooms, hotspots, dialogs, audio, views, GUIs, scripts |
| `MCP_TOOLS_WRITE.md` | Creating or modifying entities — rooms, characters, inventory, audio, hotspots, objects, dialogs |
| `MCP_TOOLS_EVENTS.md` | Binding events, writing/updating script functions, declaring variables |
| `MCP_TOOLS_BUILD.md` | Checking build readiness, launching the game, exporting a zip |
| `MCP_WORKFLOWS.md` | Full step-by-step recipes: new game, multi-room, puzzles, characters |
| `MCP_SCRIPTING.md` | AGS Script syntax, dialogue patterns, global vs room script rules |
| `MCP_TROUBLESHOOTING.md` | Error messages → root cause → fix |

## All Tools at a Glance

### Read (never modify the project)
| Tool | What it returns |
|------|----------------|
| `project_info` | Game name, resolution, entity counts |
| `character_list` | All characters (add `include_details:true` for full fields) |
| `character_get` | One character by `character_id` |
| `room_list` | All rooms |
| `room_get` | One room; loads `.crm` if available for hotspot/object counts |
| `room_hotspots` | Named hotspots in a room (requires `.crm`) |
| `room_objects` | Room objects/sprites (requires `.crm`) |
| `room_walkable_areas` | Walkable area zones |
| `room_regions` | Region zones (lighting/tint) |
| `dialog_list` | All dialogs |
| `dialog_get` | One dialog with all options by `dialog_id` |
| `inventory_list` | All inventory items |
| `inventory_get` | One item by `inventory_id` |
| `gui_list` | All GUIs |
| `gui_get` | One GUI with controls by `gui_id` |
| `view_list` | All views |
| `view_get` | One view with loops and frames by `view_id` |
| `audio_list` | All audio clips |
| `audio_get` | One clip by `audio_id` |
| `script_list` | All script modules |
| `script_get` | Full script content by `script_id` |

### Write (modify `Game.agf` and/or `.crm`)
| Tool | What it does |
|------|-------------|
| `character_add` | Create a new character |
| `character_update` | Update character properties |
| `room_add` | Create room + blank `.crm` + blank `.asc` |
| `room_update` | Update room display name |
| `room_hotspot_add` | Add a named hotspot (uses first free slot 1–49) |
| `room_hotspot_update` | Update hotspot name/description/walk-to |
| `room_object_add` | Add a room object (sprite) |
| `room_object_update` | Update object position/sprite/visibility |
| `dialog_add` | Create a new dialog topic |
| `dialog_option_add` | Add an option to a dialog |
| `inventory_item_add` | Create an inventory item |
| `inventory_item_update` | Update item sprites/description |
| `inventory_item_rename` | Rename item (warns about script refs) |
| `audio_add` | Import audio file into project |

### Events & Scripts (bind handlers, write `.asc` code)
| Tool | What it does |
|------|-------------|
| `list_event_types` | Show all valid event names per entity type |
| `event_get_handlers` | Get current event bindings for any entity |
| `event_bind_character` | Bind function to character event |
| `event_bind_room` | Bind function to room lifecycle event |
| `event_bind_hotspot` | Bind function to hotspot event |
| `event_bind_inventory` | Bind function to inventory item event |
| `script_function_exists` | Check if a function exists in a script file |
| `script_function_add` | Append new function to a script file |
| `script_function_update` | Replace body of existing function |
| `declare_variable` | Declare global variable (idempotent) |

### Build
| Tool | What it does |
|------|-------------|
| `game_build_check` | Validate all referenced files exist on disk |
| `game_build` | Report state of existing compiled output (no rebuild) |
| `game_test_run` | Launch compiled game exe |
| `game_export` | Zip game for distribution |

## Critical Rules

1. **The MCP cannot compile scripts.** After writing `.asc` files, the AGS Editor must
   rebuild the project. Use `game_build_check` to validate, then open the Editor.

2. **`.crm` must exist** before reading/modifying hotspots or objects. `room_add` creates it.
   If missing, those tools return a `note` explaining why.

3. **Hotspot slot 0 is always "No Hotspot"** — never use it. Slots 1–49 are available.

4. **`script_function_add` fails if function exists.** Use `script_function_exists` first,
   then call `add` or `update` accordingly.

5. **`declare_variable` is idempotent** — safe to call multiple times.

6. **Room scripts** (`room1`, `room1.asc`) are for room-local logic.
   **GlobalScript** is for logic that spans rooms (character handlers, puzzle state).
