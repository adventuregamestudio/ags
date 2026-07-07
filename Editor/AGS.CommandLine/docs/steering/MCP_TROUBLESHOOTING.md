---
inclusion: manual
---
# AGS MCP — Troubleshooting

Quick-reference: error message → root cause → fix.

---

## Script Module Errors

**`"Script module 'room3' not found"`**
- The `.asc` file doesn't exist on disk, or the module name is wrong.
- Room scripts are `roomN.asc`. Accepted: `room3`, `Room3`, `room3.asc`.
- Fix: verify the room was created with `room_add`. Check with `game_build_check`.

**`"Function 'X' already exists in room1"`**
- `script_function_add` was called for a function that's already in the file.
- Fix: use `script_function_update` to change the body, not `add`.

**`"Function 'X' does not exist in room1. Use script_function_add first."`**
- `script_function_update` was called but the function hasn't been created.
- Fix: call `script_function_add` first, then `update` on subsequent changes.

**`"Could not parse function 'X'. Check for unmatched braces."`**
- The function body in the `.asc` file has unbalanced `{` or `}`.
- Fix: open the `.asc` file, find the function, fix the brace balance manually.

---

## Room / Hotspot Errors

**`"Room .crm not found: room3.crm"`**
- The `.crm` binary is missing.
- Fix: delete the room from `Game.agf` in the AGS Editor, then re-add with `room_add`.
  Or open the AGS Editor, open the room, and save to regenerate the `.crm`.

**`"Hotspot 0 not found in room 1"`**
- Slot 0 is always "No Hotspot" and can never be used.
- Fix: use hotspot IDs 1–49. Check current hotspots with `room_hotspots`.

**`"No free hotspot slots available in this room (max 49)"`**
- All 49 slots are filled. AGS hard limit — cannot be increased.
- Fix: consider splitting content across multiple rooms.

**`"Room .crm not found"` from `room_hotspots` / `room_objects`**
- Not a true error — the room just hasn't been compiled yet.
- The tool returns `count: 0` with a `note` field explaining why.
- Fix: create the room with `room_add` (which generates the `.crm`).

---

## Audio Errors

**`"Audio clip 'aTheme' already exists"`**
- `audio_add` was called with a name already registered.
- Fix: use `audio_list` to see what exists. Use a different name or the existing clip.

**`"Audio file not found: C:\path\file.mp3"`**
- The `file_path` doesn't exist on disk.
- Fix: verify the absolute path. The file must exist before calling `audio_add`.

**`"Unsupported audio format '.xyz'"`**
- Only `.mp3`, `.ogg`, `.wav`, `.flac` are supported.
- Fix: convert the file to a supported format.

**`"type must be 'music', 'sound', or a valid integer type ID"`**
- The `type` parameter was something unexpected.
- Fix: use `"music"` or `"sound"`.

---

## Event Binding Errors

**Event bound but handler never fires at runtime**
1. Verify with `event_get_handlers` that the binding is actually set.
2. Verify with `script_function_exists` that the function is in the script.
3. **Scripts haven't been compiled.** The MCP writes source files, but the engine
   runs compiled bytecode. Open the AGS Editor → Build → Compile scripts.
4. For room/hotspot events: the `.crm` may have been overwritten if the AGS Editor
   had the room open. Close the room in the Editor before using MCP room tools.

**`"Unknown event 'on_enter' for hotspot"`**
- Used a room event name on a hotspot, or vice versa.
- Fix: call `list_event_types` to see valid events per entity type.
- Room: `on_enter`, `on_exit`, `on_firstload`, `on_before_fade_in`, `on_after_fade_out`
- Hotspot: `on_interact`, `on_look_at`, `on_use_inv`, `on_walk_on`, `on_talk`, `on_any_click`, `on_mouse_move`

---

## Inventory Errors

**After `inventory_item_rename`, old name still in scripts**
- The rename updates `Game.agf` only. Any AGS Script code using the old name (e.g.,
  `cEgo.AddInventory(iOldKey)`) will fail to compile.
- Fix: use `script_function_update` to replace occurrences of the old name in affected functions.
- The `warnings` array in the rename response lists handlers that need attention.

---

## Build Errors

**`game_build` always says "No rebuild performed"**
- This is expected and by design. The MCP cannot trigger AGS compilation.
- Fix: open the AGS Editor → Build → Build game EXE.

**`game_test_run` returns `launched: false`**
- No compiled exe exists in `Compiled/Windows/`.
- Fix: build the project in the AGS Editor first.

**`build_is_stale: true` from `game_test_run`**
- Source files were modified after the last build. Game still launches, but changes
  won't be in effect.
- Fix: rebuild in the AGS Editor, then run again.

**`game_export` returns `exported: false`**
- No compiled output in `Compiled/Windows/`.
- Fix: build first, then export.

---

## General Errors

**`"project_path is required"`**
- Every single tool requires `project_path`. No exceptions.

**`"'X' is not a valid AGS identifier"`**
- Identifiers must start with a letter or underscore, contain only letters/digits/underscore.
- `cGuard` ✓ — `3guard` ✗ — `guard name` ✗ — `guard-name` ✗

**`declare_variable` says `exists: true` but you don't see the variable**
- The detection regex found the name appearing in a comment, string, or struct elsewhere.
- Fix: open the `.asc` file and search manually for the variable name.

**Character doesn't appear in the room**
- `starting_room` is not set to the room number you're testing.
- Fix: `character_update(project_path, character_id=0, starting_room=1, starting_x=160, starting_y=150)`
- Also ensure the character has a walking view assigned (do this in the AGS Editor).
