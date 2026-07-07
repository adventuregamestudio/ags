---
inclusion: manual
---
# AGS MCP — Game Development Workflows

Step-by-step recipes for common tasks. Each workflow lists the exact tools in order.

---

## Workflow A: Minimal Game Skeleton

Use this to bootstrap a playable game from a blank project.

```
1. project_info(project_path)
   → confirm starting state

2. character_add(project_path, name="cEgo", real_name="Hero", starting_room=1, starting_x=160, starting_y=150)
   → returns character_id

3. room_add(project_path, name="Starting Room")
   → returns room_id=1 (or next available)
   → creates room1.crm + room1.asc automatically

4. room_hotspot_add(project_path, room_id=1, name="hExit", description="The way out", walk_to_x=300, walk_to_y=180)
   → returns hotspot_id=1

5. script_function_exists(project_path, script_module="room1", function_name="room1_Load")
   → if false:
   script_function_add(project_path, script_module="room1", function_name="room1_Load",
     body="  // Room setup code here")

6. event_bind_room(project_path, room_id=1, event="on_enter", script_method="room1_Load", script_module="room1")

7. script_function_add(project_path, script_module="room1", function_name="hExit_Interact",
     body="  cEgo.Say(\"Time to move on.\");")

8. event_bind_hotspot(project_path, room_id=1, hotspot_id=1, event="on_interact",
     script_method="hExit_Interact")

9. game_build_check(project_path)
   → fix any issues, then open AGS Editor to compile
```

---

## Workflow B: Adding More Rooms and Connections

```
# Create second room
room_add(project_path, name="Dungeon")
→ returns room_id=2

# Add entry hotspot in room 1 that goes to room 2
room_hotspot_add(project_path, room_id=1, name="hDoorToDungeon", walk_to_x=280, walk_to_y=180)
script_function_add(project_path, script_module="room1", function_name="hDoorToDungeon_WalkOn",
  body="  cEgo.ChangeRoom(2, 40, 150);")
event_bind_hotspot(project_path, room_id=1, hotspot_id=2, event="on_walk_on",
  script_method="hDoorToDungeon_WalkOn")

# Add a return hotspot in room 2
room_hotspot_add(project_path, room_id=2, name="hStairsUp", walk_to_x=40, walk_to_y=150)
script_function_add(project_path, script_module="room2", function_name="hStairsUp_Interact",
  body="  cEgo.ChangeRoom(1, 280, 150);")
event_bind_hotspot(project_path, room_id=2, hotspot_id=1, event="on_interact",
  script_method="hStairsUp_Interact")
```

**Key AGS Script:** `cEgo.ChangeRoom(roomNumber, x, y)` — moves character to room at position.

---

## Workflow C: Character with Dialogue

```
# 1. Add the NPC
character_add(project_path, name="cGuard", real_name="Guard", starting_room=1, starting_x=220, starting_y=150)

# 2. Create the dialog
dialog_add(project_path, name="dGuardChat")
→ returns dialog_id=0

# 3. Add options (option IDs are 1-based)
dialog_option_add(project_path, dialog_id=0, text="Who are you?")
dialog_option_add(project_path, dialog_id=0, text="Let me through.")
dialog_option_add(project_path, dialog_id=0, text="Never mind.", say_as_character=false)

# 4. Write the talk handler in GlobalScript (characters move between rooms)
script_function_add(project_path, script_module="GlobalScript",
  function_name="cGuard_Talk",
  body="  dGuardChat.Start();")

event_bind_character(project_path, character_id=1, event="on_talk_to",
  script_method="cGuard_Talk", script_module="GlobalScript")

# 5. Write a look handler
script_function_add(project_path, script_module="GlobalScript",
  function_name="cGuard_Look",
  body="  cEgo.Say(\"A stern-looking guard.\");")

event_bind_character(project_path, character_id=1, event="on_look_at",
  script_method="cGuard_Look", script_module="GlobalScript")
```

For dialog scripting logic (option responses), see `MCP_SCRIPTING.md`.

---

## Workflow D: Inventory Puzzle (Use Item on Hotspot)

```
# 1. Add the item
inventory_item_add(project_path, name="iKey", description="A rusty key")

# 2. Declare puzzle state
declare_variable(project_path, script_module="GlobalScript", var_type="int", var_name="doorUnlocked", default_value="0")

# 3. Hotspot look handler
script_function_add(project_path, script_module="room1", function_name="hDoor_Look",
  body="  cEgo.Say(\"A locked door.\");")
event_bind_hotspot(project_path, room_id=1, hotspot_id=1, event="on_look_at",
  script_method="hDoor_Look")

# 4. Hotspot interact handler
script_function_add(project_path, script_module="room1", function_name="hDoor_Interact",
  body="
  if (doorUnlocked) {
    cEgo.ChangeRoom(2, 160, 150);
  } else {
    cEgo.Say(\"It's locked.\");
  }")
event_bind_hotspot(project_path, room_id=1, hotspot_id=1, event="on_interact",
  script_method="hDoor_Interact")

# 5. Use inventory item on hotspot
script_function_add(project_path, script_module="room1", function_name="hDoor_UseInv",
  body="
  if (cEgo.ActiveInventory == iKey) {
    doorUnlocked = 1;
    cEgo.LoseInventory(iKey);
    cEgo.Say(\"The door clicks open.\");
  } else {
    cEgo.Say(\"That doesn't work here.\");
  }")
event_bind_hotspot(project_path, room_id=1, hotspot_id=1, event="on_use_inv",
  script_method="hDoor_UseInv")

# 6. Give player the key (add a pickup somewhere — e.g., room object)
room_object_add(project_path, room_id=1, name="oKeyOnTable", description="A key on the table", x=80, y=140)
script_function_add(project_path, script_module="room1", function_name="oKeyOnTable_Interact",
  body="
  if (!cEgo.HasInventory(iKey)) {
    cEgo.AddInventory(iKey);
    cEgo.Say(\"I pick up the key.\");
    oKeyOnTable.Visible = false;
  }")
event_bind_hotspot(project_path, room_id=1, hotspot_id=2, event="on_interact",
  script_method="oKeyOnTable_Interact")
```

---

## Workflow E: Add Background Music

```
# 1. Import the audio file
audio_add(project_path,
  file_path="C:\\music\\theme.mp3",
  name="aTheme",
  type="music")

# 2. Play it when the first room loads
script_function_update(project_path, script_module="room1",
  function_name="room1_Load",
  body="  aTheme.Play(eAudioPriorityNormal, eRepeat);")
```

---

## Workflow F: Validate and Export

```
# Check everything is in order
game_build_check(project_path)
→ if can_build: true → open AGS Editor and compile
→ if can_build: false → fix reported errors first

# After AGS Editor build:
game_build(project_path)
→ confirm built: true

game_test_run(project_path, windowed_mode=true)
→ confirm launched: true, check build_is_stale

# Package for distribution
game_export(project_path, export_dir="C:\\exports\\mygame", version="1.0.0")
→ returns zip path
```
