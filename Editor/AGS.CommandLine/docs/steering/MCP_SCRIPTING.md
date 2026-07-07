---
inclusion: manual
---
# AGS MCP — Script Writing & Dialogue Patterns

---

## AGS Script Essentials

AGS Script is similar to C. Key syntax facts:

```c
// Types
int count = 0;
bool done = false;
float speed = 2.5;
String name = "hero";   // String (capital S) is AGS built-in

// Character methods
cEgo.Say("Hello!");                     // Say (blocking)
cEgo.SayBackground("Hi");              // Non-blocking
cEgo.Walk(100, 150, eBlock);           // Walk and wait
cEgo.Walk(100, 150, eNoBlock);         // Walk, continue script immediately
cEgo.FaceCharacter(cGuard);
cEgo.FaceLocation(200, 100, eBlock);
cEgo.ChangeRoom(2);                    // Go to room 2
cEgo.ChangeRoom(2, 100, 150);          // Go to room 2 at position
cEgo.AddInventory(iKey);
cEgo.LoseInventory(iKey);
bool has = cEgo.HasInventory(iKey);    // Returns bool

// Game flow
Wait(40);                   // Wait 40 game cycles (~1 second at 40fps)
Display("Message.");        // Show text popup
GiveScore(10);
int v = GetGlobalInt(1);    // 50 global int slots (0–49)
SetGlobalInt(1, 42);

// Room navigation
cEgo.ChangeRoom(2, 160, 150);
```

---

## How `body` Maps to the File

`script_function_add` and `script_function_update` take a `body` string containing
everything that goes **inside** the braces. Use `\n` for newlines. The tool adds
4-space indentation automatically.

Input body:
```
"  if (done) {\n    cEgo.Say(\"All done!\");\n  }"
```

Result in the `.asc` file:
```c
void MyFunction()
{
    if (done) {
        cEgo.Say("All done!");
    }
}
```

**Escaping quotes in bodies:** Use `\"` inside string literals in the body parameter.

---

## Global vs Room Script — When to Use Each

| Situation | Use |
|-----------|-----|
| Character talk/look/interact handlers | **GlobalScript** — characters roam between rooms |
| Inventory item handlers | **GlobalScript** |
| Puzzle state variables | **GlobalScript** — must persist across rooms |
| Utility functions used in multiple rooms | **GlobalScript** |
| Hotspot handlers (room-specific) | **Room script** (`room1.asc`) |
| Room enter/exit logic | **Room script** |
| Variables only needed in one room | **Room script** |

GlobalScript is always imported globally — its functions and variables are visible
everywhere. Room scripts are local to that room.

---

## Dialogue Scripting

### How AGS Dialogs Work

1. `dMyDialog.Start()` is called → AGS shows the options panel
2. Player picks an option → AGS calls the dialog's script handler
3. Within the handler, you call character Say lines and set option states
4. Call `StopDialog()` to end, or let it return to the options panel

### Setting Up Dialog Logic via MCP

Since the MCP cannot write dialog script (the special mini-language in the AGS Editor's
dialog tab), use a regular script function called from a character's talk handler:

```
# The talk handler starts the dialog
script_function_add(project_path, script_module="GlobalScript",
  function_name="cMerchant_Talk",
  body="
  // Conditionally show options before starting
  if (cEgo.HasInventory(iGem)) {
    dMerchantChat.SetOptionState(3, eOptionOn);
  }
  dMerchantChat.Start();")

event_bind_character(project_path, character_id=2, event="on_talk_to",
  script_method="cMerchant_Talk")
```

### Dialog Option State API

```c
// Option states
dMyDialog.SetOptionState(1, eOptionOn);           // Show option 1
dMyDialog.SetOptionState(2, eOptionOff);          // Hide option 2
dMyDialog.SetOptionState(3, eOptionOffForever);   // Permanently disable

// Check state
DialogOptionState s = dMyDialog.GetOptionState(1);
// s == eOptionOn / eOptionOff / eOptionOffForever

// Get text
String t = dMyDialog.GetOptionText(2);
```

### Pattern: One-Shot Options (hide after use)

```
script_function_add(project_path, script_module="GlobalScript",
  function_name="AskAboutSword",
  body="
  cEgo.Say(\"What about that sword?\");
  cBlacksmith.Say(\"Finest steel. Not for sale.\");
  dBlacksmithChat.SetOptionState(2, eOptionOff);
  ")
```

### Pattern: Conditional Options

```
script_function_add(project_path, script_module="GlobalScript",
  function_name="cMerchant_Talk",
  body="
  if (GetGlobalInt(5) > 0) {
    dMerchantChat.SetOptionState(4, eOptionOn);   // 'I have gold'
  } else {
    dMerchantChat.SetOptionState(4, eOptionOff);
  }
  dMerchantChat.Start();
  ")
```

### Pattern: Multi-Step Conversation Sequence

```
script_function_add(project_path, script_module="GlobalScript",
  function_name="RunIntroConversation",
  body="
  cGuard.Say(\"Halt! Who goes there?\");
  cEgo.Say(\"Just a traveller.\");
  Wait(20);
  cGuard.Say(\"Move along then.\");
  cEgo.Walk(200, 150, eBlock);
  SetGlobalInt(1, 1);  // flag: met the guard
  ")
```

---

## Puzzle State Patterns

### Boolean flag (has something happened?)

```
declare_variable(project_path, script_module="GlobalScript", var_type="int", var_name="metKing", default_value="0")

# In script: if (metKing) { ... } else { ... }
# Set with: metKing = 1;
```

### Counter (how many times has something happened?)

```
declare_variable(project_path, script_module="GlobalScript", var_type="int", var_name="timesClicked")

# In script: timesClicked++; if (timesClicked >= 3) { ... }
```

### Using `GetGlobalInt` / `SetGlobalInt`

AGS provides 50 global int slots (0–49) that are automatically saved/loaded with the game.
Good for simple persistent flags without declaring variables.

```c
SetGlobalInt(0, 1);    // Set slot 0 = 1
if (GetGlobalInt(0) == 1) { ... }
```

---

## Common Mistakes to Avoid

- **Wrong:** `printf("hello");` → **Right:** `Display("hello");`
- **Wrong:** `string name = "x";` → **Right:** `String name = "x";` (capital S)
- **Wrong:** putting character handlers in room scripts — they won't fire when the character
  is in a different room than expected
- **Wrong:** forgetting `eBlock` when you need walk-then-talk sequences
  (`cEgo.Walk(x, y, eBlock)` waits; without it the next line runs immediately)
- **Wrong:** calling `script_function_add` for a function that already exists
  → use `script_function_exists` first, then `update` if needed
