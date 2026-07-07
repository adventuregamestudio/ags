---
inclusion: manual
---
# AGS MCP — Write Tools Reference

All write tools load `Game.agf`, modify it, and save atomically. Room tools that touch
hotspots or objects also load and save the `.crm` binary.

All return `{ "success": true, ... }` on success or `{ "success": false, "error": "..." }`.

---

## `character_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `name` | string | yes | Script name e.g. `cGuard` — must be valid identifier |
| `real_name` | string | no | Display name (defaults to `name`) |
| `view` | int | no | Normal walk view ID |
| `speech_view` | int | no | Talk view ID |
| `talking_color` | int | no | Speech color index |
| `starting_room` | int | no | Room number to start in |
| `starting_x` / `starting_y` | int | no | Starting position |

Returns: `{ "character_id": 2, "message": "..." }`
Fails if: name is invalid identifier or already exists.

---

## `character_update`

Params: `project_path`, `character_id` (required), then any of:
`real_name`, `view`, `speech_view`, `idle_view`, `idle_time`, `talking_color`,
`starting_room`, `starting_x`, `starting_y`

---

## `room_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `name` | string | yes | Room display name |

Returns: `{ "room_id": 3, "message": "Room 'Kitchen' created as room3" }`

Creates: entry in `Game.agf`, `room3.asc` (blank script), `room3.crm` (blank binary via AGS.Native).
Room number is auto-assigned (first unused integer ≥ 1).

---

## `room_update`

Params: `project_path`, `room_id` (int), `name` (string)
Updates the room's display name in `Game.agf`.

---

## `room_hotspot_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `room_id` | int | yes | |
| `name` | string | yes | e.g. `hDoor` — valid identifier |
| `description` | string | no | Text shown on hover |
| `walk_to_x` / `walk_to_y` | int | no | Walk-to point before interacting |

Returns: `{ "hotspot_id": 1, "message": "..." }`
Fails if: `.crm` missing, name is invalid, all 49 slots taken, name already used.

---

## `room_hotspot_update`

Params: `project_path`, `room_id`, `hotspot_id`, then any of: `name`, `description`,
`walk_to_x`, `walk_to_y`

---

## `room_object_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `room_id` | int | yes | |
| `name` | string | yes | e.g. `oLamp` |
| `description` | string | no | |
| `sprite` | int | no | Sprite number |
| `x` / `y` | int | no | Position |

---

## `room_object_update`

Params: `project_path`, `room_id`, `object_id`, then any of: `name`, `description`,
`sprite`, `x`, `y`, `baseline` (int), `visible` (bool), `clickable` (bool)

---

## `dialog_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `name` | string | yes | e.g. `dGuardTalk` |
| `show_always` | bool | no | Show text parser (default: false) |

Returns: `{ "dialog_id": 2, "message": "..." }`

---

## `dialog_option_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `dialog_id` | int | yes | |
| `text` | string | yes | Option text (max 256 chars) |
| `show_always` | bool | no | Always visible? (default: true) |
| `say_as_character` | bool | no | Player character says it aloud? (default: true) |

Returns: `{ "option_index": 1, "message": "..." }` — option IDs are 1-based.

---

## `inventory_item_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `name` | string | yes | e.g. `iKey` |
| `description` | string | no | Display text |
| `sprite` | int | no | Inventory display sprite |
| `cursor_sprite` | int | no | Cursor sprite when item is active |

Returns: `{ "inventory_id": 3, "message": "..." }` — IDs are 1-based.

---

## `inventory_item_update`

Params: `project_path`, `inventory_id`, then any of: `description`, `sprite`, `cursor_sprite`

---

## `inventory_item_rename`

Params: `project_path`, `inventory_id`, `name` (new identifier)

Returns `old_name`, `new_name`, and `warnings` array listing any event handlers that
referenced the old name — those need manual script updates.

---

## `audio_add`

| Param | Type | Required | Notes |
|-------|------|----------|-------|
| `project_path` | string | yes | |
| `file_path` | string | yes | Absolute path to source file |
| `name` | string | no | Script name e.g. `aTheme`. Auto-generated from filename if omitted |
| `type` | string | no | `"music"` (type 2) or `"sound"` (type 3, default) |

Supported formats: `.mp3`, `.ogg`, `.wav`, `.flac`

Auto-name rule: `"Palmtree Panic.mp3"` → `aPalmtreePanic` (prefix `a` + PascalCase stem,
invalid chars replaced with `_`). Always provide `name` explicitly if the filename has
numbers or special characters.

Returns: `{ "audio_id": 0, "script_name": "aPalmtreePanic", "cache_file": "au1.mp3", ... }`

Copies file to `AudioCache/auN.ext` in the project folder.

---

## Naming Rules (apply to all write tools)

Valid identifier: letters, digits, underscore; must start with letter or underscore.
`cGuard` ✓ — `3guard` ✗ — `guard name` ✗

Convention (not enforced):
- Characters: `c` prefix → `cHero`
- Inventory: `i` prefix → `iKey`
- Dialogs: `d` prefix → `dGuardTalk`
- Hotspots: `h` prefix → `hDoor`
- Objects: `o` prefix → `oLamp`
- Audio: `a` prefix → `aTheme`
