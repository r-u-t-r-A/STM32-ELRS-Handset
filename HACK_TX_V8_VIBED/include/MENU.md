# Menu system

This document describes how the OLED menu works on the Pistol TX handset and how to add new menus.

## Architecture overview

The UI has **two layers**:

1. **Top-level screens** — selected with NEXT/PREV while not inside a submenu.
2. **Submenus** — entered with OK on a top-level screen; exited with BACK at the root of that submenu.

Global navigation state lives in `src/main.cpp`:

| Variable | Role |
|----------|------|
| `main_menu_pos` | Current top-level screen (`handset_screen_t` in `handset_menu.h`) |
| `isInLowerLevel` | `true` when a submenu is open |

There are **two menu implementations**:

| Type | Used for | Code |
|------|----------|------|
| **Handset static menus** | Settings stored on the radio (EEPROM, mixers, trim) | `handset_menu.c` |
| **ELRS dynamic menu** | Parameters loaded at runtime from the TX module over CRSF | `crsf_dynamic.c`, `draw_dynamic_menu()` in `src/main.cpp` |

Do **not** put handset-only settings into `crsf_dynamic`. Do **not** hard-code ELRS module parameters into `handset_menu.c`.

```
                    ┌─────────────────────────────────────┐
  NEXT/PREV ───────►│  Top-level screen (main_menu_pos)   │
                    │  Main │ ELRS │ Trim                 │
                    └──────────┬──────────────────────────┘
                               │ OK
                               ▼
                    ┌─────────────────────────────────────┐
                    │  Submenu (isInLowerLevel = true)    │
                    │  • handset_main / handset_trim      │
                    │  • draw_dynamic_menu (ELRS)         │
                    └─────────────────────────────────────┘
```

## Top-level screens

Defined in `handset_menu.h` as `handset_screen_t`:

| ID | Name | Preview (`show_data()`) | OK enters |
|----|------|-------------------------|-----------|
| `SCREEN_MAIN` (0) | Main | Stick values, battery, mixer index | Main settings submenu |
| `SCREEN_ELRS` (1) | ELRS | RSSI, LQ, RX battery | ELRS dynamic param menu (requires `params_loaded`) |
| `SCREEN_TRIM` (2) | Trim | Yaw/throttle fine values | Trim submenu |

Top-level navigation (`process_ui_buttons()`):

- **NEXT** — advance screen; wraps `2 → 0`
- **PREV** — go back one screen; stops at `0` (no wrap)

## Buttons

Physical buttons map to `ENUM_BUTTON` in `src/main.cpp`:

| Button | Value | In list view | In edit mode |
|--------|-------|--------------|--------------|
| BACK | 0 | Exit submenu | Cancel edit |
| NEXT | 1 | Move cursor down / next screen | Increase value |
| PREV | 2 | Move cursor up / prev screen | Decrease value |
| OK | 3 | Enter edit / open folder | Save (handset) or send to module (ELRS) |

`handset_menu.c` uses `MENU_ACT_*` with the **same numeric values** (`BACK=0`, `NEXT=1`, `PREV=2`, `OK=3`). Cast when calling handlers:

```c
handset_main_handle_btn((uint8_t)btn);
```

## Display rules

- OLED: 128×64, font `u8x8_font_5x7_f`, **8 px per character**, **16 characters per row**.
- Text helper: `chop_chars(text, max_chars)` in `oled_text.h` — hard cut, no ellipsis.
- Usage: `oled.print(chop_chars(line, OLED_ROW_CHARS));`
- Submenus redraw **only on button press** via `oled.clearDisplay()` + draw function. They do **not** refresh every frame (avoids flicker).
- Top-level previews refresh from `show_data()` every 100 ms in `loop()`.

## Selection highlight (cursor vs edit)

Shared helpers in `handset_menu.c`:

| Function | Purpose |
|----------|---------|
| `menu_line_inverted(selected, edit_mode)` | Returns whether the row uses inverse font |
| `menu_blink_reset()` | Call when entering edit mode |
| `menu_blink_tick(now)` | Call from `loop()` while editing; toggles every `MENU_BLINK_MS` (500 ms) |

| State | Selected row appearance |
|-------|-------------------------|
| Cursor on row, not editing | Steady inverted |
| Editing value on row | Blinks inverted / normal every 500 ms |

`loop()` in `src/main.cpp` calls `menu_blink_tick()` when `isInLowerLevel` and the active menu is in edit mode, then redraws the matching draw function.

## Handset static menus (`handset_menu.c`)

### Main settings (`handset_main_*`)

Opened from `SCREEN_MAIN` with OK.

| Row | Label | Variable | Values | EEPROM |
|-----|-------|----------|--------|--------|
| 0 | Protocol | `control_protocol` | ELRS, ESP, SIM (0–2) | 5 |
| 1 | Mixer | `mixer_selected` | `mixer_labels[]` (0–4) | 12 |
| 2 | Buzzer | `use_buzzer` | OFF / ON | 14 |
| 3 | LEDs | `use_leds` | OFF / ON | 15 |

Changing mixer reloads `throttle_fine` / `yaw_fine` from that mixer’s EEPROM block (`32 + mixer_selected * 4`).

### Trim (`handset_trim_*`)

Opened from `SCREEN_TRIM` with OK.

| Row | Label | Variable | Range | EEPROM |
|-----|-------|----------|-------|--------|
| 0 | Yaw trim | `yaw_fine` | 0–255 | `33 + mixer_selected * 4` |
| 1 | Thr trim | `throttle_fine` | 0–255 | `32 + mixer_selected * 4` |

### Per-menu API pattern

Each static submenu exports four functions in `handset_menu.h`:

```c
void handset_<name>_enter(void);             // reset line_index, edit_mode
void handset_<name>_draw(void);              // paint all rows
void handset_<name>_handle_btn(uint8_t btn); // navigation + edit
uint8_t handset_<name>_edit_mode(void);      // for blink tick in loop()
```

Internal state uses `static_menu_state_t` (local to `handset_menu.c` — do not confuse with `handset_menu_state_t` in `crsf.h`):

```c
typedef struct {
    uint8_t line_index;  // cursor row
    uint8_t edit_mode;   // 0 = browse, 1 = editing value
} static_menu_state_t;
```

Redraw pattern inside the handler:

```c
static void xxx_redraw(void) {
    oled.clearDisplay();
    handset_xxx_draw();
}
```

Browse mode (not editing):

- **NEXT** — move cursor down
- **PREV** — move cursor up
- **OK** — enter edit mode on selected row

Edit mode:

- **NEXT** — increase value
- **PREV** — decrease value
- **OK** — save to EEPROM, exit edit mode
- **BACK** — discard edit, exit edit mode

## ELRS dynamic menu

Opened from `SCREEN_ELRS` with OK after `param_manager.params_loaded`.

- State: `dynamic_menu_state_t menu_state` in `src/main.cpp`
- Draw: `draw_dynamic_menu()` in `src/main.cpp`
- Navigation: `handle_menu_navigation()`, `handle_value_edit()` in `src/main.cpp`
- Data: loaded from module via `crsf_dynamic.c` (folders, text selections, commands, etc.)
- Folders: `>` prefix; BACK goes up one folder
- Compact rows (Packet Rate, Telem Ratio): show option value only on the line

ELRS edit sends values with `dynamic_param_send_value()`; handset menus save to EEPROM with `EEPROM_update()`.

## File map

| File | Responsibility |
|------|------------------|
| `handset_menu.h` | Screen enum, EEPROM addresses, static menu API |
| `handset_menu.c` | Main + trim menus, blink helpers |
| `oled_text.h` / `oled_text.c` | `chop_chars()`, `OLED_ROW_CHARS` |
| `src/main.cpp` | `process_ui_buttons()`, `show_data()`, `loop()` routing, `draw_dynamic_menu()` |
| `crsf_dynamic.c` | ELRS parameter load/parse/store |

---

## How to add a new top-level screen with a static submenu

Example: add `SCREEN_MIXER` with editable rows. Copy the **Main** or **Trim** pattern.

### Step 1 — Extend the screen enum

In `handset_menu.h`:

```c
typedef enum {
    SCREEN_MAIN = 0,
    SCREEN_ELRS = 1,
    SCREEN_TRIM = 2,
    SCREEN_MIXER = 3,   // new
    SCREEN_COUNT = 4    // was 3; always last + 1
} handset_screen_t;
```

### Step 2 — Implement the submenu in `handset_menu.c`

1. Add `static static_menu_state_t mixer_state;` (unique name).
2. Add item labels, count, and value formatting (`mixer_format_value`).
3. Implement `mixer_adjust()` / `mixer_save_item()` for each row.
4. Implement the four public functions: `enter`, `draw`, `handle_btn`, `edit_mode`.
5. Declare them in `handset_menu.h`.

Draw each row like this:

```c
if (menu_line_inverted(i == mixer_state.line_index, mixer_state.edit_mode)) {
    oled.setInverseFont(1);
}
oled.setCursor(0, i);
oled.print(chop_chars(line, OLED_ROW_CHARS));
oled.setInverseFont(0);
```

On OK in edit mode: save (EEPROM or RAM), set `edit_mode = 0`, call `menu_blink_reset()`, redraw.

### Step 3 — Add a preview in `show_data()`

In `src/main.cpp`, inside `show_data()`:

```c
} else if (main_menu_pos == SCREEN_MIXER && isInLowerLevel == false) {
    oled.setCursor(0, 0);
    oled.print("Mixer:");
    // summary lines, e.g. current values + "OK to edit"
}
```

Previews update live every 100 ms. Submenus do not use `show_data()` while open.

### Step 4 — Route buttons in `process_ui_buttons()`

Add the submenu handler **before** the top-level NEXT/PREV block (same pattern as Main/Trim):

```c
if (main_menu_pos == SCREEN_MIXER && isInLowerLevel) {
    if (btn == BACK && !handset_mixer_edit_mode()) {
        isInLowerLevel = false;
        oled.clearDisplay();
    } else {
        handset_mixer_handle_btn((uint8_t)btn);
    }
    return;
}
```

Add OK handler to enter the submenu:

```c
} else if (btn == OK && main_menu_pos == SCREEN_MIXER) {
    isInLowerLevel = true;
    handset_mixer_enter();
    oled.clearDisplay();
    handset_mixer_draw();
}
```

### Step 5 — Blink redraw in `loop()`

Inside `if (menu_initialized && isInLowerLevel)`:

```c
} else if (main_menu_pos == SCREEN_MIXER && handset_mixer_edit_mode()) {
    blink_redraw = menu_blink_tick(currentMillis);
}
// in the redraw section:
} else if (main_menu_pos == SCREEN_MIXER) {
    handset_mixer_draw();
}
```

### Step 6 — Boot load and EEPROM

- Load saved values in `setup()` with `EEPROM_read()` + `constrain()`.
- Add `#define EEPROM_..._ADDR` in `handset_menu.h` (pick a free address).
- Save on OK in edit with `EEPROM_update()`.

### Step 7 — Wire runtime behaviour

If hardware is not ready yet, use a global variable (like `use_buzzer` / `use_leds`) and connect it to drivers later. The menu only needs read/write of that variable.

---

## How to add a row to an existing static submenu (e.g. Main)

1. Increase `MAIN_ITEM_COUNT` in `handset_menu.c`.
2. Add label to `main_item_labels[]`.
3. Extend `main_format_value()`, `main_adjust()`, and `main_save_item()` with a new `case` index.
4. If inserting in the middle, renumber existing `case` values.
5. Load/save the new variable in `setup()` / `main_save_item()`.

No changes to `process_ui_buttons()` are needed if the submenu already exists.

---

## How to add a new ELRS-only display feature

ELRS parameters come from the module; you normally **do not** add rows manually.

For custom display of a known field name:

- Adjust `draw_dynamic_menu()` in `src/main.cpp` (e.g. `strncmp(field->name, "Packet Rate", ...)`).
- Or extend `dynamic_field_compact_display()` / `dynamic_field_display_value()` in `crsf_dynamic.c`.

---

## Checklist (new static top-level screen)

- [ ] `SCREEN_*` added to `handset_screen_t`; `SCREEN_COUNT` incremented
- [ ] `handset_<name>_enter/draw/handle_btn/edit_mode` in `handset_menu.c` + declarations in `handset_menu.h`
- [ ] Preview block in `show_data()`
- [ ] Submenu branch in `process_ui_buttons()`
- [ ] OK handler to enter submenu
- [ ] Blink branch in `loop()` for edit mode
- [ ] EEPROM address defined; load in `setup()`, save on OK
- [ ] Button values match `MENU_ACT_*` (cast `btn` to `uint8_t`)
- [ ] Build: `pio run -e bluepill_f103c8`

## Common mistakes

- **Wrong file:** menu docs live next to menu code in `include/MENU.md`.
- **Flicker:** calling `draw_*()` every frame without a button press. Only redraw on change or blink tick in edit mode.
- **Wrong button mapping:** `MENU_ACT_*` must match `ENUM_BUTTON` order in `src/main.cpp`.
- **Name clash:** do not use `handset_menu_state_t` for static menus; that type exists in `crsf.h`. Use `static_menu_state_t` locally.
- **Pin macro clash:** avoid names like `BTN_BACK` inside `handset_menu.c`; the board defines them as GPIO. Use `MENU_ACT_BACK` instead.
- **ELRS vs handset:** module params → `crsf_dynamic`; radio settings → `handset_menu.c`.
