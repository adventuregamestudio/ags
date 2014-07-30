Reference
#########

This section contains a reference for various parts of the system except the scripting
language, which has its own separate Scripting section.

Event Types
===========

The following events are available in the "Events" section of the Properties Window
(when clicking the lightning bolt icon).

Hotspot events
--------------


* *Player stands on hotspot* -- Occurs repeatedly while the player character
  is standing on the hotspot.
* *Look at hotspot* -- Occurs when the player clicks on the hotspot while in
  the "Look" mode (cursor mode 1).
* *Interact with hotspot* -- Occurs when the player clicks on the hotspot while
  in the "Interact" mode (cursor mode 2).
* *Use inventory on hotspot* -- Occurs when the player clicks on the hotspot
  while in the "Use inventory" mode (cursor mode 4). You can use the
  :ref:`player.ActiveInventory <Character.ActiveInventory>` property to distinguish
  which item they used.


* *Speak to hotspot* -- Occurs when the player clicks on the hotspot while in
  the "Talk" mode (cursor mode 3).
* *Any click on hotspot* -- Occurs when the player clicks on the hotspot in
  any cursor mode (except Walk). This allows you to add extra modes like
  smell, taste, push, pull, and so on. This event also occurs as well as
  the other event for the Look, Interact and Talk modes.
* *Mouse moves over hotspot* -- Occurs repeatedly while the mouse cursor is
  over the hotspot. You can use this to highlight the cursor, and for other
  various effects.


Object events
-------------


* *Look at object* -- Occurs when the player clicks on the object while in
  the "Look" mode (cursor mode 1).
* *Interact with object* -- Occurs when the player clicks on the object in
  the "Interact" mode (cursor mode 2).
* *Speak to object* -- Occurs when the player clicks on the object in the
  "Talk" mode (cursor mode 3).
* *Use inventory on object* -- Works like "Use inventory on hotspot" - see
  that description (above) for more information.


Room events
-----------


* *Walk off left* -- Occurs when the player character walks off the left edge
  of the screen.
* *Walk off right* -- Occurs when the player walks off the right edge of the
  screen.
* *Walk off bottom* -- Occurs when the player character walks off the bottom
  edge of the screen.
* *Walk off top* -- Occurs when the player character walks off the top edge
  of the screen.
* *First time enters room* -- Occurs the first time the player enters the
  room. This event occurs AFTER the screen has faded in, so it allows you to
  display a message describing the scene.
* *Player enters room (before fadein)* -- Occurs just after the room is loaded into memory.
  This event occurs every time the player enters the screen, and it happens
  BEFORE the screen has faded in, which allows you to change object graphics
  and do other things to the screen which the player won't notice.

  *NOTE:* This event is ONLY meant for adjusting things such as object and
  character placement. Do NOT use this event for any sort of automated intro
  to the room - use the "Enters Room After Fade In" event for that instead.
* *Repeatedly execute* -- Occurs repeatedly on every interpreter cycle. The
  normal game speed is 40 cycles per second, so this event occurs about
  every 25 milliseconds.
* *Player enters room (after fadein)* -- Occurs every time the player enters the
  room, AFTER the screen has faded-in. Suitable for displaying text
  descriptions and so on, that you want the player to see.
* *Player leaves room* -- Occurs when the player leaves the screen, just
  before the screen fades out.


Inventory item events
---------------------


* *Look at inventory* -- Occurs when the player clicks on the inventory item
  while in the "look" mode.
* *Interact with inventory* -- Currently, because the Interact mode selects the
  inventory item, this event can only be triggered by manually calling
  the InventoryItem.RunInteraction script function (ie. you have to use the Handle
  Inv Clicks in Script option).
* *Speak to inventory* -- Only applies to the Lucasarts-style inventory,
  occurs when the player clicks the Talk icon on the inventory item.
* *Use inventory on inv* -- Occurs when the player uses another inventory
  object on this one. You can use the :ref:`player.ActiveInventory <Character.ActiveInventory>`
  property to distinguish which item they used.

  This event allows the player to combine items, and so on. For example, if they had
  picked up a laptop computer and a battery separately, then you could use this to
  allow them to insert the battery into the computer.
* *Other click on inventory* -- Only applies to the Lucasarts-style inventory,
  occurs when the player clicks any other cursor mode (apart from look, talk
  and use_inv) on the item.


Character events
----------------


* *Look at character* -- Occurs when the player clicks on a character while in the "look" mode.
* *Interact with character* -- Occurs when the player clicks on a character while in the "interact" mode.
* *Speak to character* -- Occurs when the player clicks on a character while in the "talk" mode.
* *Use inventory on character* -- Occurs when the player uses an inventory
  object on a character.  This event could be used to allow the player to give items to characters.
* *Any click on character* -- Occurs when the player clicks any other cursor mode
  (apart from look, talk and use_inv) on the character.


Region events
-------------


* *While player stands on region* -- Occurs repeatedly while the player character stands on this region.
* *Player walks onto region* -- Occurs when the player moves from another region onto this one. Will
  also activate on whichever region they start on when they enter the screen.
* *Player walks off region* -- Occurs when the player leaves the current region. Does not occur
  if they go to a different room.


System limits
=============

This section tells you the maximums for various parts of the system. If you
have been wondering "How many rooms can I have?" or something similar,
chances are this section will answer it.

There are maximum...

=========  ==========================================
       40  objects per room
=========  ==========================================
      299  state-saving rooms per game
      300  inventory items
    30000  imported sprites
       30  controls on each GUI
      500  dialog topics
       30  options per dialog topic
       20  screen overlays at a time
        5  background frames per room
       20  mouse cursors
        8  audio channels
      100  local messages per room (excluding script)
       30  fonts
unlimited  words in the text parser dictionary
unlimited  characters
unlimited  views
unlimited  GUIs
unlimited  loops per view
unlimited  frames per loop
=========  ==========================================


If you think any of these limits is a serious problem, contact me and I can
probably increase it.


.. _KeyboardShortcuts:

Keyboard Shortcuts
==================

The AGS Editor provides various keyboard shortcuts to help you get your work done quickly.
These are summarized below:


==============  ========================
F1              Help
==============  ========================
F2              Game Statistics
F3              Find Next
Ctrl+F4         lose tab
F5              Run with Debugger
Ctrl+F5         un without Debugger
F7              Build EXE
F9              Toggle Breakpoint
F11             Step Into

Ctrl+A          Select All
Ctrl+B          Match Brace
Ctrl+C          Copy
Ctrl+D          Duplicate line to next
Ctrl+E          Replace
Ctrl+F          Find
Ctrl+G          Open GlobalScript.asc
Ctrl+H          Open GlobalScript.ash
Ctrl+L          Open Game
Ctrl+Q          Quit
Ctrl+R          Save Room
Ctrl+S          Save Game
Ctrl+V          Paste
Ctrl+W          Close Tab
Ctrl+X          Cut
Ctrl+Y          Redo
Ctrl+Z          Undo

Ctrl+Space      Show Autocomplete
Ctrl+Tab        Next tab
Ctrl+Shift+Tab  Previous tab

Tab             Indent selected lines
Shift+Tab       Un-indent selected lines
==============  ========================


.. _ASCIIcodes:

ASCII code table
================

This section lists the key codes which can be passed to  on_key_press  and
which keys they represent:

====================  ==============  ==============
**AGS KeyCode**       **Key**         **ASCII code**
====================  ==============  ==============
eKeyNone              ``none``                     0
eKeyCtrlA             ``Ctrl+A``                   1
eKeyCtrlB             ``Ctrl+B``                   2
eKeyCtrlC             ``Ctrl+C``                   3
eKeyCtrlD             ``Ctrl+D``                   4
eKeyCtrlE             ``Ctrl+E``                   5
eKeyCtrlF             ``Ctrl+F``                   6
eKeyCtrlG             ``Ctrl+G``                   7
eKeyCtrlH             ``Ctrl+H``                   8
eKeyCtrlI             ``Ctrl+I``                   9
eKeyCtrlJ             ``Ctrl+J``                  10
eKeyCtrlK             ``Ctrl+K``                  11
eKeyCtrlL             ``Ctrl+L``                  12
eKeyCtrlM             ``Ctrl+M``                  13
eKeyCtrlN             ``Ctrl+N``                  14
eKeyCtrlO             ``Ctrl+O``                  15
eKeyCtrlP             ``Ctrl+P``                  16
eKeyCtrlQ             ``Ctrl+Q``                  17
eKeyCtrlR             ``Ctrl+R``                  18
eKeyCtrlS             ``Ctrl+S``                  19
eKeyCtrlT             ``Ctrl+T``                  20
eKeyCtrlU             ``Ctrl+U``                  21
eKeyCtrlV             ``Ctrl+V``                  22
eKeyCtrlW             ``Ctrl+W``                  23
eKeyCtrlX             ``Ctrl+X``                  24
eKeyCtrlY             ``Ctrl+Y``                  25
eKeyCtrlZ             ``Ctrl+Z``                  26
eKey0                 ``0``                       48
eKey1                 ``1``                       49
eKey2                 ``2``                       50
eKey3                 ``3``                       51
eKey4                 ``4``                       52
eKey5                 ``5``                       53
eKey6                 ``6``                       54
eKey7                 ``7``                       55
eKey8                 ``8``                       56
eKey9                 ``9``                       57
eKeyA                 ``A``                       65
eKeyB                 ``B``                       66
eKeyC                 ``C``                       67
eKeyD                 ``D``                       68
eKeyE                 ``E``                       69
eKeyF                 ``F``                       70
eKeyG                 ``G``                       71
eKeyH                 ``H``                       72
eKeyI                 ``I``                       73
eKeyJ                 ``J``                       74
eKeyK                 ``K``                       75
eKeyL                 ``L``                       76
eKeyM                 ``M``                       77
eKeyN                 ``N``                       78
eKeyO                 ``O``                       79
eKeyP                 ``P``                       80
eKeyQ                 ``Q``                       81
eKeyR                 ``R``                       82
eKeyS                 ``S``                       83
eKeyT                 ``T``                       84
eKeyU                 ``U``                       85
eKeyV                 ``V``                       86
eKeyW                 ``W``                       87
eKeyX                 ``X``                       88
eKeyY                 ``Y``                       89
eKeyZ                 ``Z``                       90
eKeyAmpersand         ``&``                       38
eKeyAsterisk          ``*``                       42
eKeyAt                ``@``                       64
eKeyBackSlash         ``\``                       92
eKeyBackspace         ``Backspace``                8
eKeyCloseBracket      ``]``                       93
eKeyCloseParenthesis  ``)``                       41
eKeyColon             ``:``                       58
eKeyComma             ``,``                       44
eKeyDelete            ``Delete``                 383
eKeyDollar            ``$``                       36
eKeyDoubleQuote       ``"``                       34
eKeyEquals            ``=``                       61
eKeyEscape            ``ESC``                     27
eKeyExclamationMark   ``!``                       33
eKeyForwardSlash      ``/``                       47
eKeyGreaterThan       ``>``                       62
eKeyHash              ``#``                       35
eKeyHyphen            ``-``                       45
eKeyInsert            ``Insert``                 382
eKeyLessThan          ``<``                       60
eKeyOpenBracket       ``[``                       91
eKeyOpenParenthesis   ``(``                       40
eKeyPercent           ``%``                       37
eKeyPeriod            ``.``                       46
eKeyPlus              ``+``                       43
eKeyQuestionMark      ``?``                       63
eKeyReturn            ``RETURN``                  13
eKeySemiColon         ``;``                       59
eKeySingleQuote       ``'``                       39
eKeySpace             ``SPACE``                   32
eKeyTab               ``TAB``                      9
eKeyUnderscore        ``_``                       95
eKeyF1                ``F1``                     359
eKeyF2                ``F2``                     360
eKeyF3                ``F3``                     361
eKeyF4                ``F4``                     362
eKeyF5                ``F5``                     363
eKeyF6                ``F6``                     364
eKeyF7                ``F7``                     365
eKeyF8                ``F8``                     366
eKeyF9                ``F9``                     367
eKeyF10               ``F10``                    368
eKeyF11               ``F11``                    433
eKeyF12               ``F12``                    434
eKeyHome              ``Home``                   371
eKeyUpArrow           ``UpArrow``                372
eKeyPageUp            ``PageUp``                 373
eKeyLeftArrow         ``LeftArrow``              375
eKeyNumPad5           ``NumPad 5``               376
eKeyRightArrow        ``RightArrow``             377
eKeyEnd               ``End``                    379
eKeyDownArrow         ``DownArrow``              380
eKeyPageDown          ``PageDown``               381
====================  ==============  ==============


Use these key codes in your on_key_press function to process player input. For example::

   if (keycode == eKeyA) Display("You pressed A");
   if (keycode == eKeyPlus) Display("You pressed the Plus key");


The following extra codes can only be used with IsKeyPressed
(ie. on_key_press is never called with these codes):

========  ===========
**Code**  **Key**
========  ===========
403       Left shift
404       Right shift
405       Left ctrl
406       Right ctrl
407       Alt
========  ===========
