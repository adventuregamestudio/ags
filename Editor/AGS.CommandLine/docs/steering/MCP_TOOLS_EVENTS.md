---
inclusion: manual
---
# AGS MCP — Event & Script Tools Reference

These tools connect game entities to script functions and write AGS Script code
directly into `.asc` files on disk.

---

## Event Names Reference

Call `list_event_types` at any time to get this table from the server.

| Entity | Event name | When it fires |
|--------|-----------|---------------|
| **character** | `on_interact` | Player clicks "use/interact" on character |
| character | `on_look_at` | Player clicks "look" on character |
| character | `on_talk_to` | Player clicks "talk" on character |
| character | `on_use_inv` | Player uses inventory item on character |
| character | `on_pickup` | Player picks up character (rare) |
| character | `on_any_click` | Any click on character |
| character | `on_usermode1` / `on_usermode2` | Custom cursor modes |
| **inventory** | `on_interact` | Player clicks "use" on item in inventory |
| inventory | `on_look_at` | Player looks at item |
| inventory | `on_use_inv` | Player uses another item on this item |
| inventory | `on_other_click` | Any other click |
| **hotspot** | `on_interact` | Player interacts with hotspot |
| hotspot | `on_look_at` | Player looks at hotspot |
| hotspot | `on_use_inv` | Player uses inventory item on hotspot |
| hotspot | `on_walk_on` | Player walks onto hotspot area |
| hotspot | `on_talk` | Player talks to hotspot |
| hotspot | `on_any_click` | Any click on hotspot |
| hotspot | `on_mouse_move` | Mouse moves over hotspot |
| **room** | `on_enter` | Every time player enters the room |
| room | `on_firstload` | Only the very first time room loads |
| room | `on_exit` | Player leaves (before fade) |
| room | `on_before_fade_in` | After fade-in completes |
| room | `on_after_fade_out` | Room is being unloaded |

---

## `list_event_types`

No parameters required (not even `project_path`).
Returns the full event name table above as JSON.

---

## `event_get_handlers`

Get all event bindings currently set on an entity.

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `entity_type` | string | yes | `character`, `inventory`, `room`, or `hotspot` |
| `entity_id` | int | yes | Character ID, inventory ID, room number, or room number (for hotspot) |
| `room_id` | int | conditional | Required when `entity_type` is `hotspot` |
| `hotspot_id` | int | no | If given, filters to that single hotspot; else returns all in room |

Returns `handlers` array. Each entry: `event`, `suffix`, `script_method`, `script_module`.

---

## `event_bind_character`

| Param | Type | Required |
|-------|------|----------|
| `project_path` | string | yes |
| `character_id` | int | yes |
| `event` | string | yes | See character events above |
| `script_method` | string | yes | Function name to call |
| `script_module` | string | no | Which script (defaults to current binding) |

Returns: `{ "success": true, "message": "Bound character.on_interact → GlobalScript.cGuard_Interact" }`

---

## `event_bind_inventory`

Same shape as `event_bind_character` but uses `inventory_id` and inventory events.
Valid events: `on_interact`, `on_look_at`, `on_use_inv`, `on_other_click`

---

## `event_bind_room`

| Param | Type | Required |
|-------|------|----------|
| `project_path` | string | yes |
| `room_id` | int | yes |
| `event` | string | yes | See room events above |
| `script_method` | string | yes | |
| `script_module` | string | no | |

Writes the `.crm` binary. **Room must have an existing `.crm`** (created by `room_add`).

---

## `event_bind_hotspot`

| Param | Type | Required |
|-------|------|----------|
| `project_path` | string | yes |
| `room_id` | int | yes |
| `hotspot_id` | int | yes | Slot number 1–49 |
| `event` | string | yes | See hotspot events above |
| `script_method` | string | yes | |
| `script_module` | string | no | Defaults to `roomN` (the room's script) |

Writes the `.crm` binary.

---

## `script_function_exists`

| Param | Type | Required |
|-------|------|----------|
| `project_path` | string | yes |
| `script_module` | string | yes | `GlobalScript`, custom module name, or `room1`, `room2` etc. |
| `function_name` | string | yes | |

Returns: `{ "success": true, "exists": true/false }`

Room scripts are resolved by looking for `roomN.asc` on disk.
Accepted formats: `room1`, `Room1`, `room1.asc`, `Room1.asc`

---

## `script_function_add`

Appends a new function to the end of a `.asc` file.

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `script_module` | string | yes | |
| `function_name` | string | yes | |
| `return_type` | string | no | Default: `void` |
| `parameters` | string | no | e.g. `"int option"`. Default: empty |
| `body` | string | no | Content inside braces. Use `\n` for newlines |

Returns: `{ "function_location": { "script_module": "...", "file_path": "...", "line_number": 42 } }`
**Fails if function already exists.** Always check with `script_function_exists` first.

---

## `script_function_update`

Replaces the body of an existing function using brace-aware parsing.

| Param | Type | Required |
|-------|------|----------|
| `project_path` | string | yes |
| `script_module` | string | yes |
| `function_name` | string | yes | Must already exist |
| `body` | string | yes | New body content (without outer braces) |

**Fails if function does not exist** — use `script_function_add` first.
Fails if function has unmatched braces (corrupted source).

---

## `declare_variable`

Declares a global variable at the top of a script file.
**Idempotent** — if variable already exists (by name), returns `exists: true` without
adding a duplicate.

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `script_module` | string | yes | |
| `var_type` | string | yes | `int`, `bool`, `String`, `float`, etc. |
| `var_name` | string | yes | |
| `default_value` | string | no | e.g. `"0"`, `"false"` |

Returns: `{ "exists": false, "message": "Variable 'int doorOpened' declared in GlobalScript" }`

---

## Standard Event Handler Naming Convention

Following AGS convention keeps scripts readable and consistent with the Editor:

| Entity + event | Recommended function name |
|---------------|--------------------------|
| `cGuard` → `on_interact` | `cGuard_Interact` |
| `cGuard` → `on_look_at` | `cGuard_Look` |
| `cGuard` → `on_talk_to` | `cGuard_Talk` |
| `hDoor` → `on_interact` | `hDoor_Interact` |
| `hDoor` → `on_look_at` | `hDoor_Look` |
| `hDoor` → `on_use_inv` | `hDoor_UseInv` |
| `iKey` → `on_interact` | `iKey_Look` |
| Room → `on_enter` | `room1_Load` |
| Room → `on_firstload` | `room1_FirstLoad` |

---

## Add + Bind Pattern (always do this order)

```
1. script_function_exists  →  does it exist?
2a. if no:  script_function_add
2b. if yes: script_function_update  (if body needs changing)
3. event_bind_*             →  wire it to the entity
```

Binding an event to a function that doesn't exist in the script will cause a compile
error when the AGS Editor builds the project.
