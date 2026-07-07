---
inclusion: manual
---
# AGS MCP — Read Tools Reference

All read tools are non-destructive. They load `Game.agf` on every call (no caching).
Room tools that read `.crm` data require the compiled room binary to exist.

---

## `project_info`
**Params:** `project_path`
**Returns:** `game_name`, `resolution`, `color_depth`, `character_count`, `room_count`,
`dialog_count`, `view_count`, `gui_count`, `script_count`, `inventory_count`

---

## `character_list` / `character_get`

`character_list` — params: `project_path`, `include_details` (bool, optional)
- Without details: `[{"id": 0, "name": "cEgo"}, ...]`
- With details: adds `real_name`, `view`, `speech_view`, `idle_view`, `idle_delay`,
  `speech_color`, `starting_room`, `starting_x`, `starting_y`, `is_player`

`character_get` — params: `project_path`, `character_id` (int)
- Returns: `{ "character": { ...all fields... } }`

---

## `room_list` / `room_get`

`room_list` — params: `project_path`, `include_details` (bool, optional)
- Without details: `[{"id": 1, "name": "Forest"}, ...]`
- With details: adds `script_file`, `crm_file`

`room_get` — params: `project_path`, `room_id` (int)
- **When `.crm` exists:** returns `id`, `name`, `width`, `height`, `color_depth`,
  `hotspot_count`, `object_count`, `background_count`, `script_file`, `has_script`, `crm_file`
- **When `.crm` missing:** returns only `id`, `name`, `script_file`, `crm_file`

---

## `room_hotspots`

Params: `project_path`, `room_id` (int) — requires `.crm`

Filters out slot 0 and auto-generated default names (`hHotspot1`, etc.).
Returns: `{ "room_id": 1, "count": 2, "hotspots": [{"id": 1, "name": "hDoor", "description": "...", "walk_to_x": 150, "walk_to_y": 180}] }`

If `.crm` missing: returns `count: 0` with a `note` field.

---

## `room_objects`

Params: `project_path`, `room_id` (int) — requires `.crm`

Each object: `id`, `name`, `description`, `sprite`, `x`, `y`, `baseline`,
`baseline_overridden`, `visible`, `clickable`

---

## `room_walkable_areas`

Params: `project_path`, `room_id` (int) — requires `.crm`

Each area: `id`, `scaling_level`, `min_scaling`, `max_scaling`,
`area_specific_view`, `use_continuous_scaling`

---

## `room_regions`

Params: `project_path`, `room_id` (int) — requires `.crm`

Each region: `id`, `light_level`, `use_colour_tint`, `red_tint`, `green_tint`,
`blue_tint`, `tint_saturation`, `tint_luminance`

---

## `dialog_list` / `dialog_get`

`dialog_list` — params: `project_path`, `include_details` (bool)
- With details: adds `option_count`, `show_text_parser`

`dialog_get` — params: `project_path`, `dialog_id` (int)
- Returns dialog + all options. Each option: `id`, `text`, `show`, `say`

---

## `inventory_list` / `inventory_get`

`inventory_list` — params: `project_path`, `include_details` (bool)
- With details: adds `description`, `sprite`, `cursor_sprite`

`inventory_get` — params: `project_path`, `inventory_id` (int)
- Full item: adds `player_starts_with`, `hotspot_x`, `hotspot_y`

---

## `gui_list` / `gui_get`

`gui_list` — params: `project_path`, `include_details` (bool)
- With details: adds `x`, `y`, `width`, `height`, `control_count`, `type`

`gui_get` — params: `project_path`, `gui_id` (int)
- Full GUI with controls array. Each control: `id`, `name`, `type`, `x`, `y`, `width`, `height`

---

## `view_list` / `view_get`

`view_list` — params: `project_path`, `include_details` (bool)
- With details: adds `loop_count`, `frame_count`

`view_get` — params: `project_path`, `view_id` (int)
- Full view with loops and frames.
- Loop fields: `id`, `direction`, `frame_count`, `run_next`, `frames`
- Frame fields: `id`, `sprite`, `flipped`, `delay`

---

## `audio_list` / `audio_get`

`audio_list` — params: `project_path`, `include_details` (bool)
- With details: adds `file_type`, `type_id`, `source_file`

`audio_get` — params: `project_path`, `audio_id` (int)
- Full clip: adds `bundling_type`, `default_volume`, `default_repeat`

---

## `script_list` / `script_get`

`script_list` — params: `project_path`, `include_details` (bool)
- Returns: `[{"id": 0, "name": "GlobalScript", "header": "GlobalScript.ash", "script": "GlobalScript.asc"}, ...]`
- With details: adds `line_count`

`script_get` — params: `project_path`, `script_id` (int)
- Returns full script source content
